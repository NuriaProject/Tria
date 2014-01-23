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

#include <clang/AST/DeclTemplate.h>
#include <clang/AST/ASTContext.h>
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

QString TriaASTConsumer::typeName (const clang::Type *type) {
	const clang::CXXRecordDecl *decl = type->getAsCXXRecordDecl ();
	if (decl) {
		return QString::fromStdString (decl->getQualifiedNameAsString ());
	}
	
	if (type->isPointerType ()) {
		const clang::PointerType *ptr = type->getAs< clang::PointerType > ();
		return typeName (ptr->getPointeeType ()) + QStringLiteral("*");
	}
	
	return QString::fromStdString (clang::QualType (type, 0).getAsString ());
}

QString TriaASTConsumer::typeName (const clang::QualType &type) {
	return typeName (type.getTypePtr ());
}

static bool hasTypeValueSemantics (const clang::Type *type) {
	if (type->isPointerType ()) {
		return true;
	}
	
	const clang::CXXRecordDecl *record = type->getAsCXXRecordDecl ();
	if (!record) {
		// FIXME: This can break for typedef's
		return true;
	}
	
	// Search for the default- and copy-constructor, make sure they're not deleted
	bool seenDefaultCtor = false;
	bool seenCopyCtor = false;
	for (auto it = record->ctor_begin (); !seenCopyCtor && !seenCopyCtor && it != record->ctor_end (); ++it) {
		const clang::CXXConstructorDecl *cur = *it;
		if (!cur->isDefaultConstructor () && !cur->isCopyConstructor ()) {
			continue;
		}
		
		// Make sure the ctor or copy-ctor is publicly visible and not deleted.
		(cur->isDefaultConstructor () ? seenDefaultCtor : seenCopyCtor) = true;
		clang::AccessSpecifier access = cur->getAccess ();
		
		if (cur->isDeleted () || access == clang::AS_private || access == clang::AS_protected) {
			return false;
		}
		
	}
	
	// Probably has value-semantics
	// FIXME: Also check for a assignment operator.
	return ((seenDefaultCtor || record->needsImplicitDefaultConstructor ()) &&
		(seenCopyCtor || record->needsImplicitCopyConstructor ()));
	
}

static bool hasTypeValueSemantics (const clang::QualType &type) {
	return hasTypeValueSemantics (type.getTypePtr ());
}

BaseDef TriaASTConsumer::processBase (clang::CXXBaseSpecifier *specifier) {
	BaseDef base;
	
	base.name = typeName (specifier->getType ());
	base.access = specifier->getAccessSpecifier ();
	base.isVirtual = specifier->isVirtual ();
	
	return base;
}

MethodDef TriaASTConsumer::processMethod (ClassDef &classDef, clang::CXXMethodDecl *decl) {
	static const QString fromMethod = QStringLiteral ("from");
	static const QString toMethod = QStringLiteral ("to");
	
	clang::CXXConstructorDecl *ctor = llvm::dyn_cast< clang::CXXConstructorDecl > (decl);
	clang::CXXDestructorDecl *dtor = llvm::dyn_cast< clang::CXXDestructorDecl > (decl);
	clang::CXXConversionDecl *convDecl = llvm::dyn_cast< clang::CXXConversionDecl > (decl);
	
	MethodDef def;
	
	// Base information
	def.access = (decl->getAccess () == clang::AS_none) ? clang::AS_public : decl->getAccess ();
	def.isVirtual = decl->isVirtual ();
	def.isConst = clang::Qualifiers::fromCVRMask (decl->getTypeQualifiers ()).hasConst ();
	def.annotations = annotationsFromDecl (decl);
	
	// Skip non-public methods and methods which are default-implemented
	if (def.access != clang::AS_public || decl->isDefaulted () ||
	    !hasTypeValueSemantics (decl->getResultType ())) {
		this->m_generator->avoidType (def.returnType);
		def.access = clang::AS_private;
		return def;
	}
	
	// C'tors and D'tors don't have names nor do they really return anything
	if (ctor) {
		def.type = ConstructorMethod;
		def.returnType = classDef.name;
	} else if (dtor) {
		def.type = DestructorMethod;
		def.returnType = QStringLiteral ("void");
	} else if (convDecl) {
		def.access = clang::AS_private;
		return def;
	} else {
		def.name = llvmToString (decl->getName ());
		def.returnType = typeName (decl->getResultType ());
		def.type = decl->isStatic () ? StaticMethod : MemberMethod;
		
		// Also register result-type in the meta-system later on!
		declareType (decl->getResultType ());
		
	}
	
	// Arguments
	bool canConvert = (def.type == MemberMethod);
	for (int i = 0; i < int (decl->getNumParams ()); i++) {
		const clang::ParmVarDecl *param = decl->getParamDecl (i);
		VariableDef var;
		
		var.access = clang::AS_public;
		var.name = llvmToString (param->getName ());
		var.type = typeName (param->getType ());
		var.isOptional = param->hasDefaultArg ();
		var.isConst = param->getType ().getQualifiers ().hasConst ();
		def.hasOptionalArguments = var.isOptional;
		
		// Ignore methods with arguments without value-semantics
		if (!hasTypeValueSemantics (param->getType ())) {
			this->m_generator->avoidType (var.type);
			def.access = clang::AS_private;
			return def;
		}
		
		// Register type
		declareType (param->getType ());
		
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
		conv.isConst = def.isConst && (def.type == MemberMethod || def.arguments.first ().isConst);
		
		// 
		classDef.conversions.append (conv);
		
	}
	
	// 
	return def;
}

