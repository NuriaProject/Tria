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

#include "generator.hpp"

#include <algorithm>

#include <QDateTime>
#include <QIODevice>
#include <QBuffer>
#include <QRegExp>

#include <clang/Tooling/Tooling.h>
#include "definitions.hpp"

Generator::Generator (const QString &fileName)
	: m_fileName (fileName), m_stringPos (0)
{
	
}

void Generator::addClassDefinition (const ClassDef &theClass) {
	this->m_classes.append (theClass);
}

static QByteArray escapeName (QString name) {
	return name.replace (QRegExp ("[^A-Za-z0-9]"), "_").toLatin1 ();
	
}

static QByteArray identPrefix (QString name) {
	return QByteArrayLiteral ("tria_") + escapeName (name) + QByteArrayLiteral ("_");
}

QByteArray Generator::toByteArray (const QString &string) {
	return QByteArrayLiteral ("QByteArrayLiteral (\"") + string.toUtf8 () + QByteArrayLiteral ("\")");
	
//	return QByteArrayLiteral ("QByteArray::fromRawData (") + stringAcessor (string) +
//			QByteArrayLiteral (", ") + QByteArray::number (string.length ()) +
//			QByteArrayLiteral (")");
}

bool Generator::generate (QIODevice *device) {
	
	// Write prologue. Nothing really dynamic here
	writeHeader (device);
	
	// #include the processed file itself and go into our namespace
	device->write ("#include <nuria/metaobject.hpp>\n"
		       "#include <nuria/variant.hpp>\n"
		       "#include <QByteArray>\n"
		       "#include <QMetaType>\n"
		       "#include <QVector>\n"
		       "#include \"");
	device->write (this->m_fileName.toUtf8 ());
	device->write ("\"\n\n");
	
	// Q_DECLARE_METATYPE
	for (const ClassDef &def : this->m_classes) {
		writeDeclareMetatypeForClass (def, device);
	}
	
	device->write ("\n"
		       "namespace TriaObjectData {\n\n");
	
	// Write common stuff
	device->write ("enum Categories {\n"
		       "  ObjectCategory = 0,\n"
		       "  MethodCategory = 1,\n"
		       "  FieldCategory = 2,\n"
		       "  EnumCategory = 3\n"
		       "};\n\n");
	
	// Generate class definitions. Write into a buffer.
	QBuffer classDefBuffer;
	classDefBuffer.open (QIODevice::ReadWrite);
	
	// 
	for (ClassDef &def : this->m_classes) {
		writeClassDef (def, &classDefBuffer);
	}
	
	// Write string buffer
	writeStringBuffer (device);
	
	// Write buffer
	device->write (classDefBuffer.data ());
	
	// Generate helper class
	writeInstantiorClass (device);
	
	// Close namespace, done
	device->write ("}\n");
	
	return true;
}

int Generator::registerString (const QString &string) {
	if (this->m_stringPositions.contains (string)) {
		return this->m_stringPositions.value (string);
	}
	
	// Push string on the buffer
	int offset = this->m_stringPos;
	this->m_stringPos += string.length () + 1;
	
	this->m_stringBuffer.append (string.toUtf8 ());
	this->m_stringBuffer.append ("\\0");
	
	// Cache and return.
	this->m_stringPositions.insert (string, offset);
	return offset;
}

QByteArray Generator::stringAcessor (const QString &string) {
	int offset = registerString (string);
	
	QString access = QStringLiteral ("&stringBuffer[") +
			 QString::number (offset) +
			 QStringLiteral ("]");
	return access.toLatin1 ();
}

void Generator::writeHeader (QIODevice *device) {
	device->write ("/*******************************************************************************\n"
		       " * Meta-code generated by Tria\n"
		       " * Source file: ");
	device->write (this->m_fileName.toUtf8 ());
	device->write ("\n"
		       " * Date: ");
	device->write (QDateTime::currentDateTime ().toString (Qt::ISODate).toLatin1 ());
	
	device->write ("\n"
		       " * LLVM version: " QT_STRINGIFY(LLVM_VERSION_MAJOR) "." QT_STRINGIFY(LLVM_VERSION_MINOR) "\n"
		       " *\n"
		       " * W A R N I N G!\n"
		       " * This code is auto-generated. All changes you make WILL BE LOST!\n"
		       "*******************************************************************************/\n\n");
	
}

