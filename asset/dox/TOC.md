# mulle-mmap Library Documentation for AI
<!-- Keywords: mmap, file-mapping -->

## 1. Introduction & Purpose

mulle-mmap is a cross-platform C library providing high-level abstractions for memory mapping files and allocating memory pages directly from the OS. It offers an easy-to-use API for mapping files into memory for high-performance I/O operations and managing OS-level page allocation. The library abstracts platform differences between Unix (mmap) and Windows (MapViewOfFile) under a unified interface with MIT licensing.

## 2. Key Concepts & Design Philosophy

- **Cross-Platform Abstraction**: Hides platform-specific details (POSIX mmap vs. Windows MapViewOfFile) behind unified API
- **Access Modes**: Distinguishes between read-only and read-write mappings with optional "no unmap" flag for special cases
- **Lazy Mapping**: Can map file ranges incrementally or map entire files
- **Explicit Lifecycle**: Functions prefixed with `_` are unchecked; safe versions have NULL checks
- **Page Management**: Direct OS page allocation independent of mappings for custom memory layouts
- **Flush Control**: Explicit control over syncing mapped pages to disk (important for durability)

## 3. Core API & Data Structures

### mulle-mmap.h - Main API

#### struct mulle_mmap
- **Purpose**: Encapsulates a memory-mapped region of a file or memory pages
- **Key Fields**:
  - `data_`: Pointer to mapped memory
  - `length_`: Logical size of mapped data
  - `mapped_length_`: Actual OS page-aligned size
  - `accessmode_`: Read-only or read-write
  - `is_handle_internal_`: Whether handle was opened internally
  - `file_handle_`: OS file handle (int on Unix, HANDLE on Windows)
  - `file_mapping_handle_`: Windows-specific mapping handle
- **Initialization**: Use `_mulle_mmap_init(p, mode)` to zero-initialize

#### Access Modes (enum mulle_mmap_accessmode)

