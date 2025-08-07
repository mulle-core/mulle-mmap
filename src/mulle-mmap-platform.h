#ifndef mulle_mmap_platform_h__
#define mulle_mmap_platform_h__

#include "mulle-mmap.h"

/* Platform-specific function declarations.
 * These functions have different implementations in mulle-mmap-posix.c 
 * and mulle-mmap-windows.c
 */

// System page size - platform specific implementation
size_t   mulle_mmap_get_system_pagesize_platform( void);

// File operations - platform specific implementations  
mulle_mmap_file_t   mulle_mmap_file_open( char *path, enum mulle_mmap_accessmode mode);
int64_t             mulle_mmap_file_query_size( mulle_mmap_file_t handle);

// Memory mapping result structure (needs to handle Windows mapping handle)
struct mulle_mmap_result
{
   char      *data;
   int64_t   length;
   int64_t   mapped_length;
#ifdef _WIN32
   mulle_mmap_file_t   file_mapping_handle;
#endif
};

// Core memory mapping - platform specific implementation
int   mulle_mmap_memory_map( mulle_mmap_file_t handle,
                            int64_t offset,
                            int64_t length,
                            enum mulle_mmap_accessmode mode,
                            struct mulle_mmap_result *ctx);

// Synchronization - platform specific implementations (low-level, no checking)
int   _mulle_mmap_sync( struct mulle_mmap *p);

// Unmapping - platform specific implementations (low-level, no checking)  
int   _mulle_mmap_unmap( struct mulle_mmap *p);

// Page allocation - platform specific implementations
void   *mulle_mmap_alloc_pages_platform( size_t size);
void   *mulle_mmap_alloc_shared_pages_platform( size_t size);
int     mulle_mmap_free_pages_platform( void *p, size_t size);

// Platform-specific state queries (low-level, no checking)
int   _mulle_mmap_is_mapped( struct mulle_mmap *p);

#endif
