# mulle-mmap Library Documentation for AI
<!-- Keywords: mmap, memory, filemap, shared, pages, crossplatform, C -->
## 1. Introduction & Purpose

- mulle-mmap is a small cross-platform C library for memory-mapped files and page-level OS memory allocation.
- Solves efficient file I/O and OS page allocation needs by exposing simple mapping and page APIs that wrap POSIX mmap/munmap and Windows CreateFileMapping/MapViewOfFile.
- Key features: page allocation (zero-filled), shared-memory pages for IPC, file mapping (whole file or ranges), safe wrappers and platform-specific handles.
- Component of mulle-core; depends on mulle-allocator / mulle-c11 primitives via project reflect headers.

## 2. Key Concepts & Design Philosophy

- Thin, explicit mapping abstraction: struct mulle_mmap holds mapping metadata (data pointer, lengths, accessmode, handles).
- Separate platform implementations (posix/windows) keep public API stable; use accessmode flags for read vs write.
- Emphasizes explicit lifecycle: init -> map -> use -> conditional_sync/unmap -> done.
- Provides both high-level safe inline wrappers and low-level platform-specific functions when needed.

## 3. Core API & Data Structures

This section is organized by the primary public header: mulle-mmap.h

### 3.1. [mulle-mmap.h]

#### struct mulle_mmap_shared_memory
- Purpose: represent allocated shared memory usable for IPC across processes.
- Key fields:
  - void *address  : mapped address in this process
  - size_t size    : size in bytes
  - mulle_mmap_file_t handle : platform handle (Windows HANDLE or Unix fd cast)
- Lifecycle functions:
  - mulle_mmap_alloc_shared_memory(size_t size) -> returns struct (address, size, handle)
  - mulle_mmap_free_shared_memory(struct *mem) -> int (0 on success)
- Core ops:
  - mulle_mmap_map_shared_memory(handle, size, preferred_addr) -> void* (map an existing handle)
- Notes: On Windows, keep the returned handle open if you intend children to inherit it.

#### struct mulle_mmap
- Purpose: describe a single memory mapping of a file or region.
- Key fields:
  - void *data_           : pointer to usable bytes (may be NULL for empty mapping)
  - size_t length_        : logical length requested by user
  - size_t mapped_length_ : actual mapped length (page aligned)
  - enum mulle_mmap_accessmode accessmode_ : mulle_mmap_read / mulle_mmap_write
  - file handle fields for platform-specific handles
- Lifecycle functions:
  - _mulle_mmap_init(struct *p, accessmode)  or mulle_mmap_init() safe wrapper
  - _mulle_mmap_done(struct *p)  or mulle_mmap_done() safe wrapper
- Core operations:
  - _mulle_mmap_map_file_range(struct *p, path, offset, length) -> int
  - _mulle_mmap_map_file(struct *p, path) -> maps whole file (inline wrapper provided)
  - _mulle_mmap_map_range(struct *p, handle, offset, length) -> int
  - _mulle_mmap_map(struct *p, handle) -> maps whole handle (inline)
  - _mulle_mmap_unmap/_mulle_mmap_sync (low-level) and safe wrappers mulle_mmap_unmap/mulle_mmap_sync
- Inspection functions:
  - mulle_mmap_get_bytes(struct *p) -> void* (pointer to mapped bytes)
  - mulle_mmap_get_length(struct *p) -> size_t (logical length)
  - mulle_mmap_get_mapped_length(struct *p) -> size_t (OS-mapped length)
  - mulle_mmap_get_mapping_offset(struct *p) -> size_t (mapped_length - length)
  - mulle_mmap_is_open, mulle_mmap_is_mapped, mulle_mmap_is_empty, mulle_mmap_is_writable
- Notes: equality helper mulle_mmap_equal(p,q) compares mode, pointer and length.

#### Enum mulle_mmap_accessmode
- mulle_mmap_read  : read-only mapping
- mulle_mmap_write : read-write mapping
- mulle_mmap_no_unmap : flag to avoid unmapping when done (advanced use)

