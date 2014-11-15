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

#include <QJsonDocument>
#include <QDateTime>
#include <memory>
#include <QDebug>
#include <QFile>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnostic.h>
#include "definitions.hpp"
#include "compiler.hpp"
#include "luashell.hpp"
#include <lua.hpp>
#include <cstdio>

#define METATABLE_SOURCERANGE "clang::SourceRange"

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

LuaGenerator::LuaGenerator (Definitions *definitions, Compiler *compiler)
	: m_definitions (definitions), m_compiler (compiler)
{
	
}

bool LuaGenerator::parseConfig (const std::string &string, GenConf &config) {
	size_t delim = string.find (':');
	if (delim == std::string::npos) {
		qCritical() << "Invalid Lua generator config:" << string.c_str ();
		return false;
	}
	
	// Store
	config.luaScript = QString (QLatin1String (string.c_str (), delim));
	config.outFile = QString (QLatin1String (string.c_str () + delim + 1));
	
	// Look for optional 'arguments' field
	int argsPos = config.outFile.indexOf (QChar (':'));
	if (argsPos > -1) {
		config.args = config.outFile.mid (argsPos + 1);
		config.outFile.chop (config.outFile.length () - argsPos);
	}
	
	// Sanity check
	if (config.luaScript.isEmpty () || config.outFile.isEmpty ()) {
		qCritical() << "Lua: no lua or outfile was given:" << string.c_str ();
		return false;
	}
	
	// 
	return true;
}

static bool openFileOrStdout (QFile *file, QString name) {
	QIODevice::OpenMode openMode = QIODevice::WriteOnly;
	
	// Write to stdout or append
	if (name == QLatin1String ("-")) {
		return file->open (stdout, QIODevice::WriteOnly);
	} else if (name.startsWith ('+') && name.length () > 1) {
		openMode |= QIODevice::Append;
		name = name.mid (1);
	}
	
	// Open regular file
	file->setFileName (name);
	return file->open (openMode);
}

bool LuaGenerator::generate (const GenConf &config) {
	
	// Read lua file
	QByteArray scriptData;
	if (!loadScript (config.luaScript, scriptData)) {
		return false;
	}
	
	// Open outfile
	QFile outHandle;
	if (!openFileOrStdout (&outHandle, config.outFile)) {
		qCritical() << "Lua: failed to open outfile" << config.outFile;
		return false;
	}
	
	// 
	return runScript (config, scriptData, &outHandle);
}

bool LuaGenerator::loadScript (const QString &path, QByteArray &code) {
	
	// Shell?
	if (path == QLatin1String ("SHELL")) {
		return true;
	}
	
	// Usual script file
	QFile scriptFile (path);
	if (!scriptFile.open (QIODevice::ReadOnly)) {
		qCritical() << "Lua: failed to open script" << path;
		return false;
	}
	
	// 
	code = scriptFile.readAll ();
	return true;
}

static inline bool loadFromByteArray (lua_State *lua, const QByteArray &code, const QString &displayName) {
	return (luaL_loadbuffer (lua, code.constData (), code.length (), qPrintable(displayName)) == 0);
}

static void reportExecuteError (lua_State *lua, const QString &displayName) {
	printf ("Failed to execute script %s: %s", qPrintable(displayName), lua_tostring(lua, 1));
}

static inline bool executeByteArray (lua_State *lua, const QByteArray &code, const QString &displayName) {
	bool r = loadFromByteArray (lua, code, displayName);
	return (r && lua_pcall (lua, 0, 0, 0) == 0);
}

bool LuaGenerator::runScript (const GenConf &config, const QByteArray &script, QFile *outFile) {
	std::unique_ptr< lua_State, decltype(&lua_close) > lua (lua_open (), &lua_close);
	initState (lua.get (), config, outFile);
	exportDefinitions (lua.get ());
	
	// Execute script
	if (config.luaScript == QLatin1String ("SHELL")) {
		startShell (lua.get ());
	} else if (!executeByteArray (lua.get (), script, config.luaScript)) {
		reportExecuteError (lua.get (), config.luaScript);
		outFile->remove ();
		return false;
	}
	
	// 
	return true;
}

void LuaGenerator::startShell (lua_State *lua) {
	LuaShell shell (lua);
	shell.run ();
}

void LuaGenerator::initState (lua_State *lua, const GenConf &config, QFile *file) {
	luaL_openlibs (lua);
	
	// 
	addLog (lua);
	addWrite (lua, file);
	addJson (lua);
	addLibLoader (lua);
	addInformation (lua, config);
	registerSourceRangeMetatable (lua);
	
}

