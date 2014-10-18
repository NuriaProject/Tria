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

#include "luagenerator.hpp"

#include <QDateTime>
#include <memory>
#include <QDebug>
#include <QFile>

#include <clang/Tooling/Tooling.h>
#include "definitions.hpp"
#include <lua.hpp>
#include <cstdio>

#if 0
static void dumpStack (lua_State *lua) {
	int count = lua_gettop (lua);
	
	printf ("Stack dump of state %p\n", lua);
	for (int i = 1; i <= count; i++) {
		int type = lua_type (lua, i);
		lua_pushvalue (lua, i);
		printf (" %2i %3i | %s => '%s'\n", i, i - count - 1,
		        lua_typename (lua, type),
		        lua_tostring(lua, -1));
		lua_pop(lua, 1);
	}
	
	printf ("=======\n");
	fflush (stdout);
}
#endif

LuaGenerator::LuaGenerator (Definitions *definitions)
	: m_definitions (definitions)
{
	
}

static bool parseConfig (const std::string &config, QByteArray &luaFile, QByteArray &outFile) {
	size_t delim = config.find (':');
	if (delim == std::string::npos) {
		qCritical() << "Invalid Lua generator config:" << config.c_str ();
		return false;
	}
	
	// 
	luaFile = QByteArray (config.c_str (), delim);
	outFile = QByteArray (config.c_str () + delim + 1);
	
	// 
	if (luaFile.isEmpty () || outFile.isEmpty ()) {
		qCritical() << "Lua generator, no lua or outfile was given:" << config.c_str ();
		return false;
	}
	
	// 
	return true;
}

bool LuaGenerator::generate (const std::string &config, const std::string &sourceName) {
	QByteArray luaFile;
	QByteArray outFile;
	
	// Read config ("luafile,outfile")
	if (!parseConfig (config, luaFile, outFile)) {
		return false;
	}
	
	// Read lua file
	QFile scriptFile (luaFile);
	if (!scriptFile.open (QIODevice::ReadOnly)) {
		qCritical() << "Lua generator, failed to open script" << luaFile;
		return false;
	}
	
	// Open outfile
	QFile outHandle (outFile);
	if (!outHandle.open (QIODevice::WriteOnly)) {
		qCritical() << "Lua generator, failed to open outfile" << outFile;
		return false;
	}
	
	// 
	QByteArray sourceFile (sourceName.c_str (), sourceName.length ());
	return runScript (sourceFile, outFile, luaFile, scriptFile.readAll (), &outHandle);
}

bool LuaGenerator::runScript (const QByteArray &soucePath, const QByteArray &outPath, const QByteArray &scriptName,
                              const QByteArray &script, QFile *outFile) {
	std::unique_ptr< lua_State, decltype(&lua_close) > lua (lua_open (), &lua_close);
	initState (lua.get (), soucePath, outPath, outFile);
	exportDefinitions (lua.get ());
	
	// Execute script
	int r = luaL_loadstring (lua.get (), script.constData ());
	if (r != 0 || lua_pcall (lua.get (), 0, 0, 0) > 0) {
		qCritical() << "Lua generator, failed to run" << scriptName
		            << "error:" << lua_tostring(lua.get (), 1);
		outFile->remove ();
		return false;
	}
	
	// 
	return true;
}

void LuaGenerator::initState (lua_State *lua, const QByteArray &sourceFile,
                              const QByteArray &outFile, QFile *file) {
	luaL_openlibs (lua);
	
	// 
	addLog (lua);
	addWrite (lua, file);
	addInformation (lua, sourceFile, outFile);
	
}

static inline void insertString (lua_State *lua, const char *name, const QString &string) {
	lua_pushstring (lua, string.toUtf8 ().constData ());
	lua_setfield (lua, -2, name);
}

static inline void insertBool (lua_State *lua, const char *name, bool value) {
	lua_pushboolean (lua, value);
	lua_setfield (lua, -2, name);
}

void LuaGenerator::addInformation (lua_State *lua, const QByteArray &sourceFile,
                                   const QByteArray &outFile) {
	lua_createtable (lua, 0, 6);
	
	lua_pushliteral(lua, __TIME__);
	lua_setfield (lua, -2, "compileTime");
	
	lua_pushliteral(lua, __DATE__);
	lua_setfield (lua, -2, "compileDate");
	
	lua_pushliteral(lua, QT_STRINGIFY(LLVM_VERSION_MAJOR) "." QT_STRINGIFY(LLVM_VERSION_MINOR));
	lua_setfield (lua, -2, "llvmVersion");
	
	lua_pushstring (lua, sourceFile.constData ());
	lua_setfield (lua, -2, "sourceFile");
	
	lua_pushstring (lua, outFile.constData ());
	lua_setfield (lua, -2, "outFile");
	
	insertString (lua, "currentDateTime", QDateTime::currentDateTime ().toString (Qt::ISODate));
	
	lua_setfield (lua, LUA_GLOBALSINDEX, "tria");
}

