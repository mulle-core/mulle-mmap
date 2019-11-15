#include <mulle-mmap/mulle-mmap.h>

int  main( int argc, char *argv[])
{
   struct mulle_mmap   info;

   // more or less a compile and link test, this should do nothing
   _mulle_mmap_init( &info, mulle_mmap_read);
   _mulle_mmap_done( &info);
   return( 0);
}


