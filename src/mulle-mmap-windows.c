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

int   _mulle_mmap_unmap( struct mulle_mmap *p)
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

   // Reset fields to their default values.
   _mulle_mmap_init( p, p->accessmode_);
   return( rval);
}

void   *mulle_mmap_alloc_pages( size_t size)
{
   return( VirtualAlloc( NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
}

void   *mulle_mmap_alloc_shared_pages( size_t size)
{
   abort();
   MULLE_C_UNUSED( size);
   return( NULL);
}

int   _mulle_mmap_free_pages( void *p, size_t size)
{
   return( ! VirtualFree( p, size, MEM_RELEASE));
}

// Windows-specific helper functions
static inline DWORD   mulle_mmap_int64_high( int64_t n)
{
   return n >> 32;
}

static inline DWORD   mulle_mmap_int64_low( int64_t n)
{
   return n & 0xffffffff;
}

// Platform-specific functions with standard names
size_t   mulle_mmap_get_system_pagesize( void)
{
   SYSTEM_INFO SystemInfo;
   GetSystemInfo(&SystemInfo);
   return( SystemInfo.dwAllocationGranularity);
}

mulle_mmap_file_t   mulle_mmap_file_open( char *path, enum mulle_mmap_accessmode mode)
{
   return( CreateFileA( path,
                       mode == mulle_mmap_read ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE),
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       0,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       0));
}

int64_t   mulle_mmap_file_query_size( mulle_mmap_file_t handle)
{
   LARGE_INTEGER   file_size;

   if( GetFileSizeEx( handle, &file_size) == 0)
      return ( -1);

   return( (int64_t) file_size.QuadPart);
}

int   mulle_mmap_memory_map( mulle_mmap_file_t handle,
                            int64_t offset,
                            int64_t length,
                            enum mulle_mmap_accessmode mode,
                            struct mulle_mmap_result *ctx)
{
   int64_t             aligned_offset;
   int64_t             length_to_map;
   char                *mapping_start;
   int64_t             max_file_size;
   mulle_mmap_file_t   file_mapping_handle;

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

   max_file_size = offset + length;
   file_mapping_handle = CreateFileMapping(
           handle,
           0,
           SEC_RESERVE | /* added https://devblogs.microsoft.com/oldnewthing/20150130-00/?p=44793) */
              (mode == mulle_mmap_read ? PAGE_READONLY : PAGE_READWRITE),
           (SIZE_T) mulle_mmap_int64_high( max_file_size),
           (SIZE_T) mulle_mmap_int64_low( max_file_size),
           0);
   if( file_mapping_handle == MULLE_MMAP_INVALID_HANDLE)
      return( -1);

   mapping_start = (char *) MapViewOfFile(
           file_mapping_handle,
           mode == mulle_mmap_read ? FILE_MAP_READ : FILE_MAP_WRITE,
           (SIZE_T) mulle_mmap_int64_high( aligned_offset),
           (SIZE_T) mulle_mmap_int64_low( aligned_offset),
           (SIZE_T) length_to_map);
   if( mapping_start == NULL)
      return( 1);

   ctx->file_mapping_handle = file_mapping_handle;
   ctx->data                = mapping_start + offset - aligned_offset;
   ctx->length              = length;
   ctx->mapped_length       = length_to_map;

   return( 0);
}

int   _mulle_mmap_is_mapped( struct mulle_mmap *p)
{
   return( p->file_mapping_handle_ != MULLE_MMAP_INVALID_HANDLE);
}

#endif // _WIN32
