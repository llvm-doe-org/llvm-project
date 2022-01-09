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
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace clang;

namespace {
class RewriteOpenACC : public ASTConsumer,
                       public RecursiveASTVisitor<RewriteOpenACC> {
  std::string InFileName;
  Rewriter Rewrite;
  std::unique_ptr<raw_ostream> OutFile;
  ASTContext *Context;
  Preprocessor &PP;
  OpenACCPrintKind OpenACCPrint;

  /// Where \p RecordedEnd is the reported end location for an OpenACC
  /// directive's associated code, assume it is the location of the token before
  /// final semicolon if \p FinalSemicolonIsNext, or assume it is the location
  /// of the final semicolon or right brace otherwise.  Return the location of
  /// the final token if it is not within a macro expansion.  Otherwise, report
  /// an error using \p reportError, and return an invalid location.
  SourceLocation getStmtRewritableEnd(
      SourceLocation RecordedEnd, bool FinalSemicolonIsNext,
      const llvm::function_ref<void(SourceLocation Loc)> reportError) {
    SourceManager &SM = Context->getSourceManager();
    const LangOptions &LO = Context->getLangOpts();
    if (!FinalSemicolonIsNext) {
#ifndef NDEBUG
      Token Tok;
      Lexer::getRawToken(SM.getSpellingLoc(RecordedEnd), Tok, SM, LO);
      tok::TokenKind Kind = Tok.getKind();
      char Ch = SM.getCharacterData(RecordedEnd)[0];
      assert((Kind == tok::r_brace || Kind == tok::semi) &&
             "expected Stmt's end token to be a right brace or semicolon");
      assert((Ch == '}' || Ch == ';') &&
             "expected token to look like a right brace or semicolon");
#endif
      if (!Rewrite.isRewritable(RecordedEnd)) {
        // The final token (semicolon or closing brace) is within a macro
        // expansion, so refuse to rewrite.
        reportError(RecordedEnd);
        return SourceLocation();
      }
      return RecordedEnd;
    }
#ifndef NDEBUG
    Token Tok;
    Lexer::getRawToken(SM.getSpellingLoc(RecordedEnd), Tok, SM, LO);
    assert(Tok.getKind() != tok::semi &&
           "expected recorded end token not to be a semicolon");
#endif
    Optional<Token> Next = Lexer::findNextToken(RecordedEnd, SM, LO);
    // If Tok is expanded from a macro and is not the last token in the
    // expansion, findNextToken refuses to look for the next token and returns
    // None.  Assume the final semicolon is the next token and is thus within
    // the expansion, and so refuse to rewrite.
    if (!Next.hasValue()) {
      reportError(RecordedEnd);
      return SourceLocation();
    }
    // If Next is an identifier, assume it's a macro whose expansion's first
    // token is the final semicolon, and so refuse to rewrite.
    if (Next.getValue().getKind() == tok::raw_identifier) {
      reportError(Next.getValue().getLocation());
      return SourceLocation();
    }
    assert(Next.getValue().getKind() == tok::semi &&
           "expected semicolon at end of OpenACC associated code");
    SourceLocation SemiLoc = Next.getValue().getLocation();
    assert(SM.getCharacterData(SemiLoc)[0] == ';' &&
           "expected tok::semi to look like a semicolon");
    assert(Rewrite.isRewritable(SemiLoc) &&
           "expected Lexer::findNextToken not to return token within macro "
           "expansion");
    return SemiLoc;
  }

public:
  RewriteOpenACC(StringRef InFileName, std::unique_ptr<raw_ostream> OS,
                 CompilerInstance &CI)
      : InFileName(InFileName), OutFile(std::move(OS)), Context(nullptr),
        PP(CI.getPreprocessor()),
        OpenACCPrint(CI.getFrontendOpts().OpenACCPrint) {
  }
  ~RewriteOpenACC() override {}
  void HandleTranslationUnit(ASTContext &C) override {
    Context = &C;
    SourceManager &SM = Context->getSourceManager();
    Rewrite.setSourceMgr(SM, Context->getLangOpts());
    if (OpenACCPrint != OpenACCPrint_ACC) {
      // If the output will be (uncommented) OpenMP, and if the original
      // _OPENACC macro definition from Clang was ever used, insert it at the
      // beginning of the translation unit.
      if (OpenACCPrint != OpenACCPrint_ACC_OMP) {
        MacroDirective *MD =
          PP.getLocalMacroDirectiveHistory(PP.getIdentifierInfo("_OPENACC"));
        assert(MD && "expected _OPENACC to be defined");
        while (MacroDirective *Prev = MD->getPrevious())
          MD = Prev;
        if (MD->getMacroInfo()->isUsed()) {
          SourceLocation Loc = SM.getComposedLoc(SM.getMainFileID(), 0);
          Context->getDiagnostics().Report(
              Loc, diag::warn_rewrite_acc_omp_openacc_macro_inserted);
          Context->getDiagnostics().Report(
              Loc, diag::note_rewrite_acc_omp_openacc_macro_inserted);
          const char *Start =
              SM.getCharacterData(MD->getDefinition().getLocation());
          const char *End = Start;
          while (*End != '\n' && *End != 0)
            ++End;
          *OutFile << "#define " << StringRef(Start, End - Start)
                   << " // inserted by Clang for OpenACC-to-OpenMP translation"
                   << '\n';
        }
      }
      // Find directives to rewrite.
      TraverseAST(*Context);
    }
    Rewrite.getEditBuffer(SM.getMainFileID()).write(*OutFile);
    Context = nullptr;
  }
  bool TraverseStmt(Stmt *S, DataRecursionQueue *Queue = nullptr) {
    // When our VisitACCDirectiveStmt returns false, it's to indicate that
    // children of the ACCNode shouldn't be visited, but we still want to
    // continue on to other parts of the tree, so we return true here.
    if (ACCDirectiveStmt *ACCNode = dyn_cast_or_null<ACCDirectiveStmt>(S)) {
      if (VisitACCDirectiveStmt(ACCNode) && ACCNode->hasAssociatedStmt())
        return TraverseStmt(ACCNode->getAssociatedStmt(), Queue);
      return true;
    }
    return RecursiveASTVisitor<RewriteOpenACC>::TraverseStmt(S, Queue);
  }
  bool VisitACCDirectiveStmt(ACCDirectiveStmt *ACCNode) {
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
    // be unusable for the sake of rewriting anyway (based on how it renders in
    // diagnostics, the spelling column points to the end of the _Pragma's
    // string, but we haven't verified that against the parser implementation).
    // In that case, we force a full construct rewrite so that we don't need the
    // _Pragma's end location, and we complain if there is no associated
    // statement.
    //
    // If a full construct rewrite is required, either because of _Pragma form
    // or because the associated statement requires modification, we also check
    // the end location of the associated statement and complain if it appears
    // within a macro expansion.
    //
    // In summary, we complain using either:
    // - err_rewrite_acc_start_in_macro if the start of the pragma appears in a
    //   macro expansion (must be _Pragma form).
    // - err_rewrite_acc_end_in_pragma_op if there is no associated statement
    //   and we have _Pragma form.
    // - err_rewrite_acc_end_in_macro if the associated statement must be
    //   rewritten (always true for _Pragma form) but its last token appears in
    //   a macro expansion.
    PrintingPolicy Policy(Context->getLangOpts());
    SourceRange DirectiveRange = ACCNode->getDirectiveRange();
    if (!Rewrite.isRewritable(DirectiveRange.getBegin())) {
      Context->getDiagnostics().Report(DirectiveRange.getBegin(),
                                       diag::err_rewrite_acc_start_in_macro);
      return false;
    }
    if (!Rewrite.isRewritable(DirectiveRange.getEnd()) &&
        !ACCNode->hasAssociatedStmt()) {
      Context->getDiagnostics().Report(DirectiveRange.getEnd(),
                                       diag::err_rewrite_acc_end_in_pragma_op);
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
      SourceLocation End =
          getStmtRewritableEnd(RewriteRange.getEnd(), FinalSemicolonIsNext,
                               [this](SourceLocation Loc) {
                                 Context->getDiagnostics().Report(
                                     Loc, diag::err_rewrite_acc_end_in_macro);
                               });
      if (End.isInvalid())
        return false;
      // The end location points at the start of the end token, but our rewrite
      // range needs to point immediately afterward.
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

    // Generate new text.
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
                                                 /*IndentPreReuses=*/false,
                                                 /*IndentPost=*/1,
                                                 /*ComStart=*/false);
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
                                               /*IndentPreReuses=*/false,
                                               /*IndentPost=*/1,
                                               /*ComStart=*/false);
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
                                                 /*IndentPreReuses=*/true,
                                                 /*IndentPost=*/1,
                                                 /*ComStart=*/false);
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
                                                 /*IndentPreReuses=*/true,
                                                 /*IndentPost=*/1,
                                                 /*ComStart=*/true);
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
                                               /*IndentPreReuses=*/true,
                                               /*IndentPost=*/1,
                                               /*ComStart=*/true);
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
  bool VisitDecl(Decl *D) {
    SourceManager &SM = Context->getSourceManager();
    const LangOptions &LO = Context->getLangOpts();

    // Is this a function prototype or definition?
    FunctionDecl *FD = dyn_cast<FunctionDecl>(D);
    if (!FD)
      return true;

    // Is there an OpenACC routine directive to rewrite?
    ACCDeclAttr *ACCAttr = FD->getAttr<ACCRoutineDeclAttr>();
    if (!ACCAttr)
      return true;
    InheritableAttr *OMPAttr = ACCAttr->getOMPNode(FD);
    if (!OMPAttr)
      return true;
    assert(ACCAttr->isInherited() == OMPAttr->isInherited() &&
           "expected OpenACC attribute and its OpenMP translation to either "
           "both be inherited or neither");
    if (ACCAttr->isInherited())
      return true;
    assert(!ACCAttr->isImplicit() &&
           "expected that implicit ACCDeclAttr is never generated");
    assert(!OMPAttr->isImplicit() &&
           "expected that explicit ACCDeclAttr is never translated to "
           "implicit OMPDeclAttr");

    // Can we rewrite the directive?
    //
    // TODO: If the directive is in a _Pragma, we give up.  Maybe we can try
    // harder as we do in VisitACCDirectiveStmt.  See comments there for how all
    // this works.  To summarize: If the directive end location is not
    // rewritable, apparently we have _Pragma form.  Otherwise, we have #pragma
    // form, whose begin and end locations are always rewritable.
    SourceRange DirectiveRange = ACCAttr->getRange();
    if (!Rewrite.isRewritable(DirectiveRange.getEnd())) {
      Context->getDiagnostics().Report(
          DirectiveRange.getEnd(), diag::err_rewrite_acc_routine_in_pragma_op);
      return true;
    }
    assert(Rewrite.isRewritable(DirectiveRange.getBegin()) &&
           "expected start of directive to be rewritable because end is");

    // Can we insert after the associated function prototype or definition?
    //
    // For a function prototype, the end location is always the token before the
    // final semicolon.  For a function definition, the end location is always
    // the final right brace.
    SourceLocation FDEnd = getStmtRewritableEnd(
        FD->getEndLoc(), /*FinalSemicolonIsNext=*/FD != FD->getDefinition(),
        [this, ACCAttr](SourceLocation Loc) {
          Context->getDiagnostics().Report(
              Loc, diag::err_rewrite_acc_routine_function_end_in_macro)
              << ACCAttr->isImplicit();
        });
    if (FDEnd.isInvalid())
      return true;

    // What is the existing indentation?
    StringRef IndentText =
        Lexer::getIndentationForLine(DirectiveRange.getBegin(), SM);
    unsigned IndentWidth = IndentText.size();

    // Set up buffers for the new text.
    std::string RewriteBeginString;
    std::string RewriteEndString;
    llvm::raw_string_ostream RewriteBeginStream(RewriteBeginString);
    llvm::raw_string_ostream RewriteEndStream(RewriteEndString);

    // Generate new text.
    PrintingPolicy PolicyOMP(LO);
    PolicyOMP.OpenACCPrint = OpenACCPrint_OMP;
    RewriteEndStream << '\n' << IndentText;
    switch (OpenACCPrint) {
    case OpenACCPrint_ACC:
    case OpenACCPrint_OMP_HEAD:
      llvm_unreachable("unexpected OpenACC print kind while rewriting input");
    case OpenACCPrint_OMP:
      OMPAttr->printPretty(RewriteBeginStream, PolicyOMP);
      break;
    case OpenACCPrint_OMP_ACC: {
      OMPAttr->printPretty(RewriteBeginStream, PolicyOMP);
      clang::commented_raw_ostream ComStream(RewriteBeginStream, IndentWidth,
                                             /*IndentPreReuses=*/true,
                                             /*IndentPost=*/1,
                                             /*ComStart=*/true);
      ComStream << Rewrite.getRewrittenText(
          CharSourceRange::getCharRange(DirectiveRange));
      break;
    }
    case OpenACCPrint_ACC_OMP: {
      RewriteBeginStream << Rewrite.getRewrittenText(
          CharSourceRange::getCharRange(DirectiveRange));
      RewriteBeginStream << '\n' << IndentText;
      clang::commented_raw_ostream ComStream(RewriteBeginStream, IndentWidth,
                                             /*IndentPreReuses=*/false,
                                             /*IndentPost=*/1,
                                             /*ComStart=*/false);
      ComStream << "// ";
      OMPAttr->printPretty(ComStream, PolicyOMP);
      RewriteEndStream << "// ";
      break;
    }
    }
    RewriteEndStream << "#pragma omp end declare target";
    // We previously asserted that FDEnd is either ';' or '}'.  The next
    // character comes after that token.  If the rest of the line isn't blank,
    // insert a newline at the end directive.  Otherwise, don't or you'll create
    // a blank line.
    FileID File = SM.getFileID(FDEnd);
    StringRef Buffer = SM.getBufferData(File);
    for (const char *P = SM.getCharacterData(FDEnd) + 1;
         P != Buffer.end() && *P != '\n'; ++P) {
      if (!std::isspace(*P)) {
        RewriteEndStream << '\n';
        break;
      }
    }
    RewriteBeginStream.flush();
    RewriteEndStream.flush();

    // RewriteBeginString will replace DirectiveRange, which doesn't include
    // the original directive's trailing newline, so don't include one here.
    if (RewriteBeginString.back() == '\n')
      RewriteBeginString.pop_back();

    // Perform replacement.
    Rewrite.ReplaceText(CharSourceRange::getCharRange(DirectiveRange),
                        RewriteBeginString);
    Rewrite.InsertTextAfterToken(FDEnd, RewriteEndString);
    return true;
  }
};
} // end anonymous namespace

std::unique_ptr<ASTConsumer>
clang::CreateOpenACCRewriter(StringRef InFile, std::unique_ptr<raw_ostream> OS,
                             CompilerInstance &CI) {
  return std::make_unique<RewriteOpenACC>(InFile, std::move(OS), CI);
}