void Generator::writeStringBuffer (QIODevice *device) {
	static const int lineLength = 80;
	
	device->write ("static const char *stringBuffer = ");
	
	// 
	int length = this->m_stringBuffer.length ();
	for (int i = 0; i < length; i += lineLength) {
		device->write ("\n\"");
		device->write (this->m_stringBuffer.mid (i, lineLength));
		device->write ("\"");
	}
	
	//
	if (this->m_stringBuffer.isEmpty ()) {
		device->write ("\"\"");
	}
	
	device->write (";\n\n");
	
}

void Generator::writeRegisterMetatypeForClass (const ClassDef &def, QIODevice *device) {
	device->write ("    qRegisterMetaType< ");
	device->write (def.name.toLatin1 ());
	device->write ("* > ();\n");
	
	if (def.hasValueSemantics) {
		device->write ("    qRegisterMetaType< ");
		device->write (def.name.toLatin1 ());
		device->write (" > ();\n");
	}
	
}

void Generator::writeDeclareMetatypeForClass (const ClassDef &def, QIODevice *device) {
	device->write ("Q_DECLARE_METATYPE(");
	device->write (def.name.toLatin1 ());
	device->write ("*)\n");
	
	if (def.hasValueSemantics) {
		device->write ("Q_DECLARE_METATYPE(");
		device->write (def.name.toLatin1 ());
		device->write (")\n");
	}
	
}

void Generator::writeMemberConverters (const ClassDef &def, QIODevice *device) {
	for (const ConversionDef &cur : def.conversions) {
		if (cur.type != MemberMethod) {
			continue;
		}
		
		QByteArray from = escapeName (cur.fromType);
		QByteArray to = escapeName (cur.toType);
		
		device->write ("static ");
		device->write (cur.toType.toLatin1 ());
		device->write (" *tria_convert_");
		device->write (from);
		device->write ("_to_");
		device->write (to);
		device->write (" (const ");
		device->write (from);
		device->write (" &value) {\n"
			       "    return new ");
		device->write (to);
		device->write (" (value.");
		device->write (cur.methodName.toLatin1 ());
		device->write (" ());\n"
			       "}\n\n");
		
	}
	
}

void Generator::writeInstantiorClass (QIODevice *device) {
	QByteArray prefix = identPrefix (this->m_fileName);
	
	// Open class
	device->write ("struct Q_DECL_HIDDEN ");
	device->write (prefix);
	device->write ("Register {\n"
		       "  ");
	device->write (prefix);
	device->write ("Register () {\n");
	
	// Register all known classes
	for (const ClassDef &cur : this->m_classes) {
		device->write ("    // Register class ");
		device->write (cur.name.toLatin1 ());
		device->write ("\n"
			       "    Nuria::MetaObject::registerMetaObject (new ");
		device->write (prefix);
		device->write (escapeName (cur.name));
		device->write ("_metaObject);\n");
		writeConversionRegisterers (cur, device);
		writeRegisterMetatypeForClass (cur, device);
		device->write ("\n");
	}
	
	// End class
	device->write ("  }\n"
		       "};\n\n");
	
	// Create global(!) instance
	device->write (prefix);
	device->write ("Register ");
	device->write (prefix);
	device->write ("instantior;\n\n");
	
}

void Generator::writeConversionRegisterers (const ClassDef &def, QIODevice *device) {
	for (const ConversionDef &cur : def.conversions) {
		device->write ("    Nuria::Variant::registerConversion");
		
		if (cur.type == ConstructorMethod) { // Constructor
			device->write ("< ");
			device->write (cur.fromType.toLatin1 ());
			device->write (", ");
			device->write (cur.toType.toLatin1 ());
			device->write (" > ();\n");
		} else if (cur.type == StaticMethod) { // from*
			device->write (" (&");
			device->write (def.name.toLatin1 ());
			device->write ("::");
			device->write (cur.methodName.toLatin1 ());
			device->write (");\n");
		} else if (cur.type == MemberMethod) { // to*
			device->write (" (&tria_convert_");
			device->write (escapeName (cur.fromType));
			device->write ("_to_");
			device->write (escapeName (cur.toType));
			device->write (");\n");
		}
		
	}
	
}

template< typename T >
static bool sortByName (const T &lhs, const T &rhs) {
	return lhs.name < rhs.name;
}

