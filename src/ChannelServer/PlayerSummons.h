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
#ifndef PLAYERSUMMONS_H
#define PLAYERSUMMONS_H

#include "Types.h"

class Player;
class Summon;

class PlayerSummons {
public:
	PlayerSummons(Player *player) : player(player), summon(0), puppet(0) { }
	void addSummon(Summon *summon, int32_t time);
	void removeSummon(bool puppet, bool fromTimer);
	Summon * getSummon() {
		return summon;
	}
	Summon * getPuppet() {
		return puppet;
	}
	Summon * getSummon(int32_t summonid);
private:
	Player *player;
	Summon *summon;
	Summon *puppet;
};

#endif
