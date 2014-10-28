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

#ifndef DEFS_HPP
#define DEFS_HPP

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <QMetaType>
#include <QString>
#include <QVector>
#include <QMap>

enum MethodType {
	ConstructorMethod = 0,
	DestructorMethod,
	MemberMethod,
	StaticMethod
};

enum AnnotationType {
	IntrospectAnnotation,
	SkipAnnotation,
	ReadAnnotation,
	WriteAnnotation,
	RequireAnnotation,
	CustomAnnotation,
};

struct AnnotationDef {
	clang::SourceRange loc;
	AnnotationType type;
	QString name;
	QString value;
	QMetaType::Type valueType = QMetaType::QVariant;
	int index = -1;
};

typedef QVector< AnnotationDef > Annotations;

struct VariableDef {
	clang::SourceRange loc;
	clang::AccessSpecifier access;
	
	QString name;
	QString type;
	
	QString getter;
	QString setterArgName;
	QString setter;
	
	Annotations annotations;
	
	bool isReference = false;
	bool isConst = false;
	bool isPodType = false;
	bool isOptional = false;
	bool setterReturnsBool = false;
};

typedef QVector< VariableDef > Variables;

struct MethodDef {
	clang::SourceRange loc;
	clang::AccessSpecifier access;
	MethodType type;
	bool isVirtual;
	bool isConst;
	QString name;
	QString returnType;
	Variables arguments;
	Annotations annotations;
	bool hasOptionalArguments = false;
	bool returnTypeIsPod = false;
	
};

typedef QVector< MethodDef > Methods;

struct ConversionDef {
	clang::SourceRange loc;
	QString methodName;
	MethodType type;
	QString fromType;
	QString toType;
	bool isConst;
};

typedef QVector< ConversionDef > Conversions;

struct BaseDef {
	clang::AccessSpecifier access;
	clang::SourceRange loc;
	bool isVirtual;
	QString name;
	
};

typedef QVector< BaseDef > Bases;

struct EnumDef {
	clang::SourceRange loc;
	
	QString name;
	QMap< QString, int > elements;
	Annotations annotations;
	
};

typedef QVector< EnumDef > Enums;

struct ClassDef {
	clang::AccessSpecifier access;
	clang::SourceRange loc;
	Bases bases;
	QString name;
	QString file;
	Variables variables;
	Methods methods;
	Enums enums;
	Conversions conversions;
	Annotations annotations;
	
	bool isFakeClass = false;
	bool hasValueSemantics = false;
	bool hasDefaultCtor = false;
	bool hasCopyCtor = false;
	bool hasAssignmentOperator = false;
	bool implementsCtor = false;
	bool implementsCopyCtor = false;
	
};

// 
QDebug operator<< (QDebug dbg, const VariableDef &variable);
QDebug operator<< (QDebug dbg, const BaseDef &base);
QDebug operator<< (QDebug dbg, const MethodDef &method);
QDebug operator<< (QDebug dbg, const ClassDef &def);

#endif // DEFS_HPP