template< typename Container, typename T >
static void sortAnnotations (Container &c) {
	for (T &t : c) {
		std::sort (t.annotations.begin (), t.annotations.end (), &sortByName< AnnotationDef >);
	}
	
}

void Generator::writeClassDef (ClassDef &def, QIODevice *device) {
	QByteArray prefix = identPrefix (this->m_fileName);
	
	// Sort methods, fields and enums for faster access
	std::sort (def.annotations.begin (), def.annotations.end (), &sortByName< AnnotationDef >);
	std::sort (def.bases.begin (), def.bases.end (), &sortByName< BaseDef >);
	std::sort (def.methods.begin (), def.methods.end (), &sortByName< MethodDef >);
	std::sort (def.variables.begin (), def.variables.end (), &sortByName< VariableDef >);
	std::sort (def.enums.begin (), def.enums.end (), &sortByName< EnumDef >);
	
	// Sort annotations in everything ..
	sortAnnotations< Methods, MethodDef > (def.methods);
	sortAnnotations< Variables, VariableDef > (def.variables);
	
	for (EnumDef &cur : def.enums) {
		std::sort (cur.values.begin (), cur.values.end ());
		std::sort (cur.annotations.begin (), cur.annotations.end (), &sortByName< AnnotationDef >);
	}
	
	// Converters
	writeMemberConverters (def, device);
	
	// Prologue
	device->write ("class Q_DECL_HIDDEN ");
	device->write (prefix);
	device->write (escapeName (def.name.toLatin1 ()));
	device->write ("_metaObject : public Nuria::MetaObject {\n"
		       "public:\n");
	
	// Generate MetaObject::className
	device->write ("  QByteArray _className () const {\n"
		       "    return ");
	device->write (toByteArray (def.name));
	device->write (";\n"
		       "  }\n\n");
	
	// Generate various methods
	writeDestroyMethod (def, device);
	writeBasesMethod (def, device);
	writeCountMethods (def, device);
	writeAnnotationMethods (def, device);
	writeMethodMethods (def, device);
	writeFieldMethods (def, device);
	writeEnumMethods (def, device);
	writeGateCallMethod (def, device);
	
	// Done.
	device->write ("};\n\n");
	
	
}

void Generator::writeDestroyMethod (const ClassDef &def, QIODevice *device) {
	device->write ("  void _destroy (void *instance) {\n"
		       "    delete reinterpret_cast< ");
	device->write (def.name.toLatin1 ());
	device->write (" * > (instance);\n"
		       "  }\n\n");
	
}

void Generator::writeBasesMethod (const ClassDef &def, QIODevice *device) {
	device->write ("  QVector< QByteArray > _baseClasses () {\n"
		       "    return QVector< QByteArray > ");
	
	int count = def.bases.length ();
	if (count > 0) {
		device->write ("{ ");
		for (int i = 0; i < count; i++) {
			device->write (toByteArray (def.bases.at (i).name));
			if (i + 1 < count) {
				device->write (", ");
			}
			
		}
		
		device->write (" };\n");
	} else {
		device->write ("();\n");
	}
	
	device->write ("  }\n\n");
}

template< typename Container, typename T >
static void writeGenericSwitch (const Container &container, QIODevice *device, const QByteArray &variable,
				  std::function< QByteArray(const T &) > func) {
	if (!container.isEmpty ()) {
		device->write ("    switch (");
		device->write (variable);
		device->write (") {\n");
		
		for (int i = 0; i < container.length (); i++) {
			const T &cur = container.at (i);
			
			device->write ("    case ");
			device->write (QByteArray::number (i));
			device->write (": ");
			device->write (func (cur));
			device->write (";\n");
			
		}
		
		device->write ("    }\n");
	}
	
}

template< typename Container, typename T >
static void writeGenericFunction (const Container &container, QIODevice *device, const QByteArray &signature,
				  const QByteArray &defaultResult, const QByteArray &variable,
				  std::function< QByteArray(const T &) > func) {
	
	device->write ("  ");
	device->write (signature);
	device->write (" const {\n");
	
	writeGenericSwitch (container, device, variable, func);
	
	device->write ("    return ");
	device->write (defaultResult);
	device->write (";\n"
		       "  }\n\n");
	
}

