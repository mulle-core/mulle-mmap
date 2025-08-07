# Memory Management Tests (Section 5)

This directory contains comprehensive tests for the memory management functionality of mulle-mmap, implementing section 5 of the TEST_IMPLEMENTATION.md plan.

## Test Files

### Core Test Files

1. **test.c** - Comprehensive Memory Management Tests (Tests 60-64)
   - System page size query (`mulle_mmap_get_system_pagesize()`)
   - Basic page operations (regular and shared pages)
   - Various page size allocations
   - Data integrity pattern testing
   - Cross-page boundary validation

2. **test_page_alloc.c** - Page Allocation Tests (Tests 60-64)
   - Single page allocation with `mulle_mmap_alloc_pages()`
   - Multiple page allocation and deallocation
   - Various page size multiples (1, 2, 3, 4, 8, 16, 32 pages)
   - Non-page-aligned allocation sizes
   - Zero-fill guarantee verification
   - Write access validation

3. **test_shared_pages.c** - Shared Page Tests (Test 62)
   - Basic shared page allocation with `mulle_mmap_alloc_shared_pages()`
   - Parent-child process shared memory communication
   - Multiple shared page allocation
   - Inter-process data sharing validation
   - Shared memory persistence across process boundaries

4. **test_edge_cases.c** - Edge Cases and Error Handling (Tests 65-67)
   - Freeing NULL pointer
   - Freeing with wrong size
   - Double-free scenarios
   - Zero-size allocation attempts
   - Very large allocation attempts
   - Memory alignment expectations
   - Concurrent multiple allocations

## Test Coverage

This test suite covers:

### Basic Memory Management (✅ Complete)
- [x] `mulle_mmap_alloc_pages()` - Regular page allocation
- [x] `mulle_mmap_alloc_shared_pages()` - Shared page allocation  
- [x] `mulle_mmap_free_pages()` - Page deallocation
- [x] `mulle_mmap_get_system_pagesize()` - System page size query
- [x] Zero-fill guarantee verification
- [x] Page alignment validation

### Page Size Variations (✅ Complete)
- [x] Single page allocation (4KB typical)
- [x] Multiple page allocation (2-32 pages)
- [x] Non-page-aligned sizes (100, 1000, 5000, 10000 bytes)
- [x] Various page multiples testing
- [x] Cross-page boundary writes

### Shared Memory (✅ Complete)  
- [x] Basic shared page allocation
- [x] Parent-child process communication
- [x] Inter-process data sharing
- [x] Shared memory persistence
- [x] Multiple shared page management

### Error Handling & Edge Cases (✅ Complete)
- [x] NULL pointer handling
- [x] Invalid size parameters
- [x] Double-free detection
- [x] Zero-size allocation behavior
- [x] Very large allocation limits
- [x] Memory alignment verification
- [x] Concurrent allocation patterns

### Data Integrity (✅ Complete)
- [x] Zero-fill guarantee on allocation
- [x] Write/read verification
- [x] Pattern integrity across pages
- [x] Cross-page boundary access
- [x] Data persistence validation

## Running Tests

To run individual test files:
```bash
mulle-sde test run test.c              # Comprehensive tests
mulle-sde test run test_page_alloc.c   # Page allocation tests  
mulle-sde test run test_shared_pages.c # Shared memory tests
mulle-sde test run test_edge_cases.c   # Edge cases and error handling
```

To run all memory management tests:
```bash
mulle-sde test run 60_memory_management/
```

## Expected Behavior

All tests should pass and demonstrate:

1. **Page Allocation**: Successful allocation of various page sizes
2. **Zero-Fill Guarantee**: All allocated pages are initialized to zero
3. **Write Access**: All allocated pages are writable and readable
4. **Proper Cleanup**: All pages can be freed without errors
5. **Shared Memory**: Inter-process communication works correctly
6. **Error Handling**: Invalid operations fail gracefully
7. **Alignment**: Page-aligned memory allocation
8. **Resource Management**: No memory leaks or resource corruption

## Key Features Tested

### Memory Allocation Functions
- **`mulle_mmap_alloc_pages(size)`** - Allocate anonymous pages
- **`mulle_mmap_alloc_shared_pages(size)`** - Allocate shared memory pages
- **`mulle_mmap_free_pages(ptr, size)`** - Free allocated pages
- **`mulle_mmap_get_system_pagesize()`** - Query system page size

### Critical Guarantees
- **Zero-Fill**: All allocated pages are guaranteed to be zero-filled
- **Alignment**: Pages are allocated at page-aligned addresses
- **Size Handling**: Both page-aligned and non-aligned sizes work
- **Shared Memory**: True inter-process shared memory functionality

### Error Conditions
- **NULL Safety**: Functions handle NULL pointers appropriately
- **Size Validation**: Invalid sizes are detected and rejected
- **Double-Free**: Attempts to free already-freed memory are handled
- **Resource Limits**: Very large allocations are rejected appropriately

## Implementation Notes

- Tests use the actual system page size via `mulle_mmap_get_system_pagesize()`
- Shared memory tests use `fork()` to create child processes for validation
- Edge case tests intentionally test boundary conditions and error scenarios
- All tests perform thorough cleanup to prevent resource leaks
- Tests are designed to work across different operating systems and page sizes

These tests provide comprehensive validation of mulle-mmap's memory management capabilities and serve as both functional tests and documentation of expected behavior.
