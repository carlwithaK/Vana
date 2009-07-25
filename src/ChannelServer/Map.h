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
#include "Mob.h"
#include "Pos.h"
#include <boost/scoped_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/tr1/memory.hpp>
#include <boost/tr1/unordered_map.hpp>
#include <ctime>
#include <vector>
#include <string>

using std::string;
using std::vector;
using std::tr1::shared_ptr;
using std::tr1::unordered_map;

class Drop;
class Instance;
class Mist;
class Mob;
class PacketCreator;
class Player;
class Reactor;
namespace Timer {
	class Container;
};

struct MapInfo {
	MapInfo() : musicname(""), top(0), left(0), right(0), bottom(0) {}
	bool clock;
	bool town;
	int8_t fieldType;
	int8_t continent;
	int8_t starthour;
	int8_t endhour;
	int16_t left;
	int16_t top;
	int16_t bottom;
	int16_t right;
	int32_t id;
	int32_t rm;
	int32_t forcedReturn;
	int32_t shipInterval;
	int32_t fieldLimit;
	int32_t link;
	int32_t timemob;
	double spawnrate;
	string musicname;
	string message;
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
typedef unordered_map<int8_t, PortalInfo> SpawnPoints;

struct NPCSpawnInfo {
	int32_t id;
	Pos pos;
	int16_t fh;
	int16_t rx0;
	int16_t rx1;
	int8_t facingside;
};
typedef vector<NPCSpawnInfo> NPCSpawnsInfo;

struct ReactorSpawnInfo {
	ReactorSpawnInfo() : spawnat(-1), spawned(false) { }
	int32_t id;
	int32_t time;
	int32_t link;
	clock_t spawnat;
	bool spawned;
	Pos pos;
};
typedef vector<ReactorSpawnInfo> ReactorSpawnsInfo;

struct SeatInfo {
	Pos pos;
	Player *occupant;
};

typedef std::map<int16_t, SeatInfo> SeatsInfo;

struct MobSpawnInfo {
	MobSpawnInfo() : spawnat(-1), spawned(false) { }
	int8_t facingside;
	int16_t fh;
	int32_t id;
	int32_t time;
	int32_t link;
	bool spawned;
	clock_t spawnat;
	Pos pos;
};
typedef vector<MobSpawnInfo> MobSpawnsInfo;

class Map {
public:
	Map(MapInfoPtr info);
	// Map Info
	MapInfoPtr getInfo() const { return info; }
	void setMusic(const string &musicname);

	// Footholds
	void addFoothold(const FootholdInfo &foothold) { footholds.push_back(foothold); }
	Pos findFloor(const Pos &pos);
	Pos findRandomPos();
	int16_t getFhAtPosition(const Pos &pos);

	// Seats
	void addSeat(int16_t id, const SeatInfo &seat) { seats[id] = seat; }
	bool seatOccupied(int16_t id);
	void playerSeated(int16_t id, Player *player);

	// Portals
	void addPortal(const PortalInfo &portal);
	PortalInfo * getPortal(const string &name);
	PortalInfo * getSpawnPoint(int8_t pid = -1);
	PortalInfo * getNearestSpawnPoint(const Pos &pos);

	// Players
	void addPlayer(Player *player);
	size_t getNumPlayers() const { return this->players.size(); }
	Player * getPlayer(uint32_t i) const { return this->players[i]; }
	void removePlayer(Player *player);
	void dispelPlayers(int16_t prop, const Pos &origin, const Pos &lt, const Pos &rb);
	void statusPlayers(uint8_t status, uint8_t level, int16_t count, int16_t prop, const Pos &origin, const Pos &lt, const Pos &rb);
	void sendPlayersToTown(int32_t mobid, int16_t prop, int16_t count, const Pos &origin, const Pos &lt, const Pos &rb);
	void buffPlayers(int32_t buffid);

	// NPCs
	int32_t addNPC(const NPCSpawnInfo &npc);
	void removeNPC(int32_t index);
	bool isValidNpcIndex(uint32_t id) { return (id < npcs.size()); }
	NPCSpawnInfo getNpc(int32_t id) const { return this->npcs[id]; }

	// Mobs
	void setWebbedCount(int32_t w) { webbed = w; }
	void addMobSpawn(const MobSpawnInfo &spawn);
	void checkMobSpawn(clock_t time, bool spawnAll = false);
	void removeMob(int32_t id, int32_t spawnid);
	void healMobs(int32_t hp, int32_t mp, const Pos &origin, const Pos &lt, const Pos &rb);
	void statusMobs(vector<StatusInfo> &statuses, const Pos &origin, const Pos &lt, const Pos &rb);
	void checkShadowWeb();
	void setMobSpawning(int32_t spawn) { spawnmobs = spawn; }
	void spawnZakum(const Pos &pos, int16_t fh = 0);
	void updateMobControl(Mob *mob, bool spawn = false, Player *display = 0);
	int32_t getWebbedCount() const { return webbed; }
	int32_t spawnShell(int32_t mobid, const Pos &pos, int16_t fh);
	int32_t spawnMob(int32_t mobid, const Pos &pos, int16_t fh = 0, Mob *owner = 0, int8_t summoneffect = 0);
	int32_t spawnMob(int32_t spawnid, const MobSpawnInfo &info);
	int32_t killMobs(Player *player, int32_t mobid = 0, bool playerkill = true, bool showpacket = true);
	int32_t countMobs(int32_t mobid = 0);
	Mob * getMob(int32_t id, bool isMapId = true);
	unordered_map<int32_t, Mob *> getMobs() const { return mobs; }

	// Reactors
	void addReactorSpawn(const ReactorSpawnInfo &spawn);
	void addReactor(Reactor *reactor);
	void removeReactor(int32_t id);
	void killReactors(bool showpacket = true);
	void checkReactorSpawn(clock_t time, bool spawnAll = false);
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

	// Mists
	void addMist(Mist *mist);
	void checkMists();
	void removeMist(int32_t id);
	void clearMists(bool showPacket = true);
	Mist * getMist(int32_t id);
	int32_t getPoisonMistCount();

	// Timer stuff
	void runTimer();
	void timeMob(bool firstLoad = true);
	void setMapTimer(int32_t t);
	Timer::Container * getTimers() const { return timers.get(); }

	// Show all map objects
	void showObjects(Player *player);

	// Packet stuff
	void sendPacket(PacketCreator &packet, Player *player = 0);
	void showMessage(const string &message, int8_t type);

	// Instance
	void setInstance(Instance *instance) { this->instance = instance; }
	Instance * getInstance() const { return instance; }
private:
	MapInfoPtr info;
	FootholdsInfo footholds;
	PortalsInfo portals;
	SpawnPoints spawnpoints;
	NPCSpawnsInfo npcs;
	ReactorSpawnsInfo reactorspawns;
	MobSpawnsInfo mobspawns;
	SeatsInfo seats;
	vector<Player *> players;
	vector<Reactor *> reactors;
	unordered_map<int32_t, Mob *> mobs;
	unordered_map<int32_t, Drop *> drops;
	unordered_map<int32_t, Mist *> mists;
	boost::recursive_mutex drops_mutex;
	boost::scoped_ptr<Timer::Container> timers;
	LoopingId objectids;
	Instance *instance;
	time_t timerstart;
	int32_t timer;
	int32_t poisonmists;
	int32_t webbed;
	int32_t timemob;
	int32_t spawnmobs;

	void updateMobControl(Player *player);
	int32_t getTimeMobId() const { return timemob; }
};

#endif