void Generator::writeCountMethods (const ClassDef &def, QIODevice *device) {
	
	std::function< QByteArray(const MethodDef &) > annotationsMethod = [this](const MethodDef &def) {
		return QByteArrayLiteral ("return ") + QByteArray::number (def.annotations.length ());
	};
	
	std::function< QByteArray(const VariableDef &) > annotationsFields = [this](const VariableDef &def) {
		return QByteArrayLiteral ("return ") + QByteArray::number (def.annotations.length ());
	};
	
	std::function< QByteArray(const EnumDef &) > annotationsEnums = [this](const EnumDef &def) {
		return QByteArrayLiteral ("return ") + QByteArray::number (def.annotations.length ());
	};
	
	device->write ("  int _annotationCount (int category, int index) const {\n");
	device->write ("    switch (category) {\n");
	device->write ("    case ObjectCategory: return ");
	device->write (QByteArray::number (def.annotations.length ()));
	device->write (";\n");
	device->write ("    case MethodCategory:\n");
	writeGenericSwitch (def.methods, device, "index", annotationsMethod);
	device->write ("    break;\n"
		       "    case FieldCategory:\n");
	writeGenericSwitch (def.variables, device, "index", annotationsFields);
	device->write ("    break;\n"
		       "    case EnumCategory:\n");
	writeGenericSwitch (def.enums, device, "index", annotationsEnums);
	device->write ("    break;\n"
		       "    }\n"
		       "  return 0;\n"
		       "  }\n\n");
	
	// 
	device->write ("  int _methodCount () const {\n");
	device->write ("    return ");
	device->write (QByteArray::number (def.methods.length ()));
	device->write (";\n  }\n\n");
	
	// 
	device->write ("  int _fieldCount () const {\n");
	device->write ("    return ");
	device->write (QByteArray::number (def.variables.length ()));
	device->write (";\n  }\n\n");
	
	// 
	device->write ("  int _enumCount () const {\n");
	device->write ("    return ");
	device->write (QByteArray::number (def.enums.length ()));
	device->write (";\n  }\n\n");
	
}

static QByteArray writeAnnotationValue (const AnnotationDef &def) {
	if (def.value.isEmpty ()) {
		return QByteArrayLiteral ("return QVariant ()");
	} else if (def.valueIsString) {
		return QByteArrayLiteral ("return QStringLiteral (\"") + 
				def.value.toUtf8 () + QByteArrayLiteral ("\")");
	}
	
	return QByteArrayLiteral ("return QVariant::fromValue (") + def.value.toUtf8 () + QByteArrayLiteral (")");
	
}

template< typename Container, typename T >
static QByteArray annotationHelper (const Container &container, const QByteArray &variable,
				    std::function< QByteArray(const T &) > func) {
	QBuffer buffer;
	buffer.open (QIODevice::WriteOnly);
	writeGenericSwitch (container, &buffer, variable, func);
	
	if (buffer.buffer ().isEmpty ()) {
		return QByteArrayLiteral ("break");
	}
	
	return QByteArrayLiteral ("\n") + buffer.buffer () + QByteArrayLiteral ("    break");
}

template< typename T, typename Sub >
static std::function< QByteArray(const T &) > iterateAnnotationsFunc (Sub func) {
	return [func](const T &def) {
		return annotationHelper (def.annotations, "nth", func);
        };
}