static const char *logLevelName (int level) {
	switch (level) {
	case QtCriticalMsg: return "error";
	case QtWarningMsg: return "warning";
	default: return "note";
	}
	
}

static void printStackElement (lua_State *lua, int idx) {
	int type = lua_type (lua, idx);
	switch (type) {
	case LUA_TNIL:
		printf ("nil ");
		break;
	case LUA_TBOOLEAN:
		printf (lua_toboolean (lua, idx) ? "true " : "false ");
		break;
	case LUA_TLIGHTUSERDATA:
	case LUA_TUSERDATA:
		printf ("userdata(%p) ", lua_touserdata (lua, idx));
		break;
	case LUA_TFUNCTION:
		printf ("<function> ");
		break;
	case LUA_TTABLE:
		printf ("<table> "); // TODO: Dump table
		break;
	default: {
		const char *str = lua_tolstring (lua, idx, NULL);
		if (str) {
			printf ("%s ", str);
		} else {
			printf ("<instance of %s> ", lua_typename (lua, lua_type (lua, idx)));
		}
	} break;
	}
	
}

static int luaLog (lua_State *lua) {
	int level = lua_tointeger (lua, lua_upvalueindex(1));
	int argc = lua_gettop (lua);
	
	printf ("%s: ", logLevelName (level));
	for (int i = 1; i <= argc; i++) {
		printStackElement (lua, i);
	}
	
	// 
	printf ("\n");
	return 0;
	
}

void LuaGenerator::addLog (lua_State *lua) {
	
	// 
	lua_createtable (lua, 0, 3);
	
	lua_pushinteger (lua, QtCriticalMsg);
	lua_pushcclosure (lua, &luaLog, 1);
	lua_setfield (lua, -2, "error");
	
	lua_pushinteger (lua, QtWarningMsg);
	lua_pushcclosure (lua, &luaLog, 1);
	lua_setfield (lua, -2, "warn");
	
	lua_pushinteger (lua, QtDebugMsg);
	lua_pushcclosure (lua, &luaLog, 1);
	lua_setfield (lua, -2, "note");
	
	// 
	lua_setfield (lua, LUA_GLOBALSINDEX, "log");
}

static int luaWrite (lua_State *lua) {
	QFile *file = (QFile *)lua_topointer (lua, lua_upvalueindex(1));
	if (lua_gettop (lua) != 1 || !lua_isstring (lua, 1)) {
		return luaL_error (lua, "write() expects a single string argument.");
	}
	
	// 
	size_t len = 0;
	const char *str = lua_tolstring (lua, 1, &len);
	file->write (str, len);
	
	// 
	return 0;
	
}

void LuaGenerator::addWrite (lua_State *lua, QFile *file) {
	
	lua_pushlightuserdata (lua, file);
	lua_pushcclosure (lua, luaWrite, 1);
	lua_setfield (lua, LUA_GLOBALSINDEX, "write");
	
}

void LuaGenerator::exportDefinitions (lua_State *lua) {
	
	// _G.definitions
	lua_createtable (lua, 0, 4);
	
	// 
	exportStringSet (lua, "declaredTypes", this->m_definitions->declaredTypes ());
	exportStringSet (lua, "declareTypes", this->m_definitions->declareTypes ());
	exportStringSet (lua, "avoidedTypes", this->m_definitions->avoidedTypes ());
	exportClassDefinitions (lua);
	
	// 
	lua_setfield (lua, LUA_GLOBALSINDEX, "definitions");
	
}

void LuaGenerator::exportStringSet (lua_State *lua, const char *name, const StringSet &set) {
	lua_createtable (lua, set.size (), 0);
	
	int i = 1;
	for (auto it = set.constBegin (), end = set.constEnd (); it != end; ++it, i++) {
		lua_pushstring (lua, it->toLatin1 ().constData ());
		lua_rawseti (lua, -2, i);
	}
	
	// 
	lua_setfield (lua, -2, name);
}

