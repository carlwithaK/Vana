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
#include "Map.h"
#include "Drops.h"
#include "LoopingId.h"
#include "MapPacket.h"
#include "Maps.h"
#include "Mobs.h"
#include "MobsPacket.h"
#include "NPCPacket.h"
#include "PacketCreator.h"
#include "Player.h"
#include "PlayerPacket.h"
#include "ReactorPacket.h"
#include "Reactors.h"

Map::Map (int mapid) : mapid(mapid), mobids(new LoopingId(100)), dropids(new LoopingId(100)) { }

// Players
void Map::addPlayer(Player *player) {
	this->players.push_back(player);
	if (info.fieldType == 82)
		MapPacket::makeApple(player);
	if (player->skills->getActiveSkillLevel(9101004) == 0)
		MapPacket::showPlayer(player);
}

void Map::removePlayer(Player *player) {
	for (size_t i = 0; i < this->players.size(); i++) {
		if (this->players[i]->getPlayerid() == player->getPlayerid()) {
			this->players.erase(this->players.begin() + i);
			break;
		}
	}
	MapPacket::removePlayer(player);
	updateMobControl();
}

// Reactors
void Map::addReactorSpawn(ReactorSpawnInfo spawn) {
	reactorspawns.push_back(spawn);
	Reactor *reactor = new Reactor(mapid, spawn.id, spawn.pos);
	ReactorPacket::spawnReactor(reactor);
}

void Map::addReactor(Reactor *reactor) {
	this->reactors.push_back(reactor);
	reactor->setID(this->reactors.size() - 1 + 200);
}

// Footholds
Pos Map::findFloor(Pos pos) {
	// Determines where a drop falls using the footholds data
	// to check the platforms and finds the correct one.
	// Should have the point of origin passed as the pos parameter
	short DROP_HEIGHT = 100;
	short dropX = pos.x;
	short dropY = SHRT_MAX;
	short dropBounce = pos.y - DROP_HEIGHT;
	for (size_t i = 0; i < footholds.size(); i++) {
		if (dropX > footholds[i].pos1.x && dropX < footholds[i].pos2.x ||
			dropX > footholds[i].pos2.x && dropX < footholds[i].pos1.x) {
				short platformHeight = (short)((float) (dropX - footholds[i].pos1.x)) / (footholds[i].pos2.x - footholds[i].pos1.x) *
					(footholds[i].pos2.y - footholds[i].pos1.y) + footholds[i].pos1.y;
				if (dropBounce < platformHeight && platformHeight < dropY) {
					dropY = platformHeight;
				}
		}
	}
	return Pos(dropX, dropY);
}

// Mobs
void Map::addMobSpawn(MobSpawnInfo spawn) {
	mobspawns.push_back(spawn);
	new Mob(mapid, spawn.id, spawn.pos, mobspawns.size()-1, spawn.fh);
}

void Map::checkSpawn(clock_t time) {
	// (Re-)spawn Mobs
	for (size_t i = 0; i < mobrespawns.size(); i++) {
		int id = mobrespawns[i].spawnid;
		if ((time - mobrespawns[i].killed) > (mobspawns[id].time * CLOCKS_PER_SEC)) {
			new Mob(mapid, mobspawns[id].id, mobspawns[id].pos, id, mobspawns[id].fh);
			mobrespawns.erase(mobrespawns.begin()+i);
			i--;
		}
	}
}

void Map::addMob(Mob *mob) {
	int id = this->mobids->next();
	mob->setID(id);
	this->mobs[id] = mob;
	MobsPacket::spawnMob(mob);
	updateMobControl(mob);
}

void Map::updateMobControl() {
	for (hash_map <int, Mob *>::iterator iter = mobs.begin(); iter != mobs.end(); iter++) {
		if (iter->second != 0)
			updateMobControl(iter->second);
	}
}

