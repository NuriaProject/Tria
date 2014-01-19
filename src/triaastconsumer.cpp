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

#include "triaastconsumer.hpp"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Attr.h>

#include <QString>
#include <QDebug>

#include "definitions.hpp"
#include "generator.hpp"

static const QString introspectAnnotation = QStringLiteral ("nuria_introspect");
static const QString skipAnnotation = QStringLiteral ("nuria_skip");

TriaASTConsumer::TriaASTConsumer (clang::CompilerInstance &compiler, const llvm::StringRef &fileName,
				  Generator *generator)
	: m_generator (generator) , m_compiler (compiler)
{
	Q_UNUSED(fileName)
	
}

void TriaASTConsumer::Initialize (clang::ASTContext &ctx) {
	this->m_context = &ctx;
}

static bool containsAnnotation (const Annotations &list, const QString &name) {
	for (const AnnotationDef &cur : list) {
		if (cur.name == name) {
			return true;
		}
		
	}
	
	return false;
}

static Annotations annotationsFromDecl (clang::Decl *decl) {
	static const QString specialPrefix = QStringLiteral ("nuria_annotate:");
	Annotations list;
	
	AnnotationDef current;
	bool nextIsValue = false;
	
	const clang::AttrVec &attributes = decl->getAttrs ();
	for (const clang::Attr *attr : attributes) {
		const clang::AnnotateAttr *cur = llvm::dyn_cast< clang::AnnotateAttr > (attr);
		if (!cur) {
			continue;
		}
		
		// 
		llvm::StringRef annotation = cur->getAnnotation ();
		QString data = QString::fromUtf8 (annotation.data (), annotation.size ());
		
		if (nextIsValue) {
			if (data.startsWith (QLatin1Char ('"'))) {
				data = data.mid (1, data.length () - 2);
				current.valueIsString = true;
			}
			
			current.value = data;
			list.append (current);
			
			current = AnnotationDef ();
			nextIsValue = false;
			
		} else if (data.startsWith (specialPrefix)) {
			// Next will be the value
			current.name = data.mid (specialPrefix.length ());
			nextIsValue = true;
			
		} else {
			// Annotation without data
			AnnotationDef def;
			def.name = data;
			
			list.append (def);
		}
		
	}
	
	// 
	std::sort (list.begin (), list.end (), [](const AnnotationDef &l, const AnnotationDef &r) {
		return l.name < r.name;
	});
	
	return list;
}

static inline QString llvmToString (const llvm::StringRef &str) {
	return QString::fromLatin1 (str.data (), str.size ());
}

static BaseDef processBase (clang::CXXBaseSpecifier *specifier) {
	BaseDef base;
	
	base.name = QString::fromStdString (specifier->getType ().getAsString ());
	base.access = specifier->getAccessSpecifier ();
	base.isVirtual = specifier->isVirtual ();
	
	// Cut off leading "struct/class "
	base.name = base.name.mid (base.name.indexOf (QLatin1Char (' ')) + 1);
	
	return base;
}

static MethodDef processMethod (ClassDef &classDef, clang::CXXMethodDecl *decl) {
	static const QString fromMethod = QStringLiteral ("from");
	static const QString toMethod = QStringLiteral ("to");
	
	clang::CXXConstructorDecl *ctor = llvm::dyn_cast< clang::CXXConstructorDecl > (decl);
	clang::CXXDestructorDecl *dtor = llvm::dyn_cast< clang::CXXDestructorDecl > (decl);
	
	MethodDef def;
	
	// Base information
	def.access = (decl->getAccess () == clang::AS_none) ? clang::AS_public : decl->getAccess ();
	def.isVirtual = decl->isVirtual ();
	def.annotations = annotationsFromDecl (decl);
	
	// Skip non-public methods
	if (def.access != clang::AS_public) {
		return def;
	}
	
	// C'tors and D'tors don't have names nor do they really return anything
	if (ctor) {
		def.type = ConstructorMethod;
	} else if (dtor) {
		def.type = DestructorMethod;
	} else {
		def.name = llvmToString (decl->getName ());
		def.returnType = QString::fromStdString (decl->getResultType ().getAsString ());
		def.type = decl->isStatic () ? StaticMethod : MemberMethod;
	}
	
	// Arguments
	bool canConvert = (def.type == MemberMethod);
	for (int i = 0; i < int (decl->getNumParams ()); i++) {
		const clang::ParmVarDecl *param = decl->getParamDecl (i);
		VariableDef var;
		
		var.access = clang::AS_public;
		var.name = llvmToString (param->getName ());
		var.type = QString::fromStdString (param->getType ().getAsString ());
		var.isOptional = param->hasDefaultArg ();
		
		// This may be a viable conversion method if it only expects a
		// single argument (Plus optional ones)
		canConvert = (var.isOptional || i < 1);
		
		def.arguments.append (var);
	}
	
	// Final check if this is a conversion method
	if (canConvert && !containsAnnotation (def.annotations, skipAnnotation) &&
	    (def.type == ConstructorMethod ||
	     (def.type == StaticMethod && def.name.startsWith (fromMethod)) ||
	     (def.type == MemberMethod && def.name.startsWith (toMethod)))) {
		ConversionDef conv;
		conv.methodName = def.name;
		conv.type = def.type;
		conv.fromType = (def.type == MemberMethod)
				? classDef.name : def.arguments.first ().type;
		conv.toType = (def.type == ConstructorMethod)
			      ? classDef.name : def.returnType;
		
		// 
		classDef.conversions.append (conv);
		
	}
	
	return def;
}

