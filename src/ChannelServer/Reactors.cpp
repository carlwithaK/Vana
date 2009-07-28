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
#include "Reactors.h"
#include "Drop.h"
#include "DropHandler.h"
#include "GameLogicUtilities.h"
#include "LuaReactor.h"
#include "Maps.h"
#include "PacketReader.h"
#include "Player.h"
#include "Pos.h"
#include "ReactorPacket.h"
#include "ScriptDataProvider.h"
#include "Timer/Thread.h"
#include "Timer/Time.h"
#include "Timer/Timer.h"
#include <functional>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

using std::string;

// Reactor class
Reactor::Reactor(int32_t mapid, int32_t reactorid, Pos pos, int32_t link) :
state(0),
reactorid(reactorid),
mapid(mapid),
alive(true),
pos(pos),
link(link)
{
	Maps::getMap(mapid)->addReactor(this);
}

void Reactor::setState(int8_t state, bool is) {
	this->state = state;
	if (is) {
		ReactorPacket::triggerReactor(this);
	}
}

void Reactor::restore() {
	revive();
	setState(0, false);
	ReactorPacket::spawnReactor(this);
}

void Reactor::drop(Player *player) {
	DropHandler::doDrops(player->getId(), mapid, 0, reactorid, pos, false, false);
}

// Reactors namespace
unordered_map<int32_t, unordered_map<uint8_t, ReactorEventsInfo> > Reactors::reactorinfo;
unordered_map<int32_t, int16_t> Reactors::maxstates;

void Reactors::addEventInfo(int32_t id, uint8_t state, const ReactorEventInfo &revent) {
	reactorinfo[id][state].push_back(revent);
}

void Reactors::setMaxStates(int32_t id, int16_t state) {
	if (maxstates.find(id) == maxstates.end()) {
		maxstates[id] = state;	
	}
	else if (maxstates[id] < state) {
		maxstates[id] = state;
	}
}

void Reactors::hitReactor(Player *player, PacketReader &packet) {
	int32_t id = packet.get<int32_t>() - 200;

	Reactor *reactor = Maps::getMap(player->getMap())->getReactor(id);

	if (reactor != 0 && reactor->isAlive()) {
		int32_t reactorid = (reactor->getLink() != 0 ? reactor->getLink() : reactor->getReactorId());

		if (reactor->getState() < maxstates[reactorid]) {
			ReactorEventInfo *revent = &reactorinfo[reactorid][reactor->getState()][0]; // There's only one way to hit something
			if (revent->nextstate < maxstates[reactorid]) {
				if (revent->type >= 100)
					return;

				ReactorPacket::triggerReactor(reactor);
				reactor->setState(revent->nextstate, true);
				return;
			}
			else {
				string filename = ScriptDataProvider::Instance()->getReactorScript(reactor->getReactorId());

				struct stat fileInfo;
				if (!stat(filename.c_str(), &fileInfo)) { // Script found
					LuaReactor(filename, player->getId(), id, reactor->getMapId());
				}
				else { // Default action of dropping an item
					reactor->drop(player);
				}

				reactor->setState(revent->nextstate, false);
				reactor->kill();
				Maps::getMap(reactor->getMapId())->removeReactor(id);
				ReactorPacket::destroyReactor(reactor);
			}
		}
	}
}

struct Reaction {
	void operator()() {
		reactor->setState(state, true);
		drop->removeDrop();
		string filename = ScriptDataProvider::Instance()->getReactorScript(reactor->getReactorId());
		LuaReactor(filename, player->getId(), reactor->getId() - 200, reactor->getMapId());
		return;
	}
	Reactor *reactor;
	Drop *drop;
	Player *player;
	int8_t state;
};

void Reactors::checkDrop(Player *player, Drop *drop) {
	for (size_t i = 0; i < Maps::getMap(drop->getMap())->getNumReactors(); i++) {
		Reactor *reactor = Maps::getMap(drop->getMap())->getReactor(i);
		int32_t reactorid = (reactor->getLink() != 0 ? reactor->getLink() : reactor->getReactorId());

		if (reactor->getState() < maxstates[reactorid]) {
			size_t vsize = reactorinfo[reactorid][reactor->getState()].size();
			ReactorEventInfo *revent;
			for (size_t j = 0; j < vsize; j++) {
				revent = &reactorinfo[reactorid][reactor->getState()][j];
				if (revent->type == 100 && drop->getObjectId() == revent->itemid) {
					if (GameLogicUtilities::isInBox(reactor->getPos(), revent->lt, revent->rb, drop->getPos())) {
						Reaction reaction;
						reaction.reactor = reactor;
						reaction.drop = drop;
						reaction.player = player;
						reaction.state = revent->nextstate;

						Timer::Id id(Timer::Types::ReactionTimer, drop->getId(), 0);
						new Timer::Timer(reaction, id, 0, Timer::Time::fromNow(3000));
					}
					return;
				}
			}
		}
	}
}

void Reactors::checkLoot(Drop *drop) {
	Timer::Id id(Timer::Types::ReactionTimer, drop->getId(), 0);
	Timer::Thread::Instance()->getContainer()->removeTimer(id);
}