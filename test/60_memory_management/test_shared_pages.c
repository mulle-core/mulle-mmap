#include <mulle-mmap/mulle-mmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>


static void test_shared_page_allocation( void)
{
   void   *shared_pages;
   size_t page_size;
   size_t i;
   int    rval;

   printf( "=== Test: Shared Page Allocation ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   printf( "System page size: %zu bytes\n", page_size);
   
   printf( "Phase 1: Allocating shared page\n");
   
   // Allocate one shared page
   shared_pages = mulle_mmap_alloc_shared_pages( page_size);
   if( ! shared_pages)
   {
      printf( "ERROR: Failed to allocate shared page: %s\n", strerror( errno));
      return;
   }
   
   printf( "  SUCCESS: Allocated shared %zu bytes at %p\n", page_size, shared_pages);
   
   printf( "Phase 2: Testing zero-filled guarantee\n");
   
   // Verify pages are zero-filled
   unsigned char *byte_ptr = (unsigned char *) shared_pages;
   for( i = 0; i < page_size; i++)
   {
      if( byte_ptr[i] != 0)
      {
         printf( "  ERROR: Shared page not zero-filled at offset %zu (found 0x%02x)\n", i, byte_ptr[i]);
         mulle_mmap_free_pages( shared_pages, page_size);
         return;
      }
   }
   
   printf( "  SUCCESS: All %zu bytes are zero-filled\n", page_size);
   
   printf( "Phase 3: Testing write access\n");
   
   // Test write access
   byte_ptr[0] = 0xCA;
   byte_ptr[page_size - 1] = 0xFE;
   byte_ptr[page_size / 2] = 0xBA;
   byte_ptr[page_size / 4] = 0xBE;
   
   if( byte_ptr[0] == 0xCA && byte_ptr[page_size - 1] == 0xFE && 
       byte_ptr[page_size / 2] == 0xBA && byte_ptr[page_size / 4] == 0xBE)
   {
      printf( "  SUCCESS: Shared page is writable and readable\n");
   }
   else
   {
      printf( "  ERROR: Shared page write/read verification failed\n");
   }
   
   printf( "Phase 4: Freeing shared page\n");
   
   // Free the shared page (using regular free_pages)
   rval = mulle_mmap_free_pages( shared_pages, page_size);
   if( rval != 0)
   {
      printf( "  ERROR: Failed to free shared page: %s\n", strerror( errno));
      return;
   }
   
   printf( "  SUCCESS: Shared page freed successfully\n");
   printf( "Test completed successfully\n\n");
}


static void test_shared_pages_parent_child( void)
{
   void     *shared_pages;
   size_t   page_size;
   pid_t    pid;
   int      status;
   volatile unsigned char *byte_ptr;

   printf( "=== Test: Shared Pages Between Parent and Child Process ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   printf( "System page size: %zu bytes\n", page_size);
   
   printf( "Phase 1: Allocating shared page in parent\n");
   
   // Allocate shared page
   shared_pages = mulle_mmap_alloc_shared_pages( page_size);
   if( ! shared_pages)
   {
      printf( "ERROR: Failed to allocate shared page: %s\n", strerror( errno));
      return;
   }
   
   byte_ptr = (volatile unsigned char *) shared_pages;
   printf( "  SUCCESS: Allocated shared page at %p\n", shared_pages);
   
   // Initialize with known pattern in parent
   byte_ptr[0] = 0x12;
   byte_ptr[1] = 0x34;
   byte_ptr[page_size - 2] = 0x56;
   byte_ptr[page_size - 1] = 0x78;
   
   printf( "  Parent initialized shared memory with pattern: 0x12, 0x34, ..., 0x56, 0x78\n");
   
   printf( "Phase 2: Forking child process\n");
   
   // Fork child process
   pid = fork();
   if( pid == -1)
   {
      printf( "ERROR: Failed to fork: %s\n", strerror( errno));
      mulle_mmap_free_pages( shared_pages, page_size);
      return;
   }
   
   if( pid == 0)
   {
      // Child process
      printf( "  CHILD: Checking shared memory content\n");
      
      // Verify child can see parent's data
      if( byte_ptr[0] == 0x12 && byte_ptr[1] == 0x34 && 
          byte_ptr[page_size - 2] == 0x56 && byte_ptr[page_size - 1] == 0x78)
      {
         printf( "  CHILD: SUCCESS - Can read parent's data from shared memory\n");
      }
      else
      {
         printf( "  CHILD: ERROR - Cannot read parent's data correctly\n");
         printf( "    Got: 0x%02x, 0x%02x, ..., 0x%02x, 0x%02x\n", 
                 byte_ptr[0], byte_ptr[1], byte_ptr[page_size - 2], byte_ptr[page_size - 1]);
         exit( 1);
      }
      
      // Modify data in child
      byte_ptr[2] = 0xAB;
      byte_ptr[3] = 0xCD;
      byte_ptr[page_size / 2] = 0xEF;
      
      printf( "  CHILD: Modified shared memory (added 0xAB, 0xCD, 0xEF)\n");
      printf( "  CHILD: Exiting\n");
      exit( 0);
   }
   else
   {
      // Parent process
      printf( "  PARENT: Waiting for child (PID %d)\n", pid);
      
      // Wait for child to complete
      if( waitpid( pid, &status, 0) == -1)
      {
         printf( "  ERROR: Failed to wait for child: %s\n", strerror( errno));
         mulle_mmap_free_pages( shared_pages, page_size);
         return;
      }
      
      if( WIFEXITED( status) && WEXITSTATUS( status) == 0)
      {
         printf( "  PARENT: Child completed successfully\n");
      }
      else
      {
         printf( "  PARENT: Child failed (exit status: %d)\n", WEXITSTATUS( status));
         mulle_mmap_free_pages( shared_pages, page_size);
         return;
      }
      
      printf( "Phase 3: Parent checking child modifications\n");
      
      // Check if parent can see child's modifications
      if( byte_ptr[2] == 0xAB && byte_ptr[3] == 0xCD && byte_ptr[page_size / 2] == 0xEF)
      {
         printf( "  PARENT: SUCCESS - Can see child's modifications in shared memory\n");
         printf( "  Shared memory is working correctly between processes\n");
      }
      else
      {
         printf( "  PARENT: ERROR - Cannot see child's modifications\n");
         printf( "    Expected: 0xAB, 0xCD, 0xEF at positions 2, 3, %zu\n", page_size / 2);
         printf( "    Got: 0x%02x, 0x%02x, 0x%02x\n", 
                 byte_ptr[2], byte_ptr[3], byte_ptr[page_size / 2]);
      }
   }
   
   printf( "Phase 4: Cleanup\n");
   
   // Free shared page
   int rval = mulle_mmap_free_pages( shared_pages, page_size);
   if( rval == 0)
   {
      printf( "  SUCCESS: Shared page freed\n");
   }
   else
   {
      printf( "  ERROR: Failed to free shared page: %s\n", strerror( errno));
   }
   
   printf( "Test completed successfully\n\n");
}


static void test_multiple_shared_pages( void)
{
   void   *shared_pages;
   size_t page_size;
   size_t num_pages = 3;
   size_t alloc_size;
   size_t i;
   int    rval;

   printf( "=== Test: Multiple Shared Pages ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   alloc_size = page_size * num_pages;
   
   printf( "Allocating %zu shared pages (%zu bytes total)\n", num_pages, alloc_size);
   
   printf( "Phase 1: Allocating multiple shared pages\n");
   
   // Allocate multiple shared pages
   shared_pages = mulle_mmap_alloc_shared_pages( alloc_size);
   if( ! shared_pages)
   {
      printf( "ERROR: Failed to allocate shared pages: %s\n", strerror( errno));
      return;
   }
   
   printf( "  SUCCESS: Allocated %zu shared bytes at %p\n", alloc_size, shared_pages);
   
   printf( "Phase 2: Testing zero-filled guarantee across all shared pages\n");
   
   // Verify all pages are zero-filled
   unsigned char *byte_ptr = (unsigned char *) shared_pages;
   for( i = 0; i < alloc_size; i++)
   {
      if( byte_ptr[i] != 0)
      {
         printf( "  ERROR: Shared page not zero-filled at offset %zu (found 0x%02x)\n", i, byte_ptr[i]);
         mulle_mmap_free_pages( shared_pages, alloc_size);
         return;
      }
   }
   
   printf( "  SUCCESS: All %zu bytes across %zu shared pages are zero-filled\n", alloc_size, num_pages);
   
   printf( "Phase 3: Testing write access across all shared pages\n");
   
   // Test write access on each shared page
   for( i = 0; i < num_pages; i++)
   {
      size_t page_offset = i * page_size;
      unsigned char pattern = 0x20 + i;  // Different pattern per page
      
      // Write pattern to start, middle, and end of each page
      byte_ptr[page_offset] = pattern;
      byte_ptr[page_offset + page_size / 2] = pattern + 1;
      byte_ptr[page_offset + page_size - 1] = pattern + 2;
   }
   
   // Verify patterns
   for( i = 0; i < num_pages; i++)
   {
      size_t page_offset = i * page_size;
      unsigned char pattern = 0x20 + i;
      
      if( byte_ptr[page_offset] != pattern ||
          byte_ptr[page_offset + page_size / 2] != pattern + 1 ||
          byte_ptr[page_offset + page_size - 1] != pattern + 2)
      {
         printf( "  ERROR: Shared page %zu write/read verification failed\n", i);
         mulle_mmap_free_pages( shared_pages, alloc_size);
         return;
      }
   }
   
   printf( "  SUCCESS: All %zu shared pages are writable and readable\n", num_pages);
   
   printf( "Phase 4: Freeing shared pages\n");
   
   // Free the shared pages
   rval = mulle_mmap_free_pages( shared_pages, alloc_size);
   if( rval != 0)
   {
      printf( "  ERROR: Failed to free shared pages: %s\n", strerror( errno));
      return;
   }
   
   printf( "  SUCCESS: Shared pages freed successfully\n");
   printf( "Test completed successfully\n\n");
}


int main( void)
{
   printf( "=== Shared Page Allocation Tests ===\n\n");
   printf( "These tests verify mulle_mmap_alloc_shared_pages() functionality:\n");
   printf( "1. Basic shared page allocation and deallocation\n");
   printf( "2. Shared memory between parent and child processes\n");
   printf( "3. Multiple shared page allocation\n\n");
   
   // Test 1: Basic shared page allocation
   test_shared_page_allocation();
   
   // Test 2: Shared memory between processes
   test_shared_pages_parent_child();
   
   // Test 3: Multiple shared pages
   test_multiple_shared_pages();
   
   printf( "=== All Shared Page Allocation Tests Completed ===\n");
   return( 0);
}