#### Page allocation helpers
- mulle_mmap_get_system_pagesize() -> size_t
- mulle_mmap_alloc_pages(size_t size) -> void* (pages are guaranteed zero-filled)
- mulle_mmap_free_pages(void *p, size_t size) -> int (safe inline wrapper: mulle_mmap_free_pages)

#### Shared pages (fast unix variant)
- mulle_mmap_alloc_shared_pages_nowindows(size) -> void* (UNIX only, faster, fork-only)
- mulle_mmap_free_shared_pages_nowindows(void *address, size_t size)

## 4. Performance Characteristics

- Mapping and allocation use OS primitives; indexing into mapped memory is O(1).
- Mapping creation/unmap/sync are OS calls (cost depends on kernel and file size).
- Page allocation/free are O(1) wrt requested pages but may involve system calls.
- Mapped length is page-granular; requesting non-page-aligned sizes is adjusted internally.
- Trade-offs: library keeps minimal state and delegates heavy work to OS; memory vs performance trade-offs are OS-determined.
- Thread-safety: API is not explicitly thread-safe; mappings and structures should be externally synchronized if shared between threads.

## 5. AI Usage Recommendations & Patterns

- Always use _mulle_mmap_init / mulle_mmap_init before mapping and _mulle_mmap_done / mulle_mmap_done to clean up.
- For simple mapping, use the safe inline wrappers (mulle_mmap_map_file, mulle_mmap_map_range, unmap/sync inline wrappers).
- When mapping for child processes, preserve the shared memory handle (Windows) until after CreateProcess.
- Prefer mule_mmap_no_unmap only when you intentionally want to keep pages alive after done() (advanced).
- Do not access internal fields (underscore-prefixed) directly unless you need low-level behavior.
- Be aware: mulle_mmap_alloc_pages returns zero-filled memory; free with mulle_mmap_free_pages(p,size).

Common pitfalls
- Forgetting to call done/unmap may keep handles open or mapped regions alive during process lifetime.
- Closing Windows mapping handle before spawning child makes inheritance impossible.
- Expect mapped_length != length when offset or OS alignment causes expansion; use get_mapping_offset to compute user-visible data start.

## 6. Integration Examples

### Example 1: Allocate and free pages

```c
#include <mulle-mmap/mulle-mmap.h>

void example_alloc_pages( void)
{
   size_t page_size;
   void   *pages;

   page_size = mulle_mmap_get_system_pagesize();
   pages     = mulle_mmap_alloc_pages( page_size * 4);
   if( pages)
   {
      /* pages are zero-filled */
      memset( pages, 0xFF, page_size);
      mulle_mmap_free_pages( pages, page_size * 4);
   }
}
```

### Example 2: Shared memory for parent/child IPC (Unix/Windows-aware)

```c
#include <mulle-mmap/mulle-mmap.h>

void example_shared( void)
{
   struct mulle_mmap_shared_memory  mem;
   size_t                           ps;

   ps  = mulle_mmap_get_system_pagesize();
   mem = mulle_mmap_alloc_shared_memory( ps);
   if( mem.address)
   {
      /* initialize and later pass mem.handle to child */
      strcpy( (char *) mem.address, "hello");
      /* child must inherit handle (Windows) or fork preserves mapping (Unix) */
      mulle_mmap_free_shared_memory( &mem);
   }
}
```

### Example 3: Map part of a file

```c
#include <mulle-mmap/mulle-mmap.h>

void example_map_range( void)
{
   struct mulle_mmap  info;
   char               *bytes;

   _mulle_mmap_init( &info, mulle_mmap_read);
   if( _mulle_mmap_map_file_range( &info, "data.bin", 1024, 4096) == 0)
   {
      bytes = mulle_mmap_get_bytes( &info);
      /* process bytes[0..length-1] */
      _mulle_mmap_done( &info);
   }
}
```

## 7. Dependencies

- mulle-c11

---

Notes for an AI: Use the public header (mulle-mmap.h) as canonical source for function names and inline-wrapper semantics. Tests under test/ demonstrate intended usage and edge cases: zero-fill guarantees, partial unmap, shared-memory parent/child, range mapping and error handling.