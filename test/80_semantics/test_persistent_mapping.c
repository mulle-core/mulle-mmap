#include <mulle-mmap/mulle-mmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>


static void create_test_file( char *filename, char *content, size_t content_size)
{
   FILE   *fp;
   
   printf( "Creating test file: %s (%zu bytes)\n", filename, content_size);
   
   fp = fopen( filename, "w");
   if( ! fp)
   {
      printf( "ERROR: Failed to create test file: %s\n", strerror( errno));
      exit( 1);
   }
   
   if( fwrite( content, 1, content_size, fp) != content_size)
   {
      printf( "ERROR: Failed to write test content: %s\n", strerror( errno));
      fclose( fp);
      exit( 1);
   }
   
   fclose( fp);
   printf( "Test file created successfully\n\n");
}


static void test_persistent_mapping_after_close( void)
{
   struct mulle_mmap   info;
   char                *test_content;
   char                *test_filename;
   char                *mapped_data;
   size_t              mapped_length;
   size_t              test_size;
   int                 fd;
   int                 rval;
   size_t              i;
   
   printf( "=== Test: Memory Mapping Persistence After File Descriptor Close ===\n\n");
   
   // Create test content
   test_content = "This is a test file for persistent memory mapping.\n"
                  "We will map this file into memory, close the file descriptor,\n"
                  "and then verify we can still read the mapped memory.\n"
                  "This demonstrates the fundamental behavior of mmap().\n"
                  "The mapped pages remain accessible even after closing the file.\n";
   test_size = strlen( test_content);
   test_filename = "persistent_test.txt";
   
   // Create the test file
   create_test_file( test_filename, test_content, test_size);
   
   printf( "Phase 1: Opening file and mapping into memory\n");
   
   // Open file manually to get file descriptor
   fd = open( test_filename, O_RDONLY);
   if( fd == -1)
   {
      printf( "ERROR: Failed to open test file: %s\n", strerror( errno));
      return;
   }
   printf( "  Opened file with descriptor: %d\n", fd);
   
   // Initialize mmap structure
   _mulle_mmap_init( &info, mulle_mmap_read);
   
   // Map using file descriptor
   rval = _mulle_mmap_map( &info, fd);
   if( rval)
   {
      printf( "ERROR: Failed to map file: %s\n", strerror( errno));
      close( fd);
      _mulle_mmap_done( &info);
      return;
   }
   
   // Get mapped data
   mapped_data   = (char *) _mulle_mmap_get_bytes( &info);
   mapped_length = _mulle_mmap_get_length( &info);
   
   printf( "  File mapped successfully\n");
   printf( "  Mapped length: %zu bytes\n", mapped_length);
   
   // Verify we can read the data while file is open
   printf( "  Verifying data access while file is open...\n");
   if( mapped_length != test_size)
   {
      printf( "    ERROR: Mapped length (%zu) != expected length (%zu)\n", mapped_length, test_size);
      close( fd);
      _mulle_mmap_done( &info);
      return;
   }
   
   if( memcmp( mapped_data, test_content, test_size) != 0)
   {
      printf( "    ERROR: Mapped data does not match original content\n");
      close( fd);
      _mulle_mmap_done( &info);
      return;
   }
   printf( "  SUCCESS: Data matches original content while file is open\n\n");
   
   printf( "Phase 2: Closing file descriptor\n");
   
   // Now close the original file descriptor
   close( fd);
   printf( "  File descriptor %d closed\n", fd);
   
   // Verify the mmap structure still reports the file as mapped
   if( ! _mulle_mmap_is_mapped( &info))
   {
      printf( "  ERROR: mmap should still report as mapped after fd close\n");
      _mulle_mmap_done( &info);
      return;
   }
   printf( "  SUCCESS: mmap still reports as mapped after fd close\n\n");
   
   printf( "Phase 3: Accessing mapped memory after file descriptor close\n");
   
   // Try to read the mapped data after closing the file
   printf( "  Testing memory access after file descriptor close...\n");
   
   // Verify we can still access all the data
   if( memcmp( mapped_data, test_content, test_size) != 0)
   {
      printf( "    ERROR: Mapped data corruption after file descriptor close\n");
      _mulle_mmap_done( &info);
      return;
   }
   printf( "  SUCCESS: All mapped data still accessible and correct\n");
   
   // Read and display first few lines to demonstrate access
   printf( "  First 100 characters of mapped data:\n");
   printf( "  \"");
   for( i = 0; i < 100 && i < mapped_length; i++)
   {
      if( mapped_data[i] == '\n')
         printf( "\\n");
      else if( mapped_data[i] >= 32 && mapped_data[i] < 127)
         printf( "%c", mapped_data[i]);
      else
         printf( "\\x%02x", (unsigned char) mapped_data[i]);
   }
   printf( "\"\n\n");
   
   // Test byte-by-byte access to verify no access violations
   printf( "  Testing byte-by-byte access to entire mapped region...\n");
   for( i = 0; i < mapped_length; i++)
   {
      volatile unsigned char byte = mapped_data[i];
      if( byte != (unsigned char) test_content[i])
      {
         printf( "    ERROR: Byte mismatch at offset %zu: got 0x%02x, expected 0x%02x\n", 
                 i, byte, (unsigned char) test_content[i]);
         _mulle_mmap_done( &info);
         return;
      }
   }
   printf( "  SUCCESS: All %zu bytes accessible and correct\n\n", mapped_length);
   
   printf( "Phase 4: Cleanup\n");
   
   // Clean up the mmap
   _mulle_mmap_done( &info);
   printf( "  Memory mapping cleaned up\n");
   
   // Remove test file
   unlink( test_filename);
   printf( "  Test file removed\n");
   
   printf( "  Test completed successfully\n\n");
   printf( "=== CONCLUSION: Memory mapping persists after file descriptor close ===\n");
   printf( "This demonstrates that mmap() creates an independent mapping that\n");
   printf( "survives the closure of the original file descriptor.\n\n");
}