void Generator::writeAnnotationMethods (const ClassDef &def, QIODevice *device) {
	// Nuria::MetaObject::annotationName
	std::function< QByteArray(const AnnotationDef &) > annotationName = [this](const AnnotationDef &a) {
                return QByteArrayLiteral ("return ") + toByteArray (a.name);
        };
	
	device->write ("  QByteArray _annotationName (int category, int index, int nth) const {\n"
		       "    switch (category) {\n"
		       "    case ObjectCategory:\n");
	writeGenericSwitch (def.annotations, device, "nth", annotationName);
	device->write ("    break;\n"
		       "    case MethodCategory:\n");
	writeGenericSwitch (def.methods, device, "index", iterateAnnotationsFunc< MethodDef > (annotationName));
	device->write ("    break;\n"
		       "    case FieldCategory:\n");
	writeGenericSwitch (def.variables, device, "index", iterateAnnotationsFunc< VariableDef > (annotationName));
	device->write ("    break;\n"
		       "    case EnumCategory:\n");
	writeGenericSwitch (def.enums, device, "index", iterateAnnotationsFunc< EnumDef > (annotationName));
	device->write ("    break;\n"
		       "    }\n\n"
		       "    return QByteArray ();\n"
		       "  }\n\n");
	
	// Nuria::MetaObject::annotationValue
	std::function< QByteArray(const AnnotationDef &) > valueFunc = writeAnnotationValue;
	device->write ("  QVariant _annotationValue (int category, int index, int nth) const {\n"
		       "    switch (category) {\n"
		       "    case ObjectCategory:\n");
	writeGenericSwitch (def.annotations, device, "nth", valueFunc);
	device->write ("    break;\n"
		       "    case MethodCategory:\n");
	writeGenericSwitch (def.methods, device, "index", iterateAnnotationsFunc< MethodDef > (valueFunc));
	device->write ("    break;\n"
		       "    case FieldCategory:\n");
	writeGenericSwitch (def.variables, device, "index", iterateAnnotationsFunc< VariableDef > (valueFunc));
	device->write ("    break;\n"
		       "    case EnumCategory:\n");
	writeGenericSwitch (def.enums, device, "index", iterateAnnotationsFunc< EnumDef > (valueFunc));
	device->write ("    break;\n"
		       "    }\n\n"
		       "    return QVariant ();\n"
		       "  }\n\n");
	
}

void Generator::writeMethodMethods (const ClassDef &def, QIODevice *device) {
	
	// QByteArray methodName (int index) const
	std::function< QByteArray(const MethodDef &) > methodName = [this](const MethodDef &def) {
		return toByteArray (def.name);
	};
	
	writeMethodGeneric (def.methods, device, "QByteArray _methodName (int index)", 
			    "QByteArray ()", methodName);
	
	// MetaMethod::Type methodType (int index) const
	std::function< QByteArray(const MethodDef &) > methodType = [this](const MethodDef &def) {
		switch (def.type) {
		case ConstructorMethod: return QByteArrayLiteral ("Nuria::MetaMethod::Constructor");
		case DestructorMethod: return QByteArray ();
		case MemberMethod: return QByteArrayLiteral ("Nuria::MetaMethod::Method");
		case StaticMethod: return QByteArrayLiteral ("Nuria::MetaMethod::Static");
		}
	};
	
	writeMethodGeneric (def.methods, device, "Nuria::MetaMethod::Type _methodType (int index)", 
			    "Nuria::MetaMethod::Method", methodType);
	
	
	// QByteArray methodReturnType (int index) const
	std::function< QByteArray(const MethodDef &) > returnType = [this](const MethodDef &def) {
		return toByteArray (def.returnType);
	};
	
	writeMethodGeneric (def.methods, device, "QByteArray _methodReturnType (int index)", 
			    "QByteArray ()", returnType);
	
	// QVector< QByteArray > methodArgumentNames (int index) const
	std::function< QByteArray(const MethodDef &) > argumentNames = [this](const MethodDef &def) {
		QByteArray arr = QByteArrayLiteral ("QVector< QByteArray > ");
		
		int count = def.arguments.length ();
		if (count > 0) {
			arr.append ("{ ");
			
			for (int i = 0; i < count; i++) {
				arr.append (toByteArray (def.arguments.at (i).name));
				
				if (i + 1 < count) {
					arr.append (", ");
				}
				
			}
		
			arr.append (" }");
		} else {
			arr.append ("()");
		}
		
		return arr;
	};
	
	writeMethodGeneric (def.methods, device, "QVector< QByteArray > _methodArgumentNames (int index)", 
			    "QVector< QByteArray > ()", argumentNames);
	
	// QVector< QByteArray > methodArgumentTypes (int index) const
	std::function< QByteArray(const MethodDef &) > argumentTypes = [this](const MethodDef &def) {
		QByteArray arr = QByteArrayLiteral ("QVector< QByteArray > ");
		
		int count = def.arguments.length ();
		if (count > 0) {
			arr.append ("{ ");
			
			for (int i = 0; i < count; i++) {
				arr.append (toByteArray (def.arguments.at (i).type));
				
				if (i + 1 < count) {
					arr.append (", ");
				}
				
			}
		
			arr.append (" }");
		} else {
			arr.append ("()");
		}
		
		return arr;
	};
	
	writeMethodGeneric (def.methods, device, "QVector< QByteArray > _methodArgumentTypes (int index)", 
			    "QVector< QByteArray > ()", argumentTypes);
	
	// Callback methodCallback (void *instance, int index) const
	std::function< QByteArray(const MethodDef &) > argumentCallback = [&def, this](const MethodDef &m) {
		return methodToCallback (def, m);
	};
	
	writeMethodGeneric (def.methods, device, "Nuria::Callback _methodCallback (void *instance, int index)", 
			    "Nuria::Callback ()", argumentCallback);
	
}

