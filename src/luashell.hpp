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

#ifndef LUASHELL_HPP
#define LUASHELL_HPP

#include "definitions.hpp"

struct lua_State;
class QFile;

class LuaShell {
public:
	
	enum State {
		Ready,
		MissingData,
		SyntaxError,
		QuitShell
	};
	
	// 
	LuaShell (lua_State *lua);
	
	void run ();
	
	
private:
	lua_State *m_lua;
	
	void printHeader ();
	bool invoke ();
	State readLine (QByteArray &buffer, QFile *input);
	State compileLine (QByteArray &buffer);
	
};

#endif // LUASHELL_HPP