static void test_mapping_with_file_removal( void)
{
   struct mulle_mmap   info;
   char                *test_content;
   char                *test_filename;
   char                *mapped_data;
   size_t              mapped_length;
   size_t              test_size;
   int                 rval;
   
   printf( "=== Test: Memory Mapping After File Removal ===\n\n");
   
   // Create test content
   test_content = "Test content for file removal scenario.\n"
                  "This tests whether mapped memory survives file deletion.\n";
   test_size = strlen( test_content);
   test_filename = "removable_test.txt";
   
   // Create the test file
   create_test_file( test_filename, test_content, test_size);
   
   printf( "Phase 1: Mapping file with _mulle_mmap_map_file\n");
   
   // Initialize and map file
   _mulle_mmap_init( &info, mulle_mmap_read);
   
   rval = _mulle_mmap_map_file( &info, test_filename);
   if( rval)
   {
      printf( "ERROR: Failed to map file: %s\n", strerror( errno));
      _mulle_mmap_done( &info);
      return;
   }
   
   mapped_data   = (char *) _mulle_mmap_get_bytes( &info);
   mapped_length = _mulle_mmap_get_length( &info);
   
   printf( "  File mapped successfully: %zu bytes\n", mapped_length);
   
   // Verify initial data access
   if( memcmp( mapped_data, test_content, test_size) != 0)
   {
      printf( "ERROR: Initial data verification failed\n");
      _mulle_mmap_done( &info);
      return;
   }
   printf( "  Initial data verification: SUCCESS\n\n");
   
   printf( "Phase 2: Removing file while mapped\n");
   
   // Remove the file while it's mapped
   if( unlink( test_filename) == -1)
   {
      printf( "ERROR: Failed to remove file: %s\n", strerror( errno));
      _mulle_mmap_done( &info);
      return;
   }
   printf( "  File '%s' removed from filesystem\n", test_filename);
   
   // Verify the mapping is still valid
   if( ! _mulle_mmap_is_mapped( &info))
   {
      printf( "  WARNING: mmap reports as not mapped after file removal\n");
   }
   else
   {
      printf( "  mmap still reports as mapped after file removal\n");
   }
   
   printf( "\nPhase 3: Accessing mapped memory after file removal\n");
   
   // Try to access the mapped data after file removal
   printf( "  Testing memory access after file removal...\n");
   
   if( memcmp( mapped_data, test_content, test_size) != 0)
   {
      printf( "    WARNING: Mapped data changed after file removal\n");
   }
   else
   {
      printf( "  SUCCESS: Mapped data still accessible and correct after file removal\n");
   }
   
   printf( "  First 50 characters: \"");
   for( size_t i = 0; i < 50 && i < mapped_length; i++)
   {
      if( mapped_data[i] >= 32 && mapped_data[i] < 127)
         printf( "%c", mapped_data[i]);
      else if( mapped_data[i] == '\n')
         printf( "\\n");
      else
         printf( "\\x%02x", (unsigned char) mapped_data[i]);
   }
   printf( "\"\n\n");
   
   printf( "Phase 4: Cleanup\n");
   
   // Clean up
   _mulle_mmap_done( &info);
   printf( "  Memory mapping cleaned up\n");
   
   printf( "  Test completed\n\n");
   printf( "=== CONCLUSION: Memory mapping behavior with file removal ===\n");
   printf( "On most Unix systems, mapped memory survives file removal because\n");
   printf( "the kernel maintains the file data until all references are closed.\n\n");
}


int main( void)
{
   printf( "=== Memory Mapping Persistence Tests ===\n\n");
   printf( "These tests verify fundamental mmap() behavior:\n");
   printf( "1. Mapped memory remains accessible after closing file descriptor\n");
   printf( "2. Mapped memory behavior when original file is removed\n\n");
   
   // Test 1: Mapping persistence after file descriptor close
   test_persistent_mapping_after_close();
   
   // Test 2: Mapping behavior after file removal
   test_mapping_with_file_removal();
   
   printf( "=== All Memory Mapping Persistence Tests Completed ===\n");
   return( 0);
}
