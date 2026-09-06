## 1.1.0


feature: add Android support for shared memory allocation

* shared memory allocation now works on Android by calling the Linux `shm_open/shm_unlink` syscalls directly
* no dependency on `shm_open(3),` which Android's public libc headers do not expose





feature: add public range mapping API and flag-combinable access modes

* new public ``mulle_mmap_map_file_range`` function to map a range of a file
* access modes (`read`, `write`, ``no_unmap`)` now combine as flags, so read-write mappings work correctly
* Windows: close the file-mapping handle on mapping failure and return a consistent error code



* add API documentation link to README
* relocate API TOC to standard path `asset/dox/api/toc/`


### 1.0.1

Various small improvements
