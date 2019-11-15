// don't collide with mulle_mmap_h__
#ifndef mulle_mulle_mmap_h__
#define mulle_mulle_mmap_h__

#include "include.h"

#include <stdint.h>

/*
 *  (c) 2019 nat ORGANIZATION
 *
 *  version:  major, minor, patch
 */
#define MULLE_MMAP_VERSION  ((0 << 20) | (7 << 8) | 56)


static inline unsigned int   mulle_mmap_get_version_major( void)
{
   return( MULLE_MMAP_VERSION >> 20);
}


static inline unsigned int   mulle_mmap_get_version_minor( void)
{
   return( (MULLE_MMAP_VERSION >> 8) & 0xFFF);
}


static inline unsigned int   mulle_mmap_get_version_patch( void)
{
   return( MULLE_MMAP_VERSION & 0xFF);
}


extern uint32_t   mulle_mmap_get_version( void);

/*
   Add other library headers here like so, for exposure to library
   consumers.

   # include "foo.h"
*/
#include "mmap.h"

#endif
