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
#ifndef mulle_mmap_h__
#define mulle_mmap_h__

#include "include.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>


#define MULLE_MMAP_VERSION  ((0 << 20) | (2 << 8) | 0)


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


MULLE_MMAP_GLOBAL
uint32_t   mulle_mmap_get_version( void);


#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif // WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif // ifdef _WIN32


/**
 * This is used by `mulle_mmap` to determine whether to create a read-only or
 * a read-write memory mapping.
 */


#ifdef _WIN32
typedef HANDLE  mulle_mmap_file_t;
# define MULLE_MMAP_INVALID_HANDLE  INVALID_HANDLE_VALUE
#else
typedef int     mulle_mmap_file_t;
# define MULLE_MMAP_INVALID_HANDLE  -1
#endif



enum mulle_mmap_accessmode
{
   mulle_mmap_read  = 0,
   mulle_mmap_write = 1
};


struct mulle_mmap
{
   void                         *data_;
   size_t                       length_;
   size_t                       mapped_length_;
   int                          is_handle_internal_;
   enum mulle_mmap_accessmode   accessmode_;
   /* end of memset 0 init */

   mulle_mmap_file_t            file_handle_;
#ifdef _WIN32
   mulle_mmap_file_t            file_mapping_handle_;
#endif
};


static inline void   _mulle_mmap_init( struct mulle_mmap *p,
                                       enum mulle_mmap_accessmode mode)
{
   memset( p, 0, (char *) &p->accessmode_ - (char *) p);

   p->accessmode_          = mode;
   p->file_handle_         = MULLE_MMAP_INVALID_HANDLE;
#ifdef _WIN32
   p->file_mapping_handle_ = MULLE_MMAP_INVALID_HANDLE;
#endif
}


/**
* If this is a read-write mapping, the destructor invokes sync. Regardless
* of the access mode, unmap is invoked as a final step.
*/
MULLE_MMAP_GLOBAL
void   _mulle_mmap_done( struct mulle_mmap *p);

/**
 * On UNIX systems 'file_handle' and 'mapping_handle' are the same. On Windows,
 * however, a mapped region of a file gets its own handle, which is returned by
 * 'mapping_handle'.
 */
static inline mulle_mmap_file_t
   _mulle_mmap_get_file_handle( struct mulle_mmap *p)
{
   return( p->file_handle_);
}


#ifdef _WIN32
static inline mulle_mmap_file_t
   _mulle_mmap_get_file_mapping_handle( struct mulle_mmap *p)
{
   return( p->file_mapping_handle_);
}
#endif


/*
 * A little additional interface to just get pages from the OS.
 */
//
// size should probably be a multiple of pagesize. It is known that the
// returned pages are zero filled!
//
MULLE_MMAP_GLOBAL
void   *mulle_mmap_alloc_pages( size_t size);
// free with this

MULLE_MMAP_GLOBAL
int     mulle_mmap_free_pages( void *p, size_t size);


/* like mulle_mmap_alloc_pages but produces shared memory pages instead
 * https://stackoverflow.com/questions/5656530/how-to-use-shared-memory-with-linux-in-c
 */
MULLE_MMAP_GLOBAL
void   *mulle_mmap_alloc_shared_pages( size_t size);

    /** Returns whether a valid memory mapping has been created. */
static inline int   _mulle_mmap_is_open( struct mulle_mmap *p)
{
   return( p->file_handle_ != MULLE_MMAP_INVALID_HANDLE);
}

    /**
     * Returns true if no mapping was established, that is, conceptually the
     * same as though the length that was mapped was 0. This function is
     * provided so that this class has Container semantics.
     */
static inline int   _mulle_mmap_is_empty( struct mulle_mmap *p)
{
   return( ! p->length_);
}


static inline int   _mulle_mmap_is_writable( struct mulle_mmap *p)
{
   return( p->accessmode_ == mulle_mmap_write);
}


MULLE_MMAP_GLOBAL
int   _mulle_mmap_is_mapped( struct mulle_mmap *p);

/**
 * `size` and `length` both return the logical length, i.e. the number of bytes
 * user requested to be mapped, while `mapped_length` returns the actual number of
 * bytes that were mapped which is a multiple of the underlying operating system's
 * page allocation granularity.
 */
static inline size_t   _mulle_mmap_get_length( struct mulle_mmap *p)
{
   return( p->length_);
}


static inline size_t   _mulle_mmap_get_mapped_length( struct mulle_mmap *p)
{
   return( p->mapped_length_);
}


static inline size_t   _mulle_mmap_get_mapping_offset( struct mulle_mmap *p)
{
   return( p->mapped_length_ - p->length_);
}


static inline void   *_mulle_mmap_get_data( struct mulle_mmap *p)
{
   return( p->data_);
}


