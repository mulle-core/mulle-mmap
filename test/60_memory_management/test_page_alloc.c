#include <mulle-mmap/mulle-mmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


static void test_single_page_allocation( void)
{
   void   *pages;
   size_t page_size;
   size_t i;
   int    rval;

   printf( "=== Test: Single Page Allocation ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   printf( "System page size: %zu bytes\n", page_size);
   
   printf( "Phase 1: Allocating single page\n");
   
   // Allocate one page
   pages = mulle_mmap_alloc_pages( page_size);
   if( ! pages)
   {
      printf( "ERROR: Failed to allocate page: %s\n", strerror( errno));
      return;
   }
   
   printf( "  SUCCESS: Allocated %zu bytes\n", page_size);
   
   printf( "Phase 2: Testing zero-filled guarantee\n");
   
   // Verify pages are zero-filled
   unsigned char *byte_ptr = (unsigned char *) pages;
   for( i = 0; i < page_size; i++)
   {
      if( byte_ptr[i] != 0)
      {
         printf( "  ERROR: Page not zero-filled at offset %zu (found 0x%02x)\n", i, byte_ptr[i]);
         mulle_mmap_free_pages( pages, page_size);
         return;
      }
   }
   
   printf( "  SUCCESS: All %zu bytes are zero-filled\n", page_size);
   
   printf( "Phase 3: Testing write access\n");
   
   // Test write access
   byte_ptr[0] = 0xDE;
   byte_ptr[page_size - 1] = 0xAD;
   byte_ptr[page_size / 2] = 0xBE;
   byte_ptr[page_size / 4] = 0xEF;
   
   if( byte_ptr[0] == 0xDE && byte_ptr[page_size - 1] == 0xAD && 
       byte_ptr[page_size / 2] == 0xBE && byte_ptr[page_size / 4] == 0xEF)
   {
      printf( "  SUCCESS: Page is writable and readable\n");
   }
   else
   {
      printf( "  ERROR: Page write/read verification failed\n");
   }
   
   printf( "Phase 4: Freeing page\n");
   
   // Free the page
   rval = mulle_mmap_free_pages( pages, page_size);
   if( rval != 0)
   {
      printf( "  ERROR: Failed to free page: %s\n", strerror( errno));
      return;
   }
   
   printf( "  SUCCESS: Page freed successfully\n");
   printf( "Test completed successfully\n\n");
}


static void test_multiple_page_allocation( void)
{
   void   *pages;
   size_t page_size;
   size_t alloc_size;
   size_t num_pages;
   size_t i, j;
   int    rval;

   printf( "=== Test: Multiple Page Allocation ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   num_pages = 4;  // Allocate 4 pages
   alloc_size = page_size * num_pages;
   
   printf( "Allocating %zu pages (%zu bytes total)\n", num_pages, alloc_size);
   
   printf( "Phase 1: Allocating multiple pages\n");
   
   // Allocate multiple pages
   pages = mulle_mmap_alloc_pages( alloc_size);
   if( ! pages)
   {
      printf( "ERROR: Failed to allocate pages: %s\n", strerror( errno));
      return;
   }
   
   printf( "  SUCCESS: Allocated %zu bytes\n", alloc_size);
   
   printf( "Phase 2: Testing zero-filled guarantee across all pages\n");
   
   // Verify all pages are zero-filled
   unsigned char *byte_ptr = (unsigned char *) pages;
   for( i = 0; i < alloc_size; i++)
   {
      if( byte_ptr[i] != 0)
      {
         printf( "  ERROR: Page not zero-filled at offset %zu (found 0x%02x)\n", i, byte_ptr[i]);
         mulle_mmap_free_pages( pages, alloc_size);
         return;
      }
   }
   
   printf( "  SUCCESS: All %zu bytes across %zu pages are zero-filled\n", alloc_size, num_pages);
   
   printf( "Phase 3: Testing write access across all pages\n");
   
   // Test write access on each page
   for( i = 0; i < num_pages; i++)
   {
      size_t page_offset = i * page_size;
      unsigned char pattern = 0x10 + i;  // Different pattern per page
      
      // Write pattern to start, middle, and end of each page
      byte_ptr[page_offset] = pattern;
      byte_ptr[page_offset + page_size / 2] = pattern + 1;
      byte_ptr[page_offset + page_size - 1] = pattern + 2;
   }
   
   // Verify patterns
   for( i = 0; i < num_pages; i++)
   {
      size_t page_offset = i * page_size;
      unsigned char pattern = 0x10 + i;
      
      if( byte_ptr[page_offset] != pattern ||
          byte_ptr[page_offset + page_size / 2] != pattern + 1 ||
          byte_ptr[page_offset + page_size - 1] != pattern + 2)
      {
         printf( "  ERROR: Page %zu write/read verification failed\n", i);
         mulle_mmap_free_pages( pages, alloc_size);
         return;
      }
   }
   
   printf( "  SUCCESS: All %zu pages are writable and readable\n", num_pages);
   
   printf( "Phase 4: Freeing pages\n");
   
   // Free the pages
   rval = mulle_mmap_free_pages( pages, alloc_size);
   if( rval != 0)
   {
      printf( "  ERROR: Failed to free pages: %s\n", strerror( errno));
      return;
   }
   
   printf( "  SUCCESS: Pages freed successfully\n");
   printf( "Test completed successfully\n\n");
}


static void test_various_page_sizes( void)
{
   size_t page_size;
   size_t test_sizes[] = {1, 2, 3, 4, 8, 16, 32};
   size_t num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);
   size_t i;

   printf( "=== Test: Various Page Size Multiples ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   printf( "System page size: %zu bytes\n\n", page_size);
   
   for( i = 0; i < num_tests; i++)
   {
      void   *pages;
      size_t alloc_size = page_size * test_sizes[i];
      int    rval;
      
      printf( "Test %zu: Allocating %zu pages (%zu bytes)\n", i + 1, test_sizes[i], alloc_size);
      
      // Allocate
      pages = mulle_mmap_alloc_pages( alloc_size);
      if( ! pages)
      {
         printf( "  ERROR: Failed to allocate %zu bytes: %s\n", alloc_size, strerror( errno));
         continue;
      }
      
      printf( "  SUCCESS: Allocated %zu bytes\n", alloc_size);
      
      // Quick zero-fill check (just first and last bytes)
      unsigned char *byte_ptr = (unsigned char *) pages;
      if( byte_ptr[0] != 0 || byte_ptr[alloc_size - 1] != 0)
      {
         printf( "  ERROR: Pages not zero-filled\n");
      }
      else
      {
         printf( "  SUCCESS: Pages are zero-filled\n");
      }
      
      // Quick write test
      byte_ptr[0] = 0xAA;
      byte_ptr[alloc_size - 1] = 0xBB;
      
      if( byte_ptr[0] == 0xAA && byte_ptr[alloc_size - 1] == 0xBB)
      {
         printf( "  SUCCESS: Pages are writable\n");
      }
      else
      {
         printf( "  ERROR: Page write test failed\n");
      }
      
      // Free
      rval = mulle_mmap_free_pages( pages, alloc_size);
      if( rval == 0)
      {
         printf( "  SUCCESS: Pages freed successfully\n");
      }
      else
      {
         printf( "  ERROR: Failed to free pages: %s\n", strerror( errno));
      }
      
      printf( "\n");
   }
   
   printf( "Various page size tests completed\n\n");
}


static void test_non_page_aligned_sizes( void)
{
   size_t page_size;
   size_t test_sizes[] = {100, 1000, 5000, 10000};
   size_t num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);
   size_t i;

   printf( "=== Test: Non-Page-Aligned Allocation Sizes ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   printf( "System page size: %zu bytes\n", page_size);
   printf( "Testing allocation of non-page-aligned sizes\n\n");
   
   for( i = 0; i < num_tests; i++)
   {
      void   *pages;
      size_t requested_size = test_sizes[i];
      int    rval;
      
      printf( "Test %zu: Requesting %zu bytes (non-page-aligned)\n", i + 1, requested_size);
      
      // Allocate non-page-aligned size
      pages = mulle_mmap_alloc_pages( requested_size);
      if( ! pages)
      {
         printf( "  INFO: Allocation failed for non-page-aligned size: %s\n", strerror( errno));
         printf( "  (This may be expected behavior)\n");
      }
      else
      {
         printf( "  SUCCESS: Allocated %zu bytes\n", requested_size);
         
         // Test basic functionality
         unsigned char *byte_ptr = (unsigned char *) pages;
         
         // Test zero-fill for requested size
         size_t j;
         int zero_filled = 1;
         for( j = 0; j < requested_size; j++)
         {
            if( byte_ptr[j] != 0)
            {
               zero_filled = 0;
               break;
            }
         }
         
         if( zero_filled)
         {
            printf( "  SUCCESS: Requested area is zero-filled\n");
         }
         else
         {
            printf( "  WARNING: Requested area not zero-filled\n");
         }
         
         // Test write
         byte_ptr[0] = 0xCC;
         byte_ptr[requested_size - 1] = 0xDD;
         
         if( byte_ptr[0] == 0xCC && byte_ptr[requested_size - 1] == 0xDD)
         {
            printf( "  SUCCESS: Requested area is writable\n");
         }
         else
         {
            printf( "  ERROR: Write test failed\n");
         }
         
         // Free with requested size
         rval = mulle_mmap_free_pages( pages, requested_size);
         if( rval == 0)
         {
            printf( "  SUCCESS: Pages freed successfully\n");
         }
         else
         {
            printf( "  WARNING: Free may have failed: %s\n", strerror( errno));
         }
      }
      
      printf( "\n");
   }
   
   printf( "Non-page-aligned size tests completed\n\n");
}


int main( void)
{
   printf( "=== Page Allocation Tests ===\n\n");
   printf( "These tests verify mulle_mmap_alloc_pages() functionality:\n");
   printf( "1. Single page allocation and deallocation\n");
   printf( "2. Multiple page allocation and deallocation\n");
   printf( "3. Various page size multiples\n");
   printf( "4. Non-page-aligned allocation sizes\n\n");
   
   // Test 1: Single page
   test_single_page_allocation();
   
   // Test 2: Multiple pages
   test_multiple_page_allocation();
   
   // Test 3: Various page sizes
   test_various_page_sizes();
   
   // Test 4: Non-page-aligned sizes
   test_non_page_aligned_sizes();
   
   printf( "=== All Page Allocation Tests Completed ===\n");
   return( 0);
}
