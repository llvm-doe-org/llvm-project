//===-- clang/lib/AST/CommentedStream.cpp - Formatted streams ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the implementation of commented_raw_ostream.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/CommentedStream.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>

using namespace clang;

void commented_raw_ostream::write_impl(const char *Ptr, size_t Size) {
  for (const char *End = Ptr + Size; Ptr != End; ++Ptr) {
    if (ComStart) {
      for (unsigned i = 0; i < IndentPre; ++i) {
        if (IndentPreReuses && Ptr != End && *Ptr == ' ')
          ++Ptr;
        TheStream.write(' ');
      }
      TheStream.write("//", 2);
      for (unsigned i = 0; i < IndentPost; ++i)
        TheStream.write(' ');
      ComStart = false;
      if (Ptr == End)
        break;
    }
    TheStream.write(*Ptr);
    if (*Ptr == '\n')
      ComStart = true;
  }
}