- `mulle_mmap_read` (0): Map file for read-only access
- `mulle_mmap_write` (1): Map file for read-write access
- `mulle_mmap_no_unmap` (0x80): Flag to prevent unmapping (for special use cases, can be OR'd with read/write)

#### File Operations

- `mulle_mmap_file_open(path, mode)` → `mulle_mmap_file_t`: Opens a file for mapping; returns handle or INVALID_HANDLE
- `mulle_mmap_file_query_size(handle)` → `int64_t`: Returns file size; -1 on error

#### Memory Mapping Operations

- `mulle_mmap_memory_map(handle, offset, length, mode)` → `int`: Maps file region starting at offset; 0 on success
- `_mulle_mmap_map_file_range(p, path, offset, length)` → `int`: Maps file range into mulle_mmap struct
- `_mulle_mmap_map_range(p, handle, offset, length)` → `int`: Maps from open file handle
- `_mulle_mmap_unmap(p)` → `int`: Unmaps memory and closes file handle; 0 on success

#### Synchronization & State

- `_mulle_mmap_sync(p)` → `int`: Flushes mapped pages to disk (only effective with write access)
- `_mulle_mmap_conditional_sync(p)` → `int`: Syncs only if write mode; 0 on success
- `_mulle_mmap_is_mapped(p)` → `int`: Returns non-zero if currently mapped

#### Page Allocation (OS-Level)

- `mulle_mmap_alloc_pages(size)` → `void *`: Allocates OS memory pages; returns pointer or NULL
- `_mulle_mmap_free_pages(p, size)` → `int`: Frees allocated pages; 0 on success
- `mulle_mmap_free_pages(p, size)` → `int`: Safe version with NULL check

#### Accessor Inline Functions

- `mulle_mmap_data(p)` → `void *`: Safe accessor for mapped data (checks validity)
- `_mulle_mmap_data(p)` → `void *`: Unchecked data pointer access
- `mulle_mmap_length(p)` → `size_t`: Safe accessor for logical length
- `_mulle_mmap_length(p)` → `size_t`: Unchecked length access
- `mulle_mmap_mapped_length(p)` → `size_t`: Safe accessor for page-aligned length
- `_mulle_mmap_mapped_length(p)` → `size_t`: Unchecked mapped length access

## 4. Performance Characteristics

- **Memory Access**: O(1) access to mapped data (direct memory access after mapping)
- **Mapping Operations**: O(1) to O(n) depending on OS (typically O(1) for logical operations)
- **File Size Query**: O(1) on most systems (stat call)
- **Page Allocation**: O(1) allocation, may trigger page faults on first access
- **Sync Operations**: O(n) where n is mapped size; can be expensive on large mappings
- **Memory Overhead**: Mapped pages counted against process memory; no additional heap allocation
- **Thread-Safety**: mulle_mmap itself has no synchronization; sharing across threads requires external locking

## 5. AI Usage Recommendations & Patterns

### Best Practices

- **Check Return Values**: All functions return error codes; always check for < 0 or NULL
- **Alignment Awareness**: mapped_length is OS-page aligned; use length for logical size
- **Explicit Sync**: Call _mulle_mmap_sync() before unmapping if durability is critical
- **Resource Cleanup**: Always call _mulle_mmap_unmap() (or rely on process cleanup)
- **Start Small**: Map file ranges incrementally rather than entire large files when possible
- **Access Mode**: Use read-only mapping when possible for OS optimization

### Common Pitfalls

- **Page Alignment**: Don't assume data_ptr is aligned to offset; mapped_length includes padding
- **After Unmap**: Don't access data_ after calling _mulle_mmap_unmap()
- **Partial Writes**: Writing to read-only mapping causes segfault; check accessmode
- **File Truncation**: Don't truncate mapped file while mapped; behavior is undefined
- **No Auto-Flush**: Changes to write-mapped pages aren't guaranteed on disk until _mulle_mmap_sync()

## 6. Integration Examples

### Example 1: Basic File Mapping

```c
#include <mulle-mmap/mulle-mmap.h>
#include <stdio.h>

int main() {
    struct mulle_mmap map;
    _mulle_mmap_init(&map, mulle_mmap_read);
    
    if (_mulle_mmap_map_file_range(&map, "input.txt", 0, -1) < 0) {
        perror("Failed to map file");
        return 1;
    }
    
    size_t n = _mulle_mmap_length(&map);
    if (n > 100) n = 100;
    fwrite(_mulle_mmap_data(&map), 1, n, stdout);
    
    _mulle_mmap_unmap(&map);
    return 0;
}
```

### Example 2: Read-Write File Modification

```c
#include <mulle-mmap/mulle-mmap.h>
#include <string.h>

int main() {
    struct mulle_mmap map;
    _mulle_mmap_init(&map, mulle_mmap_write);
    
    if (_mulle_mmap_map_file_range(&map, "data.txt", 0, 1024) < 0)
        return 1;
    
    char *data = (char *)_mulle_mmap_data(&map);
    if (data[0] == 'A')
        data[0] = 'B';
    
    _mulle_mmap_sync(&map);
    _mulle_mmap_unmap(&map);
    return 0;
}
```

### Example 3: Partial Range Mapping

```c
#include <mulle-mmap/mulle-mmap.h>

int main() {
    struct mulle_mmap map;
    _mulle_mmap_init(&map, mulle_mmap_read);
    
    if (_mulle_mmap_map_file_range(&map, "large.bin", 1000, 1000) < 0)
        return 1;
    
    unsigned char *data = (unsigned char *)_mulle_mmap_data(&map);
    size_t len = _mulle_mmap_length(&map);
    
    for (size_t i = 0; i < len; i++) {
        if (data[i] == 0xFF)
            break;
    }
    
    _mulle_mmap_unmap(&map);
    return 0;
}
```

### Example 4: OS Page Allocation

```c
#include <mulle-mmap/mulle-mmap.h>
#include <string.h>

int main() {
    size_t size = 64 * 1024;
    void *pages = mulle_mmap_alloc_pages(size);
    
    if (!pages) {
        perror("Page allocation failed");
        return 1;
    }
    
    memset(pages, 0xAA, size);
    printf("Allocated %zu bytes at %p\n", size, pages);
    
    mulle_mmap_free_pages(pages, size);
    return 0;
}
```

### Example 5: Query File Size Before Mapping

```c
#include <mulle-mmap/mulle-mmap.h>
#include <stdio.h>

int main() {
    mulle_mmap_file_t handle = mulle_mmap_file_open("input.dat", mulle_mmap_read);
    if (handle == MULLE_MMAP_INVALID_HANDLE) {
        perror("Open failed");
        return 1;
    }
    
    int64_t size = mulle_mmap_file_query_size(handle);
    printf("File size: %lld bytes\n", size);
    
    return 0;
}
```

### Example 6: Sequential Block Processing

```c
#include <mulle-mmap/mulle-mmap.h>
#include <stdio.h>

int process_file_in_blocks(const char *path, size_t block_size) {
    mulle_mmap_file_t handle = mulle_mmap_file_open((char *)path, mulle_mmap_read);
    if (handle == MULLE_MMAP_INVALID_HANDLE)
        return -1;
    
    int64_t total_size = mulle_mmap_file_query_size(handle);
    
    for (int64_t offset = 0; offset < total_size; offset += block_size) {
        struct mulle_mmap map;
        _mulle_mmap_init(&map, mulle_mmap_read);
        
        size_t map_size = (offset + block_size < total_size) ? block_size : 
                          (total_size - offset);
        
        if (mulle_mmap_memory_map(handle, offset, map_size, mulle_mmap_read) < 0)
            return -1;
        
        printf("Processing block at offset %lld, size %zu\n", offset, map_size);
        _mulle_mmap_unmap(&map);
    }
    
    return 0;
}

int main() {
    return process_file_in_blocks("data.bin", 4096);
}
```

## 7. Dependencies

- mulle-c11