static void pushSourceRange (lua_State *lua, const clang::SourceRange &range) {
	
	// Copy range to lua
	void *ptr = lua_newuserdata (lua, sizeof(clang::SourceRange));
	new (ptr) clang::SourceRange (range);
	
	// Set metatable
	luaL_getmetatable(lua, METATABLE_SOURCERANGE);
	lua_setmetatable (lua, -2);
	
}

static inline void insertString (lua_State *lua, const char *name, const QString &string) {
	lua_pushstring (lua, string.toUtf8 ().constData ());
	lua_setfield (lua, -2, name);
}

static inline void insertBool (lua_State *lua, const char *name, bool value) {
	lua_pushboolean (lua, value);
	lua_setfield (lua, -2, name);
}

static inline void insertSourceRange (lua_State *lua, const char *name, const clang::SourceRange &range) {
	pushSourceRange (lua, range);
	lua_setfield (lua, -2, name);
}

void LuaGenerator::addInformation (lua_State *lua, const GenConf &config) {
	lua_createtable (lua, 0, 6);
	
	lua_pushliteral(lua, __TIME__);
	lua_setfield (lua, -2, "compileTime");
	
	lua_pushliteral(lua, __DATE__);
	lua_setfield (lua, -2, "compileDate");
	
	lua_pushliteral(lua, QT_STRINGIFY(LLVM_VERSION_MAJOR) "." QT_STRINGIFY(LLVM_VERSION_MINOR));
	lua_setfield (lua, -2, "llvmVersion");
	
	insertString (lua, "arguments", config.args);
	exportStringList (lua, "sourceFiles", this->m_definitions->sourceFiles ());
	insertString (lua, "outFile", config.outFile);
	insertString (lua, "currentDateTime", QDateTime::currentDateTime ().toString (Qt::ISODate));
	
	lua_setfield (lua, LUA_GLOBALSINDEX, "tria");
}

static clang::SourceRange getSourceRangePointer (lua_State *lua, int pos) {
	void *ptr = luaL_checkudata (lua, pos, METATABLE_SOURCERANGE);
	luaL_argcheck(lua, ptr != nullptr, 1, METATABLE_SOURCERANGE " expected");
	
	// 
	return *(clang::SourceRange *)ptr;
}

static clang::SourceRange getSourceRangeTable (lua_State *lua, int pos) {
	lua_getfield (lua, pos, "loc");
	clang::SourceRange result = getSourceRangePointer (lua, -1);
	lua_pop(lua, 1);
	return result;
}

static clang::SourceRange getSourceRange (lua_State *lua) {
	if (lua_istable(lua, 1)) {
		return getSourceRangeTable (lua, 1);
	}
	
	return getSourceRangePointer (lua, 1);
}

static llvm::StringRef luaStringToStringRef (lua_State *lua, int idx) {
	size_t len = 0;
	const char *message = lua_tolstring (lua, idx, &len);
	return llvm::StringRef (message, len);
}

static clang::StoredDiagnostic luaStringToStoredDiag (lua_State *lua, int idx, clang::DiagnosticsEngine::Level level,
                                                      clang::DiagnosticsEngine *diag) {
	return clang::StoredDiagnostic (level, clang::Diagnostic (diag, luaStringToStringRef (lua, idx)));
}

static bool logString (lua_State *lua, Compiler *compiler, clang::DiagnosticsEngine::Level level) {
	if (lua_isstring (lua, 1)) {
		compiler->diag ()->Report (luaStringToStoredDiag (lua, 1, level, compiler->diag ()));
		return true;
	}
	
	return false;
}

static clang::SourceRange *sourceRangePointer (lua_State *lua, int idx) {
	void *ptr = nullptr;
	
	if (lua_istable(lua, idx)) {
		lua_getfield (lua, idx, "loc");
		ptr = luaL_checkudata (lua, -1, METATABLE_SOURCERANGE);
		lua_pop(lua, 1);
	} else {
		ptr = luaL_checkudata (lua, 1, METATABLE_SOURCERANGE);
	}
	
	// 
	return (clang::SourceRange *)ptr;
}

static bool logRangeString (lua_State *lua, Compiler *compiler, clang::DiagnosticsEngine::Level level) {
	if (!lua_isstring (lua, 2)) {
		return false;
	}
	
	// 
	clang::SourceRange &range = *sourceRangePointer (lua, 1);
	llvm::StringRef msg = luaStringToStringRef (lua, 2);
	compiler->textDiag ()->emitDiagnostic (range.getBegin (), level, msg,
	                                       llvm::ArrayRef< clang::CharSourceRange > (),
	                                       llvm::ArrayRef< clang::FixItHint > (),
	                                       &compiler->compiler ()->getSourceManager ());
	return true;
}