static VariableDef processVariable (clang::FieldDecl *decl) {
	VariableDef def;
	
	def.access = (decl->getAccess () == clang::AS_none) ? clang::AS_public : decl->getAccess ();
	def.type = QString::fromStdString (decl->getType ().getAsString ());
	def.name = llvmToString (decl->getName ());
	def.annotations = annotationsFromDecl (decl);
	
	return def;
}

static EnumDef processEnum (clang::EnumDecl *decl) {
	EnumDef def;
	
	def.name = llvmToString (decl->getName ());
	def.annotations = annotationsFromDecl (decl);
	
	for (auto it = decl->enumerator_begin (); it != decl->enumerator_end (); ++it) {
		const clang::EnumConstantDecl *cur = *it;
		def.values.append (llvmToString (cur->getName ()));
	}
	
	return def;
}

void TriaASTConsumer::HandleTagDeclDefinition (clang::TagDecl *decl) {
	/*
	clang::EnumDecl *enumDecl = llvm::dyn_cast< clang::EnumDecl >(decl);
	if (enumDecl) {
		
		qDebug() << "enum" << QString::fromStdString (enumDecl->getNameAsString ());
	}
	*/
	
	clang::CXXRecordDecl *record = llvm::dyn_cast< clang::CXXRecordDecl >(decl);
	if (!record) {
		return;
	}
	
	// 
	ClassDef classDef;
	classDef.access = decl->getAccess ();
	classDef.name = QString::fromStdString (decl->getQualifiedNameAsString ());
	classDef.annotations = annotationsFromDecl (record);
	
	// Skip if not to be 'introspected'
	if (!containsAnnotation (classDef.annotations, introspectAnnotation)) {
		return;
	}
	
	// Remove NURIA_INTROSPECT annotation from the list
	for (auto it = classDef.annotations.begin (); it != classDef.annotations.end ();) {
		if (it->name == introspectAnnotation) {
			it = classDef.annotations.erase (it);
		} else {
			++it;
		}
		
	}
	
	// Parent classes
	for (auto it = record->bases_begin (); it != record->bases_end (); ++it) {
		classDef.bases.append (processBase (it));
	}
	
	// Methods
	for (auto it = record->method_begin (); it != record->method_end (); ++it) {
		MethodDef method = processMethod (classDef, *it);
		
		if (method.access == clang::AS_public && method.type != DestructorMethod &&
		    !containsAnnotation (method.annotations, skipAnnotation)) {
			classDef.methods.append (method);
		}
		
	}
	
	// Variables
	for (auto it = record->field_begin (); it != record->field_end (); ++it) {
		VariableDef field = processVariable (*it);
		
		if (field.access == clang::AS_public &&
		    !containsAnnotation (field.annotations, skipAnnotation)) {
			classDef.variables.append (field);
		}
		
	}
	
	// Find enums
	for (auto it = record->decls_begin (); it != record->decls_end (); ++it) {
		clang::EnumDecl *enumDecl = llvm::dyn_cast< clang::EnumDecl >(decl);
		if (!enumDecl) {
			continue;
		}
		
		// 
		clang::AccessSpecifier access = enumDecl->getAccess ();
		EnumDef enumDef = processEnum (enumDecl);
		
		if ((access != clang::AS_public && access != clang::AS_none) ||
		    containsAnnotation (enumDef.annotations, skipAnnotation)) {
			continue;
		}
		
		// 
		classDef.enums.append (enumDef);
		
	}
	
	// Does this type have a public c'tor which takes no arguments,
	// a public copy-ctor and assignment operator?
	classDef.hasDefaultCtor = record->hasDefaultConstructor () ||
				  record->hasUserProvidedDefaultConstructor ();
	classDef.hasCopyCtor = record->hasCopyConstructorWithConstParam () ||
			       record->hasUserDeclaredCopyConstructor ();
	classDef.hasAssignmentOperator = record->hasCopyAssignmentWithConstParam () ||
					 record->hasUserDeclaredCopyAssignment ();
	
	classDef.hasValueSemantics = (classDef.hasDefaultCtor &&
				      classDef.hasCopyCtor &&
				      classDef.hasAssignmentOperator);
	
	// Done.
	this->m_generator->addClassDefinition (classDef);
	
}
