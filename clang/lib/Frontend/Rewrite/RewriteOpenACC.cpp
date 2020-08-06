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
#include "clang/Basic/DiagnosticFrontend.h"
#include "clang/Basic/SourceManager.h"
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
  RewriteOpenACC(StringRef InFileName, std::unique_ptr<raw_ostream> OS,
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

    // We cannot rewrite if the OpenMP node failed to be constructed.
    if (!ACCNode->hasOMPNode())
      return false;

    // Are we rewriting the full OpenACC construct or just the directive?
    //
    // TODO: If the start or end location of the text that needs to be
    // rewritten appears within a macro expansion (as indicated by
    // isRewritable), we do not attempt to rewrite, and we produce a
    // diagnostic.  Those cases make it difficult to rewrite precisely the
    // right text.  It might be possible to handle some subcases, but we
    // haven't implemented that handling yet.
    //
    // #pragma form works as follows.  It should not be possible in C/C++
    // syntax for the start location, which points to the "#pragma" itself, to
    // appear in a macro expansion.  Moreover, Clang's end token for the
    // directive is tok::annot_pragma_openacc_end, which points at the end of
    // the line not the last token within the directive.  Thus, even if the
    // final clauses of the directive expand from a macro, the end location is
    // not within a macro expansion either.
    //
    // _Pragma form works as follows.  If it appears inside a macro expansion,
    // its start and end locations are identified as not rewritable, and so we
    // complain and refuse to rewrite.  FIXME: If it does not appear inside a
    // macro expansion (that's probably rare as that seems to be its purpose in
    // life), its end location is identified as not rewritable and appears to
    // be unusable for the sake of rewriting anyway (we think the spelling
    // column counts the number of characters in the _Pragma string, but we
    // haven't verified that against the parser implementation).  In that case,
    // we force a full construct rewrite so that we don't need the _Pragma's
    // end location, and we then complain if there is no associated statement.
    //
    // If a full construct rewrite is required, either because of _Pragma form
    // or because the associated statement requires modification, we also check
    // the end location of the associated statement and complain if it appears
    // within a macro expansion.
    //
    // In summary, we complain if either (1) the start of the pragma appears in
    // a macro expansion (must be _Pragma form), (2) the associated statement
    // must be rewritten (always true for _Pragma form) but its last token
    // appears in a macro expansion, or (3) there is no associated statement
    // and we have _Pragma form.
    PrintingPolicy Policy(Context->getLangOpts());
    SourceRange DirectiveRange = ACCNode->getDirectiveRange();
    if (!Rewrite.isRewritable(DirectiveRange.getBegin())) {
      Context->getDiagnostics().Report(DirectiveRange.getBegin(),
                                       diag::err_rewrite_acc_start_in_macro);
      return false;
    }
    bool DirectiveOnly = !ACCNode->ompStmtPrintsDifferently(Policy, Context) &&
                         Rewrite.isRewritable(DirectiveRange.getEnd());
    SourceRange RewriteRange;
    if (DirectiveOnly)
      RewriteRange = DirectiveRange;
    else {
      bool FinalSemicolonIsNext;
      RewriteRange = ACCNode->getConstructRange(&FinalSemicolonIsNext);
      SourceLocation End = RewriteRange.getEnd();
      if (FinalSemicolonIsNext) {
#ifndef NDEBUG
        Token Tok;
        Lexer::getRawToken(SM.getSpellingLoc(End), Tok, SM, LO);
        assert(Tok.getKind() != tok::semi &&
               "expected getConstructRange's end token not to be a semicolon");
#endif
        Optional<Token> Next = Lexer::findNextToken(End, SM, LO);
        // If Tok is expanded from a macro and is not the last token in the
        // expansion, findNextToken refuses to look for the next token and
        // returns None.  Assume the final semicolon is the next token and is
        // thus within the expansion, and so refuse to rewrite.
        if (!Next.hasValue()) {
          Context->getDiagnostics().Report(
              End, diag::err_rewrite_acc_end_in_macro);
          return false;
        }
        // If Next is an identifier, assume it's a macro whose expansion's
        // first token is the final semicolon, and so refuse to rewrite.
        if (Next.getValue().getKind() == tok::raw_identifier) {
          Context->getDiagnostics().Report(
              Next.getValue().getLocation(),
              diag::err_rewrite_acc_end_in_macro);
          return false;
        }
        assert(Next.getValue().getKind() == tok::semi &&
               "expected semicolon at end of OpenACC associated statement");
        End = Next.getValue().getLocation();
        assert(Rewrite.isRewritable(End) &&
               "expected Lexer::findNextToken not to return token within "
               "macro expansion");
      } else if (!Rewrite.isRewritable(End)) {
        // The final token (semicolon or closing brace) is within a macro
        // expansion, so refuse to rewrite.
        //
        // FIXME: This should always happen if there's no associated
        // statement and we have _Pragma form, but the following diagnostic
        // would then be misleading.  Currently, Clang doesn't support OpenACC
        // constructs without associated statements, so we haven't developed a
        // diagnostic yet.  The cleanest approach will probably be to check
        // !Rewrite.isRewritable(DirectiveRange.getEnd()) &&
        // !ACCNode->hasAssociatedStmt() right before assigning
        // DirectiveOnly above, which happens after checking
        // !Rewrite.isRewritable(DirectiveRange.getBegin()).
        Context->getDiagnostics().Report(
            End, diag::err_rewrite_acc_end_in_macro);
        return false;
      }
      // The end location points at the start of a token not the end.
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

    switch (OpenACCPrint) {
    case OpenACCPrint_ACC:
    case OpenACCPrint_OMP_HEAD:
      llvm_unreachable("unexpected OpenACC print kind while rewriting input");
    case OpenACCPrint_OMP:
      if (DirectiveOnly) {
        if (!ACCNode->directiveDiscardedForOMP()) {
          PrintingPolicy PolicyOMP(Policy);
          PolicyOMP.OpenACCPrint = OpenACCPrint_OMP_HEAD;
          ACCNode->printPretty(RewriteStream, nullptr, PolicyOMP, IndentLevel,
                               "\n", Context);
        }
      } else {
        PrintingPolicy PolicyOMP(Policy);
        PolicyOMP.OpenACCPrint = OpenACCPrint_OMP;
        ACCNode->printPretty(RewriteStream, nullptr, PolicyOMP, IndentLevel,
                             "\n", Context);
      }
      LastLineCommented = false;
      break;
    case OpenACCPrint_ACC_OMP:
      if (DirectiveOnly) {
        RewriteStream << Rewrite.getRewrittenText(
            CharSourceRange::getCharRange(ACCNode->getDirectiveRange()));
        if (ACCNode->directiveDiscardedForOMP())
          RewriteStream << " // discarded in OpenMP translation";
        else {
          clang::commented_raw_ostream ComStream(RewriteStream, IndentWidth,
                                                 false, 1, false);
          ComStream << '\n';
          PrintingPolicy PolicyOMP(Policy);
          PolicyOMP.OpenACCPrint = OpenACCPrint_OMP_HEAD;
          ACCNode->printPretty(ComStream, nullptr, PolicyOMP, 0, "\n",
                               Context);
        }
      } else {
        RewriteStream << "// v----------ACC----------v\n"
                      << IndentText
                      << Rewrite.getRewrittenText(
                             CharSourceRange::getCharRange(RewriteRange));
        clang::commented_raw_ostream ComStream(RewriteStream, IndentWidth,
                                               false, 1, false);
        ComStream << "\n---------ACC->OMP--------\n";
        PrintingPolicy PolicyOMP(Policy);
        PolicyOMP.OpenACCPrint = OpenACCPrint_OMP;
        ACCNode->printPretty(ComStream, nullptr, PolicyOMP, 0, "\n", Context);
        ComStream << "^----------OMP----------^";
      }
      LastLineCommented = true;
      break;
    case OpenACCPrint_OMP_ACC:
      if (DirectiveOnly) {
        if (ACCNode->directiveDiscardedForOMP()) {
          clang::commented_raw_ostream ComStream(RewriteStream, IndentWidth,
                                                 true, 1, false);
          ComStream << "// "
                    << Rewrite.getRewrittenText(
                           CharSourceRange::getCharRange(
                               ACCNode->getDirectiveRange()))
                    << " // discarded in OpenMP translation";
        } else {
          PrintingPolicy PolicyOMP(Policy);
          PolicyOMP.OpenACCPrint = OpenACCPrint_OMP_HEAD;
          ACCNode->printPretty(RewriteStream, nullptr, PolicyOMP, IndentLevel,
                               "\n", Context);
          clang::commented_raw_ostream ComStream(RewriteStream, IndentWidth,
                                                 true, 1, true);
          ComStream << Rewrite.getRewrittenText(
              CharSourceRange::getCharRange(ACCNode->getDirectiveRange()));
        }
      } else {
        RewriteStream << "// v----------OMP----------v\n";
        PrintingPolicy PolicyOMP(Policy);
        PolicyOMP.OpenACCPrint = OpenACCPrint_OMP;
        ACCNode->printPretty(RewriteStream, nullptr, PolicyOMP, IndentLevel,
                             "\n", Context);
        clang::commented_raw_ostream ComStream(RewriteStream, IndentWidth,
                                               true, 1, true);
        ComStream << "---------OMP<-ACC--------\n";
        ComStream << Rewrite.getRewrittenText(
                         CharSourceRange::getCharRange(RewriteRange));
        ComStream << "\n^----------ACC----------^";
      }
      LastLineCommented = true;
      break;
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

    // Trim any leading space as the text we're replacing should already be
    // preceded by any necessary indentation.
    RewriteString.erase(0, RewriteString.find_first_not_of(" "));

    // Perform the replacement.
    if (!RewriteString.empty())
      Rewrite.ReplaceText(CharSourceRange::getCharRange(RewriteRange),
                          RewriteString);
    else {
      // If the line will end up blank, expand the range to include the entire
      // line.
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

    // Recurse to children if we didn't print the OpenACC and OpenMP
    // associated statements separately.
    return DirectiveOnly;
  }
};
} // end anonymous namespace

std::unique_ptr<ASTConsumer>
clang::CreateOpenACCRewriter(StringRef InFile, std::unique_ptr<raw_ostream> OS,
                             OpenACCPrintKind OpenACCPrint) {
  return std::make_unique<RewriteOpenACC>(InFile, std::move(OS), OpenACCPrint);
}
