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

#ifndef DEFINITIONS_HPP
#define DEFINITIONS_HPP

#include <clang/Basic/Specifiers.h>
#include <QStringList>
#include <QString>
#include <QVector>

enum MethodType {
	ConstructorMethod = 0,
	DestructorMethod,
	MemberMethod,
	StaticMethod
};

static const char *methodTypeStr[] = { "Constructor", "Destructor",
				       "Method", "Static" };
static const char *accessStr[] = { "public", "protected", "private", "none" };

struct AnnotationDef {
	QString name;
	QString value;
	bool valueIsString = false;
	int index = -1;
};

typedef QVector< AnnotationDef > Annotations;

struct VariableDef {
	clang::AccessSpecifier access;
	
	QString name;
	QString type;
	
	QString getter;
	QString setter;
	
	Annotations annotations;
	
	bool isOptional = false;
};

typedef QVector< VariableDef > Variables;

struct MethodDef {
	clang::AccessSpecifier access;
	MethodType type;
	bool isVirtual;
	QString name;
	QString returnType;
	Variables arguments;
	Annotations annotations;
	
};

typedef QVector< MethodDef > Methods;

struct ConversionDef {
	QString methodName;
	MethodType type;
	QString fromType;
	QString toType;
};

typedef QVector< ConversionDef > Conversions;

struct BaseDef {
	clang::AccessSpecifier access;
	bool isVirtual;
	QString name;
	
};

typedef QVector< BaseDef > Bases;

struct EnumDef {
	QString name;
	QStringList values;
	Annotations annotations;
	
};

typedef QVector< EnumDef > Enums;

struct ClassDef {
	clang::AccessSpecifier access;
	Bases bases;
	QString name;
	Variables variables;
	Methods methods;
	Enums enums;
	Conversions conversions;
	Annotations annotations;
	
	bool hasValueSemantics;
	bool hasDefaultCtor;
	bool hasCopyCtor;
	bool hasAssignmentOperator;
	
};

// 
QDebug operator<< (QDebug dbg, const VariableDef &variable);
QDebug operator<< (QDebug dbg, const BaseDef &base);
QDebug operator<< (QDebug dbg, const MethodDef &method);
QDebug operator<< (QDebug dbg, const ClassDef &def);

#endif // DEFINITIONS_HPP
