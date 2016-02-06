//===--- AvoidStdBindCheck.cpp - clang-tidy--------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AvoidStdBindCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

void AvoidStdBindCheck::registerMatchers(MatchFinder *Finder) {
  // FIXME: Add matchers.
  Finder->addMatcher(
      cxxConstructExpr(
          hasType(qualType(hasDeclaration(
              classTemplateSpecializationDecl(matchesName("::std::bind"))))))
          .bind("bind"),
      this);
}

void AvoidStdBindCheck::check(const MatchFinder::MatchResult &Result) {
  // FIXME: Add callback implementation.
  const auto *MatchedDecl = Result.Nodes.getNodeAs<CXXConstructExpr>("bind");
  
  diag(MatchedDecl->getLocation(), "use of std::bind is deprecated");
  /*
  if (MatchedDecl->getName().startswith("avoid-std-bind_"))
    return;
  diag(MatchedDecl->getLocation(), "function '%0' is insufficiently avoid-std-bind")
      << MatchedDecl->getName()
      << FixItHint::CreateInsertion(MatchedDecl->getLocation(), "avoid-std-bind_");
      */
}

} // namespace misc
} // namespace tidy
} // namespace clang
