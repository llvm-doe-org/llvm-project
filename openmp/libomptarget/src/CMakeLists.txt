##===----------------------------------------------------------------------===##
# 
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# 
##===----------------------------------------------------------------------===##
#
# Build offloading library libomptarget.so.
#
##===----------------------------------------------------------------------===##

libomptarget_say("Building offloading runtime library libomptarget.")

set(src_files
  api.cpp
  device.cpp
  interface.cpp
  rtl.cpp
  omptarget.cpp
)

# Build libomptarget library with libdl dependency.
add_library(omptarget SHARED ${src_files})
target_link_libraries(omptarget
  PUBLIC
    ${CMAKE_DL_LIBS}
  PRIVATE
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports")
if(OPENMP_CLANG_LIB_SUBDIR AND NOT OPENMP_STANDALONE_BUILD)
  add_custom_command(TARGET omptarget POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
      $<TARGET_FILE_DIR:omptarget>/${OPENMP_CLANG_LIB_SUBDIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:omptarget>
      $<TARGET_FILE_DIR:omptarget>/${OPENMP_CLANG_LIB_SUBDIR}
  )
endif()

# Install libomptarget under the lib destination folder.
install(TARGETS omptarget LIBRARY COMPONENT omptarget
  DESTINATION "${OPENMP_INSTALL_LIBDIR}")
if(OPENMP_CLANG_LIB_SUBDIR)
  install(TARGETS omptarget LIBRARY COMPONENT omptarget DESTINATION
    "${OPENMP_INSTALL_LIBDIR}/${OPENMP_CLANG_LIB_SUBDIR}")
endif()

# Set the -I for headers from the runtime.
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../../runtime/src)