void Map::updateMobControl(Mob *mob) {
	if (players.size() > 0 && mob->getControl() == 0) {
		int maxpos = mob->getPos() - players[0]->getPos();
		int player = 0;
		for (size_t j = 0; j < players.size(); j++) {
			int curpos = mob->getPos() - players[j]->getPos();
			if (curpos < maxpos) {
				maxpos = curpos;
				player = j;
			}
		}
		mob->setControl(players[player]);
	}
	else if (players.size() == 0) {
		mob->setControl(0);
	}
}

void Map::removeMob(int id) {
	if (mobs.find(id) != mobs.end()) {
		if (mobs[id]->getSpawnID() > -1 && mobspawns[mobs[id]->getSpawnID()].time > -1) // Add spawn point to respawns if mob was spawned by a spawn point.
			mobrespawns.push_back(MobRespawnInfo(mobs[id]->getSpawnID(), clock()));
		this->mobs.erase(id);
	}
}

void Map::killMobs(Player *player) {
	hash_map <int, Mob *> mobs = this->mobs;
	for (hash_map <int, Mob *>::iterator iter = mobs.begin(); iter != mobs.end(); iter++) { // While loops cause problems
		if (iter->second != 0)
			iter->second->die(player);
	}
}

void Map::killMobs(Player *player, int mobid) {
	hash_map <int, Mob *> mobs = this->mobs;
	for (hash_map <int, Mob *>::iterator iter = mobs.begin(); iter != mobs.end(); iter++) {
		if (iter->second != 0)
			if (iter->second->getMobID() == mobid)
				iter->second->die(player);
	}
}

// Drops
void Map::addDrop(Drop *drop) {
	int id = dropids->next();
	drop->setID(id);
	drop->setPos(findFloor(drop->getPos()));
	this->drops[id] = drop;
}

void Map::clearDrops() { // Clear all drops
	hash_map <int, Drop *> drops = this->drops;
	for (hash_map <int, Drop *>::iterator iter = drops.begin(); iter != drops.end(); iter++) {
		if (iter->second != 0) // Check just in case drop is removed by timer
			iter->second->removeDrop();
	}
}

void Map::clearDrops(int time) { // Clear drops based on how long they have been in the map
	time -= 60000;
	hash_map <int, Drop *> drops = this->drops;
	for (hash_map <int, Drop *>::iterator iter = drops.begin(); iter != drops.end(); iter++) {
		if (iter->second != 0)
			if (iter->second->getDropped() < time)
				iter->second->removeDrop();
	}
}

void Map::showObjects(Player *player) { // Show all Map Objects
	// Players
	for (size_t i = 0; i < players.size(); i++) {
		if (player != players[i] && players[i]->skills->getActiveSkillLevel(9101004) == 0) {
			PacketCreator packet = MapPacket::playerPacket(players[i]);
			packet.send(player);
			// Bug in global; would be fixed here:
			// Hurricane/Pierce do not display properly if using when someone enters the map
		}
	}
	// NPCs
	for (size_t i = 0; i < npcs.size(); i++) {
		NPCPacket::showNPC(player, npcs[i], i);
	}
	// Reactors
	for (size_t i = 0; i < reactors.size(); i++) {
		ReactorPacket::showReactor(player, reactors[i]);
	}
	// Mobs
	updateMobControl();
	for (hash_map <int, Mob *>::iterator iter = mobs.begin(); iter != mobs.end(); iter++) {
		if (iter->second != 0)
			MobsPacket::showMob(player, iter->second);
	}
	// Drops
	for (hash_map <int, Drop *>::iterator iter = drops.begin(); iter != drops.end(); iter++) {
		if (iter->second != 0)
			iter->second->showDrop(player);
	}
	if (info.clock)
		Maps::showClock(player);
}

void Map::sendPacket(PacketCreator &packet, Player *player) {
	for (size_t i = 0; i < this->players.size(); i++) {
		if (this->players[i] != player) {
			packet.send(this->players[i]);
		}
	}
}

void Map::showMessage(string message, char type) {
	for (size_t i = 0; i < players.size(); i++)
		PlayerPacket::showMessage(players[i], message, type);
}