#include <mulle-mmap/mulle-mmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


static void test_unaligned_offsets( void)
{
   struct mulle_mmap   info;
   size_t              page_size;
   size_t              test_offsets[] = {1, 3, 17, 63, 127, 255, 511, 1023, 2047, 4095};
   size_t              num_offsets = sizeof(test_offsets) / sizeof(test_offsets[0]);
   size_t              i;
   int                 rval;

   printf( "Testing unaligned offset mappings:\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   printf( "  System page size: %zu bytes\n", page_size);

   for( i = 0; i < num_offsets; i++)
   {
      size_t offset = test_offsets[i];
      
      // Skip offsets that are too large for our large file (16KB)
      if( offset >= 16384)
         continue;
         
      printf( "  Testing offset %zu:\n", offset);
      
      _mulle_mmap_init( &info, mulle_mmap_read);
      
      rval = _mulle_mmap_map_file_range( &info, "large.txt", offset, 1024);
      if( rval)
      {
         printf( "    Failed to map at offset %zu: %s\n", offset, strerror( errno));
         _mulle_mmap_done( &info);
         continue;
      }
      
      size_t actual_length = _mulle_mmap_get_length( &info);
      size_t mapped_length = _mulle_mmap_get_mapped_length( &info);
      size_t mapping_offset = _mulle_mmap_get_mapping_offset( &info);
      void *data = _mulle_mmap_get_data( &info);
      
      printf( "    SUCCESS - Length: %zu, Mapped: %zu, MapOffset: %zu, Data: %p\n", 
              actual_length, mapped_length, mapping_offset, data);
      
      // Verify we can read the data
      if( actual_length > 0 && data != NULL)
      {
         unsigned char first_byte = *((unsigned char *) data);
         printf( "    First byte at offset %zu: 0x%02x\n", offset, first_byte);
      }
      
      _mulle_mmap_done( &info);
   }
   
   printf( "  Unaligned offset tests completed\n\n");
}


static void test_length_variations( void)
{
   struct mulle_mmap   info;
   size_t              test_lengths[] = {1, 2, 3, 4, 5, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191};
   size_t              num_lengths = sizeof(test_lengths) / sizeof(test_lengths[0]);
   size_t              i;
   int                 rval;

   printf( "Testing various length parameters:\n");

   for( i = 0; i < num_lengths; i++)
   {
      size_t length = test_lengths[i];
      
      printf( "  Testing length %zu:\n", length);
      
      _mulle_mmap_init( &info, mulle_mmap_read);
      
      rval = _mulle_mmap_map_file_range( &info, "large.txt", 0, length);
      if( rval)
      {
         printf( "    Failed to map length %zu: %s\n", length, strerror( errno));
         _mulle_mmap_done( &info);
         continue;
      }
      
      size_t actual_length = _mulle_mmap_get_length( &info);
      size_t mapped_length = _mulle_mmap_get_mapped_length( &info);
      
      printf( "    SUCCESS - Requested: %zu, Actual: %zu, Mapped: %zu\n", 
              length, actual_length, mapped_length);
      
      _mulle_mmap_done( &info);
   }
   
   // Test SIZE_MAX
   printf( "  Testing SIZE_MAX length:\n");
   _mulle_mmap_init( &info, mulle_mmap_read);
   
   rval = _mulle_mmap_map_file_range( &info, "large.txt", 0, SIZE_MAX);
   if( rval)
   {
      printf( "    Failed to map with SIZE_MAX: %s\n", strerror( errno));
   }
   else
   {
      size_t actual_length = _mulle_mmap_get_length( &info);
      printf( "    SUCCESS - SIZE_MAX mapped to actual file size: %zu\n", actual_length);
   }
   
   _mulle_mmap_done( &info);
   printf( "  Length variation tests completed\n\n");
}


static void test_boundary_conditions( void)
{
   struct mulle_mmap   info;
   int                 rval;
   size_t              file_size = 16384; // large.txt size

   printf( "Testing boundary conditions:\n");

   // Test 1: Offset at end of file
   printf( "  Test 1: Mapping at end of file (offset = file size)\n");
   _mulle_mmap_init( &info, mulle_mmap_read);
   rval = _mulle_mmap_map_file_range( &info, "large.txt", file_size, 100);
   if( rval)
   {
      printf( "    SUCCESS: Failed as expected for offset at end of file: %s\n", strerror( errno));
   }
   else
   {
      printf( "    INFO: Mapping at end of file succeeded, length: %zu\n", _mulle_mmap_get_length( &info));
   }
   _mulle_mmap_done( &info);

   // Test 2: Offset beyond file
   printf( "  Test 2: Mapping beyond end of file (offset > file size)\n");
   _mulle_mmap_init( &info, mulle_mmap_read);
   rval = _mulle_mmap_map_file_range( &info, "large.txt", file_size + 1000, 100);
   if( rval)
   {
      printf( "    SUCCESS: Failed as expected for offset beyond file: %s\n", strerror( errno));
   }
   else
   {
      printf( "    ERROR: Should have failed for offset beyond file\n");
   }
   _mulle_mmap_done( &info);

   // Test 3: Length extending beyond file
   printf( "  Test 3: Length extending beyond file end\n");
   _mulle_mmap_init( &info, mulle_mmap_read);
   rval = _mulle_mmap_map_file_range( &info, "large.txt", file_size - 1000, 2000);
   if( rval)
   {
      printf( "    Failed: %s\n", strerror( errno));
   }
   else
   {
      size_t actual_length = _mulle_mmap_get_length( &info);
      printf( "    SUCCESS: Length truncated to file boundary, actual length: %zu\n", actual_length);
   }
   _mulle_mmap_done( &info);

   // Test 4: Single byte at very end of file
   printf( "  Test 4: Single byte at end of file\n");
   _mulle_mmap_init( &info, mulle_mmap_read);
   rval = _mulle_mmap_map_file_range( &info, "large.txt", file_size - 1, 1);
   if( rval)
   {
      printf( "    Failed: %s\n", strerror( errno));
   }
   else
   {
      size_t actual_length = _mulle_mmap_get_length( &info);
      void *data = _mulle_mmap_get_data( &info);
      printf( "    SUCCESS: Mapped last byte, length: %zu, data: %p\n", actual_length, data);
      if( data != NULL)
      {
         unsigned char last_byte = *((unsigned char *) data);
         printf( "    Last byte value: 0x%02x\n", last_byte);
      }
   }
   _mulle_mmap_done( &info);

   printf( "  Boundary condition tests completed\n\n");
}


static void test_page_boundary_alignments( void)
{
   struct mulle_mmap   info;
   size_t              page_size;
   size_t              offsets_around_page[8];
   size_t              i;
   int                 rval;

   printf( "Testing page boundary alignments:\n");
   
   page_size = mulle_mmap_get_system_pagesize();
   printf( "  System page size: %zu bytes\n", page_size);

   // Test offsets around page boundaries
   offsets_around_page[0] = page_size - 4;
   offsets_around_page[1] = page_size - 2;
   offsets_around_page[2] = page_size - 1;
   offsets_around_page[3] = page_size;
   offsets_around_page[4] = page_size + 1;
   offsets_around_page[5] = page_size + 2;
   offsets_around_page[6] = page_size + 4;
   offsets_around_page[7] = page_size * 2;

   for( i = 0; i < 8; i++)
   {
      size_t offset = offsets_around_page[i];
      
      // Skip offsets beyond our large file
      if( offset >= 16384)
         continue;
         
      printf( "  Testing page-relative offset %zu (page_size %s %zu):\n", 
              offset, 
              (offset < page_size) ? "-" : "+", 
              (offset < page_size) ? (page_size - offset) : (offset - page_size));
      
      _mulle_mmap_init( &info, mulle_mmap_read);
      
      rval = _mulle_mmap_map_file_range( &info, "large.txt", offset, page_size);
      if( rval)
      {
         printf( "    Failed to map at page-relative offset %zu: %s\n", offset, strerror( errno));
         _mulle_mmap_done( &info);
         continue;
      }
      
      size_t actual_length = _mulle_mmap_get_length( &info);
      size_t mapped_length = _mulle_mmap_get_mapped_length( &info);
      size_t mapping_offset = _mulle_mmap_get_mapping_offset( &info);
      void *data = _mulle_mmap_get_data( &info);
      
      printf( "    SUCCESS - Length: %zu, Mapped: %zu, MapOffset: %zu\n", 
              actual_length, mapped_length, mapping_offset);
      printf( "    Data alignment: %p (aligned to %zu: %s)\n", 
              data, 
              page_size,
              ((uintptr_t) data % page_size == 0) ? "YES" : "NO");
      
      _mulle_mmap_done( &info);
   }
   
   printf( "  Page boundary alignment tests completed\n\n");
}


int main( void)
{
   printf( "=== Edge Cases and Alignment Tests ===\n\n");

   // Test unaligned offsets
   test_unaligned_offsets();
   
   // Test various lengths
   test_length_variations();
   
   // Test boundary conditions
   test_boundary_conditions();
   
   // Test page boundary alignments
   test_page_boundary_alignments();

   printf( "=== All Edge Case and Alignment Tests Completed ===\n");
   return( 0);
}
