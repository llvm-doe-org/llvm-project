//===-- clang/lib/AST/CommentedStream.h - Formatted streams -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a raw_ostream implementation for commenting every line
// with C++-style comments ("//").
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_AST_COMMENTEDSTREAM_H
#define LLVM_CLANG_LIB_AST_COMMENTEDSTREAM_H

#include "llvm/Support/raw_ostream.h"

namespace clang {

/// commented_raw_ostream - A raw_ostream that wraps another one to comment
/// every line with C++-style comments ("//").
///
/// The implementation is loosely based on llvm::formatted_raw_ostream.
///
class commented_raw_ostream : public llvm::raw_ostream {
  /// TheStream - The real stream we output to.
  raw_ostream &TheStream;

  /// IndentPre - The number of space characters to insert before the "//" at
  /// the beginning of each line.
  unsigned IndentPre;

  /// IndentPreReuses - Whether IndentPre should reuse any existing spaces at
  /// the beginning of each line.
  bool IndentPreReuses;

  /// IndentPost - The number of space characters to insert after the "//" at
  /// the beginning of each line.
  unsigned IndentPost;

  /// ComStart - Whether the next character is the start of a new line.
  bool ComStart;

  void write_impl(const char *Ptr, size_t Size) override;

  /// current_pos - Return the current position within the stream.
  uint64_t current_pos() const override {
    // Our current position in the stream is all the contents which have been
    // written to the underlying stream (*not* the current position of the
    // underlying stream).
    return TheStream.tell();
  }

public:
  /// commented_raw_ostream - Open the specified file for
  /// writing. If an error occurs, information about the error is
  /// put into ErrorInfo, and the stream should be immediately
  /// destroyed; the string will be empty if no error occurred.
  ///
  /// IndentPre and IndentPost specify the number of space characters to insert
  /// before and after the "//" at the beginning of each line.
  ///
  /// If IndentPreReuses, then the "//" is instead inserted after IndentPre
  /// existing leading spaces, which are first extended to IndentPre spaces if
  /// there are fewer.
  ///
  /// ComStart specifies whether to insert indentation and "//" immediately
  /// on the next character because that character is assumed to start a new
  /// line.
  ///
  commented_raw_ostream(raw_ostream &Stream, unsigned IndentPre,
                        bool IndentPreReuses, unsigned IndentPost,
                        bool ComStart)
      : raw_ostream(true), TheStream(Stream), IndentPre(IndentPre),
        IndentPreReuses(IndentPreReuses), IndentPost(IndentPost),
        ComStart(ComStart) {}

  raw_ostream &resetColor() override {
    TheStream.resetColor();
    return *this;
  }

  raw_ostream &reverseColor() override {
    TheStream.reverseColor();
    return *this;
  }

  raw_ostream &changeColor(enum Colors Color, bool Bold, bool BG) override {
    TheStream.changeColor(Color, Bold, BG);
    return *this;
  }

  bool is_displayed() const override {
    return TheStream.is_displayed();
  }
};

} // end clang namespace


#endif
