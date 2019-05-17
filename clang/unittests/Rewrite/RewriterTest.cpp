//===- unittests/Rewrite/RewriteTest.cpp - RewriteBuffer tests ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/Tooling.h"
#include "gtest/gtest.h"

using namespace clang;

namespace {

struct RangeTypeTest {
  std::unique_ptr<ASTUnit> AST;
  Rewriter Rewrite;
  SourceLocation FileStart;
  CharSourceRange CRange;
  CharSourceRange TRange;
  SourceRange SRange;
  SourceLocation makeLoc(int Off) { return FileStart.getLocWithOffset(Off); }
  CharSourceRange makeCharRange(int StartOff, int EndOff) {
    return CharSourceRange::getCharRange(makeLoc(StartOff), makeLoc(EndOff));
  }
  RangeTypeTest(StringRef Code, int StartOff, int EndOff) {
    AST = tooling::buildASTFromCode(Code);
    ASTContext &C = AST->getASTContext();
    Rewrite = Rewriter(C.getSourceManager(), C.getLangOpts());
    FileStart = AST->getStartOfMainFileID();
    CRange = makeCharRange(StartOff, EndOff);
    SRange = CRange.getAsRange();
    TRange = CharSourceRange::getTokenRange(SRange);
  }
};

TEST(Rewriter, GetRewrittenTextRangeTypes) {
  StringRef Code = "int main() { return 0; }";
  //                             ^~~+++
  RangeTypeTest T(Code, 13, 16);
  EXPECT_EQ(T.Rewrite.getRewrittenText(T.CRange), "ret");
  EXPECT_EQ(T.Rewrite.getRewrittenText(T.TRange), "return");
  EXPECT_EQ(T.Rewrite.getRewrittenText(T.SRange), "return");
  T.Rewrite.InsertText(T.makeLoc(13), "x");
  EXPECT_EQ(T.Rewrite.getRewrittenText(T.CRange), "xret");
  EXPECT_EQ(T.Rewrite.getRewrittenText(T.TRange), "xreturn");
  EXPECT_EQ(T.Rewrite.getRewrittenText(T.SRange), "xreturn");
}

TEST(Rewriter, ReplaceTextRangeTypes) {
  StringRef Code = "int main(int argc, char *argv[]) { return argc; }";
  //                                                          ^~++
  //                                                          ^~~~~
  RangeTypeTest T(Code, 42, 44);
  T.Rewrite.ReplaceText(T.CRange, "foo");
  EXPECT_EQ(T.Rewrite.getRewrittenText(T.makeCharRange(42, 47)), "foogc;");
  T.Rewrite.ReplaceText(T.TRange, "bar");
  EXPECT_EQ(T.Rewrite.getRewrittenText(T.makeCharRange(42, 47)), "bar;");
  T.Rewrite.ReplaceText(T.SRange, "0");
  EXPECT_EQ(T.Rewrite.getRewrittenText(T.makeCharRange(42, 47)), "0;");
}

} // anonymous namespace
