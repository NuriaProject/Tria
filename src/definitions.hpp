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

#include "defs.hpp"

#include <functional>

#include <QMap>
#include <QSet>

class QIODevice;
typedef QSet< QString > StringSet;

class Definitions {
public:
	Definitions (const QString &fileName);
	
	/** Adds \a theClass. */
	void addClassDefinition (const ClassDef &theClass);
	
	/** Returns all class definitions */
	QVector< ClassDef > classDefintions () const;
	
	/** Registers \a type as already Q_DECLARE_METATYPE'd. */
	void addDeclaredType (const QString &type);
	
	/** Tells the generator to generate a metatype declaration. */
	void declareType (const QString &type);
	
	/** Takes \a type out of the to-be-declared list. */
	void undeclareType (const QString &type);
	
	/** Will avoid \a type when generating code. */
	void avoidType (const QString &type);
	
	/** Generates code and writes it to \a device. */
	bool generate (QIODevice *device);
	
private:
	
	/** Registers \a string and returns its offset. */
	int registerString (const QString &string);
	QByteArray stringAcessor (const QString &string);
	QByteArray toByteArray (const QString &string);
	
	void writeHeader (QIODevice *device);
	void writeStringBuffer (QIODevice *device);
	
	void writeRegisterMetatypeForClass (const ClassDef &def, QIODevice *device);
	void writeRegisterMetatype (const QString &type, QIODevice *device);
	void writeDeclareMetatypeForClass (const ClassDef &def, QIODevice *device);
	void writeDeclareMetatype (const QString &type, QIODevice *device);
	void writeMemberConverters (const ClassDef &def, QIODevice *device);
	void writeMemberConverter (const ConversionDef &def, QIODevice *device);
	void writeStaticConverter (const ConversionDef &def, QIODevice *device);
	
	void writeInstantiorClass (QIODevice *device);
	void writeConversionRegisterers (const ClassDef &def, QIODevice *device);
	
	void writeClassDef (ClassDef &def, QIODevice *device);
	void writeDestroyMethod (const ClassDef &def, QIODevice *device);
	void writeBasesMethod (const ClassDef &def, QIODevice *device);
	void writeCountMethods (const ClassDef &def, QIODevice *device);
	void writeAnnotationMethods (const ClassDef &def, QIODevice *device);
	void writeMethodMethods (const ClassDef &def, QIODevice *device);
	void writeFieldMethods (const ClassDef &def, QIODevice *device);
	void writeEnumMethods (const ClassDef &def, QIODevice *device);
	void writeGateCallMethod (const ClassDef &def, QIODevice *device);
	
	void writeMethodGeneric (const Methods &methods, QIODevice *device, const QByteArray &signature,
				 const QByteArray &defaultResult,
				 std::function< QByteArray(const MethodDef &) > fun
				 , const QString &prologue = QString());
	void writeFieldGeneric (const Variables &variables, QIODevice *device, const QByteArray &signature,
				 const QByteArray &defaultResult,
				 std::function< QByteArray(const VariableDef &) > func);
	void writeEnumGeneric (const Enums &enums, QIODevice *device, const QByteArray &signature,
				const QByteArray &defaultResult,
				std::function< QByteArray(const EnumDef &) > func);
	
	QByteArray generateMethodArgumentTester (const ClassDef &def, const MethodDef &m);
	QByteArray methodToCallback (const ClassDef &def, const MethodDef &m, bool safe);
	QByteArray generateGetter (const ClassDef &def, const VariableDef &var);
	QByteArray generateSetter (const ClassDef &def, const VariableDef &var);
	void cleanUpClassDef (ClassDef &def);
	
	// 
	QString m_fileName;
	StringSet m_declaredTypes;
	StringSet m_declareTypes;
	StringSet m_avoidedTypes;
	QVector< ClassDef > m_classes;
	
	// 
	QByteArray m_stringBuffer;
	int m_stringPos;
	QMap< QString, int > m_stringPositions;
	
};

#endif // DEFINITIONS_HPP
