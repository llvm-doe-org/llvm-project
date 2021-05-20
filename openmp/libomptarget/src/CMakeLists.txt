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

set(LIBOMPTARGET_SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/api.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/interface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rtl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/omptarget.cpp
)

set(LIBOMPTARGET_SRC_FILES ${LIBOMPTARGET_SRC_FILES} PARENT_SCOPE)

include_directories(${LIBOMPTARGET_LLVM_INCLUDE_DIRS})

# Build libomptarget library with libdl dependency. Add LLVMSupport
# dependency if building in-tree with profiling enabled.
if(OPENMP_STANDALONE_BUILD OR (NOT OPENMP_ENABLE_LIBOMPTARGET_PROFILING))
  add_library(omptarget SHARED ${LIBOMPTARGET_SRC_FILES})
  target_link_libraries(omptarget
    PUBLIC
      ${CMAKE_DL_LIBS}
    PRIVATE
      "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports")
else()
  set(LLVM_LINK_COMPONENTS
    Support
    )
  add_llvm_library(omptarget SHARED ${LIBOMPTARGET_SRC_FILES}
      LINK_LIBS
      PUBLIC
        ${CMAKE_DL_LIBS}
      PRIVATE
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports"
      )
  target_compile_definitions(omptarget PUBLIC OMPTARGET_PROFILE_ENABLED)
endif()
if(OPENMP_CLANG_LIB_SUBDIR AND NOT OPENMP_STANDALONE_BUILD)
  add_custom_command(TARGET omptarget POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
      $<TARGET_FILE_DIR:omptarget>/${OPENMP_CLANG_LIB_SUBDIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:omptarget>
      $<TARGET_FILE_DIR:omptarget>/${OPENMP_CLANG_LIB_SUBDIR}
  )
endif()

# libomptarget needs to be set separately because add_llvm_library doesn't
# conform with location configuration of its parent scope.
set_target_properties(omptarget
  PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${LIBOMPTARGET_LIBRARY_DIR})

# Install libomptarget under the lib destination folder.
install(TARGETS omptarget LIBRARY COMPONENT omptarget
  DESTINATION "${OPENMP_INSTALL_LIBDIR}")
if(OPENMP_CLANG_LIB_SUBDIR)
  install(TARGETS omptarget LIBRARY COMPONENT omptarget DESTINATION
    "${OPENMP_INSTALL_LIBDIR}/${OPENMP_CLANG_LIB_SUBDIR}")
endif()

# Set the -I for headers from the runtime.
include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}/../../runtime/src
  ${CMAKE_CURRENT_BINARY_DIR}/../../libacc2omp/src
)
