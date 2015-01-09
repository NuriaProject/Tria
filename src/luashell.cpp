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

#include "luashell.hpp"

#include <clang/Basic/Version.h>
#include "definitions.hpp"
#include <lua.hpp>
#include <cstdio>
#include <QFile>

LuaShell::LuaShell (lua_State *lua)
        : m_lua (lua)
{
	
}

void LuaShell::run () {
	QByteArray line;
	State s = MissingData;
	
	// 
	QFile input;
	input.open (stdin, QIODevice::ReadOnly);
	
	// Main loop
	printHeader ();
	while ((s = readLine (line, &input)) != QuitShell) {
		switch (s) {
		case MissingData: break;
		case Ready:
			invoke ();
			line.clear ();
			break;
		case SyntaxError:
			printf ("Syntax error: %s\n", line.constData ());
			line.clear ();
			break;
		case QuitShell:
			return;
		}
		
	}
	
}

void LuaShell::printHeader () {
	printf ("Tria Lua shell [" __TIME__ " " __DATE__  "] / " LUAJIT_VERSION
	        " / LLVM " QT_STRINGIFY(LLVM_VERSION_MAJOR) "." QT_STRINGIFY(LLVM_VERSION_MINOR) "\n");
}

bool LuaShell::invoke () {
	
	// Invoke
	int r = lua_pcall (this->m_lua, 0, LUA_MULTRET, 0);
	
	// Print error
	switch (r) {
	case LUA_ERRRUN: printf ("! Runtime error\n"); break;
	case LUA_ERRMEM: printf ("! Memory allocation error\n"); break;
	}
	
	// Print returned data
	int results = lua_gettop (this->m_lua);
	if (results > 0) {
		lua_getglobal(this->m_lua, "print");
		lua_insert (this->m_lua, 1);
		lua_pcall (this->m_lua, results, 0, 0);
	}
	
	// Done
	return (r == 0);
}

LuaShell::State LuaShell::readLine (QByteArray &buffer, QFile *input) {
	if (input->atEnd ()) {
		return QuitShell;
	}
	
	// 
	const char *prompt = (buffer.isEmpty ()) ? "> " : ">> ";
	printf (prompt);
	
	// 
	QByteArray line = input->readLine ();
	buffer.append (line);
	
	// 
	if (!buffer.isEmpty () && buffer.at (0) == '=') {
		buffer.replace (0, 1, "return ");
	}
	
	// 
	return compileLine (buffer);
}

LuaShell::State LuaShell::compileLine (QByteArray &buffer) {
	int r = luaL_loadbuffer (this->m_lua, buffer.constData (), buffer.length (), "=stdin");
	if (r == 0) return Ready;
	if (r != LUA_ERRSYNTAX) return QuitShell;
	
	// Is the input simply incomplete?
	State result = SyntaxError;
	size_t length = 0;
	const char *message = lua_tolstring (this->m_lua, -1, &length);
	if (QByteArray::fromRawData (message, length).endsWith ("'<eof>'")) {
		result = MissingData;
	} else {
		buffer = QByteArray (message, length);
	}
	
	// Get rid of the error message
	lua_pop(this->m_lua, 1);
	return result;
}
