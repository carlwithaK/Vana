/*
Copyright (C) 2008-2011 Vana Development Team

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
#pragma once

#include "GameObjects.h"
#include "Types.h"
#include <boost/tr1/unordered_map.hpp>
#include <string>

using std::string;
using std::tr1::unordered_map;

class PacketCreator;
class Party;
class Player;

class PlayerDataProvider : boost::noncopyable {
public:
	static PlayerDataProvider * Instance() {
		if (singleton == 0)
			singleton = new PlayerDataProvider;
		return singleton;
	}
	void loadData();
	void assignAndPrune(int16_t worldId);

	// Player info
	void registerPlayer(Player *player, bool online = true);
	void remove(int32_t id, int16_t channel = -1);
	void removeChannelPlayers(uint16_t channel);
	Player * getPlayer(const string &name, bool includeOffline = false);
	Player * getPlayer(int32_t id, bool includeOffline = false);
	int32_t getPlayerQuantity();

	// Channel changes
	void addPendingPlayer(int32_t id, uint16_t channelid);
	void removePendingPlayer(int32_t id);
	int16_t removePendingPlayerEarly(int32_t id);
	uint16_t getPendingPlayerChannel(int32_t id);

	// Parties
	int32_t getPartyId();
	void addParty(Party *party);
	void removeParty(int32_t id);
	Party * getParty(int32_t id);
	unordered_map<int32_t, Party *> getParties();
private:
	PlayerDataProvider() {};
	static PlayerDataProvider *singleton;

	void loadGuilds(int16_t worldId);
	void loadAlliances(int16_t worldId);
	void loadPlayers(int16_t worldId);

	int32_t pid; // For assigning party IDs
	unordered_map<int32_t, Player *> m_players; // Player info
	unordered_map<int32_t, uint16_t> m_channelSwitches; // Channel changes
	unordered_map<int32_t, Party *> m_parties;
};