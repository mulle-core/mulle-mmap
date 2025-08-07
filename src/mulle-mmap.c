/*
 * BASED ON:
 *
 * Copyright 2017 https://github.com/mandreyel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "mulle-mmap.h"
#include "include-private.h"

#include <stdint.h>

uint32_t   mulle_mmap_get_version( void)
{
   return( MULLE__MMAP_VERSION);
}

/**
 * Determines the operating system's page allocation granularity.
 *
 * On the first call to this function, it invokes the operating system specific syscall
 * to determine the page size, caches the value, and returns it. Any subsequent call to
 * this function serves the cached value, so no further syscalls are made.
 */
static size_t  mulle_mmap_pagesize;

size_t  mulle_mmap_get_system_pagesize( void)
{
   if( ! mulle_mmap_pagesize)
   {
      mulle_mmap_pagesize = mulle_mmap_get_system_pagesize_platform();
   }
   return( mulle_mmap_pagesize);
}

/**
 * Aligns `offset` to the operating system page size such that it subtracts the
 * difference until the nearest lower page boundary before `offset`, or does
 * nothing if `offset` is already page aligned.
 */
static inline size_t   mulle_mmap_pagealign_offset( size_t offset)
{
    size_t page_size;

    page_size = mulle_mmap_get_system_pagesize();
    return( (offset / page_size) * page_size);
}

static inline void   *_mulle_mmap_get_mapping_start( struct mulle_mmap *p)
{
   char   *data;

   data = _mulle_mmap_get_data( p);
   if( data)
      data -= _mulle_mmap_get_mapping_offset( p);
   return( data);
}

// Platform-specific helper functions are now in platform files

// -- mulle_mmap --

void  _mulle_mmap_done( struct mulle_mmap *p)
{
   _mulle_mmap_conditional_sync( p);
   if( ! (p->accessmode_ & mulle_mmap_no_unmap))
      _mulle_mmap_unmap( p);
#ifdef DEBUG
   mulle_memset_uint32( p, 0xDEADDEAD, sizeof( *p));
#endif
}



int   _mulle_mmap_map_file_range( struct mulle_mmap *p,
                                  char *path,
                                  size_t offset,
                                  size_t length)
{
   mulle_mmap_file_t   handle;
   int                rval;

   handle = mulle_mmap_file_open( path, p->accessmode_);
   if( handle == MULLE_MMAP_INVALID_HANDLE)
      return( -1);

   rval = _mulle_mmap_map_range( p, handle, offset, length);
   if( ! rval)
      p->is_handle_internal_ = 1;

   return( rval);
}

int    _mulle_mmap_map_range( struct mulle_mmap *p,
                              mulle_mmap_file_t handle,
                              size_t offset,
                              size_t length)
{
   int64_t                  file_size;
   struct mulle_mmap_result ctx = { 0 };  // for windows..
   int                      rval;

   file_size = mulle_mmap_file_query_size( handle);
   if( file_size == -1)
      return( -1);

    /*
     * quick check, though file size might have changed behind our back
     * already again
     */

    if( length != (size_t) -1 && (offset + length) > (size_t) file_size)
    {
       // Let the platform-specific mapping function handle the error
       return( -1);
    }

    rval = mulle_mmap_memory_map( handle,
                                 offset,
                                 length == (size_t) -1 ? (file_size - offset) : length,
                                 p->accessmode_,
                                 &ctx);
    if( ! rval)
    {
        // We must unmap the previous mapping that may have existed prior to this call.
        // Note that this must only be invoked after a new mapping has been created in
        // order to provide the strong guarantee that, should the new mapping fail, the
        // `map` function leaves this instance in a state as though the function had
        // never been invoked.
        _mulle_mmap_unmap( p);

        p->file_handle_         = handle;
        p->is_handle_internal_  = 0;
        p->data_                = ctx.data;
        p->length_              = (size_t) ctx.length;
        p->mapped_length_       = (size_t) ctx.mapped_length;
#ifdef _WIN32
        p->file_mapping_handle_ = ctx.file_mapping_handle;
#endif
    }
    return( rval);
}

int   _mulle_mmap_sync( struct mulle_mmap *p)
{
   return( mulle_mmap_sync( p));
}

int   _mulle_mmap_unmap( struct mulle_mmap *p)
{
   int   rval;

   rval = mulle_mmap_unmap( p);

   // Reset fields to their default values.
   _mulle_mmap_init( p, p->accessmode_);
   return( rval);
}

int   _mulle_mmap_is_mapped( struct mulle_mmap *p)
{
   return( mulle_mmap_is_mapped( p));
}

int   _mulle_mmap_conditional_sync( struct mulle_mmap *p)
{
   if( p->accessmode_ == mulle_mmap_read)
      return( 0);
   return( _mulle_mmap_sync( p));
}

void   *mulle_mmap_alloc_pages( size_t size)
{
   return( mulle_mmap_alloc_pages_platform( size));
}

void   *mulle_mmap_alloc_shared_pages( size_t size)
{
   return( mulle_mmap_alloc_shared_pages_platform( size));
}

int   mulle_mmap_free_pages( void *p, size_t size)
{
   return( mulle_mmap_free_pages_platform( p, size));
}
