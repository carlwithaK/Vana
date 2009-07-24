/*
Copyright (C) 2008-2009 Vana Development Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef LUANPC_H
#define LUANPC_H

#include "LuaScriptable.h"
#include <string>

using std::string;

class NPC;

class LuaNPC : public LuaScriptable {
public:
	LuaNPC(const string &filename, int32_t playerid, int16_t questid = 0);
	
	bool run();
	bool proceedNext();
	bool proceedSelection(uint8_t selected);
	bool proceedNumber(int32_t number);
	bool proceedText(const string &text);
protected:
	bool resume(int32_t nargs);
	void handleError();

	lua_State *luaThread; // For executing scripts (pausable)
};

namespace LuaExports {
	NPC * getNPC(lua_State *luaVm);

	// NPC exports

	// Miscellaneous
	int showStorage(lua_State *luaVm);
	int getDistanceNpc(lua_State *luaVm);
	int getNpcId(lua_State *luaVm);

	// NPC interaction
	int addText(lua_State *luaVm);
	int sendBackNext(lua_State *luaVm);
	int sendBackOK(lua_State *luaVm);
	int sendNext(lua_State *luaVm);
	int sendOK(lua_State *luaVm);
	int askAcceptDecline(lua_State *luaVm);
	int askChoice(lua_State *luaVm);
	int askNumber(lua_State *luaVm);
	int askStyle(lua_State *luaVm);
	int askText(lua_State *luaVm);
	int askYesNo(lua_State *luaVm);

	// Quest
	int addQuest(lua_State *luaVm);
	int endQuest(lua_State *luaVm);
	int getQuestID(lua_State *luaVm);

	// Skill
	int getMaxSkillLevel(lua_State *luaVm);
	int setMaxSkillLevel(lua_State *luaVm);
};

#endif
