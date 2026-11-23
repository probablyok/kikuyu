#include "safe_wrappers.h"
#include "cpu_macros.h"
#include <stdint.h>
#include <stdlib.h>

void* safe_malloc(size_t n) {
    void* p = (void*)malloc(n);
    if (p == NULL) {
        fprintf(stderr, "Error: Malloc call failed. Aborting...");
        abort();
    }
    return p;
}

void* safe_realloc(void* ptr, size_t n) {
    void* p = (void*)realloc(ptr, n);
    if (p == NULL) {
        fprintf(stderr, "Error: Realloc call failed. Aborting...");
        abort();
    }
    return p;
}

bool safe_read_file_text(const char* filepath, char** buffer) {
    // Open file pointer
    FILE* fp = fopen(filepath, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Unable to open \"%s\".\n", filepath);
        return true;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Read file into buffer
    *buffer = (char*)safe_malloc(file_size + 1);
    size_t bytes_read = fread(*buffer, 1, file_size, fp);

    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "Error: Failed to read entire file \"%s\".\n", filepath);
        free(*buffer);
        fclose(fp);
        return true;
    }

    (*buffer)[file_size] = '\0';
    // Close file
    fclose(fp);
    return false;
}

bool safe_read_file_bin(const char* filepath, Cpu* cpu, long mem_offset) {
    // Is mem_offset valid
    if (mem_offset < 0 || mem_offset >= CPU_MEM_MAX_SIZE) {
        fprintf(stderr, "Error: Memory offset (0x%04X) is out of bounds.\n",
                (unsigned int)mem_offset);
        return true;
    }

    // Open file pointer
    FILE* fp = fopen(filepath, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Unable to open \"%s\".\n", filepath);
        return true;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Check file contents fit in memory
    if (file_size <= 0 || (mem_offset + file_size) > CPU_MEM_MAX_SIZE) {
        fprintf(stderr,
                "Error: Binary size (%ld bytes) exceeds memory limits when offset at 0x%04X.\n",
                file_size, (unsigned int)mem_offset);
        fclose(fp);
        return true;
    }
    // Put file contents in memory
    size_t bytes_read = fread(&cpu->mem[mem_offset], 1, file_size, fp);

    // Check the binary was read correctly
    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "Error: Failed to read entire binary.\n");
        fclose(fp);
        return true;
    }

    // Close file
    fclose(fp);
    return false;
}

char* safe_read_line(FILE* stream, size_t* len_ptr, size_t* cap_ptr) {
    // Handle invalid stream case
    if (!stream) {
        return NULL;
    }

    // Setup buffer
    size_t cap = DYNAMIC_BUF_INIT_SIZE;
    size_t len = 0;
    char* buf = safe_malloc(cap * sizeof(char));

    // Read character into buffer until EOF or '\n'
    int c;
    while ((c = fgetc(stream)) != EOF) {
        // Double buffer size if new char can't fit
        if (len + 1 >= cap) {
            size_t new_cap = cap * 2;
            char* tmp = realloc(buf, new_cap);

            buf = tmp;
            cap = new_cap;
        }
        buf[len++] = (char)c;

        // Escape on newline character
        if (c == '\n') {
            break;
        }
    }

    // Return NULL if we read zero characters
    if (len == 0) {
        free(buf);
        return NULL;
    }

    // Null terminate buffered line
    buf[len] = '\0';

    // Return memory to user
    *len_ptr = len;
    *cap_ptr = cap;
    return buf;
}
