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
          hasArgument(0, callExpr(hasArgument(0, declRefExpr().bind("f")))
                             .bind("call")))
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
  Stream << "[] { return " << F->getNameInfo().getName() << "(";
  
  const auto *C = Result.Nodes.getNodeAs<CallExpr>("call");
  for (size_t i = 1, ArgCount = C->getNumArgs(); i < ArgCount; ++i) {
    if ( i!=1 ) Stream << ", ";
    auto ArgExpr = C->getArg(i);
    auto ArgRange = SourceRange(ArgExpr->getLocStart(), ArgExpr->getLocEnd());
    auto ArgText = Lexer::getSourceText(
        CharSourceRange::getTokenRange(ArgRange), *Result.SourceManager,
        Result.Context->getLangOpts());
    Stream << ArgText;
  }
  Stream << "); };";



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
