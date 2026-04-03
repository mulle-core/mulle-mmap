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

struct mulle_mmap_shared_memory   mulle_mmap_alloc_shared_memory( size_t size)
{
   struct mulle_mmap_shared_memory result = { NULL, 0, INVALID_HANDLE_VALUE };
   SECURITY_ATTRIBUTES             sa;
   HANDLE                          hMap;
   void                            *p;
   
   if( size == 0)
      return( result);
   
   // Make the handle inheritable so child processes can access it
   sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
   sa.lpSecurityDescriptor = NULL;
   sa.bInheritHandle       = TRUE;
   
   // Create anonymous shared memory mapping (backed by page file)
   hMap = CreateFileMappingA(
      INVALID_HANDLE_VALUE,  // Use paging file
      &sa,                   // Inheritable handle
      PAGE_READWRITE,        // Read/write access
      0,                     // High-order DWORD of size
      (DWORD) size,          // Low-order DWORD of size
      NULL                   // Anonymous (no name)
   );
   
   if( hMap == NULL)
      return( result);
   
   // Map the entire file mapping into address space
   p = MapViewOfFile(
      hMap,
      FILE_MAP_ALL_ACCESS,
      0,                     // High-order offset
      0,                     // Low-order offset
      size                   // Number of bytes to map
   );
   
   if( p == NULL)
   {
      CloseHandle( hMap);
      return( result);
   }
   
   // Keep the handle open - it's needed for child processes to access the mapping
   result.address = p;
   result.size    = size;
   result.handle  = hMap;
   
   return( result);
}

int   mulle_mmap_free_shared_memory( struct mulle_mmap_shared_memory *mem)
{
   int rval = 0;
   
   if( ! mem || ! mem->address)
      return( 0);
   
   if( ! UnmapViewOfFile( mem->address))
      rval = -1;
   
   if( mem->handle != INVALID_HANDLE_VALUE)
      CloseHandle( mem->handle);
   
   mem->address = NULL;
   mem->size    = 0;
   mem->handle  = INVALID_HANDLE_VALUE;
   
   return( rval);
}


// Unix-only functions - not available on Windows
void   *mulle_mmap_map_shared_memory( mulle_mmap_file_t handle,
                                      size_t size,
                                      void *preferred_addr)
{
   void   *p;

   if( handle == MULLE_MMAP_INVALID_HANDLE || size == 0)
      return( NULL);

   // Try at preferred address first; fall back to OS-chosen address
   if( preferred_addr)
   {
      p = MapViewOfFileEx( handle, FILE_MAP_ALL_ACCESS, 0, 0, size, preferred_addr);
      if( p)
      {
         return( p);
      }
      /* MapViewOfFileEx at preferred address failed, fall back */
   }

   return( MapViewOfFile( handle, FILE_MAP_ALL_ACCESS, 0, 0, size));
}


// Unix-only functions - not available on Windows
void   *mulle_mmap_alloc_shared_pages_nowindows( size_t size)
{
   errno = ENOSYS;  // Function not implemented
   return( NULL);
}

void   mulle_mmap_free_shared_pages_nowindows( void *address, size_t size)
{
   // No-op on Windows
}


int   _mulle_mmap_free_pages( void *p, size_t size)
{
   // Try UnmapViewOfFile first (for shared pages from MapViewOfFile)
   if( UnmapViewOfFile( p))
      return( 0);
   
   // Fall back to VirtualFree (for regular pages from VirtualAlloc)
   return( ! VirtualFree( p, 0, MEM_RELEASE));
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