static int luaLog (lua_State *lua) {
	int level = lua_tointeger (lua, lua_upvalueindex(1));
	Compiler *comp = (Compiler *)lua_touserdata (lua, lua_upvalueindex(2));
	int argc = lua_gettop (lua);
	bool ok = false;
	
	// Which form was used?
	if (argc == 1) { // String only form
		ok = logString (lua, comp, clang::DiagnosticsEngine::Level (level));
	} else if (argc == 2) { // SourceRange, String form
		ok = logRangeString (lua, comp, clang::DiagnosticsEngine::Level (level));
	}
	
	// 
	if (!ok) {
		return luaL_error (lua, "Log methods expect (String) or (SourceRange, String) as arguments");
	}
	
	return 0;
}

static void addLogFunc (lua_State *lua, const char *name, clang::DiagnosticsEngine::Level level, Compiler *compiler) {
	lua_pushinteger (lua, level);
	lua_pushlightuserdata (lua, compiler);
	lua_pushcclosure (lua, &luaLog, 2);
	lua_setfield (lua, -2, name);
	
}

void LuaGenerator::addLog (lua_State *lua) {
	
	// 
	lua_createtable (lua, 0, 6);
	
	addLogFunc (lua, "ignored", clang::DiagnosticsEngine::Ignored, this->m_compiler);
	addLogFunc (lua, "note", clang::DiagnosticsEngine::Note, this->m_compiler);
	addLogFunc (lua, "remark", clang::DiagnosticsEngine::Remark, this->m_compiler);
	addLogFunc (lua, "warning", clang::DiagnosticsEngine::Warning, this->m_compiler);
	addLogFunc (lua, "error", clang::DiagnosticsEngine::Error, this->m_compiler);
	addLogFunc (lua, "fatal", clang::DiagnosticsEngine::Fatal, this->m_compiler);
	
	// 
	lua_setfield (lua, LUA_GLOBALSINDEX, "log");
}

