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
#include "Instance.h"
#include "Instances.h"
#include "LuaInstance.h"
#include "MapPacket.h"
#include "Map.h"
#include "Maps.h"
#include "Party.h"
#include "Player.h"
#include "Reactors.h"
#include "Timer/Container.h"
#include "Timer/Time.h"
#include "Timer/Timer.h"
#include "TimeUtilities.h"
#include <functional>

using std::tr1::bind;

Instance::Instance(const string &name, int32_t map, int32_t playerid, int32_t time, int32_t persistent, bool showtimer) :
m_name(name),
m_max_players(0),
m_timer_counter(0),
m_persistent(persistent),
m_show_timer(showtimer),
m_timers(new Timer::Container),
m_variables(new Variables),
m_luainstance(new LuaInstance(name, playerid)),
m_start(TimeUtilities::getTickCount()),
m_reset_on_destroy(true),
m_marked_for_delete(false)
{
	setInstanceTimer(time);
}

Instance::~Instance() {
	// Maps
	for (size_t i = 0; i < getMapNum(); i++) {
		Map *map = m_maps[i];
		map->setInstance(0);
		map->clearDrops(false);
		map->killMobs(0, 0, false, false);
		map->killReactors(false);
		if (m_reset_on_destroy) { // Reset all mobs/reactors
			map->checkReactorSpawn(0, true);
			map->checkMobSpawn(0, true);
		}
	}
	m_maps.clear();

	// Parties
	for (size_t k = 0; k < m_parties.size(); k++) {
		Party *p = m_parties[k];
		if (p != 0) {
			p->setInstance(0);
		}
	}
	m_parties.clear();

	// Players
	for (unordered_map<int32_t, Player *>::iterator iter = m_players.begin(); iter != m_players.end(); iter++) {
		iter->second->setInstance(0);
	}
	m_players.clear();
	Instances::InstancePtr()->removeInstance(this);
}

void Instance::setBanned(const string &name, bool isbanned) {
	if (isbanned) {
		m_banned.push_back(name);
		removePlayerSignUp(name);
	}
	else {
		for (size_t i = 0; i < m_banned.size(); i++) {
			if (m_banned[i] == name) {
				m_banned.erase(m_banned.begin() + i);
				break;
			}
		}
	}
}

bool Instance::isBanned(const string &name) {
	for (size_t i = 0; i < m_banned.size(); i++) {
		if (m_banned[i] == name) {
			return true;
		}
	}
	return false;
}

void Instance::addPlayer(Player *player) {
	m_players[player->getId()] = player;
	player->setInstance(this);
}

void Instance::removePlayer(Player *player) {
	removePlayer(player->getId());
	player->setInstance(0);
}

void Instance::removePlayer(int32_t id) {
	if (m_players.find(id) != m_players.end()) {
		m_players.erase(id);
	}
}

void Instance::removeAllPlayers() {
	unordered_map<int32_t, Player *> temp = m_players;
	for (unordered_map<int32_t, Player *>::iterator iter = temp.begin(); iter != temp.end(); iter++) {
		removePlayer(iter->second);
	}
}

void Instance::addPlayerSignUp(Player *player) {
	m_players_order.push_back(player->getName());
}

void Instance::removePlayerSignUp(const string &name) {
	for (size_t i = 0; i < m_players_order.size(); i++) {
		if (m_players_order[i] == name) {
			m_players_order.erase(m_players_order.begin() + i);
		}
	}
}

void Instance::moveAllPlayers(int32_t mapid, PortalInfo *portal) {
	if (!Maps::getMap(mapid))
		return;
	for (unordered_map<int32_t, Player *>::iterator iter = m_players.begin(); iter != m_players.end(); iter++) {
		iter->second->setMap(mapid, portal);
	}
}

bool Instance::isPlayerSignedUp(const string &name) {
	for (size_t i = 0; i < m_players_order.size(); i++) {
		if (m_players_order[i] == name) {
			return true;
		}
	}
	return false;
}

vector<int32_t> Instance::getAllPlayerIds() {
	vector<int32_t> playerids;
	for (unordered_map<int32_t, Player *>::iterator iter = m_players.begin(); iter != m_players.end(); iter++) {
		playerids.push_back(iter->first);
	}
	return playerids;
}

const string Instance::getPlayerByIndex(uint32_t index) const {
	index--;
	return m_players_order[(index > m_players_order.size() ? m_players_order.size() : index)];
}

bool Instance::instanceHasPlayers() const {
	for (size_t i = 0; i < m_maps.size(); i++) {
		if (m_maps[i]->getNumPlayers() != 0) {
			return true;
		}
	}
	return false;
}

void Instance::addMap(Map *map) {
	m_maps.push_back(map);
	map->setInstance(this);
}

void Instance::addMap(int32_t mapid) {
	Map *map = Maps::getMap(mapid);
	addMap(map);
}

Map * Instance::getMap(int32_t mapid) {
	Map *map = 0;
	for (size_t i = 0; i < getMapNum(); i++) {
		Map *tmap = m_maps[i];
		if (tmap->getInfo()->id == mapid) {
			map = tmap;
			break;
		}
	}
	return map;
}

size_t Instance::getMapNum() {
	return m_maps.size();
}

void Instance::addParty(Party *party) {
	m_parties.push_back(party);
	party->setInstance(this);
}

