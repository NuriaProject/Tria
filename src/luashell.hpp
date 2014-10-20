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
