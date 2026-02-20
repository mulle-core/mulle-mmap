#include <mulle-mmap/mulle-mmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
# include <windows.h>

static int child_process( HANDLE hMap)
{
   volatile unsigned char *byte_ptr;
   size_t page_size = mulle_mmap_get_system_pagesize();
   void *p;
   
   printf( "  CHILD: Accessing inherited shared memory handle...\n");
   
   // Map the inherited handle
   p = MapViewOfFile( hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
   if( p == NULL)
   {
      fprintf( stderr, "  CHILD: MapViewOfFile failed (%lu)\n", GetLastError());
      return( 1);
   }
   
   byte_ptr = (volatile unsigned char *) p;
   
   printf( "  CHILD: Checking shared memory content\n");
   
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
      UnmapViewOfFile( p);
      return( 1);
   }
   
   byte_ptr[2] = 0xAB;
   byte_ptr[3] = 0xCD;
   byte_ptr[page_size / 2] = 0xEF;
   
   printf( "  CHILD: Modified shared memory (added 0xAB, 0xCD, 0xEF)\n");
   printf( "  CHILD: Exiting\n");
   
   UnmapViewOfFile( p);
   
   return( 0);
}

#else
# include <unistd.h>
# include <sys/wait.h>
#endif


static void test_shared_page_allocation( void)
{
   struct mulle_mmap_shared_memory shared_pages;
   size_t page_size;
   size_t i;
   int    rval;

   printf( "=== Test: Shared Page Allocation ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   printf( "System page size: %zu bytes\n", page_size);
   
   printf( "Phase 1: Allocating shared page\n");
   
   shared_pages = mulle_mmap_alloc_shared_memory( page_size);
   if( ! shared_pages.address)
   {
      printf( "ERROR: Failed to allocate shared page: %s\n", strerror( errno));
      return;
   }
   
   printf( "  SUCCESS: Allocated shared %zu bytes\n", page_size);
   
   printf( "Phase 2: Testing zero-filled guarantee\n");
   
   unsigned char *byte_ptr = (unsigned char *) shared_pages.address;
   for( i = 0; i < page_size; i++)
   {
      if( byte_ptr[i] != 0)
      {
         printf( "  ERROR: Shared page not zero-filled at offset %zu (found 0x%02x)\n", i, byte_ptr[i]);
         mulle_mmap_free_shared_memory( &shared_pages);
         return;
      }
   }
   
   printf( "  SUCCESS: All %zu bytes are zero-filled\n", page_size);
   
   printf( "Phase 3: Testing write access\n");
   
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
   
   rval = mulle_mmap_free_shared_memory( &shared_pages);
   if( rval != 0)
   {
      printf( "  ERROR: Failed to free shared page: %s\n", strerror( errno));
      return;
   }
   
   printf( "  SUCCESS: Shared page freed successfully\n");
   printf( "Test completed successfully\n\n");
}



#ifdef _WIN32
static void* windows_create_shared_memory( size_t page_size, struct mulle_mmap_shared_memory *out_pages)
{
   *out_pages = mulle_mmap_alloc_shared_memory( page_size);
   if( ! out_pages->address)
   {
      fprintf( stderr, "ERROR: Failed to allocate shared pages\n");
      return( NULL);
   }
   return( out_pages->address);
}

static int windows_launch_child_and_wait( HANDLE hMap)
{
   STARTUPINFOA si = { sizeof(si) };
   PROCESS_INFORMATION pi = { 0 };
   char cmdLine[512];
   DWORD exitCode;
   
   GetModuleFileNameA( NULL, cmdLine, sizeof(cmdLine));
   // Pass the handle value as a command-line argument
   sprintf( cmdLine + strlen(cmdLine), " --child %p", (void*)hMap);
   
   // CRITICAL: bInheritHandles=TRUE so child inherits the shared memory handle
   if( ! CreateProcessA( NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
   {
      fprintf( stderr, "ERROR: CreateProcess failed (%lu)\n", GetLastError());
      return( -1);
   }
   
   printf( "  PARENT: Waiting for child\n");
   
   WaitForSingleObject( pi.hProcess, INFINITE);
   GetExitCodeProcess( pi.hProcess, &exitCode);
   
   CloseHandle( pi.hProcess);
   CloseHandle( pi.hThread);
   
   return( (int)exitCode);
}

static void windows_cleanup_shared_memory( struct mulle_mmap_shared_memory *pages)
{
   mulle_mmap_free_shared_memory( pages);
}
#endif


#ifndef _WIN32
static void* unix_create_shared_memory( size_t page_size, struct mulle_mmap_shared_memory *out_pages)
{
   *out_pages = mulle_mmap_alloc_shared_memory( page_size);
   if( ! out_pages->address)
   {
      printf( "ERROR: Failed to allocate shared page: %s\n", strerror( errno));
      return( NULL);
   }
   return( mulle_mmap_shared_memory_get_address( out_pages));
}

static int unix_fork_child_and_wait( volatile unsigned char *byte_ptr, size_t page_size)
{
   pid_t pid = fork();
   int status;
   
   if( pid == -1)
   {
      printf( "ERROR: Failed to fork: %s\n", strerror( errno));
      return( -1);
   }
   
   if( pid == 0)
   {
      printf( "  CHILD: Checking shared memory content\n");
      
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
      
      byte_ptr[2] = 0xAB;
      byte_ptr[3] = 0xCD;
      byte_ptr[page_size / 2] = 0xEF;
      
      printf( "  CHILD: Modified shared memory (added 0xAB, 0xCD, 0xEF)\n");
      printf( "  CHILD: Exiting\n");
      exit( 0);
   }
   
   printf( "  PARENT: Waiting for child\n");
   
   if( waitpid( pid, &status, 0) == -1)
   {
      printf( "  ERROR: Failed to wait for child: %s\n", strerror( errno));
      return( -1);
   }
   
   if( WIFEXITED( status))
      return( WEXITSTATUS( status));
   
   return( -1);
}

static void unix_cleanup_shared_memory( struct mulle_mmap_shared_memory *pages)
{
   mulle_mmap_free_shared_memory( pages);
}
#endif


static void test_shared_pages_parent_child( void)
{
   size_t   page_size;
   volatile unsigned char *byte_ptr;
   int      child_status;

   printf( "=== Test: Shared Pages Between Parent and Child Process ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   printf( "System page size: %zu bytes\n", page_size);
   
   printf( "Phase 1: Allocating shared page in parent\n");
   
#ifdef _WIN32
   struct mulle_mmap_shared_memory shared_pages;
   byte_ptr = (volatile unsigned char *) windows_create_shared_memory( page_size, &shared_pages);
   if( ! byte_ptr)
      return;
#else
   struct mulle_mmap_shared_memory shared_pages;
   byte_ptr = (volatile unsigned char *) unix_create_shared_memory( page_size, &shared_pages);
   if( ! byte_ptr)
      return;
#endif
   
   printf( "  SUCCESS: Allocated shared page\n");
   
   byte_ptr[0] = 0x12;
   byte_ptr[1] = 0x34;
   byte_ptr[page_size - 2] = 0x56;
   byte_ptr[page_size - 1] = 0x78;
   
   printf( "  Parent initialized shared memory with pattern: 0x12, 0x34, ..., 0x56, 0x78\n");
   
   printf( "Phase 2: Launching child process\n");
   
#ifdef _WIN32
   child_status = windows_launch_child_and_wait( mulle_mmap_shared_memory_get_handle( &shared_pages));
#else
   child_status = unix_fork_child_and_wait( byte_ptr, page_size);
#endif
   
   if( child_status == 0)
   {
      printf( "  PARENT: Child completed successfully\n");
   }
   else
   {
      printf( "  PARENT: Child failed (exit status: %d)\n", child_status);
#ifdef _WIN32
      windows_cleanup_shared_memory( &shared_pages);
#else
      unix_cleanup_shared_memory( &shared_pages);
#endif
      return;
   }
   
   printf( "Phase 3: Parent checking child modifications\n");
   
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
   
   printf( "Phase 4: Cleanup\n");
   
#ifdef _WIN32
   windows_cleanup_shared_memory( &shared_pages);
#else
   unix_cleanup_shared_memory( &shared_pages);
#endif
   
   printf( "  SUCCESS: Shared page freed\n");
   printf( "Test completed successfully\n\n");
}


static void test_multiple_shared_pages( void)
{
   struct mulle_mmap_shared_memory shared_pages;
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
   
   shared_pages = mulle_mmap_alloc_shared_memory( alloc_size);
   if( ! shared_pages.address)
   {
      printf( "ERROR: Failed to allocate shared pages: %s\n", strerror( errno));
      return;
   }
   
   printf( "  SUCCESS: Allocated %zu shared bytes\n", alloc_size);
   
   printf( "Phase 2: Testing zero-filled guarantee across all shared pages\n");
   
   unsigned char *byte_ptr = (unsigned char *) shared_pages.address;
   for( i = 0; i < alloc_size; i++)
   {
      if( byte_ptr[i] != 0)
      {
         printf( "  ERROR: Shared page not zero-filled at offset %zu (found 0x%02x)\n", i, byte_ptr[i]);
         mulle_mmap_free_shared_memory( &shared_pages);
         return;
      }
   }
   
   printf( "  SUCCESS: All %zu bytes across %zu shared pages are zero-filled\n", alloc_size, num_pages);
   
   printf( "Phase 3: Testing write access across all shared pages\n");
   
   for( i = 0; i < num_pages; i++)
   {
      size_t page_offset = i * page_size;
      unsigned char pattern = 0x20 + i;
      
      byte_ptr[page_offset] = pattern;
      byte_ptr[page_offset + page_size / 2] = pattern + 1;
      byte_ptr[page_offset + page_size - 1] = pattern + 2;
   }
   
   for( i = 0; i < num_pages; i++)
   {
      size_t page_offset = i * page_size;
      unsigned char pattern = 0x20 + i;
      
      if( byte_ptr[page_offset] != pattern ||
          byte_ptr[page_offset + page_size / 2] != pattern + 1 ||
          byte_ptr[page_offset + page_size - 1] != pattern + 2)
      {
         printf( "  ERROR: Shared page %zu write/read verification failed\n", i);
         mulle_mmap_free_shared_memory( &shared_pages);
         return;
      }
   }
   
   printf( "  SUCCESS: All %zu shared pages are writable and readable\n", num_pages);
   
   printf( "Phase 4: Freeing shared pages\n");
   
   rval = mulle_mmap_free_shared_memory( &shared_pages);
   if( rval != 0)
   {
      printf( "  ERROR: Failed to free shared pages: %s\n", strerror( errno));
      return;
   }
   
   printf( "  SUCCESS: Shared pages freed successfully\n");
   printf( "Test completed successfully\n\n");
}


int main( int argc, char *argv[])
{
#ifdef _WIN32
   if( argc >= 3 && strcmp( argv[1], "--child") == 0)
   {
      // Parse handle value from command line
      HANDLE hMap;
      sscanf( argv[2], "%p", &hMap);
      return( child_process( hMap));
   }
#endif
   
   printf( "=== Shared Page Allocation Tests ===\n\n");
   printf( "These tests verify mulle_mmap_alloc_shared_memory() functionality:\n");
   printf( "1. Basic shared page allocation and deallocation\n");
   printf( "2. Shared memory between parent and child processes\n");
   printf( "3. Multiple shared page allocation\n\n");
   
   test_shared_page_allocation();
   test_shared_pages_parent_child();
   test_multiple_shared_pages();
   
   printf( "=== All Shared Page Allocation Tests Completed ===\n");
   return( 0);
}
