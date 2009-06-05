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
#ifndef MAP_H
#define MAP_H

#include "LoopingId.h"
#include "Mobs.h"
#include "Pos.h"
#include <ctime>
#include <boost/tr1/memory.hpp>
#include <boost/tr1/unordered_map.hpp>
#include <vector>
#include <string>
#include <boost/thread/recursive_mutex.hpp>

using std::string;
using std::vector;
using std::tr1::shared_ptr;
using std::tr1::unordered_map;

class Drop;
class Instance;
class Mob;
class PacketCreator;
class Player;
class Reactor;

struct MapInfo {
	MapInfo() : musicname(""), top(0), left(0), right(0), bottom(0) {}
	int32_t id;
	int32_t rm;
	int32_t forcedReturn;
	int8_t fieldType;
	int32_t fieldLimit;
	double spawnrate;
	bool clock;
	bool town;
	int32_t shipInterval;
	int16_t left;
	int16_t top;
	int16_t bottom;
	int16_t right;
	string musicname;
};
typedef shared_ptr<MapInfo> MapInfoPtr;

struct FootholdInfo {
	int16_t id;
	Pos pos1;
	Pos pos2;
};
typedef vector<FootholdInfo> FootholdsInfo;

struct PortalInfo {
	int8_t id;
	string name;
	Pos pos;
	int32_t tomap;
	string toname;
	string script;
	bool onlyOnce; // Run it only once per map entry
};
typedef unordered_map<string, PortalInfo> PortalsInfo;
typedef vector<PortalInfo> SpawnPoints;

struct NPCSpawnInfo {
	int32_t id;
	Pos pos;
	int16_t fh;
	int16_t rx0;
	int16_t rx1;
};
typedef vector<NPCSpawnInfo> NPCSpawnsInfo;

struct ReactorSpawnInfo {
	int32_t id;
	Pos pos;
	int32_t time;
};
typedef vector<ReactorSpawnInfo> ReactorSpawnsInfo;

struct ReactorRespawnInfo {
	ReactorRespawnInfo(int32_t id, clock_t killed) : id(id), killed(killed) {}
	int32_t id;
	clock_t killed;
};
typedef vector<ReactorRespawnInfo> ReactorRespawns;

struct MobSpawnInfo {
	int32_t id;
	Pos pos;
	int16_t fh;
	int32_t time;
};
typedef vector<MobSpawnInfo> MobSpawnsInfo;

struct MobRespawnInfo {
	MobRespawnInfo(int32_t spawnid, clock_t spawnat) : spawnid(spawnid), spawnat(spawnat) {}
	int32_t spawnid;
	clock_t spawnat;
};
typedef vector<MobRespawnInfo> MobRespawnsInfo;

class Map {
public:
	Map(MapInfoPtr info);
	// Map Info
	MapInfoPtr getInfo() const { return info; }
	void setMusic(const string &musicname);

	// Footholds
	void addFoothold(FootholdInfo foothold) { footholds.push_back(foothold); }
	Pos findFloor(Pos pos);
	int16_t getFhAtPosition(Pos pos);

	// Portals
	void addPortal(PortalInfo portal) {
		if (portal.name == "sp")
			spawnpoints.push_back(portal);
		else
			portals[portal.name] = portal;
	}
	PortalInfo * getPortal(const string &name) {
		return portals.find(name) != portals.end() ? &portals[name] : 0;
	}
	PortalInfo * getSpawnPoint(int32_t pid = -1);

	// Players
	void addPlayer(Player *player);
	size_t getNumPlayers() const { return this->players.size(); }
	Player * getPlayer(uint32_t i) const { return this->players[i]; }
	void removePlayer(Player *player);
	void dispelPlayers(int16_t prop, const Pos &origin, const Pos &lt, const Pos &rb);
	void statusPlayers(uint8_t status, uint8_t level, int16_t count, int16_t prop, const Pos &origin, const Pos &lt, const Pos &rb);

	// NPCs
	void addNPC(NPCSpawnInfo npc) { this->npcs.push_back(npc); }
	NPCSpawnInfo getNpc(int32_t id) const { return this->npcs[id]; }

	// Mobs
	void addMobSpawn(MobSpawnInfo spawn);
	void checkMobSpawn(clock_t time);
	void spawnMob(int32_t mobid, Pos pos, int32_t spawnid = -1, int16_t fh = 0, Mob *owner = 0, int8_t summoneffect = 0);
	void removeMob(int32_t id, int32_t spawnid);
	int32_t killMobs(Player *player, int32_t mobid = 0, bool playerkill = true, bool showpacket = true);
	int32_t countMobs(int32_t mobid = 0);
	Mob * getMob(int32_t id, bool isMapId = true);
	void healMobs(int32_t hp, int32_t mp, const Pos &origin, const Pos &lt, const Pos &rb);
	void statusMobs(const vector<StatusInfo> &statuses, const Pos &origin, const Pos &lt, const Pos &rb);

	// Reactors
	void addReactorSpawn(ReactorSpawnInfo spawn);
	void addReactor(Reactor *reactor);
	void addReactorRespawn(ReactorRespawnInfo respawn);
	void checkReactorSpawn(clock_t time);
	Reactor * getReactor(int32_t id) {
		if ((uint32_t)id < this->reactors.size())
			return this->reactors[id];
		return 0;
	}
	size_t getNumReactors() const { return this->reactors.size(); }

	// Drops
	void addDrop(Drop *drop);
	Drop * getDrop(int32_t id) {
		boost::recursive_mutex::scoped_lock l(drops_mutex);
		return (this->drops.find(id) != this->drops.end() ? this->drops[id] : 0);
	}
	void removeDrop(int32_t id) {
		boost::recursive_mutex::scoped_lock l(drops_mutex);
		if (drops.find(id) != drops.end())
			drops.erase(id);
	}
	void clearDrops(bool showPacket = true);
	void clearDrops(clock_t time);

	// Timer stuff
	void runTimer();
	void setMapTimer(int32_t t);

	// Show all map objects
	void showObjects(Player *player);

	// Packet stuff
	void sendPacket(PacketCreator &packet, Player *player = 0);
	void showMessage(string &message, int8_t type);

	// Instance
	void setInstance(Instance *instance) { this->instance = instance; }
	Instance * getInstance() const { return instance; }
private:
	MapInfoPtr info;
	FootholdsInfo footholds;
	PortalsInfo portals;
	SpawnPoints spawnpoints;
	vector<Player *> players;
	NPCSpawnsInfo npcs;
	ReactorSpawnsInfo reactorspawns;
	vector<Reactor *> reactors;
	ReactorRespawns reactorrespawns;
	MobSpawnsInfo mobspawns;
	MobRespawnsInfo mobrespawns;
	unordered_map<int32_t, Mob *> mobs;
	unordered_map<int32_t, Drop *> drops;
	boost::recursive_mutex drops_mutex;
	LoopingId objectids;
	Instance *instance;
	int32_t timer;
	time_t timerstart;

	void updateMobControl(Player *player);
	void updateMobControl(Mob *mob, bool spawn = false);
};

#endif
