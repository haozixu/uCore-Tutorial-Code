#include "loader.h"
#include "defs.h"
#include "elf.h"
#include "file.h"
#include "trap.h"

extern char INIT_PROC[];

int bin_loader(struct inode *ip, struct proc *p)
{
	ivalid(ip);
	void *page;
	uint64 length = ip->size;
	uint64 va_start = BASE_ADDRESS;
	uint64 va_end = PGROUNDUP(BASE_ADDRESS + length);
	for (uint64 va = va_start, off = 0; va < va_end;
	     va += PGSIZE, off += PAGE_SIZE) {
		page = kalloc();
		if (page == 0) {
			panic("...");
		}
		readi(ip, 0, (uint64)page, off, PAGE_SIZE);
		if (off + PAGE_SIZE > length) {
			memset(page + (length - off), 0,
			       PAGE_SIZE - (length - off));
		}
		if (mappages(p->pagetable, va, PGSIZE, (uint64)page,
			     PTE_U | PTE_R | PTE_W | PTE_X) != 0)
			panic("...");
	}

	p->max_page = va_end / PAGE_SIZE;
	p->ustack_base = va_end + PAGE_SIZE;
	// alloc main thread
	if (allocthread(p, va_start, 1) != 0) {
		panic("proc %d alloc main thread failed!", p->pid);
	}
	debugf("bin loader fin");
	return 0;
}

// NOTE: elf_loader does not call allocthread
int elf_loader(struct inode *ip, struct proc *p, uint64 *p_entry)
{
	struct elf64_header h;
	struct elf64_program_header ph;
	uint64 max_va_end = 0;

	ivalid(ip);
	if (readi(ip, 0, (uint64) &h, 0, sizeof(h)) != sizeof(h))
		goto bad_header;
	if (h.ident[0] != 0x7f || h.ident[1] != 'E' ||
		h.ident[2] != 'L' || h.ident[3] != 'F')
		goto bad_header;
	if (h.ident[EI_CLASS] != ELFCLASS64)
		goto bad_header;
	
	for (int i = 0; i < h.ph_count; ++i) {
		uint ph_off = h.ph_offset + i * h.ph_entry_size;
		if (readi(ip, 0, (uint64) &ph, ph_off, sizeof(ph)) != sizeof(ph)) {
			errorf("invalid ELF program header");
			return -1;
		}
		if (ph.type != PT_LOAD)
			continue;
		
		uint64 va_start = ph.vaddr;
		uint64 va_end = PGROUNDUP(ph.vaddr + ph.mem_size);
		max_va_end = MAX(max_va_end, va_end);
		
		int perm = PTE_U;
		if (ph.flags & PF_X)
			perm |= PTE_X;
		if (ph.flags & PF_W)
			perm |= PTE_W;
		if (ph.flags & PF_R)
			perm |= PTE_R;
		
		// NOTE: ph.file_size can be smaller than ph.mem_size
		uint off = ph.offset, off_end = ph.offset + ph.file_size; 
		for (uint64 va = va_start; va < va_end; va += PAGE_SIZE, off += PAGE_SIZE) {
			void *page = kalloc();
			if (!page)
				panic("failed to allocate pages");

			if (off < off_end) {
				uint size = off_end - off;
				readi(ip, 0, (uint64) page, off, MIN(size, PAGE_SIZE));
				if (size < PAGE_SIZE) {
					// clear the rest of the page
					memset(page + size, 0, PAGE_SIZE - size);
				}
			} else {
				memset(page, 0, PAGE_SIZE);
			}

			if (mappages(p->pagetable, va, PAGE_SIZE, (uint64) page, perm) != 0)
				panic("failed to map pages");
		}
	}
	p->max_page = max_va_end / PAGE_SIZE;
	p->ustack_base = max_va_end + PAGE_SIZE;
	if (p_entry)
		*p_entry = h.entry_point;
	return 0;

bad_header:
	errorf("invalid ELF header");
	return -1;
}

// load all apps and init the corresponding `proc` structure.
int load_init_app()
{
	struct inode *ip;
	struct proc *p = allocproc();
	init_stdio(p);
	if ((ip = namei(INIT_PROC)) == 0) {
		errorf("invalid init proc name\n");
		return -1;
	}
	debugf("load init app %s", INIT_PROC);
	bin_loader(ip, p);
	iput(ip);
	char *argv[2];
	argv[0] = INIT_PROC;
	argv[1] = NULL;
	struct thread *t = &p->threads[0];
	t->trapframe->a0 = push_argv(p, argv);
	t->state = RUNNABLE;
	add_task(t);
	return 0;
}