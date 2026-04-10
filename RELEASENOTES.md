# 1.0.0







feature: add ability to map existing shared memory and preserve shm fds across exec

* new `mulle_mmap_map_shared_memory(handle,` size, `preferred_addr)` to map an existing shared-memory region into the current process; `preferred_addr` can request fixed-address mapping (useful to keep allocator/internal pointers valid).
* POSIX: clear `FD_CLOEXEC` on `shm_open` fds so shared-memory handles survive exec() (addresses glibc >= 2.24 behavior).
* **BREAKING**: on Windows, header now requires mulle-c11-bool.h `(MULLE_BOOL_DEFINED)` before including `<windows.h>`; adjust include order to build.


feature: add Windows-compatible shared page allocations

* **BREAKING** ``mulle_mmap_alloc_shared_pages`` now returns `struct `mulle_mmap_shared_pages`` (address/size + Windows handle) and must be released with ``mulle_mmap_free_shared_pages``
* shared page mappings now work on Windows via inheritable `CreateFileMapping`/`MapViewOfFile`, with documented lifetime semantics across fork/CreateProcess
