#ifndef ELF_H
#define ELF_H

#include "types.h"

struct elf64_header {
    unsigned char ident[16];
    uint16 type; // object file type
    uint16 machine; // architecture
    uint32 version; // object file version
    uint64 entry_point; // entry point virtual address
    uint64 ph_offset; // program header table file offset
    uint64 sh_offset; // section header table file offset
    uint32 flags; // processor-specific flags
    uint16 header_size; // ELF header size in bytes
    uint16 ph_entry_size; // program header table entry size
    uint16 ph_count; // program header table entry count
    uint16 sh_entry_size; // section header table entry size
    uint16 sh_count; // section header table entry count
    uint16 sh_str_index; // section header string table index
}__attribute__((packed));

#define EI_CLASS 4
#define ELFCLASS64 2

struct elf64_program_header {
    uint32 type; // segment type
    uint32 flags; // segment flags
    uint64 offset; // segment file offset
    uint64 vaddr; // segment virtual address
    uint64 paddr; // segment physical address
    uint64 file_size; // segment size in file
    uint64 mem_size; // segment size in memory
    uint64 align; // segemnet alignment
};

#define PT_LOAD 1 // loadable program segment

#define PF_X 1 // segment is executable
#define PF_W 2 // segment is writable
#define PF_R 4 // segment is readable

#endif // ELF_H