void Generator::writeFieldMethods (const ClassDef &def, QIODevice *device) {
	
	// QByteArray fieldName (int index) const
	std::function< QByteArray(const VariableDef &) > fieldName = [this](const VariableDef &var) {
		return toByteArray (var.name);
	};
	
	writeFieldGeneric (def.variables, device, "QByteArray _fieldName (int index)", "QByteArray ()", fieldName);
	
	// QByteArray fieldType (int index) const
	std::function< QByteArray(const VariableDef &) > fieldType = [this](const VariableDef &var) {
		return toByteArray (var.type);
	};
	
	writeFieldGeneric (def.variables, device, "QByteArray _fieldType (int index)", "QByteArray ()", fieldType);
	
	// QVariant fieldRead (int index, void *instance) const
	std::function< QByteArray(const VariableDef &) > fieldRead = [&def, this](const VariableDef &var) {
		QByteArray code = QByteArrayLiteral ("QVariant::fromValue (reinterpret_cast< ") +
				  def.name.toLatin1 () + QByteArrayLiteral (" * > (instance)->") ;
		if (!var.getter.isEmpty ()) {
			code.append (var.getter.toLatin1 ());
			code.append (" ()");
		} else {
			code.append (var.name.toLatin1 ());
		}
		
		code.append (")");
		return code;
	};
	
	writeFieldGeneric (def.variables, device, "QVariant _fieldRead (int index, void *instance)",
			   "QVariant ()", fieldRead);
	
	// bool fieldWrite (int index, void *instance, const QVariant &value) const
	std::function< QByteArray(const VariableDef &) > fieldWrite = [&def, this](const VariableDef &var) {
		return generateSetter (def, var);
	};
	
	writeGenericFunction (def.variables, device,
		      "bool _fieldWrite (int index, void *instance, const QVariant &value)",
		      "false", "index", fieldWrite);
	
}

void Generator::writeEnumMethods (const ClassDef &def, QIODevice *device) {
	
	// QByteArray enumName (int index) const
	std::function< QByteArray(const EnumDef &) > enumName = [this](const EnumDef &e) {
		return toByteArray (e.name);
	};
	
	writeGenericFunction (def.enums, device, "QByteArray _enumName (int index)", "QByteArray ()", "index", enumName);
	
	// int enumElementCount (int index) const
	std::function< QByteArray(const EnumDef &) > enumCount = [this](const EnumDef &e) {
		return QByteArray::number (e.values.length ());
	};
	
	writeGenericFunction (def.enums, device, "int _enumElementCount (int index)", "-1", "index", enumCount);
	
	// QByteArray enumElementKey (int index, int at) const
	std::function< QByteArray(const EnumDef &) > enumKey = [def, this](const EnumDef &e) {
		QByteArray code;
		code.append ("\n"
			     "      switch (at) {\n");
		for (int i = 0; i < e.values.length (); i++) {
			code.append ("      case ");
			code.append (QByteArray::number (i));
			code.append (": return ");
			code.append (toByteArray (e.values.at (i)));
			code.append (";\n");
		}
		
		code.append ("      } break;");
		return code;
	};
	
	writeGenericFunction (def.enums, device, "QByteArray _enumElementKey (int index, int at)",
			    "QByteArray ()", "index", enumKey);
	
	// int enumElementValue (int index, int at) const
	std::function< QByteArray(const EnumDef &) > enumValue = [def, this](const EnumDef &e) {
		QByteArray code;
		code.append ("\n"
			     "      switch (at) {\n");
		for (int i = 0; i < e.values.length (); i++) {
			QByteArray qualified = def.name.toLatin1 () + QByteArrayLiteral ("::") +
					       e.values.at (i).toLatin1 ();
			code.append ("      case ");
			code.append (QByteArray::number (i));
			code.append (": return ");
			code.append (qualified);
			code.append (";\n");
		}
		
		code.append ("      } break;");
		return code;
	};
	
	writeGenericFunction (def.enums, device, "int _enumElementValue (int index, int at)",
			    "-1", "index", enumValue);
	
}

