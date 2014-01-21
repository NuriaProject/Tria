/* Copyright (c) 2014, The Nuria Project
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *    1. The origin of this software must not be misrepresented; you must not
 *       claim that you wrote the original software. If you use this software
 *       in a product, an acknowledgment in the product documentation would be
 *       appreciated but is not required.
 *    2. Altered source versions must be plainly marked as such, and must not be
 *       misrepresented as being the original software.
 *    3. This notice may not be removed or altered from any source
 *       distribution.
 */

#ifndef TRIAASTCONSUMER_HPP
#define TRIAASTCONSUMER_HPP

#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Decl.h>
#include "definitions.hpp"

namespace clang {
class StaticAssertDecl;
class CXXMethodDecl;
}

class Generator;
class TriaASTConsumer : public clang::ASTConsumer {
public:
	TriaASTConsumer (clang::CompilerInstance &compiler, const llvm::StringRef &fileName, Generator *generator);
	
	void Initialize (clang::ASTContext &ctx) override;
	void HandleTagDeclDefinition (clang::TagDecl *decl) override;
	
private:
	void declareType (const clang::QualType &type);
	
	BaseDef processBase (clang::CXXBaseSpecifier *specifier);
	MethodDef processMethod (ClassDef &classDef, clang::CXXMethodDecl *decl);
	VariableDef processVariable (clang::FieldDecl *decl);
	EnumDef processEnum (ClassDef &parent, clang::EnumDecl *decl);
	
	Generator *m_generator;
	clang::CompilerInstance &m_compiler;
	clang::ASTContext *m_context = nullptr;
	
};

#endif // TRIAASTCONSUMER_HPP
