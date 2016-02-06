//===--- AvoidStdBindCheck.cpp - clang-tidy--------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <cassert>
#include "AvoidStdBindCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

void AvoidStdBindCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      cxxConstructExpr(
          hasType(qualType(hasDeclaration(classTemplateSpecializationDecl()))),
          hasArgument(0, callExpr(hasArgument(0, declRefExpr().bind("f")))))
          .bind("bind"),
      this);
}

void AvoidStdBindCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<CXXConstructExpr>("bind");

  auto DiagnosticBuilder =
      diag(MatchedDecl->getLocation(), "use of std::bind is deprecated");

  std::string Buffer;
  llvm::raw_string_ostream Stream(Buffer);

  const auto *F = Result.Nodes.getNodeAs<DeclRefExpr>("f");

  if (!F)
    return;
  Stream << "[] { return " << F->getNameInfo().getName() << "(); };";

  SourceLocation DeclEnd = Lexer::getLocForEndOfToken(
      MatchedDecl->getLocEnd(), 0, *Result.SourceManager,
      Result.Context->getLangOpts());

  SourceRange ReplacedRange(MatchedDecl->getLocStart(), DeclEnd);

  DiagnosticBuilder << FixItHint::CreateReplacement(ReplacedRange,
                                                    Stream.str());
}

} // namespace misc
} // namespace tidy
} // namespace clang