void Generator::writeGateCallMethod (const ClassDef &def, QIODevice *device) {
	Q_UNUSED(def);
	
	device->write ("#define RESULT(Type) *reinterpret_cast< Type * > (result)\n"
		       "  void gateCall (GateMethod method, int category, int index, int nth, \n"
		       "                 void *result, void *additional) override {\n"
		       "    switch (method) {\n"
		       "    case Nuria::MetaObject::GateMethod::ClassName:\n"
		       "      RESULT(QByteArray) = _className (); break;\n"
		       "    case Nuria::MetaObject::GateMethod::BaseClasses:\n"
		       "      RESULT(QVector< QByteArray >) = _baseClasses (); break;\n"
		       "    case Nuria::MetaObject::GateMethod::AnnotationCount:\n"
		       "      RESULT(int) = _annotationCount (category, index); break;\n"
		       "    case Nuria::MetaObject::GateMethod::MethodCount:\n"
		       "      RESULT(int) = _methodCount (); break;\n"
		       "    case Nuria::MetaObject::GateMethod::FieldCount:\n"
		       "      RESULT(int) = _fieldCount (); break;\n"
		       "    case Nuria::MetaObject::GateMethod::EnumCount:\n"
		       "      RESULT(int) = _enumCount (); break;\n"
		       "    case Nuria::MetaObject::GateMethod::AnnotationName:\n"
		       "      RESULT(QByteArray) = _annotationName (category,  index, nth); break;\n"
		       "    case Nuria::MetaObject::GateMethod::AnnotationValue:\n"
		       "      RESULT(QVariant) = _annotationValue (category,  index, nth); break;\n"
		       "    case Nuria::MetaObject::GateMethod::MethodName:\n"
		       "      RESULT(QByteArray) = _methodName (index); break;\n"
		       "    case Nuria::MetaObject::GateMethod::MethodType:\n"
		       "      RESULT(Nuria::MetaMethod::Type) = _methodType (index); break;\n"
		       "    case Nuria::MetaObject::GateMethod::MethodReturnType:\n"
		       "      RESULT(QByteArray) = _methodReturnType (index); break;\n"
		       "    case Nuria::MetaObject::GateMethod::MethodArgumentNames:\n"
		       "      RESULT(QVector< QByteArray >) = _methodArgumentNames (index); break;\n"
		       "    case Nuria::MetaObject::GateMethod::MethodArgumentTypes:\n"
		       "      RESULT(QVector< QByteArray >) = _methodArgumentTypes (index); break;\n"
		       "    case Nuria::MetaObject::GateMethod::MethodCallback:\n"
		       "      RESULT(Nuria::Callback) = _methodCallback (additional, index); break;\n"
		       "    case Nuria::MetaObject::GateMethod::FieldName:\n"
		       "      RESULT(QByteArray) = _fieldName (index); break;\n"
		       "    case Nuria::MetaObject::GateMethod::FieldType:\n"
		       "      RESULT(QByteArray) = _fieldType (index); break;\n"
		       "    case Nuria::MetaObject::GateMethod::FieldRead:\n"
		       "      RESULT(QVariant) = _fieldRead (index, additional); break;\n"
		       "    case Nuria::MetaObject::GateMethod::FieldWrite: {\n"
		       "      void **argData = reinterpret_cast< void ** > (additional);\n"
		       "      const QVariant &value = *reinterpret_cast< QVariant * > (argData[1]);\n"
		       "      RESULT(bool) = _fieldWrite (index, argData[0], value);\n"
		       "    } break;\n"
		       "    case Nuria::MetaObject::GateMethod::EnumName:\n"
		       "      RESULT(QByteArray) = _enumName (index); break;\n"
		       "    case Nuria::MetaObject::GateMethod::EnumElementCount:\n"
		       "      RESULT(int) = _enumElementCount (index); break;\n"
		       "    case Nuria::MetaObject::GateMethod::EnumElementKey:\n"
		       "      RESULT(QByteArray) = _enumElementKey (index, nth); break;\n"
		       "    case Nuria::MetaObject::GateMethod::EnumElementValue:\n"
		       "      RESULT(int) = _enumElementValue (index, nth); break;\n"
		       "    case Nuria::MetaObject::GateMethod::DestroyInstance:\n"
		       "      _destroy (additional); break;\n"
		       "    }\n"
		       "  }\n");
	
}

