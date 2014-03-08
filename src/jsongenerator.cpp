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

#include "jsongenerator.hpp"

#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QFile>

#include "definitions.hpp"

JsonGenerator::JsonGenerator (Definitions *definitions)
	: m_definitions (definitions)
{
	
}

bool JsonGenerator::generate (QIODevice *device, bool append) {
	QJsonDocument doc;
	QJsonObject root;
	
	if (append) {
		doc = documentFromDevice (device);
	}
	
	// 
	if (!doc.isNull ()) {
		root = doc.object ();
	}
	
	// Find root object
	QString name = this->m_definitions->sourceFileName ();
	QJsonValue fileObjectValue = root.value (name);
	if (!fileObjectValue.isNull () && !fileObjectValue.isObject ()) {
		qCritical() << "Error: Field" << name << "is not a object";
		return false;
	}
	
	// Serialize
	QJsonObject fileObject;
	if (!serializeClasses (fileObject)) {
		return false;
	}
	
	// 
	root.insert (name, fileObject);
	doc.setObject (root);
	
	// Truncate file and store JSON data.
	device->reset ();
	qobject_cast< QFileDevice * > (device)->resize (0);
	device->write (doc.toJson ());
	
	// Done
	return true;
}

QJsonDocument JsonGenerator::documentFromDevice (QIODevice *device) {
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson (device->readAll (), &error);
	
	// 
	if (error.error != QJsonParseError::NoError) {
		qCritical() << "Failed to parse JSON document:" << error.errorString ();
	}
	
	return doc;
}

bool JsonGenerator::serializeClasses (QJsonObject &object) {
	
	QVector< ClassDef > classes = this->m_definitions->classDefintions ();
	for (const ClassDef &cur : classes) {
		QJsonObject obj;
		if (!serializeClass (obj, cur)) {
			return false;
		}
		
		// 
		object.insert (cur.name, obj);
		
	}
	
	// 
	return true;
	
}

bool JsonGenerator::serializeClass (QJsonObject &target, const ClassDef &classDef) {
	QJsonArray annotations;
	QJsonArray memberMethods;
	QJsonArray staticMethods;
	QJsonArray constructors;
	QJsonObject enums;
	QJsonObject fields;
	
	// Serialize
	if (!serializeAnnotations (annotations, classDef.annotations) ||
	    !serializeMethods (memberMethods, staticMethods, constructors, classDef.methods) ||
	    !serializeEnums (enums, classDef.enums) ||
	    !serializeFields (fields, classDef.variables)) {
		return false;
	}
	
	// Store.
	target.insert (QStringLiteral("annotations"), annotations);
	target.insert (QStringLiteral("memberMethods"), memberMethods);
	target.insert (QStringLiteral("staticMethods"), staticMethods);
	target.insert (QStringLiteral("constructors"), constructors);
	target.insert (QStringLiteral("enums"), enums);
	target.insert (QStringLiteral("fields"), fields);
	
	return true;
}

bool JsonGenerator::serializeAnnotations (QJsonArray &target, const Annotations &annotations) {
	for (const AnnotationDef &cur : annotations) {
		if (cur.type != CustomAnnotation) {
			continue;
		}
		
		// 
		QJsonObject obj;
		QJsonValue value = annotationValueToJson (cur);
		
		obj.insert (QStringLiteral("name"), cur.name);
		
		if (!value.isNull ()) {
			obj.insert (QStringLiteral("value"), value);
		}
		
		// 
		target.append (obj);
	}
	
	return true;
}

QJsonValue JsonGenerator::annotationValueToJson (const AnnotationDef &def) {
	
	// 
	switch (def.valueType) {
	case QMetaType::Bool:
		return !def.value.compare (QLatin1String("true"));
		
	case QMetaType::Int:
	case QMetaType::UInt:
		return def.value.toInt ();
		
	case QMetaType::Float:
	case QMetaType::Double:
		return def.value.toDouble ();
		
	case QMetaType::QString:
		return def.value;
		
	default:
		return QJsonValue ();
	}
	
}

bool JsonGenerator::serializeMethods (QJsonArray &memberMethods, QJsonArray &staticMethods,
				      QJsonArray &constructors, const Methods &methods) {
	
	for (const MethodDef &cur : methods) {
		QJsonObject obj;
		QJsonArray annotations;
		QJsonArray argNames;
		QJsonArray argTypes;
		
		// 
		serializeAnnotations (annotations, cur.annotations);
		for (const VariableDef &var : cur.arguments) {
			argNames.append (var.name);
			argTypes.append (var.type);
		}
		
		// Populate
		obj.insert (QStringLiteral("name"), cur.name);
		obj.insert (QStringLiteral("annotations"), annotations);
		obj.insert (QStringLiteral("resultType"), cur.returnType);
		obj.insert (QStringLiteral("argumentNames"), argNames);
		obj.insert (QStringLiteral("argumentTypes"), argTypes);
		
		// Store
		if (cur.type == MemberMethod) {
			memberMethods.append (obj);
		} else if (cur.type == StaticMethod) {
			staticMethods.append (obj);
		} else {
			constructors.append (obj);
		}
		
	}
	
	// 
	return true;
	
}

bool JsonGenerator::serializeEnums (QJsonObject &target, const Enums &enums) {
	for (const EnumDef &cur : enums) {
		QJsonObject obj;
		QJsonArray annotations;
		QJsonObject values;
		
		// 
		serializeAnnotations (annotations, cur.annotations);
		
		auto it = cur.elements.constBegin ();
		auto end = cur.elements.constEnd ();
		for (; it != end; ++it) {
			values.insert (it.key (), it.value ());
		}
		
		// 
		obj.insert (QStringLiteral("annotations"), annotations);
		obj.insert (QStringLiteral("values"), values);
		target.insert (cur.name, obj);
		
	}
	
	// 
	return true;
}

bool JsonGenerator::serializeFields (QJsonObject &target, const Variables &fields) {
	for (const VariableDef &cur : fields) {
		QJsonObject obj;
		QJsonArray annotations;
		bool readOnly = (cur.getter.isEmpty () != cur.setter.isEmpty ());
		
		// 
		serializeAnnotations (annotations, cur.annotations);
		obj.insert (QStringLiteral("annotations"), annotations);
		obj.insert (QStringLiteral("type"), cur.type);
		obj.insert (QStringLiteral("readOnly"), readOnly);
		target.insert (cur.name, obj);
		
	}
	
	// 
	return true;
}
