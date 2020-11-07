##===----------------------------------------------------------------------===##
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
##===----------------------------------------------------------------------===##

# void libacc2omp_say(string message_to_user);
# - prints out message_to_user
macro(libacc2omp_say message_to_user)
  message(STATUS "LIBACC2OMP: ${message_to_user}")
endmacro()

# void libacc2omp_warning_say(string message_to_user);
# - prints out message_to_user with a warning
macro(libacc2omp_warning_say message_to_user)
  message(WARNING "LIBACC2OMP: ${message_to_user}")
endmacro()

# void libacc2omp_error_say(string message_to_user);
# - prints out message_to_user with an error and exits cmake
macro(libacc2omp_error_say message_to_user)
  message(FATAL_ERROR "LIBACC2OMP: ${message_to_user}")
endmacro()
