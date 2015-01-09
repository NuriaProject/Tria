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
	void exportStringBoolMap (lua_State *lua, const char *name, const QMap< QString, bool > &map);
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
