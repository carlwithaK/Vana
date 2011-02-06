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
#ifndef PLAYERS_H
#define PLAYERS_H

#include "Types.h"
#include <boost/tr1/unordered_map.hpp>
#include <boost/utility.hpp>
#include <string>

using std::string;
using std::tr1::unordered_map;

struct Player {
	uint32_t ip;
	int32_t id;
	string name;
	int16_t channel;
	int32_t party;
	int32_t map;
	int32_t job;
	int32_t level;
	bool online;
};

class Players : boost::noncopyable {
public:
	static Players * Instance() {
		if (singleton == 0)
			singleton = new Players;
		return singleton;
	}
	void registerPlayer(uint32_t ip, int32_t id, const string &name, uint16_t channel, int32_t map, int32_t job, int32_t level);
	void remove(int32_t id, int16_t channel = -1);
	void removeChannelPlayers(uint16_t channel);
	Player * getPlayerFromName(const string &name, bool includeOffline = false);
	Player * getPlayer(int32_t id, bool includeOffline = false);
	int32_t size();
private:
	Players() {};
	static Players *singleton;

	unordered_map<int32_t, Player *> players;
};

#endif
