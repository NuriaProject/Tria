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

#include "defs.hpp"

#include <QString>
#include <QDebug>

static const char *methodTypeStr[] = { "Constructor", "Destructor", "Method", "Static" };
static const char *accessStr[] = { "public", "protected", "private", "none" };

QDebug operator<< (QDebug dbg, const AnnotationDef &annotation) {
	dbg.nospace () << annotation.name << "(" << annotation.value << ")";
	return dbg;
}

static void dumpAnnotations (QDebug &dbg, const Annotations &annotations) {
	if (annotations.isEmpty ()) {
		return;
	}
	
	// 
	dbg.nospace () << "[";
	
	for (const AnnotationDef &cur : annotations) {
		dbg.nospace () << cur << ", ";
	}
	
	dbg.nospace () << "] ";
}

QDebug operator<< (QDebug dbg, const VariableDef &variable) {
	dumpAnnotations (dbg, variable.annotations);
	dbg.nospace () << accessStr[variable.access] << " "
		       << qPrintable(variable.type) << " "
		       << qPrintable(variable.name);
	
	return dbg.maybeSpace ();
}

QDebug operator<< (QDebug dbg, const BaseDef &base) {
	dbg.nospace () << accessStr[base.access] << " ";
	if (base.isVirtual) {
		dbg << "virtual ";
	}
	
	dbg << qPrintable(base.name);
	return dbg.maybeSpace ();
}

QDebug operator<< (QDebug dbg, const MethodDef &method) {
	if (method.isVirtual) {
		dbg.nospace () << "virtual ";
	}
	
	dbg.nospace () << methodTypeStr[method.type] << " ";
	dumpAnnotations (dbg, method.annotations);
	
	dbg.nospace () << accessStr[method.access] << " "
		       << qPrintable(method.returnType.type) << " "
		       << qPrintable(method.name);
	
	dbg.nospace () << "(";
	for (const VariableDef &cur : method.arguments) {
		dbg.nospace () << cur << ", ";
	}
	
	dbg << ")";
	
	if (method.isConst) {
		dbg << " const";
	}
	
	return dbg.maybeSpace ();
}

QDebug operator<< (QDebug dbg, const ClassDef &def) {
	dbg.nospace () << "Class " << accessStr[def.access] << " " << qPrintable(def.name);
	
	dumpAnnotations (dbg, def.annotations);
	dbg.nospace () << "\n";
	
	dbg.nospace () << "Inherits: ";
	if (def.bases.isEmpty ()) {
		dbg.nospace () << "<Nothing>";
	}
	for (const BaseDef &cur : def.bases) {
		dbg << cur << ", ";
	}
	dbg << "\n";
	
	for (const VariableDef &cur : def.variables) {
		dbg << "- " << cur << "\n";
	}
	
	for (const MethodDef &cur : def.methods) {
		dbg << "- " << cur << "\n";
	}
	
	return dbg;
}
