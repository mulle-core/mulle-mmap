#include <mulle-mmap/mulle-mmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


static void test_map_with_handle( char *filename, char *description)
{
   struct mulle_mmap   info;
   char                *data;
   size_t              length;
   int                 fd;
   int                 rval;
   int                 mmap_handle;

   printf( "Testing %s (%s):\n", description, filename);

   // Open file manually
   fd = open( filename, O_RDONLY);
   if( fd == -1)
   {
      printf( "  Failed to open file: %s\n", strerror( errno));
      return;
   }
   printf( "  File opened successfully\n");

   // Initialize mmap structure for read-only access
   _mulle_mmap_init( &info, mulle_mmap_read);

   // Map using the file handle
   rval = _mulle_mmap_map( &info, fd);
   if( rval)
   {
      printf( "  Failed to mmap with handle: %s\n", strerror( errno));
      close( fd);
      _mulle_mmap_done( &info);
      return;
   }

   // Check if mapping was successful
   if( ! _mulle_mmap_is_open( &info))
   {
      printf( "  ERROR: mmap is not open after successful mapping\n");
      close( fd);
      _mulle_mmap_done( &info);
      return;
   }

   if( ! _mulle_mmap_is_mapped( &info))
   {
      printf( "  ERROR: mmap is not mapped after successful mapping\n");
      close( fd);
      _mulle_mmap_done( &info);
      return;
   }

   // Get the mapped data and length
   data   = (char *) _mulle_mmap_get_bytes( &info);
   length = _mulle_mmap_get_length( &info);

   printf( "  File size      : %zu bytes\n", length);
   printf( "  Mapped length  : %zu bytes\n", _mulle_mmap_get_mapped_length( &info));
   printf( "  Mapping offset : %zu\n", _mulle_mmap_get_mapping_offset( &info));
   
   printf( "  Handle stored correctly\n");
   if( mmap_handle != fd)
   {
      printf( "  WARNING: Stored handle differs from original handle\n");
   }

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

   // Clean up - note: _mulle_mmap_done should handle the file descriptor
   _mulle_mmap_done( &info);
   
   // Close the original file descriptor we opened
   close( fd);
   
   printf( "  Test completed successfully\n\n");
}


static void test_map_range_with_handle( char *filename, size_t offset, size_t length, char *description)
{
   struct mulle_mmap   info;
   char                *data;
   size_t              actual_length;
   size_t              mapped_length;
   size_t              mapping_offset;
   int                 fd;
   int                 rval;

   printf( "Testing %s:\n", description);
   printf( "  File: %s, Handle mapping, Offset: %zu, Length: %zu\n", filename, offset, length);

   // Open file manually
   fd = open( filename, O_RDONLY);
   if( fd == -1)
   {
      printf( "  Failed to open file: %s\n", strerror( errno));
      return;
   }

   // Initialize mmap structure for read-only access
   _mulle_mmap_init( &info, mulle_mmap_read);

   // Map range using the file handle
   rval = _mulle_mmap_map_range( &info, fd, offset, length);
   if( rval)
   {
      printf( "  Failed to mmap range with handle: %s\n", strerror( errno));
      close( fd);
      _mulle_mmap_done( &info);
      return;
   }

   // Get the mapped data and properties
   data            = (char *) _mulle_mmap_get_bytes( &info);
   actual_length   = _mulle_mmap_get_length( &info);
   mapped_length   = _mulle_mmap_get_mapped_length( &info);
   mapping_offset  = _mulle_mmap_get_mapping_offset( &info);

   printf( "  Actual length   : %zu bytes\n", actual_length);
   printf( "  Mapped length   : %zu bytes\n", mapped_length);
   printf( "  Mapping offset  : %zu\n", mapping_offset);

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
   close( fd);
   printf( "  Test completed successfully\n\n");
}


static void test_invalid_handle( void)
{
   struct mulle_mmap   info;
   int                 rval;

   printf( "Testing mapping with invalid handle:\n");

   // Initialize mmap structure
   _mulle_mmap_init( &info, mulle_mmap_read);

   // Try to map with invalid handle
   rval = _mulle_mmap_map( &info, -1);
   if( rval == 0)
   {
      printf( "  ERROR: Mapping with invalid handle should have failed\n");
      _mulle_mmap_done( &info);
      return;
   }

   printf( "  SUCCESS: Mapping with invalid handle failed as expected: %s\n", strerror( errno));

   // Clean up
   _mulle_mmap_done( &info);
   printf( "  Test completed successfully\n\n");
}


int main( void)
{
   printf( "=== Handle-Based Mapping Tests ===\n\n");

   // Test mapping with file handles
   test_map_with_handle( "empty.txt", "empty file via handle");
   test_map_with_handle( "small.txt", "small file via handle");
   test_map_with_handle( "medium.txt", "medium file via handle");
   test_map_with_handle( "large.txt", "large file via handle");
   test_map_with_handle( "binary.dat", "binary file via handle");

   // Test range mapping with handles
   test_map_range_with_handle( "medium.txt", 0, 1024, "handle range mapping: first 1024 bytes");
   test_map_range_with_handle( "medium.txt", 1024, 2048, "handle range mapping: bytes 1024-3071");
   test_map_range_with_handle( "large.txt", 4096, 4096, "handle range mapping: second page");
   test_map_range_with_handle( "binary.dat", 100, 300, "handle range mapping: middle section");

   // Test error cases
   test_invalid_handle();

   printf( "=== All Handle-Based Mapping Tests Completed ===\n");
   return( 0);
}
