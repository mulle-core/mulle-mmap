### 0.2.6



* new test output files for file mapping functionality
* new test output files for memory management operations
* added read-only mapping test with segfault handling



* improved readability of `mulle_mmap_free_pages` logic



* `mulle_mmap_get_data` is now `mulle_mmap_get_bytes,` since we dont return a `mulle_data` there
* you can now add another mode `mulle_mmap_no_unmap` which is useful for reading pages in the memory and then not getting them unmapped once you call done


* vibe refactored to place platform code into separate files like mulle-thread does, which does wonders for readability
