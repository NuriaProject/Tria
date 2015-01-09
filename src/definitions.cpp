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

#include "definitions.hpp"

#include <algorithm>

#include <QDateTime>
#include <QIODevice>
#include <QBuffer>
#include <QRegExp>
#include <QDebug>

#include <clang/Tooling/Tooling.h>
#include "triaaction.hpp"
#include "defs.hpp"
#undef bool

Definitions::Definitions (const QStringList &sourceFiles)
	: m_fileNames (sourceFiles)
{
}

Definitions::~Definitions () {
	delete this->m_timing;
}

QStringList Definitions::sourceFiles () const {
	return this->m_fileNames;
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

QMap< QString, bool > Definitions::declareTypes() const {
	return this->m_declareTypes;
}

void Definitions::declareType (const QString &type, bool isFullyDeclared) {
	if (this->m_declareTypes.value (type) == false) {
		this->m_declareTypes.insert (type, isFullyDeclared);
	}
	
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

StringMap Definitions::typedefs () const {
	return this->m_typeDefs;
}

bool Definitions::isTypeAvoided (const QString &type) {
	return this->m_avoidedTypes.contains (type);
}

void Definitions::avoidType (const QString &type) {
	this->m_avoidedTypes.insert (type);
}

TimingNode *Definitions::timing () const {
	return this->m_timing;
}

void Definitions::parsingComplete () {
	if (this->m_timing) {
		this->m_timing->stop ();
	}
	
}

void Definitions::setTimingNode (TimingNode *node) {
	this->m_timing = node;
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
		
		if ((!cur.name.isEmpty () && avoid.contains (cur.returnType.type)) ||
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
