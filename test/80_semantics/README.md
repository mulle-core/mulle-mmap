# Simple Read Tests

This directory contains basic tests for mulle-mmap's file reading capabilities.

## Test Files

### test.c
The original simple read test that demonstrates basic file mapping and reading functionality.

### test_persistent_mapping.c
**Special Test: Memory Mapping Persistence**

This test verifies one of the fundamental behaviors of memory mapping:
**Mapped memory remains accessible even after closing the original file descriptor.**

#### What it tests:

1. **File Descriptor Independence**: 
   - Maps a file into memory using `_mulle_mmap_map()`
   - Closes the original file descriptor
   - Verifies that the mapped memory is still accessible and contains correct data

2. **File Removal Behavior**:
   - Maps a file using `_mulle_mmap_map_file()` 
   - Removes the original file from the filesystem
   - Verifies that the mapped memory survives file deletion

#### Why this is important:

This demonstrates a key characteristic of `mmap()` - it creates an independent mapping of the file's data pages in virtual memory. Once mapped:

- **File descriptor closure**: The mapping persists because the kernel maintains the virtual memory mapping independently of the file descriptor
- **File removal**: On Unix systems, the mapped data typically survives file deletion because the kernel keeps the file's data blocks alive as long as there are active mappings

This behavior is crucial for applications that need to:
- Pass mapped memory between processes
- Keep data accessible while closing file handles
- Handle scenarios where files might be deleted while still in use

#### Expected output:

The test should demonstrate successful data access in both scenarios:
1. ✅ Memory accessible after file descriptor close  
2. ✅ Memory accessible after file removal (on most Unix systems)

This test validates that mulle-mmap correctly implements these fundamental mmap behaviors.

### test_write_persistence.c
**Special Test: Write Memory Mapping Persistence & File Modification**

This test verifies that write mappings remain functional and persist changes to the file even after closing the original file descriptor:
**Memory-mapped file modifications are written back to the file even after file descriptor closure.**

#### What it tests:

1. **File Creation with mmap**: 
   - Creates a test file using mmap itself (demonstrating write capability)
   - Initializes with "VfL Bochum 1848" followed by zeros

2. **Write Persistence After File Descriptor Close**:
   - Maps file for read/write using `_mulle_mmap_map()`
   - Closes the original file descriptor
   - Modifies the mapped memory (changes "VfL Bochum 1848" to "MODIFIED: VfL Bochum 1848 - Champions!")
   - Verifies mapping remains writable after fd close

3. **File Modification Verification**:
   - Cleans up the mapping (which should flush changes to disk)
   - Re-reads the file with `fopen()` to verify changes were written
   - Creates a fresh mapping to double-check persistence

4. **Explicit Sync Testing**:
   - Demonstrates modification and sync operations
   - Shows file changes are visible even while mapping is active

#### Why this is important:

This demonstrates critical mmap write behavior:

- **Write Independence**: Write mappings function independently of file descriptors
- **Automatic Flush**: Changes are automatically flushed when mapping is cleaned up
- **File Persistence**: Memory modifications are permanently written to the underlying file
- **Cross-Access Consistency**: Changes made through mmap are visible to regular file I/O operations

This behavior is essential for:
- Database engines and memory-mapped files
- Shared memory regions backed by files
- High-performance file modification without explicit write() calls
- Persistent data structures

#### Expected output:

The test should demonstrate successful write persistence:
1. ✅ File created with initial "VfL Bochum 1848" content
2. ✅ Write mapping remains functional after file descriptor close
3. ✅ Memory modifications succeed while fd is closed
4. ✅ File re-reading shows modifications were written: "MODIFIED: VfL Bochum 1848 - Champions!"
5. ✅ Fresh mapping confirms all changes were persisted

This validates that mulle-mmap correctly implements mmap write-back behavior.
