//===--- RewriteOpenACC.cpp - Rewriter from OpenACC to OpenMP -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// For rewriting the input buffer's OpenACC to OpenMP based on the AST, which
// already includes both versions.
//
//===----------------------------------------------------------------------===//

#include "clang/Rewrite/Frontend/ASTConsumers.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/CommentedStream.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtOpenACC.h"
#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace clang;

namespace {
class RewriteOpenACC : public ASTConsumer,
                       public RecursiveASTVisitor<RewriteOpenACC> {
  std::string InFileName;
  Rewriter Rewrite;
  std::unique_ptr<raw_ostream> OutFile;
  ASTContext *Context;
  OpenACCPrintKind OpenACCPrint;

public:
  RewriteOpenACC(std::string InFileName, std::unique_ptr<raw_ostream> OS,
                 OpenACCPrintKind OpenACCPrint)
      : InFileName(InFileName), OutFile(std::move(OS)), Context(nullptr),
        OpenACCPrint(OpenACCPrint) {}
  ~RewriteOpenACC() override {}
  void HandleTranslationUnit(ASTContext &C) override {
    Context = &C;
    SourceManager &SM = Context->getSourceManager();
    Rewrite.setSourceMgr(SM, Context->getLangOpts());
    if (OpenACCPrint != OpenACCPrint_ACC)
      TraverseAST(*Context);
    Rewrite.getEditBuffer(SM.getMainFileID()).write(*OutFile);
    Context = nullptr;
  }
  bool TraverseStmt(Stmt *S, DataRecursionQueue *Queue = nullptr) {
    // When our VisitACCExecutableDirective returns false, it's to indicate
    // that children of the ACCNode shouldn't be visited, but we still want to
    // continue on to other parts of the tree, so we return true here.
    if (ACCExecutableDirective *ACCNode =
            dyn_cast_or_null<ACCExecutableDirective>(S)) {
      if (VisitACCExecutableDirective(ACCNode) && ACCNode->hasAssociatedStmt())
        return TraverseStmt(ACCNode->getAssociatedStmt(), Queue);
      return true;
    }
    return RecursiveASTVisitor<RewriteOpenACC>::TraverseStmt(S, Queue);
  }
  bool VisitACCExecutableDirective(ACCExecutableDirective *ACCNode) {
    SourceManager &SM = Context->getSourceManager();
    const LangOptions &LO = Context->getLangOpts();

    // FIXME: The end location of ACCNode's pragma alone (that is, not
    // including the associated statement) is wrong for the _Pragma form: it's
    // the start location for some reason.  That's fine when we're getting
    // locations from an associated statement, but we'll need to fix the
    // location somehow in other cases.
    //
    // FIXME: Can we issue warnings when pragmas are not rewritable because
    // they're behind macros?  Try Rewriter::isRewritable.

    // We cannot rewrite if the OpenMP node failed to be constructed.
    if (!ACCNode->hasOMPNode())
      return false;

    // TODO: Handle standalone directives.
    assert(ACCNode->hasAssociatedStmt() &&
           "rewrite of standalone OpenACC directive not yet implemented");

    // Are we rewriting the entire OpenACC construct or just the directive?
    PrintingPolicy Policy(Context->getLangOpts());
    bool DirectiveOnly = !ACCNode->ompStmtPrintsDifferently(Policy, Context);
    SourceRange DirectiveRange = ACCNode->getDirectiveRange();
    SourceRange RewriteRange;
    if (DirectiveOnly)
      RewriteRange = DirectiveRange;
    else {
      // FIXME: The end location for a construct is often wrong for Rewriter.
      // First, unless a statement is a compound statement or a null statement,
      // the statement's end location points at the last token before the final
      // semicolon.  Second, for any statement, the end location points at the
      // start of a token not the end.  This hack works around all that so far,
      // but surely there's a better way.
      RewriteRange = ACCNode->getConstructRange();
      SourceLocation End = RewriteRange.getEnd();
      Token Tok;
      Lexer::getRawToken(End, Tok, SM, LO);
      if (Tok.getKind() != tok::semi && Tok.getKind() != tok::r_brace) {
        Optional<Token> Opt = Lexer::findNextToken(End, SM, LO);
        assert(Opt.hasValue() && Opt.getValue().getKind() == tok::semi &&
               "expected semicolon at end of OpenACC associated statement");
        End = Opt.getValue().getLocation();
      }
      End = Lexer::getLocForEndOfToken(End, 0, SM, LO);
      RewriteRange.setEnd(End);
    }

    // What is the existing indentation of the OpenACC directive?
    StringRef IndentText =
        Lexer::getIndentationForLine(DirectiveRange.getBegin(), SM);
    unsigned IndentWidth = IndentText.size();
    unsigned IndentLevel = IndentWidth / 2;

    // Set up a buffer for the new text.
    bool LastLineCommented = false;
    std::string RewriteString;
    llvm::raw_string_ostream RewriteStream(RewriteString);

    // Add the OpenMP version.  We never reach this point for OpenACCPrint_ACC,
    // so the OpenMP version is always included.  However, if the associated
    // statement wasn't changed and the directive was merely discarded during
    // translation, then there's nothing to print in the OpenMP version.
    if (!DirectiveOnly || !ACCNode->directiveDiscardedForOMP()) {
      PrintingPolicy PolicyOMP(Policy);
      PolicyOMP.OpenACCPrint = DirectiveOnly ? OpenACCPrint_OMP_HEAD
                                             : OpenACCPrint_OMP;
      if (OpenACCPrint == OpenACCPrint_ACC_OMP) {
        clang::commented_raw_ostream ComStream(RewriteStream, IndentWidth,
                                               false, 1, false);
        ComStream << '\n';
        ACCNode->printPretty(ComStream, nullptr, PolicyOMP, 0, "\n", Context);
        LastLineCommented = true;
      } else {
        ACCNode->printPretty(RewriteStream, nullptr, PolicyOMP, IndentLevel,
                             "\n", Context);
        LastLineCommented = false;
      }
    }

    // Add a commented version of the original OpenACC.  Printing the OpenMP
    // above includes a trailing newline, so the OpenACC copy will be separated
    // properly.
    if (OpenACCPrint == OpenACCPrint_OMP_ACC) {
      clang::commented_raw_ostream ComStream(RewriteStream, IndentWidth, true,
                                             1, true);
      ComStream << Rewrite.getRewrittenText(
          CharSourceRange::getCharRange(ACCNode->getDirectiveRange()));
      if (DirectiveOnly) {
        if (ACCNode->directiveDiscardedForOMP())
          RewriteStream << " // discarded in OpenMP translation";
      } else {
        SourceRange Range(DirectiveRange.getEnd(), RewriteRange.getEnd());
        ComStream << Rewrite.getRewrittenText(
            CharSourceRange::getCharRange(Range));
      }
      LastLineCommented = true;
    }

    // If the last line we inserted was commented and there are non-whitespace
    // characters on the remainder of the original line, we need a newline to
    // separate them.
    bool RequiresNewline = false;
    if (LastLineCommented) {
      const char *LineEnd = SM.getCharacterData(RewriteRange.getEnd());
      while (*LineEnd != '\n' && std::isspace(*LineEnd))
        ++LineEnd;
      if (*LineEnd != '\n')
        RequiresNewline = true;
    }

    // Make sure we have a trailing newline if required, but otherwise remove
    // any trailing newline so we don't create a blank line.  For example,
    // printing the OpenMP includes a trailing newline, but copying the OpenACC
    // after that does not.
    RewriteStream.flush();
    if (!RewriteString.empty() && RewriteString.back() == '\n') {
      if (!RequiresNewline)
        RewriteString.pop_back();
    } else if (RequiresNewline)
      RewriteString += '\n';

    // Perform the insertion (if OpenACC is uncommented) or replacement (if
    // OpenMP is uncommented).
    switch (OpenACCPrint) {
    case OpenACCPrint_ACC_OMP:
      if (DirectiveOnly && ACCNode->directiveDiscardedForOMP())
        Rewrite.InsertTextBefore(DirectiveRange.getEnd(),
                                 " // discarded in OpenMP translation");
      if (!RewriteString.empty())
        Rewrite.InsertTextBefore(RewriteRange.getEnd(), RewriteString);
      break;
    case OpenACCPrint_OMP:
    case OpenACCPrint_OMP_ACC:
      if (!RewriteString.empty()) {
        // Trim any leading space as the text we're replacing should already be
        // preceded by any necessary indentation.
        RewriteString.erase(0, RewriteString.find_first_not_of(" "));
        Rewrite.ReplaceText(CharSourceRange::getCharRange(RewriteRange),
                            RewriteString);
      } else {
        // If the line will end up blank, expand the range to include the
        // entire line.
        FileID File = SM.getFileID(RewriteRange.getBegin());
        StringRef Buffer = SM.getBufferData(File);
        bool LineBlank = true;
        const char *RewriteBeg = SM.getCharacterData(RewriteRange.getBegin());
        const char *LineBeg = RewriteBeg;
        const char *BufferBeg = Buffer.data();
        for (; LineBeg != BufferBeg && *(LineBeg - 1) != '\n'; --LineBeg) {
          if (!std::isspace(*(LineBeg-1))) {
            LineBlank = false;
            break;
          }
        }
        if (LineBlank) {
          const char *RewriteEnd = SM.getCharacterData(RewriteRange.getEnd());
          const char *LineEnd = RewriteEnd;
          for (const char *BufferEnd = BufferBeg + Buffer.size();
               LineEnd != BufferEnd && *(LineEnd - 1) != '\n';
               ++LineEnd) {
            if (!std::isspace(*LineEnd)) {
              LineBlank = false;
              break;
            }
          }
          if (LineBlank) {
            RewriteRange.setBegin(SM.getComposedLoc(File, LineBeg-BufferBeg));
            RewriteRange.setEnd(SM.getComposedLoc(File, LineEnd-BufferBeg));
          }
        }
        Rewrite.RemoveText(CharSourceRange::getCharRange(RewriteRange));
      }
      break;
    case OpenACCPrint_ACC:
    case OpenACCPrint_OMP_HEAD:
      llvm_unreachable("unexpected OpenACC print kind while rewriting input");
    }

    // Recurse to children if we didn't print the OpenACC and OpenMP
    // associated statements separately.
    return DirectiveOnly;
  }
};
} // end anonymous namespace

std::unique_ptr<ASTConsumer>
clang::CreateOpenACCRewriter(const std::string &InFile,
                             std::unique_ptr<raw_ostream> OS,
                             OpenACCPrintKind OpenACCPrint) {
  return llvm::make_unique<RewriteOpenACC>(InFile, std::move(OS),
                                           OpenACCPrint);
}
