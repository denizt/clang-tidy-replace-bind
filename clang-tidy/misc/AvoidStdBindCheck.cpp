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

namespace {
struct BindArgument {
  StringRef Tokens;
  Optional<StringRef> PlaceHolder;
  bool IsTemporaryExpr = false;
};

static llvm::Regex MatchPlaceholder("^_([0-9]+)$");
}

void AvoidStdBindCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      cxxConstructExpr(
          hasType(qualType(hasDeclaration(
              classTemplateSpecializationDecl(matchesName("std::bind"))))),
          hasArgument(0, callExpr(hasArgument(0, declRefExpr().bind("f")))
                             .bind("call")))
          .bind("bind"),
      this);
}

void AvoidStdBindCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<CXXConstructExpr>("bind");

  auto DiagnosticBuilder =
      diag(MatchedDecl->getLocation(), "use of std::bind is deprecated");

  if (!getLangOpts().CPlusPlus14)
    return;

  std::string Buffer;
  llvm::raw_string_ostream Stream(Buffer);

  const auto *F = Result.Nodes.getNodeAs<DeclRefExpr>("f");
  const auto *C = Result.Nodes.getNodeAs<CallExpr>("call");

  std::vector<BindArgument> BindArguments;
  for (size_t i = 1, ArgCount = C->getNumArgs(); i < ArgCount; ++i) {
    const Expr* E = C->getArg(i);
    BindArgument B;
    B.IsTemporaryExpr = dyn_cast<MaterializeTemporaryExpr>(E);
    B.Tokens = Lexer::getSourceText(
        CharSourceRange::getTokenRange(E->getLocStart(), E->getLocEnd()),
        *Result.SourceManager, Result.Context->getLangOpts());

    SmallVector<StringRef, 2> Matches;
    if(MatchPlaceholder.match(B.Tokens, &Matches))
    {
      B.PlaceHolder = (llvm::Twine("arg") + Matches[1]).str();
    }
    BindArguments.push_back(B);
  }

  bool HasCapturedArgument =
      std::find_if(BindArguments.begin(), BindArguments.end(),
                   [](const BindArgument &B) { return !B.IsTemporaryExpr; }) !=
      BindArguments.end();

  size_t PlaceholderCount =
      std::count_if(BindArguments.begin(), BindArguments.end(),
                   [](const BindArgument &B) { return B.PlaceHolder; });
  
  StringRef LambdaCap = HasCapturedArgument ? "=" : "";

  Stream << "[" << LambdaCap << "]";

  // Add lambda args for placeholders.
  {
    if (PlaceholderCount) Stream << "(";
    
    StringRef Delimiter = "";
    for (size_t I=0; I<PlaceholderCount; ++I) {
      Stream << "auto arg" << I+1;
      Delimiter = ", ";
    }
    
    if (PlaceholderCount) Stream << ")";
  }
  
  Stream << " { return " << F->getNameInfo().getName() << "(";

  // Add function args for bind.
  {
    StringRef Delimiter = "";
    for (const auto &B : BindArguments) {
      if (B.PlaceHolder)
        Stream << Delimiter << *B.PlaceHolder;
      else
        Stream << Delimiter << B.Tokens;
      Delimiter = ", ";
    }
    Stream << "); };";
  }

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
