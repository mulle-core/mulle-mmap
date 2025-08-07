# File Mapping Tests (Section 2)

This directory contains comprehensive tests for the file mapping functionality of mulle-mmap, implementing section 2 of the TEST_IMPLEMENTATION.md plan.

## Test Files

### Core Test Files

1. **test.c** - Basic File Mapping Tests (Tests 20-24)
   - Tests `_mulle_mmap_map_file()` with various file sizes
   - Tests mapping empty, small, medium, large, and binary files
   - Tests error handling for non-existent files
   - Verifies state functions and data access

2. **test_range.c** - File Range Mapping Tests (Test 21)
   - Tests `_mulle_mmap_map_file_range()` with different offsets/lengths
   - Tests range mapping with SIZE_MAX
   - Tests boundary conditions and error cases
   - Verifies correct length truncation and offset handling

3. **test_handle.c** - Handle-Based Mapping Tests (Tests 25-27)
   - Tests `_mulle_mmap_map()` with file handles
   - Tests `_mulle_mmap_map_range()` with handles
   - Tests external handle management
   - Tests error handling with invalid handles

4. **test_edge_cases.c** - Edge Cases and Alignment Tests (Tests 28-31)
   - Tests various unaligned offsets
   - Tests different length parameters including edge values
   - Tests boundary conditions (offset beyond file, length extending beyond file)
   - Tests page boundary alignments and data alignment

### Test Data Files

- **empty.txt** - 0 bytes (empty file)
- **small.txt** - 84 bytes (small text file)
- **medium.txt** - 4096 bytes (one page, random data)
- **large.txt** - 16384 bytes (four pages, random data)  
- **binary.dat** - 512 bytes (binary data, random)

All data files use random data from `/dev/urandom` to avoid OS optimizations for zero-filled pages.

## Test Coverage

This test suite covers:

### Basic File Mapping (✅ Complete)
- [x] `_mulle_mmap_map_file()` with various file sizes
- [x] Empty file mapping
- [x] Small file mapping (< page size)
- [x] Medium file mapping (= page size) 
- [x] Large file mapping (> page size)
- [x] Binary file mapping

### Range Mapping (✅ Complete)
- [x] `_mulle_mmap_map_file_range()` with different offsets/lengths
- [x] Mapping entire file with explicit range
- [x] Mapping specific byte ranges
- [x] Mapping with SIZE_MAX length
- [x] Error cases (offset beyond file, etc.)

### Handle-Based Mapping (✅ Complete)
- [x] `_mulle_mmap_map()` with file handles
- [x] `_mulle_mmap_map_range()` with handles
- [x] External handle management
- [x] Invalid handle error handling

### Edge Cases and Alignment (✅ Complete)
- [x] Unaligned offsets (1, 3, 17, 63, 127, 255, 511, 1023, etc.)
- [x] Various length parameters (1-8191 bytes)
- [x] Boundary conditions (end of file, beyond file)
- [x] Page boundary alignments
- [x] Offset/length combinations

### State Verification (✅ Complete)
- [x] `_mulle_mmap_is_open()`
- [x] `_mulle_mmap_is_mapped()` 
- [x] `_mulle_mmap_get_length()`
- [x] `_mulle_mmap_get_mapped_length()`
- [x] `_mulle_mmap_get_mapping_offset()`
- [x] `_mulle_mmap_get_data()`
- [x] `_mulle_mmap_get_file_handle()`

## Running Tests

To run individual test files:
```bash
mulle-sde test run test.c
mulle-sde test run test_range.c  
mulle-sde test run test_handle.c
mulle-sde test run test_edge_cases.c
```

To run all file mapping tests:
```bash
mulle-sde test run 20_file_mapping/
```

## Expected Behavior

All tests should pass and demonstrate:

1. **Correct Memory Mapping**: Files are properly mapped into memory with valid pointers
2. **State Management**: All state query functions return expected values
3. **Error Handling**: Invalid operations fail gracefully with appropriate error codes
4. **Resource Cleanup**: All resources are properly released after use
5. **Alignment Handling**: Unaligned offsets are handled correctly by the OS/implementation
6. **Boundary Conditions**: Edge cases are handled without crashes or undefined behavior

## Implementation Notes

- Tests use random data to avoid OS optimizations for zero-filled pages
- Page size is dynamically determined using `mulle_mmap_get_system_pagesize()`
- Tests verify both successful operations and expected failures
- Memory alignment and mapping offsets are checked where relevant
- File handle management is tested for both internal and external handles

These tests provide comprehensive coverage of the file mapping functionality and serve as regression tests for future development.