VariableDef TriaASTConsumer::processVariable (clang::FieldDecl *decl) {
	VariableDef def;
	
	def.access = (decl->getAccess () == clang::AS_none) ? clang::AS_public : decl->getAccess ();
	def.type = typeName (decl->getType ());
	def.name = llvmToString (decl->getName ());
	def.annotations = annotationsFromDecl (decl);
	
	if (hasTypeValueSemantics (decl->getType ())) {
		declareType (decl->getType ());
	} else {
		// TODO: Warn user
		this->m_generator->avoidType (def.type);
	}
	
	return def;
}

EnumDef TriaASTConsumer::processEnum (clang::EnumDecl *decl) {
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
	static const llvm::StringRef qMetaTypeIdName ("QMetaTypeId");
	
	clang::CXXRecordDecl *record = llvm::dyn_cast< clang::CXXRecordDecl >(decl);
	if (!record) {
		return;
	}
	
	// 
	if (!hasTypeValueSemantics (record->getTypeForDecl ())) {
		this->m_generator->avoidType (typeName (record->getTypeForDecl ()));
	}
	
	// Check if this is a QMetaTypeId specialization (Result of a Q_DECLARE_METATYPE)
	clang::ClassTemplateSpecializationDecl *templ = llvm::dyn_cast< clang::ClassTemplateSpecializationDecl > (decl);
	
	if (templ && templ->getName () == qMetaTypeIdName) {
		const clang::TemplateArgumentList &list = templ->getTemplateArgs ();
		if (list.size () == 1) {
			QString name = typeName (list.get (0).getAsType ());
			this->m_generator->addDeclaredType (name);
		}
		
		return;
	}
	
	// Base data
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
	
	// Find conversion operators
	for (auto it = record->conversion_begin (); it != record->conversion_end (); ++it) {
		clang::CXXConversionDecl *convDecl = llvm::dyn_cast< clang::CXXConversionDecl > (*it);
		if (!convDecl || convDecl->getAccess () == clang::AS_private ||
		    convDecl->getAccess () == clang::AS_protected) {
			continue;
		}
		
		// 
		ConversionDef conv;
		conv.type = MemberMethod;
		conv.isConst = convDecl->isConst ();
		conv.fromType = classDef.name;
		conv.toType = typeName (convDecl->getConversionType ());
		conv.methodName = QStringLiteral ("operator ") + conv.toType;
		
		declareType (convDecl->getConversionType ());
		classDef.conversions.append (conv);
		
	}
	
	// Find enums
	for (auto it = record->decls_begin (); it != record->decls_end (); ++it) {
		clang::EnumDecl *enumDecl = llvm::dyn_cast< clang::EnumDecl > (*it);
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
		
		// Store and declare
		classDef.enums.append (enumDef);
		this->m_generator->declareType (classDef.name + QStringLiteral ("::") + enumDef.name);
		
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
	
	if (!classDef.hasValueSemantics) {
		this->m_generator->avoidType (classDef.name);
	}
	
	// Done.
	this->m_generator->addClassDefinition (classDef);
	
}

void TriaASTConsumer::declareType (const clang::QualType &type) {
	if (type.getTypePtr ()->isVoidType ()) {
		return;
	}
	
	// 
	if (!hasTypeValueSemantics (type)) {
		return;
	}
	
	// Ignore if it's either a POD-type (Which is already registered) or if
	// it begins with "Q" (indicating it's a Qt type) and is known to the
	// meta-system of this running tria instance.
	QString name = typeName (type);
	
	if ((type.isPODType (*this->m_context) || name.startsWith (QLatin1Char ('Q')))
	    && QMetaType::type (qPrintable(name)) != 0) {
		return;
	}
	
	// FIXME: Check if we actually can declare it
	this->m_generator->declareType (name);
	
}
