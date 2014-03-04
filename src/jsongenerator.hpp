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

#ifndef JSONGENERATOR_HPP
#define JSONGENERATOR_HPP

#include "defs.hpp"

#include <QJsonDocument>
#include <QMap>
#include <QSet>

class QIODevice;
class Definitions;

/**
 * \brief The JsonGenerator class outputs JSON data instead of C++ code.
 * 
 * \par JSON Structure
 * The JSON data is structured in the following way:
 * 
 * File:
 * \code
 * {
 *   "<source filename>": <FileData>,
 *   ...
 * }
 * \endcode
 * 
 * FileData:
 * \code
 * { "<class name>": <ClassData>, ... }
 * \endcode
 * 
 * ClassData:
 * \code
 * {
 *   "annotations": <Annotations>,
 *   "bases": [ "<base class>", ... ],
 *   "memberMethods": <Methods>,
 *   "staticMethods": <Methods>,
 *   "constructors": <Methods>,
 *   "enums": <Enums>,
 *   "fields": <Fields>
 * }
 * \endcode
 * 
 * Annotations:
 * \code
 * [ <AnnotationData>, ... ]
 * \endcode
 * 
 * AnnotationData:
 * \code
 * {
 *   "name": <name>,
 *   "value": <value>
 * }
 * \endcode
 * 
 * \note "value" is optional. It is only set if the value in the source code is
 * either a POD type (bool, int, ...) or a string.
 * 
 * Methods:
 * \code
 * [ <MethodData>, ... ]
 * \endcode
 * 
 * MethodData:
 * \code
 * {
 *   "name": <name>,
 *   "annotations": <Annotations>,
 *   "resultType": <result type>,
 *   "argumentNames": [ "<argument name>", ... ],
 *   "argumentTypes": [ "<argument type>", ... ]
 * }
 * \endcode
 * 
 * \note The name of constructors is empty (Empty string).
 * 
 * Enums:
 * \code
 * { "<name of enum>": <EnumData>, ... }
 * \endcode
 * 
 * EnumData:
 * \code
 * {
 *   "annotations": <Annotations>,
 *   "values": { "<key>": <value>, ... }
 * }
 * \endcode
 * 
 * Fields:
 * \code
 * { "<name of field>": <FieldData>, ... }
 * \endcode
 * 
 * FieldData:
 * \code
 * {
 *   "annotations": <Annotations>
 *   "type": "<type name>",
 *   "readOnly": <true/false>
 * }
 * \endcode
 */
class JsonGenerator {
public:
	JsonGenerator (Definitions *definitions);
	
	/**
	 * Generates code and writes it to \a device.
	 * If \a append is \c true, the JSON document in \a device is not
	 * destroyed but instead all classes of the definitions structure are
	 * inserted into it. \a device is expected to be opened in read-write
	 * mode for this to work. If \a append is \c false, the JSON document
	 * will be written into it directly.
	 */
	bool generate (QIODevice *device, bool append);
	
private:
	
	QJsonDocument documentFromDevice (QIODevice *device);
	bool serializeClasses (QJsonObject &object);
	bool serializeClass (QJsonObject &target, const ClassDef &classDef);
	bool serializeAnnotations (QJsonArray &target, const Annotations &annotations);
	QJsonValue annotationValueToJson (const AnnotationDef &def);
	bool serializeMethods (QJsonArray &memberMethods, QJsonArray &staticMethods,
			       QJsonArray &constructors, const Methods &methods);
	bool serializeEnums (QJsonObject &target, const Enums &enums);
	bool serializeFields (QJsonObject &target, const Variables &fields);
	
	Definitions *m_definitions;
	
};

#endif // JSONGENERATOR_HPP
