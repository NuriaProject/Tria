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

#ifndef LUAGENERATOR_HPP
#define LUAGENERATOR_HPP

#include "definitions.hpp"

struct lua_State;
class Compiler;
class QFile;

struct GenConf {
	QString luaScript;
	QString outFile;
	QString args;
};

class LuaGenerator {
public:
	LuaGenerator (Definitions *definitions, Compiler *compiler);
	
	static bool parseConfig (const std::string &string, GenConf &config);
	bool generate (const GenConf &config);
	
private:
	
	bool loadScript (const QString &path, QByteArray &code);
	bool runScript (const GenConf &config, const QByteArray &script, QFile *outFile);
	void startShell (lua_State *lua);
	
	void initState (lua_State *lua, const GenConf &config, QFile *file);
	void addInformation (lua_State *lua, const GenConf &config);
	void addLog (lua_State *lua);
	void addJson (lua_State *lua);
	void addWrite (lua_State *lua, QFile *file);
	void addLibLoader (lua_State *lua);
	void registerSourceRangeMetatable (lua_State *lua);
	
	void exportDefinitions (lua_State *lua);
	void exportStringSet (lua_State *lua, const char *name, const StringSet &set);
	void exportStringMap (lua_State *lua, const char *name, const StringMap &map);
	void exportStringList (lua_State *lua, const char *name, const QStringList &list);
	void exportClassDefinitions (lua_State *lua);
	void exportClassDefinition (lua_State *lua, const ClassDef &def);
	void exportClassDefinitionBase (lua_State *lua, const ClassDef &def);
	void exportBases (lua_State *lua, const Bases &bases);
	void exportAnnotations (lua_State *lua, const Annotations &annotations);
	void exportVariables (lua_State *lua, const char *name, const Variables &variables);
	void pushVariable (lua_State *lua, const VariableDef &variable);
	void exportMethods (lua_State *lua, const Methods &methods);
	void exportEnums (lua_State *lua, const Enums &enums);
	void exportEnumValues (lua_State *lua, const QMap< QString, int > &elements);
	void exportConversions (lua_State *lua, const Conversions &conversions);
	
	static int requireLoader (lua_State *lua);
	
	static int jsonSerialize (lua_State *lua);
	
	static int sourceRangeToString (lua_State *lua);
	
	Definitions *m_definitions;
	Compiler *m_compiler;
	
};

#endif // LUAGENERATOR_HPP
