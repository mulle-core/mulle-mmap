#include <mulle-mmap/mulle-mmap.h>

#include <stdio.h>
#include <stdlib.h>


static char  testfile[] = "test-file.dat";

void   test_at_offset( enum mulle_mmap_accessmode mode,
                       char *buffer,
                       size_t buflen,
                       char *filename,
                       size_t offset)
{
   struct mulle_mmap   info;
   size_t              mapped_size;
   char                *p;
   char                *sentinel;
   char                v;

   printf( "%s \"%s\" offset=%ld : ", mode ? "RW" : "R", filename, (long) offset);

    // Map the region of the file to which buffer was written.
   _mulle_mmap_init( &info, mode);
   if( _mulle_mmap_map_range_of_file( &info, filename, offset, -1))
      printf( "mmap failed %s\n", strerror( errno));

   if( ! _mulle_mmap_is_open( &info))
      printf( "not open %s\n", strerror( errno));

   mapped_size = buflen - offset;
   if( _mulle_mmap_get_length( &info) != mapped_size)
      printf( "buflen mismatch\n");

    // Start at first printable ASCII character.
    p         = &buffer[ 0];
    sentinel  = &buffer[ buflen];
    for( v = 33; p < sentinel; v = ++v < 126 ? v : 33)
      if( *p++ != v)
         printf( "contents mismatch\n");

   _mulle_mmap_done( &info);
   printf( "OK\n");
}




int main()
{
    size_t   page_size;
    size_t   file_size;
    char     *p;
    char     *buffer;
    char     *sentinel;
    char     v;
    FILE     *fp;
    int      mode;

    page_size = mulle_mmap_get_system_pagesize();
    if( ! page_size)
      return( 1);

    file_size = 4 * page_size - 250; // 16134, if page size is 4KiB
    buffer    = calloc( 1, file_size);

    // Start at first printable ASCII character.
    p         = &buffer[ 0];
    sentinel  = &buffer[ file_size];

    for( v = 33; p < sentinel; v = ++v < 126 ? v : 33)
        *p++ = v;

   fp = fopen( testfile, "w");
   if( ! fp)
      return( 2);

   fwrite( buffer, 1, file_size, fp);
   if( fclose( fp))
      return( 3);

   for( mode = 0; mode <= 1; mode++)
   {
      test_at_offset( mode, buffer, file_size, testfile, 0);
      test_at_offset( mode, buffer, file_size, testfile, page_size - 3);
      test_at_offset( mode, buffer, file_size, testfile, page_size + 3);
      test_at_offset( mode, buffer, file_size, testfile, 2 * page_size + 3);
      if( ! mode)
         printf( "\n");
   }

   return( 0);
}


