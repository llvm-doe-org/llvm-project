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
configure_file(config.h.cmake config.h @ONLY)

# Configure the OpenACC standard headers and libacc2omp headers.
#
# Similar to LLVM's OpenMP headers (such as omp-tools.h), there may currently be
# no substitutions in some headers, but there may be one day, and this at least
# ensures headers are all consistently placed in the build directory.
configure_file(include/openacc.h.var openacc.h @ONLY)
configure_file(include/acc_prof.h.var acc_prof.h @ONLY)
configure_file(include/acc2omp-handlers.h.var acc2omp-handlers.h @ONLY)
configure_file(include/acc2omp-backend.h.var acc2omp-backend.h @ONLY)

# Build libacc2omp library with libdl and libomp dependencies.
add_library(acc2omp SHARED backend-internal.cpp api.cpp prof.cpp)
target_link_libraries(acc2omp
  PUBLIC
    omp omptarget ${CMAKE_DL_LIBS}
  PRIVATE
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports/acc2omp.txt")

# Install libacc2omp everywhere LLVM's libomp is installed.  See
# acc2omp-backend.h header comments for further discussion.
install(TARGETS acc2omp LIBRARY COMPONENT acc2omp
  DESTINATION "${OPENMP_INSTALL_LIBDIR}")

# Set the -I for headers from the runtime.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../../runtime/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../runtime/src)

# Install the OpenACC standard headers and libacc2omp headers.
if(${OPENMP_STANDALONE_BUILD})
  set(LIBACC2OMP_HEADERS_INSTALL_PATH "${CMAKE_INSTALL_INCLUDEDIR}")
else()
  string(REGEX MATCH "[0-9]+\\.[0-9]+(\\.[0-9]+)?" CLANG_VERSION ${PACKAGE_VERSION})
  set(LIBACC2OMP_HEADERS_INSTALL_PATH "${OPENMP_INSTALL_LIBDIR}/clang/${CLANG_VERSION}/include")
endif()
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/openacc.h
    ${CMAKE_CURRENT_BINARY_DIR}/acc_prof.h
    ${CMAKE_CURRENT_BINARY_DIR}/acc2omp-handlers.h
    ${CMAKE_CURRENT_BINARY_DIR}/acc2omp-backend.h
  DESTINATION
    "${LIBACC2OMP_HEADERS_INSTALL_PATH}"
)

# Make this available for testing.
set(LIBACC2OMP_LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)
