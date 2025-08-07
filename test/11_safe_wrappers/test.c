#include <mulle-mmap/mulle-mmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main()
{
   struct mulle_mmap   info;
   struct mulle_mmap   *null_ptr = NULL;
   int                 result;

   printf("Testing safe public wrapper functions with NULL checking\n");

   // Test safe wrappers with NULL pointer - should handle gracefully
   printf("Testing NULL pointer safety:\n");
   
   result = mulle_mmap_sync(null_ptr);
   printf("mulle_mmap_sync(NULL) returned: %d (expected: -1)\n", result);
   
   result = mulle_mmap_unmap(null_ptr);  
   printf("mulle_mmap_unmap(NULL) returned: %d (expected: -1)\n", result);
   
   result = mulle_mmap_is_mapped(null_ptr);
   printf("mulle_mmap_is_mapped(NULL) returned: %d (expected: 0)\n", result);

   result = mulle_mmap_conditional_sync(null_ptr);
   printf("mulle_mmap_conditional_sync(NULL) returned: %d (expected: -1)\n", result);

   // Test with valid initialized struct
   printf("\nTesting with valid uninitialized struct:\n");
   _mulle_mmap_init(&info, mulle_mmap_read);
   
   result = mulle_mmap_sync(&info);
   printf("mulle_mmap_sync(&info) returned: %d (expected: -1, unmapped)\n", result);
   
   result = mulle_mmap_is_mapped(&info);
   printf("mulle_mmap_is_mapped(&info) returned: %d (expected: 0, not mapped)\n", result);

   printf("\nAll safe wrapper tests completed successfully.\n");
   return( 0);
}
