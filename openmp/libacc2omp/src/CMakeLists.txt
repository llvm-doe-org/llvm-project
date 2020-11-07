##===----------------------------------------------------------------------===##
# 
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# 
##===----------------------------------------------------------------------===##
#
# Build OpenACC runtime libraries and headers.
#
##===----------------------------------------------------------------------===##

libacc2omp_say("Building OpenACC runtime libraries.")

# Configure libacc2omp.
configure_file(acc2omp-config.h.cmake acc2omp-config.h @ONLY)

# Configure the OpenACC standard headers and acc2omp-proxy.h.
#
# Similar to LLVM's OpenMP headers (such as omp-tools.h), there may currently be
# no substitutions in some headers, but there may be one day, and this at least
# ensures headers are all consistently placed in the build directory.
configure_file(include/openacc.h.var openacc.h @ONLY)
configure_file(include/acc_prof.h.var acc_prof.h @ONLY)
configure_file(include/acc2omp-proxy.h.var acc2omp-proxy.h @ONLY)

# Build libacc2omp library with libdl and libacc2omp-proxy dependencies.
add_library(acc2omp SHARED acc-prof.cpp acc2omp-proxy-internal.cpp)
target_link_libraries(acc2omp
  PUBLIC
    acc2omp-proxy ${CMAKE_DL_LIBS}
  PRIVATE
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports/acc2omp.txt")
if(OPENMP_CLANG_LIB_SUBDIR AND NOT OPENMP_STANDALONE_BUILD)
  add_custom_command(TARGET acc2omp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
      $<TARGET_FILE_DIR:acc2omp>/${OPENMP_CLANG_LIB_SUBDIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:acc2omp>
      $<TARGET_FILE_DIR:acc2omp>/${OPENMP_CLANG_LIB_SUBDIR}
  )
endif()

# Build libacc2omp-proxy library.  See acc2omp-proxy.h header comments for why
# the library name does not match the source file name.
add_library(acc2omp-proxy SHARED acc2omp-proxy-llvm.cpp)
target_link_libraries(acc2omp-proxy
  PUBLIC
    omp
  PRIVATE
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports/acc2omp-proxy.txt")
if(OPENMP_CLANG_LIB_SUBDIR AND NOT OPENMP_STANDALONE_BUILD)
  add_custom_command(TARGET acc2omp-proxy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
      $<TARGET_FILE_DIR:acc2omp-proxy>/${OPENMP_CLANG_LIB_SUBDIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:acc2omp-proxy>
      $<TARGET_FILE_DIR:acc2omp-proxy>/${OPENMP_CLANG_LIB_SUBDIR}
  )
endif()

# Install libacc2omp and libacc2omp-proxy everywhere LLVM's libomp is installed.
# See acc2omp-proxy.h header comments for further discussion.
install(TARGETS acc2omp LIBRARY COMPONENT acc2omp
  DESTINATION "${OPENMP_INSTALL_LIBDIR}")
install(TARGETS acc2omp-proxy LIBRARY COMPONENT acc2omp-proxy
  DESTINATION "${OPENMP_INSTALL_LIBDIR}")
if(OPENMP_CLANG_LIB_SUBDIR)
  install(TARGETS acc2omp LIBRARY COMPONENT acc2omp DESTINATION
    "${OPENMP_INSTALL_LIBDIR}/${OPENMP_CLANG_LIB_SUBDIR}")
  install(TARGETS acc2omp-proxy LIBRARY COMPONENT acc2omp-proxy
    DESTINATION "${OPENMP_INSTALL_LIBDIR}/${OPENMP_CLANG_LIB_SUBDIR}")
endif()

# Set the -I for headers from the runtime.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../../runtime/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/src)

# Install the OpenACC standard headers and acc2omp-proxy.h.
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/openacc.h
    ${CMAKE_CURRENT_BINARY_DIR}/acc_prof.h
    ${CMAKE_CURRENT_BINARY_DIR}/acc2omp-proxy.h
  DESTINATION
    include
)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/openacc.h
    ${CMAKE_CURRENT_BINARY_DIR}/acc_prof.h
    ${CMAKE_CURRENT_BINARY_DIR}/acc2omp-proxy.h
  DESTINATION
    "${OPENMP_INSTALL_LIBDIR}/${OPENMP_CLANG_INCLUDE_SUBDIR}"
)
