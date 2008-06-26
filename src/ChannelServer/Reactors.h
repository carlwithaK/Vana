/*
Copyright (C) 2008 Vana Development Team

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
#ifndef REACTOR_H
#define REACTOR_H

#include <hash_map>
#include <vector>
#include <string>
#include "Pos.h"

using namespace std;
using namespace stdext;

class LoopingId;
class Player;
class Drop;
class Reactor;
class ReactionTimer;
class ReadPacket;

struct ReactorSpawnInfo {
	int id;
	Pos pos;
	int time;
};
typedef vector<ReactorSpawnInfo> ReactorSpawnsInfo;

struct ReactorEventInfo {
	char state;
	short type;
	int itemid;
	short ltx;
	short lty;
	short rbx;
	short rby;
	char nextstate;
};
typedef vector<ReactorEventInfo> ReactorEventsInfo;

namespace Reactors {
	extern hash_map<int, ReactorEventsInfo> reactorinfo;
	extern hash_map<int, short> maxstates;
	extern hash_map<int, ReactorSpawnsInfo> info;
	void addSpawn(int id, ReactorSpawnInfo reactor);
	void addEventInfo(int id, ReactorEventInfo revent);
	void setMaxstates(int id, short state);
	void loadReactors(int mapid);
	void hitReactor(Player *player, ReadPacket *packet);
	void checkDrop(Player *player, Drop *drop);
	void checkLoot(Drop *drop);
};

class Reactor {
public:
	Reactor (int mapid, int reactorid, Pos pos);
	void kill() {
		this->alive = false;
	}
	void revive() {
		this->alive = true;
	}
	bool isAlive() {
		return this->alive;
	}
	void setID(int id) {
		this->id = id;
	}
	int getID() {
		return this->id;
	}
	int getReactorID() {
		return this->reactorid;
	}
	int getMapID() {
		return this->mapid;
	}
	Pos getPos() {
		return this->pos;
	}
	void setState(char state, bool is);
	char getState() {
		return this->state;
	}
private:
	Pos pos;
	bool alive;
	int id;
	int reactorid;
	int mapid;
	char state;
};

#endif
