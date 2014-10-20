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

#include "luashell.hpp"

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
	printf ("Tria Lua shell [" __TIME__ " " __DATE__  "]\n");
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
