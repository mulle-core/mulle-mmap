/*
 * Copyright (c) 2025 Mulle kybernetiK. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of Mulle kybernetiK nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _WIN32

#include "mulle-mmap.h"
#include "include-private.h"

#include <unistd.h>
#include <fcntl.h>
#if defined( __COSMOPOLITAN__) || defined( __MULLE_COSMOPOLITAN__)
#include <cosmopolitan/cosmopolitan.h>
#else
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>

static inline size_t   mulle_mmap_pagealign_offset( size_t offset)
{
    size_t page_size;
    
    page_size = mulle_mmap_get_system_pagesize();
    return( (offset / page_size) * page_size);
}

static inline void   *_mulle_mmap_get_mapping_start( struct mulle_mmap *p)
{
   char   *data;

   data = _mulle_mmap_get_bytes( p);
   if( data)
      data -= _mulle_mmap_get_mapping_offset( p);
   return( data);
}

int   _mulle_mmap_sync( struct mulle_mmap *p)
{
   if( ! _mulle_mmap_is_open( p))
      return( -1);

   if( _mulle_mmap_get_bytes( p))
   {
      if( msync( _mulle_mmap_get_mapping_start( p), p->mapped_length_, MS_SYNC) != 0)
      {
         return( -1);
      }
   }
   return( 0);
}

int   _mulle_mmap_unmap( struct mulle_mmap *p)
{
   int   rval = 0;

   if( ! _mulle_mmap_is_open( p))
      return( rval);

   if( p->data_)
   {
      rval = munmap( _mulle_mmap_get_mapping_start( p), p->mapped_length_);
   }

   if( p->is_handle_internal_)
   {
      if( close( p->file_handle_))
         rval = -1;
   }

   // Reset fields to their default values.
   _mulle_mmap_init( p, p->accessmode_);
   return( rval);
}

void   *mulle_mmap_alloc_pages( size_t size)
{
   void  *p;
   
   p = mmap( 0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
   if( p == MAP_FAILED)
      return( NULL);
   return( p);
}

void   *mulle_mmap_alloc_shared_pages( size_t size)
{
   void  *p;
   
   p = mmap( 0, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
   if( p == MAP_FAILED)
      return( NULL);
   return( p);
}

int   _mulle_mmap_free_pages( void *p, size_t size)
{
   return( munmap( p, size));
}

// Platform-specific functions with standard names
size_t   mulle_mmap_get_system_pagesize( void)
{
   return( sysconf(_SC_PAGE_SIZE));
}


mulle_mmap_file_t   mulle_mmap_file_open( char *path, 
                                          enum mulle_mmap_accessmode mode)
{
   return( open( path, mode == mulle_mmap_read ? O_RDONLY : O_RDWR));
}

int64_t   mulle_mmap_file_query_size( mulle_mmap_file_t handle)
{
   struct stat sbuf;

   if( fstat( handle, &sbuf) == -1)
      return( -1);
   return( sbuf.st_size);
}

int   mulle_mmap_memory_map( mulle_mmap_file_t handle,
                            int64_t offset,
                            int64_t length,
                            enum mulle_mmap_accessmode mode,
                            struct mulle_mmap_result *ctx)
{
   int64_t   aligned_offset;
   int64_t   length_to_map;
   char      *mapping_start;

   aligned_offset = mulle_mmap_pagealign_offset( (size_t) offset);
   length_to_map  = offset - aligned_offset + length;

   // (nat) NSData otherwise has problems with 0 byte files
   if( ! length_to_map)
   {
      ctx->data          = 0;
      ctx->length        = 0;
      ctx->mapped_length = 0;
      return( 0);
   }

   mapping_start = (char *) mmap(
            0, // Don't give hint as to where to map.
            length_to_map,
            mode == mulle_mmap_read ? PROT_READ : PROT_WRITE,
            MAP_SHARED,
            handle,
            aligned_offset);
   if( mapping_start == MAP_FAILED)
      return( -1);

   ctx->data          = mapping_start + offset - aligned_offset;
   ctx->length        = length;
   ctx->mapped_length = length_to_map;

   return( 0);
}

int   _mulle_mmap_is_mapped( struct mulle_mmap *p)
{
   return( _mulle_mmap_is_open( p));
}

#endif // !_WIN32
