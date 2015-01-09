/* Copyright (c) 2014-2015, The Nuria Project
 * The NuriaProject Framework is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 * 
 * The NuriaProject Framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with The NuriaProject Framework.
 * If not, see <http://www.gnu.org/licenses/>.
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
	
	bool isDeclATemplate (const clang::DeclContext *decl);
	bool isDeclUnnamed (const clang::DeclContext *decl);
	void declareType (const clang::QualType &type);
	
	QString typeDeclName (const clang::NamedDecl *decl);
	QString typeName (const clang::Type *type);
	QString typeName (const clang::QualType &type);
	QString fileOfDecl (clang::Decl *decl);
	
	bool hasRecordValueSemantics (const clang::CXXRecordDecl *record, bool abstractTest = true);
	bool hasTypeValueSemantics (const clang::QualType &type);
	bool hasTypeValueSemantics (const clang::Type *type);
	
	bool shouldIntrospect (const Annotations &annotations, bool isGlobal, clang::CXXRecordDecl *record = nullptr);
	void processClass (clang::CXXRecordDecl *record);
	BaseDef processBase (clang::CXXBaseSpecifier *specifier);
	bool registerReadWriteMethod (ClassDef &classDef, MethodDef &def, clang::CXXMethodDecl *decl);
	void fillVariableDef (VariableDef &def, const clang::QualType &type);
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