void LuaGenerator::addJson (lua_State *lua) {
	lua_createtable (lua, 0, 1);
	
//	lua_pushcclosure (lua, &LuaGenerator::jsonParse, 0);
//	lua_setfield (lua, -2, "parse");
	
	lua_pushcclosure (lua, &LuaGenerator::jsonSerialize, 0);
	lua_setfield (lua, -2, "serialize");
	
	lua_setfield (lua, LUA_GLOBALSINDEX, "json");
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

void LuaGenerator::addLibLoader (lua_State *lua) {
	lua_getglobal(lua, "package");
	lua_getfield (lua, -1, "loaders");
	
	// Append loader to the end
	lua_pushcclosure (lua, &LuaGenerator::requireLoader, 0);
	lua_rawseti (lua, -2, lua_objlen (lua, -2) + 1);
	
	// 
	lua_pop(lua, 2);
}

void LuaGenerator::registerSourceRangeMetatable (lua_State *lua) {
	luaL_newmetatable (lua, METATABLE_SOURCERANGE);
	
	// meta.__index = meta
	lua_pushvalue (lua, -1);
	lua_setfield (lua, -2, "__index");
	
	// meta.__tostring = sourceRangeToString()
	lua_pushlightuserdata (lua, this->m_compiler);
	lua_pushcclosure (lua, &LuaGenerator::sourceRangeToString, 1);
	lua_setfield (lua, -2, "__tostring");
	
	// 
	lua_pop(lua, 1);
}

void LuaGenerator::exportDefinitions (lua_State *lua) {
	
	// _G.definitions
	lua_createtable (lua, 0, 4);
	
	// 
	exportStringSet (lua, "declaredTypes", this->m_definitions->declaredTypes ());
	exportStringSet (lua, "declareTypes", this->m_definitions->declareTypes ());
	exportStringSet (lua, "avoidedTypes", this->m_definitions->avoidedTypes ());
	exportStringMap (lua, "typedefs", this->m_definitions->typedefs ());
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

void LuaGenerator::exportStringMap (lua_State *lua, const char *name, const StringMap &map) {
	lua_createtable (lua, 0, map.size ());
	
	for (auto it = map.constBegin (), end = map.constEnd (); it != end; ++it) {
		lua_pushstring (lua, it.key ().toLatin1 ().constData ());
		lua_pushstring (lua, it.value ().toLatin1 ().constData ());
		lua_settable (lua, -3);
	}
	
	// 
	lua_setfield (lua, -2, name);
}

void LuaGenerator::exportStringList (lua_State *lua, const char *name, const QStringList &list) {
	lua_createtable (lua, list.length (), 0);
	
	for (int i = 0; i < list.length (); i++) {
		lua_pushstring (lua, list.at (i).toLatin1 ().constData ());
		lua_rawseti (lua, -2, i + 1);
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
	lua_createtable (lua, 0, 17);
	
	// 
	lua_pushvalue (lua, -2);
	lua_setfield (lua, -2, "name");
	
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
	insertBool (lua, "hasValueSemantics", def.hasValueSemantics);
	insertBool (lua, "hasDefaultCtor", def.hasDefaultCtor);
	insertBool (lua, "hasCopyCtor", def.hasCopyCtor);
	insertBool (lua, "hasAssignmentOperator", def.hasAssignmentOperator);
	insertBool (lua, "implementsCtor", def.implementsCtor);
	insertBool (lua, "implementsCopyCtor", def.implementsCopyCtor);
	insertBool (lua, "isFakeClass", def.isFakeClass);
	insertBool (lua, "hasPureVirtuals", def.hasPureVirtuals);
	insertString (lua, "file", def.file);
	insertSourceRange (lua, "loc", def.loc);
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
		lua_createtable (lua, 0, 3);
		
		insertBool (lua, "isVirtual", base.isVirtual);
		insertSourceRange (lua, "loc", base.loc);
		
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
		lua_createtable (lua, 0, 5);
		
		insertString (lua, "name", cur.name);
		insertString (lua, "value", cur.value);
		insertSourceRange (lua, "loc", cur.loc);
		
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
	
	lua_createtable (lua, variables.length (), 0);
	for (int i = 0; i < variables.length (); i++) {
		const VariableDef &var = variables.at (i);
		lua_createtable (lua, 0, 13);
		
		// 
		lua_pushstring (lua, var.name.toLatin1 ().constData ());
		lua_setfield (lua, -2, "name");
		
		pushAccessSpecifier (lua, var.access);
		lua_setfield (lua, -2, "access");
		
		insertSourceRange (lua, "loc", var.loc);
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
		lua_rawseti (lua, -2, i + 1);
	}
	
	// 
	lua_setfield (lua, -2, name);
}

void LuaGenerator::exportMethods (lua_State *lua, const Methods &methods) {
	lua_createtable (lua, methods.length (), 0);
	for (int i = 0; i < methods.length (); i++) {
		const MethodDef &m = methods.at (i);
		lua_createtable (lua, 0, 12);
		
		// 
		pushAccessSpecifier (lua, m.access);
		lua_setfield (lua, -2, "access");
		
		pushMethodType (lua, m.type);
		lua_setfield (lua, -2, "type");
		
		insertBool (lua, "isVirtual", m.isVirtual);
		insertBool (lua, "isPure", m.isPure);
		insertBool (lua, "isConst", m.isConst);
		insertString (lua, "name", m.name);
		insertString (lua, "returnType", m.returnType);
		exportVariables (lua, "arguments", m.arguments);
		exportAnnotations (lua, m.annotations);
		insertBool (lua, "hasOptionalArguments", m.hasOptionalArguments);
		insertBool (lua, "returnTypeIsPod", m.returnTypeIsPod);
		insertSourceRange (lua, "loc", m.loc);
		
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
		lua_createtable (lua, 0, 4);
		
		// 
		lua_pushvalue (lua, -2);
		lua_setfield (lua, -2, "name");
		
		exportAnnotations (lua, e.annotations);
		exportEnumValues (lua, e.elements);
		insertSourceRange (lua, "loc", e.loc);
		
		// 
		lua_settable (lua, -3);
	}
	
	// 
	lua_setfield (lua, -2, "enums");
}

void LuaGenerator::exportEnumValues (lua_State *lua, const QMap< QString, int > &elements) {
	lua_createtable (lua, 0, elements.size ());
	
	if (elements.size () > 0) { // Reverse iterate to keep sort order
		auto it = elements.end (), end = elements.begin ();
		do {
			--it;
			lua_pushstring (lua, it.key ().toUtf8 ().constData ());
			lua_pushinteger (lua, it.value ());
			lua_settable (lua, -3);
		} while (it != end);
		
	}
	
	// 
	lua_setfield (lua, -2, "elements");
}

void LuaGenerator::exportConversions (lua_State *lua, const Conversions &conversions) {
	lua_createtable (lua, conversions.length (), 0);
	for (int i = 0; i < conversions.length (); i++) {
		const ConversionDef &conv = conversions.at (i);
		lua_createtable (lua, 0, 6);
		
		// 
		pushMethodType (lua, conv.type);
		lua_setfield (lua, -2, "type");
		
		insertString (lua, "methodName", conv.methodName);
		insertString (lua, "fromType", conv.fromType);
		insertString (lua, "toType", conv.toType);
		insertBool (lua, "isConst", conv.isConst);
		insertSourceRange (lua, "loc", conv.loc);
		
		// 
		lua_rawseti (lua, -2, i + 1);
	}
	
	// 
	lua_setfield (lua, -2, "conversions");
}

static int loadModule (lua_State *lua, const QString &path, const QString &name) {
	QFile file (path);
	if (!file.open (QIODevice::ReadOnly)) {
		return 0;
	}
	
	// 
	QByteArray code = file.readAll ();
	if (!loadFromByteArray (lua, code, name)) {
		reportExecuteError (lua, name);
		return 0;
	}
	
	return 1;
}

int LuaGenerator::requireLoader (lua_State *lua) {
	static const QString prefix = QStringLiteral(":/lua/");
	static const QString suffix = QStringLiteral(".lua");
	
	// Get module name
	size_t len = 0;
	const char *rawName = lua_tolstring (lua, 1, &len);
	QString name = QString::fromUtf8 (rawName, len);
	
	// Do we know this?
	QString fullName = prefix + name + suffix;
	if (!QFile::exists (fullName)) {
		lua_pushfstring (lua, "Unknown Tria module '%s'", rawName);
		return 1;
	}
	
	// 
	return loadModule (lua, fullName, name);
}

static QVariant luaToVariant (lua_State *lua);
static QVariantList luaArrayToVariant (lua_State *lua) {
	int arrLen = lua_objlen (lua, -1);
	QVariantList list;
	list.reserve (arrLen);
	
	for (int i = 1; i <= arrLen; i++) {
		lua_rawgeti (lua, -1, i); // Push element on stack
		list.append (luaToVariant (lua)); // Inspect
		lua_pop (lua, 1); // Pop element from stack
	}
	
	return list;
}

static QVariantMap luaMapToVariant (lua_State *lua) {
	QVariantMap map;
	
	lua_pushnil (lua); // Push key
	while (lua_next (lua, -2) != 0) {
		if (lua_isstring (lua, -2)) {
			size_t len = 0;
			const char *rawKey = lua_tolstring (lua, -2, &len);
			QString key = QString::fromUtf8 (rawKey, len);
			map.insert (key, luaToVariant (lua));
		}
		
		lua_pop(lua, 1); // Pop value
	}
	
	// Key is removed by lua_next()
	return map;
}

static QVariant luaTableToVariant (lua_State *lua) {
	int arrLen = lua_objlen (lua, -1);
	if (arrLen > 0) {
		return luaArrayToVariant (lua);
	}
	
	return luaMapToVariant (lua);
}

static QVariant luaToVariant (lua_State *lua) {
	switch (lua_type (lua, -1)) {
	case LUA_TSTRING:
		return QString::fromUtf8 (lua_tostring(lua, -1));
	case LUA_TBOOLEAN:
		return bool (lua_toboolean (lua, -1));
	case LUA_TNUMBER:
		return lua_tonumber (lua, -1);
	case LUA_TTABLE:
		return luaTableToVariant (lua);
	}
	
	return QVariant ();
}

int LuaGenerator::jsonSerialize (lua_State *lua) {
	if (lua_gettop (lua) != 1 || !lua_istable(lua, 1)) {
		return luaL_error (lua, "json.serialize expects a table as only argument.");
	}
	
	// Generate JSON
	QVariant variant = luaToVariant (lua);
	QByteArray data = QJsonDocument::fromVariant (variant).toJson (QJsonDocument::Indented);
	
	// Return
	lua_pushstring (lua, data.constData ());
	return 1;
}

int LuaGenerator::sourceRangeToString (lua_State *lua) {
	Compiler *comp = (Compiler *)lua_touserdata (lua, lua_upvalueindex(1));
	void *ptr = luaL_checkudata (lua, 1, METATABLE_SOURCERANGE);
	clang::SourceRange *range = static_cast< clang::SourceRange * > (ptr);
	
	if (!range) {
		return 0;
	}
	
	// Create string
	clang::SourceManager &sm = comp->compiler ()->getSourceManager ();
	std::string begin = range->getBegin ().printToString (sm);
	std::string end = range->getEnd ().printToString (sm);
	lua_pushliteral(lua, "[");
	lua_pushlstring (lua, begin.c_str (), begin.length ());
	lua_pushliteral(lua, "]:[");
	lua_pushlstring (lua, end.c_str (), end.length ());
	lua_pushliteral(lua, "]");
	lua_concat (lua, 5);
	return 1;
}
