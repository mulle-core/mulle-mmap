#include <mulle-mmap/mulle-mmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


static void test_free_null_pointer( void)
{
   int rval;

   printf( "=== Test: Freeing NULL Pointer ===\n\n");
   
   printf( "Phase 1: Attempting to free NULL pointer\n");
   
   // Try to free NULL pointer
   rval = mulle_mmap_free_pages( NULL, 4096);
   
   printf( "  mulle_mmap_free_pages(NULL, 4096) returned: %d\n", rval);
   
   if( rval == 0)
   {
      printf( "  SUCCESS: Function handled NULL pointer gracefully\n");
   }
   else
   {
      printf( "  ERROR: Function should return 0 for NULL pointer, got %d\n", rval);
      return;
   }
   
   printf( "Test completed\n\n");
}


static void test_free_wrong_size( void)
{
   void   *pages;
   size_t page_size;
   size_t alloc_size;
   int    rval;

   printf( "=== Test: Freeing With Wrong Size ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   alloc_size = page_size * 2;  // Allocate 2 pages
   
   printf( "System page size: %zu bytes\n", page_size);
   printf( "Allocating: %zu bytes (2 pages)\n", alloc_size);
   
   printf( "Phase 1: Allocating pages\n");
   
   // Allocate 2 pages
   pages = mulle_mmap_alloc_pages( alloc_size);
   if( ! pages)
   {
      printf( "ERROR: Failed to allocate pages: %s\n", strerror( errno));
      return;
   }
   
   printf( "  SUCCESS: Allocated %zu bytes\n", alloc_size);
   
   printf( "Phase 2: Attempting to free with wrong size\n");
   
   // Try to free with wrong size (1 page instead of 2)
   printf( "  Attempting to free %zu bytes (wrong size - should be %zu)\n", page_size, alloc_size);
   
   rval = mulle_mmap_free_pages( pages, page_size);
   
   printf( "  mulle_mmap_free_pages() with wrong size returned: %d\n", rval);
   
   if( rval != 0)
   {
      printf( "  INFO: Function detected wrong size (errno: %s)\n", strerror( errno));
      printf( "  This is good - the function is validating sizes\n");
      
      // Try to free with correct size
      printf( "  Attempting to free with correct size (%zu bytes)\n", alloc_size);
      rval = mulle_mmap_free_pages( pages, alloc_size);
      
      if( rval == 0)
      {
         printf( "  SUCCESS: Freed successfully with correct size\n");
      }
      else
      {
         printf( "  ERROR: Failed to free even with correct size: %s\n", strerror( errno));
      }
   }
   else
   {
      printf( "  INFO: Function accepted wrong size (implementation-dependent behavior)\n");
      printf( "  Pages may have been partially freed\n");
      
      // Attempt to free the remaining part (this might fail)
      printf( "  Attempting to free remaining part\n");
      char *remaining_ptr = (char *) pages + page_size;
      rval = mulle_mmap_free_pages( remaining_ptr, page_size);
      printf( "  Free remaining returned: %d\n", rval);
   }
   
   printf( "Test completed\n\n");
}


static void test_double_free( void)
{
   void   *pages;
   size_t page_size;
   int    rval1, rval2;

   printf( "=== Test: Double Free Scenarios ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   printf( "System page size: %zu bytes\n", page_size);
   
   printf( "Phase 1: Allocating page\n");
   
   // Allocate one page
   pages = mulle_mmap_alloc_pages( page_size);
   if( ! pages)
   {
      printf( "ERROR: Failed to allocate page: %s\n", strerror( errno));
      return;
   }
   
   printf( "  SUCCESS: Allocated %zu bytes\n", page_size);
   
   printf( "Phase 2: First free\n");
   
   // Free the page first time
   rval1 = mulle_mmap_free_pages( pages, page_size);
   
   printf( "  First free returned: %d\n", rval1);
   
   if( rval1 == 0)
   {
      printf( "  SUCCESS: First free succeeded\n");
   }
   else
   {
      printf( "  ERROR: First free failed: %s\n", strerror( errno));
      return;
   }
   
   printf( "Phase 3: Attempting double free\n");
   
   // Try to free the same page again
   rval2 = mulle_mmap_free_pages( pages, page_size);
   
   printf( "  Double free returned: %d\n", rval2);
   
   if( rval2 != 0)
   {
      printf( "  GOOD: Double free detected and rejected (errno: %s)\n", strerror( errno));
   }
   else
   {
      printf( "  WARNING: Double free was accepted - this could indicate a problem\n");
      printf( "  Some implementations may not detect double-free immediately\n");
   }
   
   printf( "Test completed\n\n");
}


static void test_zero_size_allocation( void)
{
   void *pages;

   printf( "=== Test: Zero Size Allocation ===\n\n");
   
   printf( "Phase 1: Attempting to allocate 0 bytes\n");
   
   // Try to allocate 0 bytes
   pages = mulle_mmap_alloc_pages( 0);
   
   if( pages == NULL)
   {
      printf( "  INFO: Zero-size allocation returned NULL (errno: %s)\n", strerror( errno));
      printf( "  This is typical behavior for zero-size allocations\n");
   }
   else
   {
      printf( "  INFO: Zero-size allocation returned pointer\n");
      printf( "  Some implementations may return a valid pointer for zero-size\n");
      
      // Try to free it
      int rval = mulle_mmap_free_pages( pages, 0);
      printf( "  Freeing zero-size allocation returned: %d\n", rval);
   }
   
   printf( "Test completed\n\n");
}


static void test_very_large_allocation( void)
{
   void   *pages;
   size_t huge_size = SIZE_MAX / 2;  // Try to allocate half of address space

   printf( "=== Test: Very Large Allocation ===\n\n");
   
   printf( "Phase 1: Attempting to allocate very large size\n");
   printf( "  Requesting: %zu bytes (SIZE_MAX/2)\n", huge_size);
   
   // Try to allocate unreasonably large amount
   pages = mulle_mmap_alloc_pages( huge_size);
   
   if( pages == NULL)
   {
      printf( "  GOOD: Very large allocation rejected (errno: %s)\n", strerror( errno));
      printf( "  System correctly enforced resource limits\n");
   }
   else
   {
      printf( "  UNEXPECTED: Very large allocation succeeded\n");
      printf( "  This is surprising - attempting to free\n");
      
      // Try to free it immediately without using it
      int rval = mulle_mmap_free_pages( pages, huge_size);
      printf( "  Free returned: %d\n", rval);
   }
   
   printf( "Test completed\n\n");
}


static void test_alignment_expectations( void)
{
   void   *pages;
   size_t page_size;
   size_t i;
   uintptr_t addr;

   printf( "=== Test: Memory Alignment Expectations ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   printf( "System page size: %zu bytes\n", page_size);
   
   printf( "Phase 1: Testing alignment of allocated pages\n");
   
   // Allocate several pages and check alignment
   for( i = 1; i <= 8; i++)
   {
      size_t alloc_size = page_size * i;
      
      pages = mulle_mmap_alloc_pages( alloc_size);
      if( ! pages)
      {
         printf( "  WARNING: Failed to allocate %zu pages: %s\n", i, strerror( errno));
         continue;
      }
      
      addr = (uintptr_t) pages;
      printf( "  %zu pages: ", i);
      
      if( addr % page_size == 0)
      {
         printf( "Page-aligned ✓\n");
      }
      else
      {
         printf( "NOT page-aligned (offset: %zu) ⚠\n", addr % page_size);
      }
      
      mulle_mmap_free_pages( pages, alloc_size);
   }
   
   printf( "Test completed\n\n");
}


static void test_concurrent_allocations( void)
{
   void   *pages[10];
   size_t page_size;
   size_t i;
   int    all_allocated = 1;
   int    all_freed = 1;

   printf( "=== Test: Concurrent Multiple Allocations ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   printf( "System page size: %zu bytes\n", page_size);
   
   printf( "Phase 1: Allocating multiple pages simultaneously\n");
   
   // Allocate multiple pages simultaneously
   for( i = 0; i < 10; i++)
   {
      size_t alloc_size = page_size * (i + 1);  // Different sizes
      
      pages[i] = mulle_mmap_alloc_pages( alloc_size);
      if( ! pages[i])
      {
         printf( "  WARNING: Failed to allocate chunk %zu: %s\n", i, strerror( errno));
         all_allocated = 0;
         break;
      }
      
      printf( "  Chunk %zu: %zu bytes\n", i, alloc_size);
      
      // Quick write test
      unsigned char *byte_ptr = (unsigned char *) pages[i];
      byte_ptr[0] = (unsigned char) (0x10 + i);
      byte_ptr[alloc_size - 1] = (unsigned char) (0x20 + i);
   }
   
   if( all_allocated)
   {
      printf( "  SUCCESS: All 10 chunks allocated successfully\n");
   }
   
   printf( "Phase 2: Verifying data integrity\n");
   
   // Verify all allocated chunks still have correct data
   for( size_t j = 0; j < i; j++)
   {
      if( pages[j])
      {
         size_t alloc_size = page_size * (j + 1);
         unsigned char *byte_ptr = (unsigned char *) pages[j];
         
         if( byte_ptr[0] == (unsigned char) (0x10 + j) && 
             byte_ptr[alloc_size - 1] == (unsigned char) (0x20 + j))
         {
            printf( "  Chunk %zu: Data integrity OK\n", j);
         }
         else
         {
            printf( "  ERROR: Chunk %zu: Data corruption detected\n", j);
         }
      }
   }
   
   printf( "Phase 3: Freeing all chunks\n");
   
   // Free all allocated chunks
   for( size_t j = 0; j < i; j++)
   {
      if( pages[j])
      {
         size_t alloc_size = page_size * (j + 1);
         int rval = mulle_mmap_free_pages( pages[j], alloc_size);
         
         if( rval == 0)
         {
            printf( "  Chunk %zu: Freed successfully\n", j);
         }
         else
         {
            printf( "  ERROR: Chunk %zu: Free failed: %s\n", j, strerror( errno));
            all_freed = 0;
         }
      }
   }
   
   if( all_freed)
   {
      printf( "  SUCCESS: All chunks freed successfully\n");
   }
   
   printf( "Test completed\n\n");
}


int main( void)
{
   printf( "=== Memory Management Edge Case Tests ===\n\n");
   printf( "These tests verify error handling and edge cases:\n");
   printf( "1. Freeing NULL pointer\n");
   printf( "2. Freeing with wrong size\n");
   printf( "3. Double-free scenarios\n");
   printf( "4. Zero-size allocation\n");
   printf( "5. Very large allocation\n");
   printf( "6. Memory alignment expectations\n");
   printf( "7. Concurrent multiple allocations\n\n");
   
   // Test 1: Free NULL
   test_free_null_pointer();
   
   // Test 2: Wrong size
   test_free_wrong_size();
   
   // Test 3: Double free
   test_double_free();
   
   // Test 4: Zero size
   test_zero_size_allocation();
   
   // Test 5: Very large size
   test_very_large_allocation();
   
   // Test 6: Alignment
   test_alignment_expectations();
   
   // Test 7: Concurrent allocations
   test_concurrent_allocations();
   
   printf( "=== All Memory Management Edge Case Tests Completed ===\n");
   printf( "Note: Some tests may show warnings or implementation-dependent behavior.\n");
   printf( "This is normal and helps understand the system's behavior under edge conditions.\n");
   
   return( 0);
}
