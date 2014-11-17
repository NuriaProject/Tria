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
#include <QStringList>

#include "definitions.hpp"
#undef bool

namespace clang {
class CXXConversionDecl;
class StaticAssertDecl;
class CXXMethodDecl;
}

class Definitions;
class TriaASTConsumer : public clang::ASTConsumer {
public:
	TriaASTConsumer (clang::CompilerInstance &compiler, const llvm::StringRef &fileName,
			 const QStringList &introspectBases, bool introspectAll,
			 const std::string &globalClass, Definitions *definitions);
	
	void Initialize (clang::ASTContext &ctx) override;
	void HandleTagDeclDefinition (clang::TagDecl *decl) override;
	bool HandleTopLevelDecl (clang::DeclGroupRef groupRef) override;
	void HandleTranslationUnit (clang::ASTContext &) override;
	
private:
	void addDefaultConstructors (clang::CXXRecordDecl *record, ClassDef &classDef);
	void addDefaultConstructor (ClassDef &classDef);
	void addDefaultCopyConstructor (ClassDef &classDef);
	void addConstructor (ClassDef &classDef, const Variables &arguments);
	
	Annotations annotationsFromDecl (clang::Decl *decl);
	QMetaType::Type typeOfAnnotationValue (const QString &valueData);
	AnnotationDef parseNuriaAnnotate (const QString &data);
	
	bool derivesFromIntrospectClass (const clang::CXXRecordDecl *record);
	
	void reportError (clang::SourceLocation loc, const QByteArray &info);
	void reportWarning (clang::SourceLocation loc, const QByteArray &info);
	void reportMessage (clang::DiagnosticsEngine::Level level, clang::SourceLocation loc,
			    const QByteArray &info);
	
	bool isRecordATemplate(const clang::CXXRecordDecl *record);
	void declareType (const clang::QualType &type);
	
	QString typeDeclName (const clang::NamedDecl *decl, const clang::Type *type);
	QString typeName (const clang::Type *type);
	QString typeName (const clang::QualType &type);
	QString fileOfDecl (clang::Decl *decl);
	
	bool hasRecordPureVirtuals (const clang::CXXRecordDecl *record);
	bool hasRecordValueSemantics (const clang::CXXRecordDecl *record, bool abstractTest = true);
	bool hasTypeValueSemantics (const clang::QualType &type);
	bool hasTypeValueSemantics (const clang::Type *type);
	
	bool shouldIntrospect (const Annotations &annotations, bool isGlobal, clang::CXXRecordDecl *record = nullptr);
	void processClass (clang::CXXRecordDecl *record);
	BaseDef processBase (clang::CXXBaseSpecifier *specifier);
	bool registerReadWriteMethod (ClassDef &classDef, MethodDef &def, clang::CXXMethodDecl *decl);
	void processMethod (ClassDef &classDef, clang::FunctionDecl *decl, bool isGlobal = false);
	VariableDef processVariable (clang::FieldDecl *decl);
	void processEnum (ClassDef &classDef, clang::EnumDecl *decl, bool isGlobal = false);
	void processConversion (ClassDef &classDef, clang::CXXConversionDecl *convDecl);
	
	// 
	QMap< clang::FileID, QString > m_pathCache;
	Definitions *m_definitions;
	QStringList m_introspectedBases;
	bool m_introspectAll;
	ClassDef m_globals;
	
	clang::CompilerInstance &m_compiler;
	clang::ASTContext *m_context = nullptr;
	
};

#endif // TRIAASTCONSUMER_HPP
