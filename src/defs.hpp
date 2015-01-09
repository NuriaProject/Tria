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
	clang::AccessSpecifier access = clang::AS_public;
	
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
	bool isPure;
	bool isConst;
	QString name;
	VariableDef returnType;
	Variables arguments;
	Annotations annotations;
	bool hasOptionalArguments = false;
	
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
	bool hasPureVirtuals = false;
	
};

// 
QDebug operator<< (QDebug dbg, const VariableDef &variable);
QDebug operator<< (QDebug dbg, const BaseDef &base);
QDebug operator<< (QDebug dbg, const MethodDef &method);
QDebug operator<< (QDebug dbg, const ClassDef &def);

#endif // DEFS_HPP
