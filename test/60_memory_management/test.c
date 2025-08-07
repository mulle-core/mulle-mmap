#include <mulle-mmap/mulle-mmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


static void test_system_pagesize( void)
{
   size_t page_size;

   printf( "=== Test: System Page Size Query ===\n\n");
   
   // Get system page size
   page_size = mulle_mmap_get_system_pagesize();
   
   printf( "System page size: %zu bytes\n", page_size);
   
   // Basic sanity checks
   if( page_size == 0)
   {
      printf( "ERROR: System page size is 0\n");
      return;
   }
   
   if( page_size & (page_size - 1))
   {
      printf( "WARNING: Page size %zu is not a power of 2\n", page_size);
   }
   else
   {
      printf( "SUCCESS: Page size is power of 2\n");
   }
   
   // Common page sizes
   if( page_size == 4096)
   {
      printf( "INFO: Standard 4KB page size\n");
   }
   else if( page_size == 8192)
   {
      printf( "INFO: 8KB page size\n");
   }
   else if( page_size == 65536)
   {
      printf( "INFO: 64KB page size (large pages)\n");
   }
   else
   {
      printf( "INFO: Non-standard page size: %zu bytes\n", page_size);
   }
   
   printf( "Test completed successfully\n\n");
}


static void test_basic_page_operations( void)
{
   void   *regular_pages;
   void   *shared_pages;
   size_t page_size;
   size_t alloc_size;
   int    rval;

   printf( "=== Test: Basic Page Operations ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   alloc_size = page_size * 2;  // 2 pages
   
   printf( "Testing with %zu pages (%zu bytes)\n", (size_t) 2, alloc_size);
   
   printf( "Phase 1: Regular page allocation\n");
   
   // Allocate regular pages
   regular_pages = mulle_mmap_alloc_pages( alloc_size);
   if( ! regular_pages)
   {
      printf( "ERROR: Failed to allocate regular pages: %s\n", strerror( errno));
      return;
   }
   
   printf( "  SUCCESS: Regular pages allocated at %p\n", regular_pages);
   
   printf( "Phase 2: Shared page allocation\n");
   
   // Allocate shared pages
   shared_pages = mulle_mmap_alloc_shared_pages( alloc_size);
   if( ! shared_pages)
   {
      printf( "ERROR: Failed to allocate shared pages: %s\n", strerror( errno));
      mulle_mmap_free_pages( regular_pages, alloc_size);
      return;
   }
   
   printf( "  SUCCESS: Shared pages allocated at %p\n", shared_pages);
   
   printf( "Phase 3: Testing zero-fill guarantee\n");
   
   // Check zero-fill for both types
   unsigned char *reg_ptr = (unsigned char *) regular_pages;
   unsigned char *shr_ptr = (unsigned char *) shared_pages;
   
   int reg_zero_filled = 1;
   int shr_zero_filled = 1;
   
   for( size_t i = 0; i < alloc_size; i++)
   {
      if( reg_ptr[i] != 0)
         reg_zero_filled = 0;
      if( shr_ptr[i] != 0)
         shr_zero_filled = 0;
   }
   
   if( reg_zero_filled)
   {
      printf( "  SUCCESS: Regular pages are zero-filled\n");
   }
   else
   {
      printf( "  ERROR: Regular pages not zero-filled\n");
   }
   
   if( shr_zero_filled)
   {
      printf( "  SUCCESS: Shared pages are zero-filled\n");
   }
   else
   {
      printf( "  ERROR: Shared pages not zero-filled\n");
   }
   
   printf( "Phase 4: Testing write access\n");
   
   // Write different patterns to each type
   reg_ptr[0] = 0xAA;
   reg_ptr[alloc_size - 1] = 0xBB;
   
   shr_ptr[0] = 0xCC;
   shr_ptr[alloc_size - 1] = 0xDD;
   
   // Verify writes
   if( reg_ptr[0] == 0xAA && reg_ptr[alloc_size - 1] == 0xBB)
   {
      printf( "  SUCCESS: Regular pages are writable\n");
   }
   else
   {
      printf( "  ERROR: Regular pages write test failed\n");
   }
   
   if( shr_ptr[0] == 0xCC && shr_ptr[alloc_size - 1] == 0xDD)
   {
      printf( "  SUCCESS: Shared pages are writable\n");
   }
   else
   {
      printf( "  ERROR: Shared pages write test failed\n");
   }
   
   printf( "Phase 5: Cleanup\n");
   
   // Free regular pages
   rval = mulle_mmap_free_pages( regular_pages, alloc_size);
   if( rval == 0)
   {
      printf( "  SUCCESS: Regular pages freed\n");
   }
   else
   {
      printf( "  ERROR: Failed to free regular pages: %s\n", strerror( errno));
   }
   
   // Free shared pages
   rval = mulle_mmap_free_pages( shared_pages, alloc_size);
   if( rval == 0)
   {
      printf( "  SUCCESS: Shared pages freed\n");
   }
   else
   {
      printf( "  ERROR: Failed to free shared pages: %s\n", strerror( errno));
   }
   
   printf( "Test completed successfully\n\n");
}


static void test_page_size_variations( void)
{
   size_t page_size;
   size_t test_multipliers[] = {1, 2, 4, 8, 16};
   size_t num_tests = sizeof(test_multipliers) / sizeof(test_multipliers[0]);
   size_t i;

   printf( "=== Test: Page Size Variations ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   printf( "System page size: %zu bytes\n\n", page_size);
   
   for( i = 0; i < num_tests; i++)
   {
      void   *reg_pages, *shr_pages;
      size_t alloc_size = page_size * test_multipliers[i];
      
      printf( "Test %zu: %zu pages (%zu bytes)\n", i + 1, test_multipliers[i], alloc_size);
      
      // Test regular pages
      reg_pages = mulle_mmap_alloc_pages( alloc_size);
      if( reg_pages)
      {
         printf( "  Regular pages: SUCCESS at %p\n", reg_pages);
         mulle_mmap_free_pages( reg_pages, alloc_size);
      }
      else
      {
         printf( "  Regular pages: FAILED (%s)\n", strerror( errno));
      }
      
      // Test shared pages
      shr_pages = mulle_mmap_alloc_shared_pages( alloc_size);
      if( shr_pages)
      {
         printf( "  Shared pages: SUCCESS at %p\n", shr_pages);
         mulle_mmap_free_pages( shr_pages, alloc_size);
      }
      else
      {
         printf( "  Shared pages: FAILED (%s)\n", strerror( errno));
      }
      
      printf( "\n");
   }
   
   printf( "Page size variation tests completed\n\n");
}


static void test_data_integrity_patterns( void)
{
   void   *pages;
   size_t page_size;
   size_t alloc_size;
   size_t i;
   unsigned char *byte_ptr;

   printf( "=== Test: Data Integrity Patterns ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   alloc_size = page_size * 4;  // 4 pages for pattern testing
   
   printf( "Allocating %zu pages for pattern testing\n", (size_t) 4);
   
   // Allocate pages
   pages = mulle_mmap_alloc_pages( alloc_size);
   if( ! pages)
   {
      printf( "ERROR: Failed to allocate pages: %s\n", strerror( errno));
      return;
   }
   
   byte_ptr = (unsigned char *) pages;
   printf( "  Allocated %zu bytes at %p\n", alloc_size, pages);
   
   printf( "Phase 1: Writing test patterns\n");
   
   // Write different patterns to each page
   for( i = 0; i < 4; i++)
   {
      size_t page_start = i * page_size;
      unsigned char pattern = 0x11 * (i + 1);  // 0x11, 0x22, 0x33, 0x44
      
      // Fill entire page with pattern
      memset( byte_ptr + page_start, pattern, page_size);
      
      printf( "  Page %zu: filled with pattern 0x%02x\n", i, pattern);
   }
   
   printf( "Phase 2: Verifying patterns\n");
   
   // Verify patterns
   int all_correct = 1;
   for( i = 0; i < 4; i++)
   {
      size_t page_start = i * page_size;
      unsigned char expected_pattern = 0x11 * (i + 1);
      int page_correct = 1;
      
      // Check a few strategic locations in each page
      size_t check_offsets[] = {0, 1, page_size/4, page_size/2, page_size - 2, page_size - 1};
      size_t num_checks = sizeof(check_offsets) / sizeof(check_offsets[0]);
      
      for( size_t j = 0; j < num_checks; j++)
      {
         if( byte_ptr[page_start + check_offsets[j]] != expected_pattern)
         {
            page_correct = 0;
            all_correct = 0;
            printf( "  ERROR: Page %zu pattern corruption at offset %zu\n", i, check_offsets[j]);
            break;
         }
      }
      
      if( page_correct)
      {
         printf( "  Page %zu: Pattern integrity OK\n", i);
      }
   }
   
   if( all_correct)
   {
      printf( "  SUCCESS: All patterns verified correctly\n");
   }
   
   printf( "Phase 3: Cross-page boundary test\n");
   
   // Test writing across page boundaries
   size_t boundary_offset = page_size - 2;
   uint32_t boundary_value = 0xDEADBEEF;
   
   *(uint32_t *)(byte_ptr + boundary_offset) = boundary_value;
   
   if( *(uint32_t *)(byte_ptr + boundary_offset) == boundary_value)
   {
      printf( "  SUCCESS: Cross-page boundary write works\n");
   }
   else
   {
      printf( "  ERROR: Cross-page boundary write failed\n");
   }
   
   printf( "Phase 4: Cleanup\n");
   
   // Free pages
   int rval = mulle_mmap_free_pages( pages, alloc_size);
   if( rval == 0)
   {
      printf( "  SUCCESS: Pages freed\n");
   }
   else
   {
      printf( "  ERROR: Failed to free pages: %s\n", strerror( errno));
   }
   
   printf( "Test completed successfully\n\n");
}


int main( void)
{
   printf( "=== Comprehensive Memory Management Tests ===\n\n");
   printf( "These tests verify the complete memory management functionality:\n");
   printf( "1. System page size query\n");
   printf( "2. Basic page operations (regular and shared)\n");
   printf( "3. Various page size allocations\n");
   printf( "4. Data integrity patterns\n\n");
   
   // Test 1: System page size
   test_system_pagesize();
   
   // Test 2: Basic operations
   test_basic_page_operations();
   
   // Test 3: Size variations
   test_page_size_variations();
   
   // Test 4: Data integrity
   test_data_integrity_patterns();
   
   printf( "=== All Comprehensive Memory Management Tests Completed ===\n");
   printf( "This completes the basic memory management functionality testing.\n");
   printf( "For edge cases and error conditions, run the separate edge case tests.\n");
   
   return( 0);
}
