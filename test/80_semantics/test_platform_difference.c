#include <mulle-mmap/mulle-mmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
# include <windows.h>
#else
# include <unistd.h>
# include <sys/wait.h>
#endif


/* This test demonstrates THE KEY DIFFERENCE:
 * 
 * Scenario: Parent allocates shared memory, writes data, frees it,
 *           THEN spawns child to read it.
 * 
 * UNIX: WORKS - fork() duplicates the mapping before parent frees
 * WINDOWS: FAILS - handle is closed before CreateProcess(), child can't inherit
 */

int main( int argc, char *argv[])
{
   struct mulle_mmap_shared_memory pages;
   size_t                         page_size;
   char                           *data;
   
#ifdef _WIN32
   // Child mode on Windows
   if( argc >= 2 && strcmp( argv[1], "--child") == 0)
   {
      printf( "Child: This will never print because handle was closed!\n");
      return( 1);
   }
#endif
   
   printf( "=== Test: Free Before Spawning Child ===\n\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   
   printf( "Step 1: Allocate shared pages\n");
   pages = mulle_mmap_alloc_shared_memory( page_size);
   if( ! pages.address)
   {
      printf( "ERROR: Failed to allocate\n");
      return( 1);
   }
   
   data = (char *) pages.address;
   data[0] = 42;
   printf( "  Wrote: %d\n", data[0]);
   
#ifdef _WIN32
   printf( "\nStep 2: FREE the shared pages\n");
   mulle_mmap_free_shared_memory( &pages);
   printf( "  Freed!\n");
   
   printf( "\nStep 3: NOW spawn child to try reading\n");
   {
      STARTUPINFOA         si = { sizeof(si) };
      PROCESS_INFORMATION  pi = { 0 };
      char                 cmdLine[512];
      
      GetModuleFileNameA( NULL, cmdLine, sizeof(cmdLine));
      strcat( cmdLine, " --child");
      
      if( ! CreateProcessA( NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
      {
         printf( "  ERROR: CreateProcess failed\n");
         return( 1);
      }
      
      WaitForSingleObject( pi.hProcess, INFINITE);
      CloseHandle( pi.hProcess);
      CloseHandle( pi.hThread);
      
      printf( "\n✗ WINDOWS FAILS: Child couldn't access - handle was closed before CreateProcess!\n");
   }
#else
   printf( "\nStep 2: Fork child FIRST\n");
   {
      pid_t pid = fork();
      
      if( pid == 0)
      {
         // Child - try to read the data (parent hasn't freed yet)
         // If this works, child exits with 0, otherwise crashes or exits with 1
         if( data[0] == 42)
            _exit( 0);
         else
            _exit( 1);
      }
      
      // Parent - NOW free after fork
      printf( "\nStep 3: Parent frees AFTER fork\n");
      mulle_mmap_free_shared_memory( &pages);
      printf( "  Parent freed!\n");
      
      // Wait for child to finish
      int status;
      waitpid( pid, &status, 0);
      
      if( WIFEXITED( status) && WEXITSTATUS( status) == 0)
         printf( "\n✓ UNIX WORKS: Child could still access - fork() duplicated mapping before parent freed!\n");
      else
         printf( "\n✗ UNIX: Child failed to access memory\n");
   }
#endif
   
   printf( "\n=== Test Complete ===\n");
   return( 0);
}