void LuaGenerator::exportClassDefinitions (lua_State *lua) {
	QVector< ClassDef > classes = this->m_definitions->classDefintions ();
	lua_createtable (lua, 0, classes.length ());
	
	// 
	for (int i = 0; i < classes.length (); i++) {
		exportClassDefinition (lua, classes.at (i));
	}
	
	// 
	lua_setfield (lua, -2, "classes");
}

void LuaGenerator::exportClassDefinition (lua_State *lua, const ClassDef &def) {
	lua_pushstring (lua, def.name.toLatin1 ().constData ());
	lua_createtable (lua, 0, 12);
	
	// 
	exportClassDefinitionBase (lua, def);
	exportBases (lua, def.bases);
	exportVariables (lua, "variables", def.variables);
	exportMethods (lua, def.methods);
	exportEnums (lua, def.enums);
	exportAnnotations (lua, def.annotations);
	exportConversions (lua, def.conversions);
	
	// 
	lua_settable (lua, -3);
}

void LuaGenerator::exportClassDefinitionBase (lua_State *lua, const ClassDef &def) {
	lua_pushboolean (lua, def.hasValueSemantics);
	lua_setfield (lua, -2, "hasValueSemantics");
	
	lua_pushboolean (lua, def.hasDefaultCtor);
	lua_setfield (lua, -2, "hasDefaultCtor");
	
	lua_pushboolean (lua, def.hasCopyCtor);
	lua_setfield (lua, -2, "hasCopyCtor");
	
	lua_pushboolean (lua, def.hasAssignmentOperator);
	lua_setfield (lua, -2, "hasAssignmentOperator");
	
	lua_pushboolean (lua, def.implementsCtor);
	lua_setfield (lua, -2, "implementsCtor");
	
	lua_pushboolean (lua, def.implementsCopyCtor);
	lua_setfield (lua, -2, "implementsCopyCtor");
	
}

static void pushAccessSpecifier (lua_State *lua, clang::AccessSpecifier spec) {
	switch (spec) {
	case clang::AS_public:
		lua_pushliteral(lua, "public");
		break;
        case clang::AS_protected:
		lua_pushliteral(lua, "protected");
		break;
        case clang::AS_private:
		lua_pushliteral(lua, "private");
		break;
        case clang::AS_none:
		lua_pushliteral(lua, "none");
		break;
	default:
		lua_pushnil (lua);
	}
	
}

void LuaGenerator::exportBases (lua_State *lua, const Bases &bases) {
	lua_createtable (lua, 0, bases.length ());
	
	for (int i = 0; i < bases.length (); i++) {
		const BaseDef &base = bases.at (i);
		
		lua_pushstring (lua, base.name.toLatin1 ().constData ());
		lua_createtable (lua, 0, 2);
		
		insertBool (lua, "isVirtual", base.isVirtual);
		
		pushAccessSpecifier (lua, base.access);
		lua_setfield (lua, -2, "access");
		
		lua_settable (lua, -3);
	}
	
	// 
	lua_setfield (lua, -2, "bases");
}

static void pushAnnotationType (lua_State *lua, AnnotationType type) {
	switch (type) {
	case IntrospectAnnotation:
		lua_pushliteral(lua, "introspect");
		break;
	case SkipAnnotation:
		lua_pushliteral(lua, "skip");
		break;
	case ReadAnnotation:
		lua_pushliteral(lua, "read");
		break;
	case WriteAnnotation:
		lua_pushliteral(lua, "write");
		break;
	case RequireAnnotation:
		lua_pushliteral(lua, "require");
		break;
	case CustomAnnotation:
		lua_pushliteral(lua, "custom");
		break;
	default:
		lua_pushnil (lua);
	}
	
}

void LuaGenerator::exportAnnotations (lua_State *lua, const Annotations &annotations) {
	lua_createtable (lua, annotations.length (), 0);
	for (int i = 0; i < annotations.length (); i++) {
		const AnnotationDef &cur = annotations.at (i);
		lua_createtable (lua, 0, 4);
		
		insertString (lua, "name", cur.name);
		insertString (lua, "value", cur.value);
		
		pushAnnotationType (lua, cur.type);
		lua_setfield (lua, -2, "type");
		
		lua_pushinteger (lua, cur.valueType);
		lua_setfield (lua, -2, "valueType");
		
		const char *typeName = QMetaType::typeName (cur.valueType);
		lua_pushlstring (lua, typeName ? typeName : "", typeName ? strlen (typeName) : 0);
		lua_setfield (lua, -2, "typeName");
		
		lua_rawseti (lua, -2, i + 1);
	}
	
	// 
	lua_setfield (lua, -2, "annotations");
}