void Generator::writeMethodGeneric (const Methods &methods, QIODevice *device, const QByteArray &signature,
				    const QByteArray &defaultResult,
				    std::function< QByteArray(const MethodDef &) > func) {
	std::function< QByteArray(const MethodDef &) > returner = [&func] (const MethodDef &m) {
		return QByteArrayLiteral ("return ") + func (m);
	};
	
	writeGenericFunction (methods, device, signature, defaultResult, "index", returner);
	
}

void Generator::writeFieldGeneric (const Variables &variables, QIODevice *device, const QByteArray &signature,
				   const QByteArray &defaultResult,
				   std::function< QByteArray (const VariableDef &) > func) {
	std::function< QByteArray(const VariableDef &) > returner = [&func] (const VariableDef &v) {
		return QByteArrayLiteral ("return ") + func (v);
	};
	
	writeGenericFunction (variables, device, signature, defaultResult, "index", returner);
	
}


void Generator::writeEnumGeneric (const Enums &enums, QIODevice *device, const QByteArray &signature,
				  const QByteArray &defaultResult,
				  std::function< QByteArray (const EnumDef &) > func) {
	std::function< QByteArray(const EnumDef &) > returner = [&func] (const EnumDef &v) {
	        return QByteArrayLiteral ("return ") + func (v);
        };
        
        writeGenericFunction (enums, device, signature, defaultResult, "index", returner);
	
}

QByteArray Generator::methodToCallback (const ClassDef &def, const MethodDef &m) {
	if (m.type == StaticMethod) {
		return QByteArrayLiteral ("Nuria::Callback (&") + def.name.toLatin1 () +
				QByteArrayLiteral ("::") + m.name.toLatin1 () + QByteArrayLiteral (")");
	} else if (m.type == ConstructorMethod) {
		QByteArray args;
		QByteArray call;
		
		for (const VariableDef &def : m.arguments) {
			args.append (def.type);
			args.append (" ");
			args.append (def.name);
			args.append (", ");
			call.append (def.name);
			call.append (", ");
		}
		
		args.chop (2); // Remove trailing ", "
		call.chop (2);
		
		QByteArray cb = QByteArrayLiteral ("Nuria::Callback::fromLambda ([](") + args +
				QByteArrayLiteral (") { return new ") + def.name.toLatin1 ();
		
		if (!args.isEmpty ()) {
			cb.append ("(");
			cb.append (call);
			cb.append (")");
		}
		
		cb.append ("; })");
		return cb;
	}
	
	return QByteArrayLiteral ("Nuria::Callback (reinterpret_cast< ") +  def.name.toLatin1 () + 
			QByteArrayLiteral (" * > (instance), &") + def.name.toLatin1 () +
			QByteArrayLiteral ("::") + m.name.toLatin1 () + QByteArrayLiteral (")");
}

QByteArray Generator::generateSetter (const ClassDef &def, const VariableDef &var) {
	QByteArray setter;
	QByteArray toValue;
	
	toValue.append ("value.value< ");
	toValue.append (var.type.toLatin1 ());
	toValue.append (" > ()");
	
	setter.append ("reinterpret_cast< ");
	setter.append (def.name.toLatin1 ());
	setter.append (" * > (instance)->");
	
	if (!var.setter.isEmpty ()) {
		setter.append (var.setter.toLatin1 ());
		setter.append (" (");
		setter.append (toValue);
		setter.append (");");
	} else {
		setter.append (var.name.toLatin1 ());
		setter.append (" = ");
		setter.append (toValue);
		setter.append (";");
	}
	
	QByteArray code;
	code.append ("\n"
		     "      if (value.userType () != qMetaTypeId< ");
	code.append (var.type.toLatin1 ());
	code.append (" > ()) {\n"
		     "        QVariant v = Nuria::Variant::convert (value, qMetaTypeId< ");
	code.append (var.type.toLatin1 ());
	code.append (" > ());\n"
		     "        if (!v.isValid ()) { return false; }\n"
		     "        ");
	code.append (QByteArray (setter).replace ("value.", "v."));
	code.append ("\n"
		     "        return true;\n"
		     "      }\n"
		     "      ");
	code.append (setter);
	code.append ("\n"
		     "      return true");
	
	return code;
}
