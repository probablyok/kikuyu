#ifndef SAFE_WRAPPERS_H
#define SAFE_WRAPPERS_H

#define DYNAMIC_BUF_INIT_SIZE 32

#include "cpu_state.h"

#include <stdio.h>
#include <stdlib.h>

/// @brief Malloc wrapper that core dumps on allocation failure
/// @param n Number of bytes to allocate
/// @return Void pointer to allocated memory
void* safe_malloc(size_t n);

/// @brief Realloc wrapper that core dumps on reallocation failure
/// @param ptr Pointer to the memory address being reallocated
/// @param n Number of bytes to use for reallocated pointer
/// @return Void pointer to reallocated memory
void* safe_realloc(void* ptr, size_t n);

/// @brief Reads the contents of a file at given path into buffer. Buffer memory
/// is allocated within function and must be freed later. Memory does not need
/// to be freed if this function returns 1
/// @param filepath The string filepath to the file that will be opened
/// @param buffer Character buffer that holds contents of file
/// @return `true` if the file was unable to be opened, `false` otherwise
bool safe_read_file_text(const char* filepath, char** buffer);

/// @brief Reads the contents of a binary file at given path into CPU memory.
/// @param filepath The string filepath to the file that will be opened
/// @param cpu Reference to the CPU struct encapsulating its memory
/// @param mem_offset Index into the CPU's memory to place binary contents
/// @return `true` if there was some error operating on the file, `false` otherwise
bool safe_read_file_bin(const char* filepath, Cpu* cpu, long offset);

/// @brief Reads one line in given file stream into given buffer. Memory for the buffer is allocated
/// within the function and may need to be freed by the user after. Firstly, if the function returns
/// NULL, then values pointed to by `len_ptr` and `cap_ptr` are not updated and no memory needs to
/// be freed. This occurs when zero characters are read, or the stream is NULL. Otherwise, the
/// returned buffer must be freed.
/// @param stream The file stream being read from
/// @param len_ptr Pointer to variable which contains the number of characters read (including
/// newline) if read buffer is not NULL
/// @param cap_ptr Pointer to the variable which contains the number of bytes allocated to the
/// returned buffer (capcity) if buffer is not NULL
/// @return NULL on invalid stream or no characters read. Otherwise, null-terminated buffer
/// containing read line from the given stream
char* safe_read_line(FILE* stream, size_t* len_ptr, size_t* cap_ptr);

#endif // SAFE_WRAPPERS_H