static void pushMethodType (lua_State *lua, MethodType type) {
	switch (type) {
	case ConstructorMethod:
		lua_pushliteral(lua, "constructor");
		break;
	case DestructorMethod:
		lua_pushliteral(lua, "destructor");
		break;
	case MemberMethod:
		lua_pushliteral(lua, "member");
		break;
	case StaticMethod:
		lua_pushliteral(lua, "static");
		break;
	default:
		lua_pushnil (lua);
	}
	
}

void LuaGenerator::exportVariables (lua_State *lua, const char *name, const Variables &variables) {
	lua_createtable (lua, 0, variables.length ());
	for (int i = 0; i < variables.length (); i++) {
		const VariableDef &var = variables.at (i);
		lua_pushstring (lua, var.name.toLatin1 ().constData ());
		lua_createtable (lua, 0, 11);
		
		// 
		pushAccessSpecifier (lua, var.access);
		lua_setfield (lua, -2, "access");
		
		insertString (lua, "type", var.type);
		insertString (lua, "getter", var.getter);
		insertString (lua, "setterArgName", var.setterArgName);
		insertString (lua, "setter", var.setter);
		exportAnnotations (lua, var.annotations);
		insertBool (lua, "isConst", var.isConst);
		insertBool (lua, "isReference", var.isReference);
		insertBool (lua, "isPodType", var.isPodType);
		insertBool (lua, "isOptional", var.isOptional);
		insertBool (lua, "setterReturnsBool", var.setterReturnsBool);
		
		// 
		lua_settable (lua, -3);
	}
	
	// 
	lua_setfield (lua, -2, name);
}

void LuaGenerator::exportMethods (lua_State *lua, const Methods &methods) {
	lua_createtable (lua, methods.length (), 0);
	for (int i = 0; i < methods.length (); i++) {
		const MethodDef &m = methods.at (i);
		lua_createtable (lua, 0, 10);
		
		// 
		pushAccessSpecifier (lua, m.access);
		lua_setfield (lua, -2, "access");
		
		pushMethodType (lua, m.type);
		lua_setfield (lua, -2, "type");
		
		insertBool (lua, "isVirtual", m.isVirtual);
		insertBool (lua, "isConst", m.isConst);
		insertString (lua, "name", m.name);
		insertString (lua, "returnType", m.returnType);
		exportVariables (lua, "arguments", m.arguments);
		exportAnnotations (lua, m.annotations);
		insertBool (lua, "hasOptionalArguments", m.hasOptionalArguments);
		insertBool (lua, "returnTypeIsPod", m.returnTypeIsPod);
		
		// 
		lua_rawseti (lua, -2, i + 1);
	}
	
	// 
	lua_setfield (lua, -2, "methods");
}

void LuaGenerator::exportEnums (lua_State *lua, const Enums &enums) {
	lua_createtable (lua, 0, enums.length ());
	for (int i = 0; i < enums.length (); i++) {
		const EnumDef &e = enums.at (i);
		lua_pushstring (lua, e.name.toLatin1 ().constData ());
		lua_createtable (lua, 0, 2);
		
		// 
		exportAnnotations (lua, e.annotations);
		exportEnumValues (lua, e.elements);
		
		// 
		lua_settable (lua, -3);
	}
	
	// 
	lua_setfield (lua, -2, "enums");
}

void LuaGenerator::exportEnumValues (lua_State *lua, const QMap< QString, int > &elements) {
	lua_createtable (lua, 0, elements.size ());
	for (auto it = elements.begin (), end = elements.end (); it != end; ++it) {
		lua_pushstring (lua, it.key ().toUtf8 ().constData ());
		lua_pushinteger (lua, it.value ());
		lua_settable (lua, -3);
	}
	
	// 
	lua_setfield (lua, -2, "elements");
}

void LuaGenerator::exportConversions (lua_State *lua, const Conversions &conversions) {
	lua_createtable (lua, conversions.length (), 0);
	for (int i = 0; i < conversions.length (); i++) {
		const ConversionDef &conv = conversions.at (i);
		lua_createtable (lua, 0, 5);
		
		// 
		pushMethodType (lua, conv.type);
		lua_setfield (lua, -2, "type");
		
		insertString (lua, "methodName", conv.methodName);
		insertString (lua, "fromType", conv.fromType);
		insertString (lua, "toType", conv.toType);
		insertBool (lua, "isConst", conv.isConst);
		
		// 
		lua_rawseti (lua, -2, i + 1);
	}
	
	// 
	lua_setfield (lua, -2, "conversions");
}