bool Instance::addTimer(const string &timername, const TimerAction &timer) {
	if (m_timer_actions.find(timername) == m_timer_actions.end()) {
		m_timer_actions[timername] = timer;
		Timer::Id id(Timer::Types::InstanceTimer, timer.time, timer.counterid);
		if (timer.time > 0) { // Positive, occurs in the future
			new Timer::Timer(bind(&Instance::timerEnd, this, timername, true),
				id, getTimers(), Timer::Time::fromNow(timer.time * 1000));
		}
		else { // Negative, occurs nth second of hour
			new Timer::Timer(bind(&Instance::timerEnd, this, timername, true),
				id, getTimers(), Timer::Time::nthSecondOfHour(static_cast<uint16_t>(-(timer.time + 1))));
		}
		return true;
	}
	return false;
}

int32_t Instance::checkTimer(const string &timername) {
	int32_t timeleft = 0;
	if (m_timer_actions.find(timername) != m_timer_actions.end()) {
		TimerAction timer = m_timer_actions[timername];
		Timer::Id id(Timer::Types::InstanceTimer, timer.time, timer.counterid);
		timeleft = getTimers()->checkTimer(id);
	}
	return timeleft;
}

void Instance::removeTimer(const string &timername) {
	if (m_timer_actions.find(timername) != m_timer_actions.end()) {
		TimerAction timer = m_timer_actions[timername];
		if (checkTimer(timername) > 0) {
			Timer::Id id(Timer::Types::InstanceTimer, timer.time, timer.counterid);
			getTimers()->removeTimer(id);
			sendMessage(TimerEnd, timername, false);
		}
		m_timer_actions.erase(timername);
	}
}

void Instance::removeAllTimers() {
	for (unordered_map<string, TimerAction>::iterator iter = m_timer_actions.begin(); iter != m_timer_actions.end(); iter++) {
		removeTimer(iter->first);
	}
	setInstanceTimer(0);
}

int32_t Instance::checkInstanceTimer() {
	int32_t timeleft = 0;
	if (m_time > 0) {
		Timer::Id id(Timer::Types::InstanceTimer, m_time, -1);
		timeleft = getTimers()->checkTimer(id) / 1000;
	}
	return timeleft;
}

void Instance::setInstanceTimer(int32_t time) {
	if (checkInstanceTimer() > 0) {
		Timer::Id id(Timer::Types::InstanceTimer, m_time, -1);
		getTimers()->removeTimer(id);
	}
	if (time < 0) {
		m_time = -(time + 1);
		Timer::Id id(Timer::Types::InstanceTimer, m_time, -1);
		new Timer::Timer(bind(&Instance::instanceEnd, this, true),
			id, getTimers(), Timer::Time::nthSecondOfHour(static_cast<uint16_t>(m_time)), m_persistent * 1000);
	}
	else if (time > 0) {
		m_time = time * 1000;
		Timer::Id id(Timer::Types::InstanceTimer, m_time, -1);
		new Timer::Timer(bind(&Instance::instanceEnd, this, true),
			id, getTimers(), Timer::Time::fromNow(m_time), m_persistent * 1000);
	}
}

void Instance::sendMessage(InstanceMessages message) {
	getLuaInstance()->run(message);
}

void Instance::sendMessage(InstanceMessages message, int32_t parameter) {
	getLuaInstance()->run(message, parameter);
}

void Instance::sendMessage(InstanceMessages message, int32_t parameter1, int32_t parameter2) {
	getLuaInstance()->run(message, parameter1, parameter2);
}

void Instance::sendMessage(InstanceMessages message, int32_t parameter1, int32_t parameter2, int32_t parameter3) {
	getLuaInstance()->run(message, parameter1, parameter2, parameter3);
}

void Instance::sendMessage(InstanceMessages message, int32_t parameter1, int32_t parameter2, int32_t parameter3, int32_t parameter4) {
	getLuaInstance()->run(message, parameter1, parameter2, parameter3, parameter4);
}

void Instance::sendMessage(InstanceMessages message, const string &parameter1, int32_t parameter2) {
	getLuaInstance()->run(message, parameter1, parameter2);
}

void Instance::timerEnd(const string &name, bool fromTimer) {
	sendMessage(TimerNaturalEnd, name, fromTimer ? 1 : 0);
	removeTimer(name);
}

void Instance::instanceEnd(bool fromTimer) {
	sendMessage(InstanceTimerNaturalEnd, fromTimer ? 1 : 0);
	if (!getPersistence()) {
		markForDelete();
	}
}

int32_t Instance::getCounterId() {
	return ++m_timer_counter;
}

void Instance::markForDelete() {
	m_marked_for_delete = true;
	removeAllTimers();
}

void Instance::respawnMobs(int32_t mapid) {
	if (mapid == Maps::NoMap) {
		for (size_t i = 0; i < getMapNum(); i++) {
			m_maps[i]->checkMobSpawn(0, true);
		}
	}
	else {
		Maps::getMap(mapid)->checkMobSpawn(0, true);
	}
}

void Instance::respawnReactors(int32_t mapid) {
	if (mapid == Maps::NoMap) {
		for (size_t i = 0; i < getMapNum(); i++) {
			m_maps[i]->checkReactorSpawn(0, true);
		}
	}
	else {
		Maps::getMap(mapid)->checkReactorSpawn(0, true);
	}
}