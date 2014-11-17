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