/**
 * Establishes a memory mapping with AccessMode as was given during init.
 * If the mapping is unsuccessful, the return value is -1 and the struct
 * remains in a state as if this function hadn't been called.
 *
 * `path`, which must be a path to an existing file, is used to retrieve a file
 * handle (which is closed when the object destructs or `unmap` is called), which is
 * then used to memory map the requested region. Upon failure, `error` is set to
 * indicate the reason and the object remains in an unmapped state.
 *
 * `offset` is the number of bytes, relative to the start of the file, where the
 * mapping should begin. When specifying it, there is no need to worry about
 * providing a value that is aligned with the operating system's page allocation
 * granularity. This is adjusted by the implementation such that the first requested
 * byte (as returned by `data` or `begin`), so long as `offset` is valid, will be at
 * `offset` from the start of the file.
 *
 * `length` is the number of bytes to map. It may be `-1`, in which
 * case a mapping of the entire file is created.
 */
MULLE_MMAP_GLOBAL
int    _mulle_mmap_map_file_range( struct mulle_mmap *p,
                                   char *path,
                                   size_t offset,
                                   size_t length);

/**
 * Establishes a memory mapping with AccessMode as was given during init.
 * If the mapping is unsuccessful, the return value is -1 and the struct
 * remains in a state as if this function hadn't been called.
 *
 * `path`, which must be a path to an existing file, is used to retrieve a file
 * handle (which is closed when the object destructs or `unmap` is called), which is
 * then used to memory map the requested region. Upon failure, `error` is set to
 * indicate the reason and the object remains in an unmapped state.
 *
 * The entire file is mapped.
 */
static inline int   _mulle_mmap_map_file( struct mulle_mmap *p,
                                          char *path)
{
   return( _mulle_mmap_map_file_range( p, path, 0, (size_t) -1));
}

/**
 * Establishes a memory mapping with AccessMode as was given during init.
 * If the mapping is unsuccessful, the return value is -1 and the struct
 * remains in a state as if this function hadn't been called.
 *
 * `handle`, which must be a valid file handle, which is used to memory map the
 * requested region. Upon failure, `error` is set to indicate the reason and the
 * object remains in an unmapped state.
 *
 * `offset` is the number of bytes, relative to the start of the file, where the
 * mapping should begin. When specifying it, there is no need to worry about
 * providing a value that is aligned with the operating system's page allocation
 * granularity. This is adjusted by the implementation such that the first requested
 * byte (as returned by `data` or `begin`), so long as `offset` is valid, will be at
 * `offset` from the start of the file.
 *
 * `length` is the number of bytes to map. It may be `-1`, in which
 * case a mapping of the entire file is created.
 */
MULLE_MMAP_GLOBAL
int    _mulle_mmap_map_range( struct mulle_mmap *p,
                              mulle_mmap_file_t handle,
                              size_t offset,
                              size_t length);

/**
 * Establishes a memory mapping with AccessMode as was given during init.
 * If the mapping is unsuccessful, the return value is -1 and the struct
 * remains in a state as if this function hadn't been called.
 *
 * `handle`, which must be a valid file handle, which is used to memory map the
 * requested region. Upon failure, `error` is set to indicate the reason and the
 * object remains in an unmapped state.
 *
 * The entire file is mapped.
 */
static inline int  _mulle_mmap_map( struct mulle_mmap *p,
                                    mulle_mmap_file_t handle)
{
   return( _mulle_mmap_map_range( p, handle, 0, (size_t) -1));
}

/**
 * If a valid memory mapping has been created prior to this call, this call
 * instructs the kernel to unmap the memory region and disassociate this object
 * from the file.
 *
 * The file handle associated with the file that is mapped is only closed if the
 * mapping was created using a file path. If, on the other hand, an existing
 * file handle was used to create the mapping, the file handle is not closed.
 */
MULLE_MMAP_GLOBAL
int   _mulle_mmap_unmap( struct mulle_mmap *p);


/** Flushes the memory mapped page to disk. Errors are reported via `error`. */
MULLE_MMAP_GLOBAL
int   _mulle_mmap_sync( struct mulle_mmap *p);

/**
 * The destructor syncs changes to disk if `AccessMode` is `write`, but not
 * if it's `read`.
 */
MULLE_MMAP_GLOBAL
int   _mulle_mmap_conditional_sync( struct mulle_mmap *p);



static inline int   mulle_mmap_equal( struct mulle_mmap *p,
                                      struct mulle_mmap *q)
{
   return( p->accessmode_ == q->accessmode_ &&
           p->data_ == q->data_ &&
           p->length_ == q->length_);
}


MULLE_MMAP_GLOBAL
size_t  mulle_mmap_get_system_pagesize( void);


#ifdef __has_include
# if __has_include( "_mulle-mmap-versioncheck.h")
#  include "_mulle-mmap-versioncheck.h"
# endif
#endif

#endif
