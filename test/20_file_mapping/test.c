#include <mulle-mmap/mulle-mmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


static void test_map_file_basic( char *filename, char *description)
{
   struct mulle_mmap   info;
   char                *data;
   size_t              length;
   int                 rval;

   printf( "Testing %s (%s):\n", description, filename);

   // Initialize mmap structure for read-only access
   _mulle_mmap_init( &info, mulle_mmap_read);

   // Map the entire file
   rval = _mulle_mmap_map_file( &info, filename);
   if( rval)
   {
      printf( "  Failed to mmap file: %s\n", strerror( errno));
      _mulle_mmap_done( &info);
      return;
   }

   // Check if mapping was successful
   if( ! _mulle_mmap_is_open( &info))
   {
      printf( "  ERROR: mmap is not open after successful mapping\n");
      _mulle_mmap_done( &info);
      return;
   }

   // Check if mapped
   if( ! _mulle_mmap_is_mapped( &info))
   {
      printf( "  ERROR: mmap is not mapped after successful mapping\n");
      _mulle_mmap_done( &info);
      return;
   }

   // Get the mapped data and length
   data   = (char *) _mulle_mmap_get_bytes( &info);
   length = _mulle_mmap_get_length( &info);

   printf( "  File size      : %zu bytes\n", length);
   printf( "  Mapped length  : %zu bytes\n", _mulle_mmap_get_mapped_length( &info));
   printf( "  Mapping offset : %zu\n", _mulle_mmap_get_mapping_offset( &info));
   printf( "  Data pointer   : %s\n", (void *) data ? "YES" : "NULL");
   
   // Verify data pointer is not NULL for non-empty files
   if( length > 0 && data == NULL)
   {
      printf( "  ERROR: Data pointer is NULL for non-empty file\n");
   }
   else if( length == 0)
   {
      printf( "  Empty file - data pointer may be NULL\n");
   }
   else
   {
      printf( "  SUCCESS: Data pointer is valid\n");
   }

   // Test read access for small files
   if( length > 0 && length <= 100)
   {
      printf( "  First byte: 0x%02x\n", (unsigned char) data[0]);
      if( length > 1)
         printf( "  Last byte : 0x%02x\n", (unsigned char) data[length - 1]);
   }

   // Clean up
   _mulle_mmap_done( &info);
   printf( "  Test completed successfully\n\n");
}


static void test_map_nonexistent_file( void)
{
   struct mulle_mmap   info;
   int                 rval;

   printf( "Testing mapping non-existent file:\n");

   // Initialize mmap structure
   _mulle_mmap_init( &info, mulle_mmap_read);

   // Try to map non-existent file
   rval = _mulle_mmap_map_file( &info, "nonexistent_file_12345.txt");
   if( rval == 0)
   {
      printf( "  ERROR: Mapping non-existent file should have failed\n");
      _mulle_mmap_done( &info);
      return;
   }

   printf( "  SUCCESS: Mapping non-existent file failed as expected: %s\n", strerror( errno));

   // Verify state after failed mapping
   if( _mulle_mmap_is_open( &info))
   {
      printf( "  ERROR: mmap should not be open after failed mapping\n");
   }
   else
   {
      printf( "  SUCCESS: mmap is not open after failed mapping\n");
   }

   if( _mulle_mmap_is_mapped( &info))
   {
      printf( "  ERROR: mmap should not be mapped after failed mapping\n");
   }
   else
   {
      printf( "  SUCCESS: mmap is not mapped after failed mapping\n");
   }

   // Clean up
   _mulle_mmap_done( &info);
   printf( "  Test completed successfully\n\n");
}


int main( void)
{
   printf( "=== Basic File Mapping Tests ===\n\n");

   // Test mapping various file sizes
   test_map_file_basic( "empty.txt", "empty file (0 bytes)");
   test_map_file_basic( "small.txt", "small file (~100 bytes)");
   test_map_file_basic( "medium.txt", "medium file (4096 bytes)");
   test_map_file_basic( "large.txt", "large file (16384 bytes)");
   test_map_file_basic( "binary.dat", "binary file (512 bytes)");

   // Test error cases
   test_map_nonexistent_file();

   printf( "=== All Basic File Mapping Tests Completed ===\n");
   return( 0);
}
