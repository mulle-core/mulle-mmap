#include <mulle-mmap/mulle-mmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


static void test_map_file_range( char *filename, size_t offset, size_t length, char *description)
{
   struct mulle_mmap   info;
   char                *data;
   size_t              actual_length;
   size_t              mapped_length;
   size_t              mapping_offset;
   int                 rval;

   printf( "Testing %s:\n", description);
   printf( "  File: %s, Offset: %zu, Length: %zu\n", filename, offset, length);

   // Initialize mmap structure for read-only access
   _mulle_mmap_init( &info, mulle_mmap_read);

   // Map the file range
   rval = _mulle_mmap_map_file_range( &info, filename, offset, length);
   if( rval)
   {
      printf( "  Failed to mmap file range: %s\n", strerror( errno));
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

   if( ! _mulle_mmap_is_mapped( &info))
   {
      printf( "  ERROR: mmap is not mapped after successful mapping\n");
      _mulle_mmap_done( &info);
      return;
   }

   // Get the mapped data and properties
   data            = (char *) _mulle_mmap_get_data( &info);
   actual_length   = _mulle_mmap_get_length( &info);
   mapped_length   = _mulle_mmap_get_mapped_length( &info);
   mapping_offset  = _mulle_mmap_get_mapping_offset( &info);

   printf( "  Actual length   : %zu bytes\n", actual_length);
   printf( "  Mapped length   : %zu bytes\n", mapped_length);
   printf( "  Mapping offset  : %zu\n", mapping_offset);
   printf( "  Data pointer    : %p\n", (void *) data);

   // Verify data pointer
   if( actual_length > 0 && data == NULL)
   {
      printf( "  ERROR: Data pointer is NULL for non-empty range\n");
   }
   else if( actual_length == 0)
   {
      printf( "  Empty range - data pointer may be NULL\n");
   }
   else
   {
      printf( "  SUCCESS: Data pointer is valid\n");
   }

   // Test read access for small ranges
   if( actual_length > 0 && actual_length <= 100)
   {
      printf( "  First byte: 0x%02x\n", (unsigned char) data[0]);
      if( actual_length > 1)
         printf( "  Last byte : 0x%02x\n", (unsigned char) data[actual_length - 1]);
   }

   // Clean up
   _mulle_mmap_done( &info);
   printf( "  Test completed successfully\n\n");
}


static void test_map_file_range_errors( void)
{
   struct mulle_mmap   info;
   int                 rval;

   printf( "Testing file range mapping error cases:\n");

   // Test 1: Invalid offset (beyond file size)
   printf( "  Test 1: Offset beyond file size\n");
   _mulle_mmap_init( &info, mulle_mmap_read);
   rval = _mulle_mmap_map_file_range( &info, "small.txt", 1000, 100);
   if( rval == 0)
   {
      printf( "    ERROR: Should have failed for offset beyond file size\n");
      _mulle_mmap_done( &info);
   }
   else
   {
      printf( "    SUCCESS: Failed as expected for offset beyond file size\n");
      _mulle_mmap_done( &info);
   }

   // Test 2: Zero length
   printf( "  Test 2: Zero length mapping\n");
   _mulle_mmap_init( &info, mulle_mmap_read);
   rval = _mulle_mmap_map_file_range( &info, "small.txt", 0, 0);
   if( rval == 0)
   {
      size_t length = _mulle_mmap_get_length( &info);
      printf( "    SUCCESS: Zero length mapping succeeded, actual length: %zu\n", length);
      _mulle_mmap_done( &info);
   }
   else
   {
      printf( "    INFO: Zero length mapping failed: %s\n", strerror( errno));
      _mulle_mmap_done( &info);
   }

   printf( "  Error case tests completed\n\n");
}


int main( void)
{
   printf( "=== File Range Mapping Tests ===\n\n");

   // Test mapping entire file with explicit range
   test_map_file_range( "small.txt", 0, SIZE_MAX, "entire file with SIZE_MAX length");
   
   // Test mapping specific ranges
   test_map_file_range( "medium.txt", 0, 1024, "first 1024 bytes of medium file");
   test_map_file_range( "medium.txt", 1024, 2048, "bytes 1024-3071 of medium file");
   test_map_file_range( "medium.txt", 2048, SIZE_MAX, "from byte 2048 to end of medium file");
   
   // Test mapping with various offsets
   test_map_file_range( "large.txt", 0, 4096, "first page of large file");
   test_map_file_range( "large.txt", 4096, 4096, "second page of large file");
   test_map_file_range( "large.txt", 8192, SIZE_MAX, "from byte 8192 to end of large file");

   // Test small ranges
   test_map_file_range( "binary.dat", 0, 256, "first half of binary file");
   test_map_file_range( "binary.dat", 256, 256, "second half of binary file");
   test_map_file_range( "binary.dat", 100, 312, "middle section of binary file");

   // Test boundary conditions
   test_map_file_range( "small.txt", 0, 1, "single byte from small file");
   test_map_file_range( "small.txt", 83, 1, "last byte of small file");

   // Test error cases
   test_map_file_range_errors();

   printf( "=== All File Range Mapping Tests Completed ===\n");
   return( 0);
}
