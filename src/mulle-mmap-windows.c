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

#ifdef _WIN32

#include "mulle-mmap.h"
#include "include-private.h"

#include <windows.h>
#include <stdlib.h>

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

int   mulle_mmap_sync_windows( struct mulle_mmap *p)
{
   if( ! _mulle_mmap_is_open( p))
      return( -1);

   if( _mulle_mmap_get_data( p))
   {
      if( FlushViewOfFile( _mulle_mmap_get_mapping_start( p),
                           p->mapped_length_) == 0
         || FlushFileBuffers( p->file_handle_) == 0)
      {
         return( -1);
      }
   }

   if( FlushFileBuffers( p->file_handle_) == 0)
      return( -1);
   
   return( 0);
}

int   mulle_mmap_unmap_windows( struct mulle_mmap *p)
{
   int   rval = 0;

   if( ! _mulle_mmap_is_open( p))
      return( rval);

   if( p->file_mapping_handle_ != MULLE_MMAP_INVALID_HANDLE)
   {
      UnmapViewOfFile( _mulle_mmap_get_mapping_start( p));
      CloseHandle( p->file_mapping_handle_);
   }

   if( p->is_handle_internal_)
   {
      CloseHandle( p->file_handle_);
   }

   return( rval);
}

void   *mulle_mmap_alloc_pages_windows( size_t size)
{
   return( VirtualAlloc( NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
}

void   *mulle_mmap_alloc_shared_pages_windows( size_t size)
{
   abort();
   MULLE_C_UNUSED( size);
   return( NULL);
}

int   mulle_mmap_free_pages_windows( void *p, size_t size)
{
   return( ! VirtualFree( p, size, MEM_RELEASE));
}

#endif // _WIN32