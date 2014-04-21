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

#include "definitions.hpp"

#include <algorithm>

#include <QDateTime>
#include <QIODevice>
#include <QBuffer>
#include <QRegExp>
#include <QDebug>

#include <clang/Tooling/Tooling.h>
#include "defs.hpp"

Definitions::Definitions (const QString &sourceFileName)
	: m_fileName (sourceFileName)
{
	
}

QString Definitions::sourceFileName () const {
	return this->m_fileName;
}

void Definitions::addClassDefinition (const ClassDef &theClass) {
	this->m_classes.append (theClass);
	cleanUpClassDef (this->m_classes.last ());
}

QVector< ClassDef > Definitions::classDefintions () const {
	return this->m_classes;
}

StringSet Definitions::declaredTypes () const {
	return this->m_declaredTypes;
}

void Definitions::addDeclaredType (const QString &type) {
	this->m_declaredTypes.insert (type);
}

bool Definitions::isTypeDeclared (const QString &type) {
	return this->m_declaredTypes.contains (type);
}

StringSet Definitions::declareTypes () const {
	return this->m_declareTypes;
}

StringSet Definitions::declareTypesWithoutDuplicates () const {
	StringSet result;
	
	for (const QString cur : this->m_declareTypes) {
		if (!this->m_typeDefs.contains (cur)) {
			result.insert (cur);
		}
		
	}
	
	return result;
}

void Definitions::declareType (const QString &type) {
	this->m_declareTypes.insert (type);
}

void Definitions::undeclareType (const QString &type) {
	this->m_declareTypes.remove (type);
}

void Definitions::addTypeDef (const QString &desugared, const QString &typeDef) {
	this->m_typeDefs.insert (typeDef, desugared);
}

StringSet Definitions::avoidedTypes () const {
	return this->m_avoidedTypes;
}

bool Definitions::isTypeAvoided (const QString &type) {
	return this->m_avoidedTypes.contains (type);
}

void Definitions::avoidType (const QString &type) {
	this->m_avoidedTypes.insert (type);
}

template< typename T >
static bool sortByName (const T &lhs, const T &rhs) {
	return lhs.name < rhs.name;
}

static bool methodLess (const MethodDef &lhs, const MethodDef &rhs) {
	if (lhs.name == rhs.name) {
		return lhs.arguments.length () < rhs.arguments.length ();
	}
	
	return lhs.name < rhs.name;
}

static bool checkArgumentsForAvoidedTypes (const StringSet &avoid, const Variables &args) {
	for (const VariableDef &cur : args) {
		if (avoid.contains (cur.type)) {
			return true;
		}
		
	}
	
	return false;
}

static void filterMethods (const StringSet &avoid, Methods &methods) {
	for (int i = 0; i < methods.length (); i++) {
		const MethodDef &cur = methods.at (i);
		
		if ((!cur.name.isEmpty () && avoid.contains (cur.returnType)) ||
		    checkArgumentsForAvoidedTypes (avoid, cur.arguments)) {
			methods.remove (i);
			i--;
		}
		
	}
	
}

static void filterFields (const StringSet &avoid, Variables &fields) {
	for (int i = 0; i < fields.length (); i++) {
		const VariableDef &cur = fields.at (i);
		
		if (avoid.contains (cur.type)) {
			fields.remove (i);
			i--;
		}
		
	}
	
}

static int firstOptionalArgument (const MethodDef &method) {
	int i;
	for (i = 0; i < method.arguments.length (); i++) {
		if (method.arguments.at (i).isOptional) {
			break;
		}
		
	}
	
	return i;
}

static void expandMethodsWithOptionalArgs (Methods &methods) {
	
	for (auto it = methods.begin (); it != methods.end (); ++it) {
		MethodDef cur = *it; // Intentionally not a reference.
		int totalArgs = cur.arguments.length ();
		for (int i = firstOptionalArgument (cur); i < totalArgs; i++, ++it) {
			MethodDef overload = cur;
			overload.arguments.resize (i);
			it = methods.insert (it, overload);
		}
		
	}
	
}

void Definitions::cleanUpClassDef (ClassDef &def) {
	
	// Filter methods and fields which can't be exposed as their type(s)
	// doesn't have value-semantics.
	filterMethods (this->m_avoidedTypes, def.methods);
	filterFields (this->m_avoidedTypes, def.variables);
	
	// Expose methods with optional arguments as overloads. Expand first to
	// catch cases where a class has static and member methods of the same
	// name.
	expandMethodsWithOptionalArgs (def.methods);
	
	// Sort methods, fields and enums for faster access
	std::sort (def.bases.begin (), def.bases.end (), &sortByName< BaseDef >);
	std::sort (def.methods.begin (), def.methods.end (), methodLess);
	std::sort (def.variables.begin (), def.variables.end (), &sortByName< VariableDef >);
	std::sort (def.enums.begin (), def.enums.end (), &sortByName< EnumDef >);
	
	// 
	for (EnumDef &cur : def.enums) {
		std::sort (cur.annotations.begin (), cur.annotations.end (), &sortByName< AnnotationDef >);
	}
	
}
