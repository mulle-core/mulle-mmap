#include <mulle-mmap/mulle-mmap.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>


static void test_readonly_file_mapping( void)
{
   char                 buffer[ 64];
   char                 original_content[ 64];
   char                 *filename;
   int                  fd;
   int                  rval;
   size_t               len;
   struct mulle_mmap    map;

   filename = "test_readonly.dat";

   printf( "=== Test: Read-only File Mapping ===\n\n");

   // Create test file with known content
   fd = open( filename, O_CREAT | O_RDWR, 0644);
   if( fd == -1)
   {
      printf( "ERROR: Failed to create test file: %s\n", strerror( errno));
      return;
   }

   strcpy( original_content, "Hello, readonly mapping test!");
   len = strlen( original_content) + 1;
   write( fd, original_content, len);
   close( fd);

   printf( "Created test file with content: '%s'\n", original_content);

   // Test 1: Read-only mapping
   printf( "\nPhase 1: Read-only mapping\n");

   mulle_mmap_init( &map, mulle_mmap_read);
   rval = mulle_mmap_map_file( &map, filename);
   if( rval)
   {
      printf( "ERROR: Failed to map file read-only: %s\n", strerror( errno));
      unlink( filename);
      return;
   }

   printf( "  SUCCESS: Mapped file read-only\n");
   printf( "  First byte: '%c' (0x%02x)\n", ((char *) map.data_)[ 0], ((char *) map.data_)[ 0]);
   printf( "  Mapped length: %zu bytes\n", map.length_);

   mulle_mmap_done( &map);

   printf( "  SUCCESS: Unmapped file\n");

   // Test 2: Read-write mapping
   printf( "\nPhase 2: Read-write mapping\n");

   mulle_mmap_init( &map, mulle_mmap_write);
   rval = mulle_mmap_map_file( &map, filename);
   if( rval)
   {
      printf( "ERROR: Failed to map file read-write: %s\n", strerror( errno));
      unlink( filename);
      return;
   }

   printf( "  SUCCESS: Mapped file read-write\n");
   printf( "  Original first byte: '%c' (0x%02x)\n", ((char *) map.data_)[ 0], ((char *) map.data_)[ 0]);

   // Write to read-write mapping
   printf( "  Writing to read-write mapping...\n");
   ((char *) map.data_)[ 0] = 'X';
   printf( "  Write succeeded\n");
   printf( "  First byte after write: '%c' (0x%02x)\n", ((char *) map.data_)[ 0], ((char *) map.data_)[ 0]);

   // Sync changes
   rval = mulle_mmap_sync( &map);
   if( rval)
   {
      printf( "WARNING: Sync failed: %s\n", strerror( errno));
   }

   mulle_mmap_done( &map);

   printf( "  SUCCESS: Unmapped file\n");

   // Check if file was modified
   printf( "\nPhase 3: Verifying file modification\n");

   fd = open( filename, O_RDONLY);
   if( fd != -1)
   {
      read( fd, buffer, sizeof( buffer));
      close( fd);
      printf( "  File content after read-write mapping: '%s'\n", buffer);
      printf( "  File was %smodified\n", buffer[ 0] == 'X' ? "" : "not ");
   }

   // Cleanup
   unlink( filename);

   printf( "\nTest completed\n\n");
}


int   main( void)
{
   printf( "Testing mulle-mmap file mapping behavior\n");
   printf( "========================================\n\n");

   test_readonly_file_mapping();

   printf( "All tests completed\n");

   return( 0);
}