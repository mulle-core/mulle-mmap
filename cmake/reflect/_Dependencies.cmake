# This file will be regenerated by `mulle-sourcetree-to-cmake` via
# `mulle-sde reflect` and any edits will be lost.
#
# This file will be included by cmake/share/Files.cmake
#
# Disable generation of this file with:
#
# mulle-sde environment set MULLE_SOURCETREE_TO_CMAKE_DEPENDENCIES_FILE DISABLE
#
if( MULLE_TRACE_INCLUDE)
   message( STATUS "# Include \"${CMAKE_CURRENT_LIST_FILE}\"" )
endif()

#
# Generated from sourcetree: 360388C4-28A5-43B9-B5DE-858CE8E4A9BA;mulle-c11;no-all-load,no-cmake-inherit,no-import,no-link,no-recurse,no-singlephase;
# Disable with : `mulle-sourcetree mark mulle-c11 no-header`
# Disable for this platform: `mulle-sourcetree mark mulle-c11 no-cmake-platform-${MULLE_UNAME}`
# Disable for a sdk: `mulle-sourcetree mark mulle-c11 no-cmake-sdk-<name>`
#
if( NOT MULLE_C11_HEADER)
   find_file( MULLE_C11_HEADER NAMES mulle-c11.h mulle-c11/mulle-c11.h)
   message( STATUS "MULLE_C11_HEADER is ${MULLE_C11_HEADER}")

   #
   # Add MULLE_C11_HEADER to HEADER_ONLY_LIBRARIES list.
   # Disable with: `mulle-sourcetree mark mulle-c11 no-cmake-add`
   #
   set( HEADER_ONLY_LIBRARIES
      ${MULLE_C11_HEADER}
      ${HEADER_ONLY_LIBRARIES}
   )
   if( MULLE_C11_HEADER)
      # intentionally left blank
   else()
      # Disable with: `mulle-sourcetree mark mulle-c11 no-require`
      message( FATAL_ERROR "MULLE_C11_HEADER was not found")
   endif()
endif()
