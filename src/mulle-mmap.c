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


#ifdef _WIN32
# include <windows.h>
#else
# include <unistd.h>
# include <fcntl.h>
# include <sys/mman.h>
# include <sys/stat.h>
#endif

#include "include-private.h"


uint32_t   mulle_mmap_get_version( void)
{
   return( MULLE_MMAP_VERSION);
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
#ifdef _WIN32
        SYSTEM_INFO SystemInfo;
        GetSystemInfo(&SystemInfo);
        mulle_mmap_pagesize = SystemInfo.dwAllocationGranularity;
#else
        mulle_mmap_pagesize = sysconf(_SC_PAGE_SIZE);
#endif
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


#ifdef _WIN32

/** Returns the 4 upper bytes of an 8-byte integer. */
static inline DWORD   mulle_mmap_int64_high( int64_t n)
{
    return n >> 32;
}

/** Returns the 4 lower bytes of an 8-byte integer. */
static inline DWORD   mulle_mmap_int64_low( int64_t n)
{
    return n & 0xffffffff;
}

#endif // _WIN32




static mulle_mmap_file_t   mulle_mmap_file_open( char *path,
                                                 enum mulle_mmap_accessmode mode)
{
   mulle_mmap_file_t   handle;

#ifdef _WIN32
   handle = CreateFileA( path,
                         mode == mulle_mmap_read ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE),
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         0,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         0);

#else // POSIX
    handle = open( path, mode == mulle_mmap_read ? O_RDONLY : O_RDWR);
#endif
    return( handle);
}


static inline int64_t   mulle_mmap_file_query_size( mulle_mmap_file_t handle)
{
#ifdef _WIN32
    LARGE_INTEGER   file_size;

    if( GetFileSizeEx( handle, &file_size) == 0)
        return ( -1);
	return( (int64_t) file_size.QuadPart);
#else // POSIX
    struct stat sbuf;

    if( fstat( handle, &sbuf) == -1)
      return( -1);
    return( sbuf.st_size);
#endif
}



struct memory_map_result
{
    char      *data;
    int64_t   length;
    int64_t   mapped_length;
#ifdef _WIN32
    mulle_mmap_file_t   file_mapping_handle;
#endif
};


static int   memory_map( mulle_mmap_file_t handle,
                         int64_t offset,
                         int64_t length,
                         enum mulle_mmap_accessmode mode,
                         struct memory_map_result *ctx)
{
    int64_t   aligned_offset;
    int64_t   length_to_map;
    char       *mapping_start;

    aligned_offset = mulle_mmap_pagealign_offset( offset);
    length_to_map  = offset - aligned_offset + length;

#ifdef _WIN32
   {
       int64_t            max_file_size;
       mulle_mmap_file_t   file_mapping_handle;

       max_file_size = offset + length;
       file_mapping_handle = CreateFileMapping(
               handle,
               0,
               mode == mulle_mmap_read ? PAGE_READONLY : PAGE_READWRITE,
               mulle_mmap_int64_high( max_file_size),
               mulle_mmap_int64_low( max_file_size),
               0);
       if( file_mapping_handle == MULLE_MMAP_INVALID_HANDLE)
         return( -1);

       mapping_start = (char *) MapViewOfFile(
               file_mapping_handle,
               mode == mulle_mmap_read ? FILE_MAP_READ : FILE_MAP_WRITE,
               mulle_mmap_int64_high( aligned_offset),
               mulle_mmap_int64_low( aligned_offset),
               length_to_map);
       if( mapping_start == nullptr)
         return( 1);
       ctx->file_mapping_handle = file_mapping_handle;
   }
#else // POSIX
    mapping_start = (char *) mmap(
            0, // Don't give hint as to where to map.
            length_to_map,
            mode == mulle_mmap_read ? PROT_READ : PROT_WRITE,
            MAP_SHARED,
            handle,
            aligned_offset);
    if( mapping_start == MAP_FAILED)
      return( -1);
#endif

    ctx->data          = mapping_start + offset - aligned_offset;
    ctx->length        = length;
    ctx->mapped_length = length_to_map;

    return( 0);
}


// -- mulle_mmap --

void  _mulle_mmap_done( struct mulle_mmap *p)
{
   _mulle_mmap_conditional_sync( p);
   _mulle_mmap_unmap( p);
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
   int64_t                    file_size;
   struct memory_map_result   ctx;
   int                        rval;

   file_size = mulle_mmap_file_query_size( handle);
   if( file_size == -1)
      return( -1);

    /*
     * quick check, though file size might have changed behind our back
     * already again
     */

    if( length != (size_t) -1 && offset + length > file_size)
    {
#ifdef _WIN32
#else
       errno = EINVAL;
#endif
       return( -1);
    }

    rval = memory_map( handle,
                       offset,
                       length == -1 ? (file_size - offset) : length,
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
        p->length_              = ctx.length;
        p->mapped_length_       = ctx.mapped_length;
#ifdef _WIN32
        p->file_mapping_handle_ = ctx.file_mapping_handle;
#endif
    }
    return( rval);
}


int   _mulle_mmap_sync( struct mulle_mmap *p)
{
    if( ! _mulle_mmap_is_open( p))
        return( -1);

    if( _mulle_mmap_get_data( p))
    {
#ifdef _WIN32
        if( FlushViewOfFile( _mulle_mmap_get_mapping_start( p),
                             p->mapped_length_) == 0
           || FlushFileBuffers( p->file_handle_) == 0)
        {
           return( -1);
        }
#else // POSIX
        if( msync( _mulle_mmap_get_mapping_start( p), p->mapped_length_, MS_SYNC) != 0)
        {
            return( -1);
        }
#endif
    }

#ifdef _WIN32
    if( FlushFileBuffers( p->file_handle_) == 0)
       return( -1);
#endif
    return( 0);
}


int   _mulle_mmap_unmap( struct mulle_mmap *p)
{
   int   rval;

   rval = 0;
    if( ! _mulle_mmap_is_open( p))
        return( rval);

    // TODO do we care about errors here?
#ifdef _WIN32
    if( _mulle_mmap_is_mapped( p))
    {
        UnmapViewOfFile( _mulle_mmap_get_mapping_start( p));
        CloseHandle( p->file_mapping_handle_);
    }
#else // POSIX
    if( p->data_)
    {
       rval = munmap( _mulle_mmap_get_mapping_start( p), p->mapped_length_);
    }
#endif

    // If `file_handle_` was obtained by our opening it (when map is called with
    // a path, rather than an existing file handle), we need to close it,
    // otherwise it must not be closed as it may still be used outside this
    // instance.
    if( p->is_handle_internal_)
    {
#ifdef _WIN32
        CloseHandle( p->file_handle_);
#else // POSIX
        if( close( p->file_handle_))
           rval = -1;
#endif
    }

    // Reset fields to their default values.
    _mulle_mmap_init( p, p->accessmode_);
    return( rval);
}


int   _mulle_mmap_is_mapped( struct mulle_mmap *p)
{
#ifdef _WIN32
    return( p->file_mapping_handle_ != MULLE_MMAP_INVALID_HANDLE);
#else // POSIX
    return( _mulle_mmap_is_open( p));
#endif
}


int   _mulle_mmap_conditional_sync( struct mulle_mmap *p)
{
   if( p->accessmode_ == mulle_mmap_read)
      return( 0);
   return( _mulle_mmap_sync( p));
}

