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
#include <clang/Basic/Version.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Attr.h>

#include <QString>
#include <QDebug>
#include <QDir>

#include "definitions.hpp"
#include "defs.hpp"
#undef bool

static const QString introspectAnnotation = QStringLiteral ("nuria_introspect");
static const QString customAnnotation = QStringLiteral ("nuria_annotate:");
static const QString skipAnnotation = QStringLiteral ("nuria_skip");
static const QString readAnnotation = QStringLiteral ("nuria_read:");
static const QString writeAnnotation = QStringLiteral ("nuria_write:");
static const QString requireAnnotation = QStringLiteral ("nuria_require:");

TriaASTConsumer::TriaASTConsumer (clang::CompilerInstance &compiler, const llvm::StringRef &fileName,
				  const QStringList &introspectBases, bool introspectAll,
                                  const std::string &globalClass, Definitions *definitions)
	: m_definitions (definitions), m_introspectedBases (introspectBases),
	  m_introspectAll (introspectAll), m_compiler (compiler)
{
	Q_UNUSED(fileName)
	
	this->m_globals.isFakeClass = true;
	this->m_globals.access = clang::AS_public;
	this->m_globals.name = QString::fromStdString (globalClass);
	
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

static QString annotationValue (const QString &name, AnnotationType &type) {
	if (name == introspectAnnotation) {
		type = IntrospectAnnotation;
		
	} else if (name == skipAnnotation) {
		type = IntrospectAnnotation;
		
	} else if (name.startsWith (readAnnotation)) {
		type = ReadAnnotation;
		return name.mid (readAnnotation.length ());
		
	} else if (name.startsWith (writeAnnotation)) {
		type = WriteAnnotation;
		return name.mid (writeAnnotation.length ());
		
	} else if (name.startsWith (requireAnnotation)) {
		type = RequireAnnotation;
		return name.mid (requireAnnotation.length ());
		
	} else {
		type = CustomAnnotation;
	}
	
	return QString ();
}

QMetaType::Type TriaASTConsumer::typeOfAnnotationValue (const QString &valueData) {
	bool isInt = false;
	valueData.toInt (&isInt);
	
	if (isInt) {
		return QMetaType::Int;
	}
	
	// 
	bool isFloat = false;
	valueData.toFloat (&isFloat);
	
	if (isFloat) {
		return QMetaType::Float;
	}
	
	// Bool?
	if (valueData == "true" || valueData == "false") {
		return QMetaType::Bool;
	}
	
	// Unknown
	return QMetaType::QVariant;
}

static int findEndOfName (const QString &data, int from) {
	for (int i = from + 1; i < data.length (); i++) {
		if (data.at (i) == QLatin1Char ('\\') && data.at (i) == QLatin1Char ('"')) {
			i++;
		} else if (data.at (i) == QLatin1Char ('"')) {
			return i;
		}
		
	}
	
	return -1;
}

AnnotationDef TriaASTConsumer::parseNuriaAnnotate (const QString &data) {
	AnnotationDef def;
	
	int namePos = customAnnotation.length ();
	int nameLen = findEndOfName (data, namePos + 1) - namePos;
	int valuePos = data.indexOf (QLatin1Char ('='), namePos + nameLen) + 1;
	
	// 
	QString valueData = data.mid (valuePos);
	if (valueData.startsWith (QLatin1Char ('"'))) {
		valueData = valueData.mid (1, valueData.length () - 2);
		def.valueType = QMetaType::QString;
	} else {
		def.valueType = typeOfAnnotationValue (valueData);
	}
	
	// 
	def.name = data.mid (namePos + 1, nameLen - 1);
	def.type = CustomAnnotation;
	def.value = valueData;
	
	return def;
}

bool TriaASTConsumer::derivesFromIntrospectClass (const clang::CXXRecordDecl *record) {
	if (!record || this->m_introspectedBases.isEmpty ()) {
		return false;
	}
	
	for (auto it = record->bases_begin (); it != record->bases_end (); ++it) {
		const clang::CXXBaseSpecifier &specifier = *it;
		const clang::Type *type = specifier.getType ().getTypePtr ();
		
		if (this->m_introspectedBases.contains (typeName (type))) {
			return true;
		}
		
		// 
		const clang::CXXRecordDecl *decl = type->getAsCXXRecordDecl ();
		if (decl && derivesFromIntrospectClass (decl)) {
			return true;
		}
		
	}
	
	// 
	return false;
	
}

Annotations TriaASTConsumer::annotationsFromDecl (clang::Decl *decl) {
	Annotations list;
	
	const clang::AttrVec &attributes = decl->getAttrs ();
	for (const clang::Attr *attr : attributes) {
		const clang::AnnotateAttr *cur = llvm::dyn_cast< clang::AnnotateAttr > (attr);
		if (!cur) {
			continue;
		}
		
		// 
		llvm::StringRef annotation = cur->getAnnotation ();
		QString data = QString::fromUtf8 (annotation.data (), annotation.size ());
		
		// Read
		AnnotationDef def;
		if (data.startsWith (customAnnotation)) {
			def = parseNuriaAnnotate (data);
		} else { // Annotation without an additional value
			def.name = data;
			def.value = annotationValue (data, def.type);
		}
		
		// Store
		def.loc = attr->getRange ();
		list.prepend (def);
		
	}
	
	// 
	std::stable_sort (list.begin (), list.end (), [](const AnnotationDef &l, const AnnotationDef &r) {
		return l.name < r.name;
	});
	
	return list;
}

static inline QString llvmToString (const llvm::StringRef &str) {
	return QString::fromLatin1 (str.data (), str.size ());
}

QString TriaASTConsumer::typeDeclName (const clang::NamedDecl *decl) {
	const clang::ClassTemplateSpecializationDecl *templ =
			llvm::dyn_cast< clang::ClassTemplateSpecializationDecl > (decl);
	QString name = QString::fromStdString (decl->getQualifiedNameAsString ());
	
	if (!templ) {
		return name;
	}
	
	// Declarations of templates
	const clang::TemplateArgumentList &args = templ->getTemplateArgs ();
	
	if (args.size () < 1) {
		return name;
	}
	
	// 
	QStringList typeNames;
	for (uint32_t i = 0; i < args.size (); i++) {
		const clang::TemplateArgument &arg = args.get (i);
		
		if (arg.getKind () != clang::TemplateArgument::Type) {
			return name;
		}
		
		typeNames.append (typeName (arg.getAsType ()));
	}
	
	//
	name += QStringLiteral ("<") + typeNames.join (QStringLiteral (", ")) + QStringLiteral (">");
	name.replace (QStringLiteral(">>"), QStringLiteral("> >"));
	
	return name;
}

QString TriaASTConsumer::typeName (const clang::Type *type) {
	const clang::CXXRecordDecl *decl = type->getAsCXXRecordDecl ();
	const clang::TypedefType *typeDef = type->getAs< clang::TypedefType > ();
	
	if (typeDef) {
		return typeDeclName (typeDef->getDecl ()->getCanonicalDecl ());
	}
	
	if (decl) {
		return typeDeclName (decl);
	}
	
	if (type->isReferenceType ()) {
		const clang::ReferenceType *ptr = type->getAs< clang::ReferenceType > ();
		return typeName (ptr->getPointeeType ());
	}
	
	if (type->isEnumeralType ()) {
		return QString::fromStdString (clang::QualType (type, 0).getCanonicalType ().getAsString ())
				.remove (QStringLiteral ("enum "));
	}
	
	if (type->isPointerType ()) {
		const clang::PointerType *ptr = type->getAs< clang::PointerType > ();
		return typeName (ptr->getPointeeType ()) + QStringLiteral("*");
	}
	
	if (type->isBooleanType ()) {
		return QStringLiteral("bool");
	}
	
	return QString::fromStdString (clang::QualType (type, 0).getAsString ());
}

QString TriaASTConsumer::typeName (const clang::QualType &type) {
	return typeName (type.getTypePtr ());
}

static QString filePathOfFileId (clang::SourceManager &mgr, clang::FileID fileId) {
	QString file = QString::fromUtf8 (mgr.getFileEntryForID (fileId)->getName ());
	if (QFileInfo (file).isAbsolute ()) {
		return file;
	}
	
	// Clean up relative path
	return QDir::current ().relativeFilePath (file);
}

QString TriaASTConsumer::fileOfDecl (clang::Decl *decl) {
	clang::SourceManager &mgr = this->m_compiler.getSourceManager ();
	clang::SourceLocation location = decl->getSourceRange ().getBegin ();
	clang::SourceLocation fileLocation = mgr.getExpansionLoc (location);
	clang::FileID fileId = mgr.getFileID (fileLocation);
	
	// 
	if (!this->m_pathCache.contains (fileId)) {
		QString path = filePathOfFileId (mgr, fileId);
		this->m_pathCache.insert (fileId, path);
		return path;
	}
	
	return this->m_pathCache.value (fileId);
}

bool TriaASTConsumer::hasRecordValueSemantics (const clang::CXXRecordDecl *record, bool abstractTest) {
	record = (!record || record->isThisDeclarationADefinition ()) ? record : record->getDefinition ();
	
	if (!record) {
		// FIXME: This can break for typedef's
		return true;
	}
	
	// 
	if ((abstractTest && record->isAbstract ()) || isDeclATemplate (record)) {
		return false;
	}
	
	// Search for the default- and copy-constructor, make sure they're not deleted
	bool seenDefaultCtor = false;
	bool seenCopyCtor = false;
	for (auto it = record->ctor_begin (); !seenCopyCtor && !seenCopyCtor && it != record->ctor_end (); ++it) {
		const clang::CXXConstructorDecl *cur = *it;
		if (!cur->isDefaultConstructor () && !cur->isCopyConstructor ()) {
			continue;
		}
		
		// 
		(cur->isDefaultConstructor () ? seenDefaultCtor : seenCopyCtor) = true;
		clang::AccessSpecifier access = cur->getAccess ();
		
		// Make sure the (copy-)ctor or assignment operator is publicly visible and not deleted.
		if (cur->isDeleted () || access == clang::AS_private || access == clang::AS_protected) {
			return false;
		}
		
	}
	
	// FIXME: Also check for a assignment operator.
	if (!(seenDefaultCtor || record->needsImplicitDefaultConstructor ()) &&
	    !(seenCopyCtor || record->needsImplicitCopyConstructor ())) {
		return false;
	}
	
	// Check that base-classes offer value-semantics
	for (auto it = record->bases_begin (); it != record->bases_end (); ++it) {
		const clang::CXXBaseSpecifier &base = *it;
		clang::CXXRecordDecl *baseRecord = base.getType ().getTypePtr ()->getAsCXXRecordDecl ();
		if (!hasRecordValueSemantics (baseRecord)) {
			return false;
		}
		
	}
	
	// 
	return true;
}

bool TriaASTConsumer::hasTypeValueSemantics (const clang::Type *type) {
	if (type->isPointerType ()) {
		return !isDeclATemplate (type->getPointeeCXXRecordDecl ());
	}
	
	return hasRecordValueSemantics (type->getAsCXXRecordDecl ());
}

bool TriaASTConsumer::hasTypeValueSemantics (const clang::QualType &type) {
	return hasTypeValueSemantics (type.getTypePtr ());
}

bool TriaASTConsumer::shouldIntrospect (const Annotations &annotations, bool isGlobal, clang::CXXRecordDecl *record) {
	if (containsAnnotation (annotations, skipAnnotation)) {
		return false;
	}
	
	if (this->m_introspectAll) {
		return true;
	}
	
	return (!isGlobal ||
	        containsAnnotation (annotations, introspectAnnotation) ||
	        derivesFromIntrospectClass (record));
}

BaseDef TriaASTConsumer::processBase (clang::CXXBaseSpecifier *specifier) {
	BaseDef base;
	
	base.loc = specifier->getSourceRange ();
	base.name = typeName (specifier->getType ());
	base.access = specifier->getAccessSpecifier ();
	base.isVirtual = specifier->isVirtual ();
	
	return base;
}

static int findField (ClassDef &classDef, const QString &name) {
	for (int i = 0; i < classDef.variables.length (); i++) {
		if (classDef.variables.at (i).name == name) {
			return i;
		}
		
	}
	
	return -1;
}

bool TriaASTConsumer::registerReadWriteMethod (ClassDef &classDef, MethodDef &def, clang::CXXMethodDecl *decl) {
	bool canRead = false;
	bool canWrite = false;
	QString fieldName;
	
	for (int i = 0; i < def.annotations.length (); i++) {
		const AnnotationDef &cur = def.annotations.at (i);
		
		if (cur.name.startsWith (readAnnotation)) {
			fieldName = cur.name.mid (readAnnotation.length ());
			canRead = true;
		} else if (cur.name.startsWith (writeAnnotation)) {
			fieldName = cur.name.mid (writeAnnotation.length ());
			canWrite = true;
		} else {
			continue;
		}
		
		// Remove annotation
		def.annotations.removeAt (i);
	}
	
	// Neither read nor write method?
	if (!canRead && !canWrite) {
		return false;
	} else if (canRead && canWrite) {
		reportError (decl->getLocation (), "A method can't be NURIA_READ and NURIA_WRITE annotated.");
		return false;
	}
	
	// Check argument count
	if (canRead && !def.arguments.isEmpty () && !def.arguments.at (0).isOptional) {
		reportError (decl->getLocation (), "A NURIA_READ method must not take any mandatory arguments.");
		return false;
	}
	
	if (canWrite && (def.arguments.isEmpty () ||
			 (def.arguments.length () > 1 &&
			  !def.arguments.at (1).isOptional))) {
		reportError (decl->getLocation (),
			     "A NURIA_WRITE method must take the new value as first argument, "
			     "subsequent arguments must be optional.");
	        return false;
	}
	
	// Find field
	int fieldPos = findField (classDef, fieldName);
	
	if (fieldPos == -1) {
		VariableDef field;
		field.name = fieldName;
		field.access = clang::AS_public;
		field.type = canRead ? def.returnType.type : def.arguments.at (0).type;
		
		classDef.variables.append (field);
		fieldPos = classDef.variables.length () - 1;
	}
	
	// Clip annotations to the field
	VariableDef &field = classDef.variables[fieldPos];
	field.annotations += def.annotations;
	
	// Type and multiple-methods checks
	if (canRead) {
		
		if (field.type != def.returnType.type) {
			reportError (decl->getLocation (),
				     "A NURIA_READ method must return the same type as the field.");
			return false;
		} else if (!field.getter.isEmpty ()) {
			reportWarning (decl->getLocation (),
				       "Multiple registered NURIA_READ methods for this field.");
		}
		
	}
	
	if (canWrite) {
		if (field.type != def.arguments.at (0).type) {
			reportError (decl->getLocation (),
				     "The type of the first argument of a NURIA_WRITE "
				     "method must be the same type as the field.");
			return false;
		} else if (!field.setter.isEmpty ()) {
			reportWarning (decl->getLocation (),
				       "Multiple registered NURIA_WRITE methods for this field.");
		} else if (def.returnType.type != "void" && def.returnType.type != "_Bool") {
			reportWarning (decl->getLocation (),
				       "A NURIA_WRITE method should return void or bool.");
		}
		
	}
	
	// Store, done.
	if (canRead) {
		field.getter = def.name;
	} else {
		field.setter = def.name;
		field.setterArgName = def.arguments.at (0).name;
		field.setterReturnsBool = (def.returnType.type == "_Bool");
	}
	
	return true;
}

static inline clang::QualType getMethodResultType (clang::FunctionDecl *decl) {
#if CLANG_VERSION_MINOR > 4
	return decl->getReturnType ();
#else
	return decl->getResultType ();
#endif
}

void TriaASTConsumer::fillVariableDef (VariableDef &def, const clang::QualType &type) {
	clang::QualType pointee = type.getTypePtr ()->getPointeeType ();
	if (!pointee.isNull ()) {
		def.isConst = pointee.isConstant (*this->m_context) || pointee.isConstQualified () ||
		              (pointee.getQualifiers ().getCVRQualifiers () & clang::Qualifiers::Const);
	}
	
	// 
	def.type = typeName (type);
	def.isReference = type.getTypePtr ()->isReferenceType ();
	def.isConst |= type.isConstant (*this->m_context) || type.isConstQualified () ||
	               (type.getQualifiers ().getCVRQualifiers () & clang::Qualifiers::Const);
	
	def.isPodType = type.isPODType (*this->m_context);
	
}

void TriaASTConsumer::processMethod (ClassDef &classDef, clang::FunctionDecl *decl, bool isGlobal) {
	static const QString fromMethod = QStringLiteral ("from");
	static const QString toMethod = QStringLiteral ("to");
	
	clang::CXXMethodDecl *method = llvm::dyn_cast< clang::CXXMethodDecl > (decl);
	clang::CXXConstructorDecl *ctor = llvm::dyn_cast< clang::CXXConstructorDecl > (decl);
	clang::CXXDestructorDecl *dtor = llvm::dyn_cast< clang::CXXDestructorDecl > (decl);
	clang::CXXConversionDecl *convDecl = llvm::dyn_cast< clang::CXXConversionDecl > (decl);
	
	MethodDef def;
	
	// Base information
	def.loc = decl->getSourceRange ();
	def.access = (decl->getAccess () == clang::AS_none) ? clang::AS_public : decl->getAccess ();
	def.isVirtual = (method) ? method->isVirtual () : false;
	def.isPure = (method) ? method->isPure () : false;
	def.isConst = false;
	def.annotations = annotationsFromDecl (decl);
	
	if (method) {
		def.isConst = clang::Qualifiers::fromCVRMask (method->getTypeQualifiers ()).hasConst ();
	}
	
	if (!shouldIntrospect (def.annotations, isGlobal)) {
		return;
	}
	
	// Skip non-public methods. skipped methods and methods which are default-implemented
	bool resultTypeHasValueSemantics = hasTypeValueSemantics (getMethodResultType (decl));
	if (def.access != clang::AS_public || decl->isDefaulted () || decl->isDeleted () ||
	    !resultTypeHasValueSemantics || containsAnnotation (def.annotations, skipAnnotation)) {
		
		if (!resultTypeHasValueSemantics) {
			this->m_definitions->avoidType (def.returnType.type);
		}
		
		return;
	}
	
	// C'tors and D'tors don't have names nor do they really return anything
	if (ctor) {
		def.type = ConstructorMethod;
		def.returnType.loc = def.loc;
		def.returnType.type = classDef.name;
	} else if (dtor || convDecl || !decl->getDeclName ().isIdentifier ()) {
		// Ignore.
		return;
	} else {
		clang::QualType resultType = getMethodResultType (decl);
		def.name = llvmToString (decl->getName ());
		
		def.returnType.loc = def.loc;
		fillVariableDef (def.returnType, resultType);
		def.type = (!method) ? StaticMethod : (method->isStatic () ? StaticMethod : MemberMethod);
		
		// Ignore method if the result is a const pointer.
		if (resultType.getTypePtr ()->isPointerType () &&
		    resultType.getTypePtr ()->getPointeeType ().isConstQualified ()) {
			return;
		}
		
		// Also register result-type in the meta-system later on!
		declareType (resultType);
		
	}
	
	// Arguments
	bool hasMandatoryArgument = false;
	bool canConvert = (def.type == MemberMethod);
	for (int i = 0; i < int (decl->getNumParams ()); i++) {
		const clang::ParmVarDecl *param = decl->getParamDecl (i);
		VariableDef var;
		
		var.loc = param->getSourceRange ();
		var.name = llvmToString (param->getName ());
		var.isOptional = param->hasDefaultArg ();
		fillVariableDef (var, param->getType ());
		
		def.hasOptionalArguments = var.isOptional;
		
		// Invent a name for unnamed arguments
		if (var.name.isEmpty ()) {
			var.name = QStringLiteral ("__nuria_arg") + QString::number (i);
		}
		
		// Ignore methods with arguments without value-semantics.
		// Also ignore if the type is private (i.e. QPrivateSignal)
		clang::CXXRecordDecl *typeRec = param->getType ().getTypePtr ()->getAsCXXRecordDecl ();
		if (!hasTypeValueSemantics (param->getType ()) ||
		    (typeRec && typeRec->getAccess () != clang::AS_public)) {
			this->m_definitions->avoidType (var.type);
			return;
		}
		
		// Register type
		declareType (param->getType ());
		
		// This may be a viable conversion method if it only expects a
		// single argument (Plus optional ones)
		canConvert = (var.isOptional || i < 1);
		hasMandatoryArgument = !var.isOptional;
		
		def.arguments.append (var);
	}
	
	// Is this a read/write method for a field?
	if (method && registerReadWriteMethod (classDef, def, method)) {
		return;
	}
	
	// Final check if this is a conversion method
	if (method && canConvert && 
	    (def.type == ConstructorMethod ||
	     (def.type == StaticMethod && def.name.startsWith (fromMethod)) ||
	     (def.type == MemberMethod && !hasMandatoryArgument && def.name.startsWith (toMethod)))) {
		ConversionDef conv;
		conv.methodName = def.name;
		conv.type = def.type;
		conv.fromType = (def.type == MemberMethod)
				? classDef.name : def.arguments.first ().type;
		conv.toType = (def.type == ConstructorMethod)
			      ? classDef.name : def.returnType.type;
		conv.isConst = def.isConst && (def.type == MemberMethod || def.arguments.first ().isConst);
		conv.loc = def.loc;
		
		// 
		classDef.conversions.append (conv);
		
	}
	
	// Remember default and copy ctors for later.
	if (ctor) {
		if (def.arguments.isEmpty ()) {
			classDef.implementsCtor = true;
		} else if (def.arguments.at (0).type == classDef.name) {
			classDef.implementsCopyCtor = true;
		}
		
	}
	
	// 
	classDef.methods.append (def);
}

VariableDef TriaASTConsumer::processVariable (clang::FieldDecl *decl) {
	VariableDef def;
	
	def.loc = decl->getSourceRange ();
	def.access = (decl->getAccess () == clang::AS_none) ? clang::AS_public : decl->getAccess ();
	def.type = typeName (decl->getType ());
	def.name = llvmToString (decl->getName ());
	def.annotations = annotationsFromDecl (decl);
	def.isPodType = decl->getType ().isPODType (*this->m_context);

	if (def.access != clang::AS_public) {
		return def;
	}
	
	if (hasTypeValueSemantics (decl->getType ())) {
		declareType (decl->getType ());
	} else {
		reportWarning (decl->getLocation (), "Type of variable doesn't have value-semantics, skipping.");
		this->m_definitions->avoidType (def.type);
	}
	
	return def;
}

void TriaASTConsumer::processEnum (ClassDef &classDef, clang::EnumDecl *decl, bool isGlobal) {
	clang::AccessSpecifier access = decl->getAccess ();
	Annotations annotations = annotationsFromDecl (decl);
	
	if ((access != clang::AS_public && access != clang::AS_none) ||
	    !shouldIntrospect (annotations, isGlobal) ||
	    isDeclATemplate (decl->getParent ())) {
		return;
	}
	
	// 
	EnumDef def;
	def.loc = decl->getSourceRange ();
	def.name = llvmToString (decl->getName ());
	def.annotations = annotations;
	
	for (auto it = decl->enumerator_begin (); it != decl->enumerator_end (); ++it) {
		const clang::EnumConstantDecl *cur = *it;
		const llvm::APSInt &value = cur->getInitVal ();
		
		QString elementName = llvmToString (cur->getName ());
		int elementValue = (value.isUnsigned () ? value.getZExtValue () : value.getSExtValue ());
		
		def.elements.insert (elementName, elementValue);
	}
	
	// Store and declare
	classDef.enums.append (def);
	declareType (clang::QualType (decl->getTypeForDecl (), 0));
//	this->m_definitions->declareType (classDef.name + QStringLiteral ("::") + def.name);
	
}
#include <qmetatype.h>
void TriaASTConsumer::processConversion (ClassDef &classDef, clang::CXXConversionDecl *convDecl) {
	if (!hasTypeValueSemantics (convDecl->getConversionType ())) {
		return;
	}
	
	// Make sure this isn't NURIA_SKIP'd
	if (containsAnnotation (annotationsFromDecl (convDecl), skipAnnotation)) {
		return;
	}
	
	ConversionDef conv;
	conv.loc = convDecl->getSourceRange ();
	conv.type = MemberMethod;
	conv.isConst = convDecl->isConst ();
	conv.fromType = classDef.name;
	conv.toType = typeName (convDecl->getConversionType ());
	conv.methodName = QStringLiteral ("operator ") + conv.toType;
	
	declareType (convDecl->getConversionType ());
	classDef.conversions.append (conv);
}

void TriaASTConsumer::HandleTagDeclDefinition (clang::TagDecl *decl) {
	clang::CXXRecordDecl *record = llvm::dyn_cast< clang::CXXRecordDecl > (decl);
	clang::EnumDecl *enumDecl = llvm::dyn_cast< clang::EnumDecl > (decl);
	
	if (isDeclUnnamed (decl)) {
		return;
	}
	
	// 
	if (record) {
		processClass (record);
	} else if (enumDecl && !this->m_globals.name.isEmpty ()) {
		processEnum (this->m_globals, enumDecl, true);
	}
	
}

bool TriaASTConsumer::HandleTopLevelDecl (clang::DeclGroupRef groupRef) {
	for (auto it = groupRef.begin (), end = groupRef.end (); it != end; ++it) {
		if (!this->m_definitions->sourceFiles ().contains (fileOfDecl (*it))) {
			continue;
		}
		
		if (clang::FunctionDecl *function = llvm::dyn_cast< clang::FunctionDecl > (*it)) {
			processMethod (this->m_globals, function, true);
		}
		
	}
	
	return true;
}

void TriaASTConsumer::HandleTranslationUnit (clang::ASTContext &) {
	if (!this->m_globals.name.isEmpty () &&
	    (!this->m_globals.enums.isEmpty () ||
	     !this->m_globals.methods.isEmpty ())) {
		this->m_definitions->addClassDefinition (this->m_globals);
	}
	
}

bool TriaASTConsumer::isDeclATemplate (const clang::DeclContext *decl) {
	while (decl) {
		const clang::CXXRecordDecl *record = clang::dyn_cast< const clang::CXXRecordDecl > (decl);
		if (record && record->getDescribedClassTemplate () != nullptr) {
			return true;
		}
		
		decl = decl->getParent ();
	}
	
	return false;
}

bool TriaASTConsumer::isDeclUnnamed (const clang::DeclContext *decl) {
	while (decl) {
		const clang::NamedDecl *named = clang::dyn_cast< const clang::NamedDecl > (decl);
		if (named && named->getDeclName ().isIdentifier () && named->getName ().empty ()) {
			return true;
		}
		
		decl = decl->getParent ();
	}
	
	return false;
}

void TriaASTConsumer::processClass (clang::CXXRecordDecl *record) {
	static const llvm::StringRef qMetaTypeIdName ("QMetaTypeId");
	
	// Check if this is a QMetaTypeId specialization (Result of a Q_DECLARE_METATYPE)
	clang::ClassTemplateSpecializationDecl *templ = llvm::dyn_cast< clang::ClassTemplateSpecializationDecl > (record);
	
	if (templ && templ->getName () == qMetaTypeIdName) {
		const clang::TemplateArgumentList &list = templ->getTemplateArgs ();
		if (list.size () == 1) {
			QString name = typeName (list.get (0).getAsType ());
			this->m_definitions->addDeclaredType (name);
		}
		
		return;
	}
	
	// 
	bool typeHasValueSemantics = hasTypeValueSemantics (record->getTypeForDecl ());
	if (!typeHasValueSemantics) {
		this->m_definitions->avoidType (typeName (record->getTypeForDecl ()));
	}
	
	// Skip templates
	if (isDeclATemplate (record)) {
		return;
	}
	
	// Base data
	ClassDef classDef;
	classDef.loc = record->getSourceRange ();
	classDef.access = record->getAccess ();
	classDef.name = typeDeclName (record); // QString::fromStdString (record->getQualifiedNameAsString ());
	classDef.file = fileOfDecl (record);
	classDef.annotations = annotationsFromDecl (record);
	
	// Skip if not to be 'introspected'
	if (!shouldIntrospect (classDef.annotations, true, record)) {
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
	
	// Does this type have a public c'tor which takes no arguments,
	// a public copy-ctor and assignment operator?
	classDef.hasDefaultCtor = record->hasDefaultConstructor () ||
				  record->hasUserProvidedDefaultConstructor ();
	classDef.hasCopyCtor = record->hasCopyConstructorWithConstParam () ||
			       record->hasUserDeclaredCopyConstructor ();
	classDef.hasAssignmentOperator = record->hasCopyAssignmentWithConstParam () ||
					 record->hasUserDeclaredCopyAssignment ();
	classDef.hasPureVirtuals = record->isAbstract ();
	
	classDef.hasValueSemantics = classDef.hasDefaultCtor && classDef.hasCopyCtor &&
				     classDef.hasAssignmentOperator && typeHasValueSemantics;
	
	if (!classDef.hasValueSemantics) {
		this->m_definitions->avoidType (classDef.name);
	}
	
	// Ignore further information if the type isn't in the source file(s)
	if (!this->m_definitions->sourceFiles ().contains (classDef.file)) {
		return;
	}
	
	// Parent classes
	for (auto it = record->bases_begin (); it != record->bases_end (); ++it) {
		classDef.bases.append (processBase (it));
	}
	
	// Methods
	for (auto it = record->method_begin (); it != record->method_end (); ++it) {
		processMethod (classDef, *it);
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
		processConversion (classDef, convDecl);
		
	}
	
	// Find enums
	for (auto it = record->decls_begin (); it != record->decls_end (); ++it) {
		clang::EnumDecl *enumDecl = llvm::dyn_cast< clang::EnumDecl > (*it);
		
		if (enumDecl && !isDeclUnnamed (enumDecl)) {
			processEnum (classDef, enumDecl);
		}
		
	}
	
	// Done.
	addDefaultConstructors (record, classDef);
	this->m_definitions->addClassDefinition (classDef);
	
}

void TriaASTConsumer::addDefaultConstructors (clang::CXXRecordDecl *record, ClassDef &classDef) {
	if (!hasRecordValueSemantics (record, false) || !classDef.hasDefaultCtor ||
	    !classDef.hasCopyCtor || !classDef.hasAssignmentOperator) {
		return;
	}
	
	// 
	if (!classDef.implementsCopyCtor) {
		addDefaultCopyConstructor (classDef);
	}
	
	if (!classDef.implementsCtor) {
		addDefaultConstructor (classDef);
	}
	
}

void TriaASTConsumer::addDefaultConstructor (ClassDef &classDef) {
	addConstructor (classDef, { });
}

void TriaASTConsumer::addDefaultCopyConstructor (ClassDef &classDef) {
	VariableDef argument;
	argument.loc = classDef.loc;
	argument.isConst = true;
	argument.name = QStringLiteral("other");
	argument.type = classDef.name;
	
	addConstructor (classDef, { argument });
}

void TriaASTConsumer::addConstructor (ClassDef &classDef, const Variables &arguments) {
	MethodDef def;
	def.access = clang::AS_public;
	def.loc = classDef.loc;
	def.type = ConstructorMethod;
	def.isVirtual = false;
	def.isConst = false;
	def.returnType.loc = classDef.loc;
	def.returnType.type = classDef.name;
	def.arguments = arguments;
	
	classDef.methods.prepend (def);
	
}

void TriaASTConsumer::reportError (clang::SourceLocation loc, const QByteArray &info) {
	reportMessage (clang::DiagnosticsEngine::Error, loc, info);
}

void TriaASTConsumer::reportWarning (clang::SourceLocation loc, const QByteArray &info) {
	reportMessage (clang::DiagnosticsEngine::Warning, loc, info);
}

void TriaASTConsumer::reportMessage (clang::DiagnosticsEngine::Level level, clang::SourceLocation loc,
				     const QByteArray &info) {
	llvm::StringRef message (info.constData (), info.length ());
	clang::DiagnosticsEngine &diag = this->m_context->getDiagnostics ();

#if CLANG_VERSION_MINOR > 4
	clang::StoredDiagnostic stored (level, clang::Diagnostic (&diag, message));
	diag.Report (stored);
#else
	unsigned int id = diag.getCustomDiagID (level, message);
	diag.Report (loc, id);
#endif
	
}

void TriaASTConsumer::declareType (const clang::QualType &type) {
	const clang::Type *ptr = type.getTypePtr ();
	const clang::Type *pointee = ptr;
	const clang::CXXRecordDecl *decl = ptr->getAsCXXRecordDecl ();
	
	if (ptr->isPointerType ()) {
		pointee = ptr->getPointeeType ().getTypePtr ();
		decl = ptr->getPointeeCXXRecordDecl ();
	}
	
	if ((type.isPODType (*this->m_context) && !ptr->isEnumeralType () && !ptr->isPointerType ()) ||
	    ptr->isVoidType () || ptr->isTemplateTypeParmType () || pointee->isTemplateTypeParmType () ||
	    isDeclUnnamed (decl) || !hasTypeValueSemantics (type)) {
		return;
	}
	
	// Ignore if it is known to the meta-system of this running tria instance.
	QString name = typeName (type);
	QString desugared = typeName (type.getDesugaredType (*this->m_context));
	int metaId = QMetaType::type (qPrintable(name));
	
	if (metaId != QMetaType::UnknownType && metaId < QMetaType::User) {
		return;
	}
	
	if (name != desugared) {
		this->m_definitions->addTypeDef (desugared, name);
        }
        
	// 
	this->m_definitions->declareType (name);
	
}
