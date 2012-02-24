/*
Copyright (C) 2008-2012 Vana Development Team

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
#include "PlayerDataProvider.h"
#include "ChannelServer.h"
#include "Connectable.h"
#include "Database.h"
#include "InterHelper.h"
#include "PacketCreator.h"
#include "PacketReader.h"
#include "Party.h"
#include "Player.h"
#include "PlayerPacket.h"
#include "Session.h"
#include "SyncPacket.h"
#include <algorithm>
#include <cstring>

PlayerDataProvider * PlayerDataProvider::singleton = nullptr;

void PlayerDataProvider::parseChannelConnectPacket(PacketReader &packet) {
	// Players
	int32_t quantity = packet.get<int32_t>();
	int32_t i;
	PlayerData *player;
	for (i = 0; i < quantity; i++) {
		player = new PlayerData;
		player->gmLevel = packet.get<int32_t>();
		player->admin = packet.getBool();
		player->level = packet.get<uint8_t>();
		player->job = packet.get<int16_t>();
		player->channel = packet.get<int16_t>();
		player->map = packet.get<int32_t>();
		player->party = packet.get<int32_t>();
	}

	// Parties
	quantity = packet.get<int32_t>();
	int8_t members, j;
	Party *party;
	for (i = 0; i < quantity; i++) {
		party = new Party(packet.get<int32_t>());
		party->setLeader(packet.get<int32_t>());
		members = packet.get<int8_t>();

		for (j = 0; j < members; j++) {
			party->addMember(packet.get<int32_t>());
		}
		m_parties[party->getId()].reset(party);
	}
}

// Players
void PlayerDataProvider::addPlayer(Player *player) {
	m_players[player->getId()] = player;
	string upper = StringUtilities::toUpper(player->getName());
	m_playersByName[upper] = player; // Store in upper case for easy non-case-sensitive search
}

void PlayerDataProvider::removePlayer(Player *player) {
	m_players.erase(player->getId());
	string upper = StringUtilities::toUpper(player->getName());
	m_playersByName.erase(upper);
}

void PlayerDataProvider::changeChannel(PacketReader &packet) {
	int32_t playerId = packet.get<int32_t>();
	ip_t ip = packet.get<ip_t>();
	port_t port = packet.get<port_t>();

	if (Player *player = getPlayer(playerId)) {
		if (ip == 0) {
			PlayerPacket::sendBlockedMessage(player, PlayerPacket::BlockMessages::CannotGo);
		}
		else {
			player->setOnline(false); // Set online to false BEFORE CC packet is sent to player
			PlayerPacket::changeChannel(player, ip, port);
			player->saveAll(true);
			player->setSaveOnDc(false);
		}
	}
}

void PlayerDataProvider::newConnectable(PacketReader &packet) {
	int32_t playerId = packet.get<int32_t>();
	ip_t ip = packet.get<ip_t>();
	Connectable::Instance()->newPlayer(playerId, ip, packet);
	SyncPacket::PlayerPacket::connectableEstablished(playerId);
}

void PlayerDataProvider::deleteConnectable(int32_t id) {
	Connectable::Instance()->playerEstablished(id);
}

Player * PlayerDataProvider::getPlayer(int32_t id) {
	return (m_players.find(id) == m_players.end()) ? nullptr : m_players[id];
}

Player * PlayerDataProvider::getPlayer(const string &name) {
	string upper = StringUtilities::toUpper(name);
	return (m_playersByName.find(upper) == m_playersByName.end()) ? nullptr : m_playersByName[upper];
}

void PlayerDataProvider::run(function<void (Player *)> func) {
	for (unordered_map<int32_t, Player *>::iterator iter = m_players.begin(); iter != m_players.end(); ++iter) {
		func(iter->second);
	}
}

void PlayerDataProvider::sendPacket(PacketCreator &packet, int32_t minGmLevel) {
	for (unordered_map<int32_t, Player *>::iterator iter = m_players.begin(); iter != m_players.end(); ++iter) {
		Player *p = iter->second;
		if (p->getGmLevel() >= minGmLevel) {
			p->getSession()->send(packet);
		}
	}
}

void PlayerDataProvider::updatePlayer(PacketReader &packet) {
	int32_t playerId = packet.get<int32_t>();
	if (PlayerData *player = getPlayerData(playerId)) {
		int8_t updateBits = packet.get<int8_t>();
		bool updateParty = false;
		bool updateGuild = false;
		bool updateAlliance = false;
		if (updateBits & Sync::Player::UpdateBits::Job) {
			player->job = packet.get<int16_t>();
			updateParty = true;
			updateGuild = true;
			updateAlliance = true;
		}
		if (updateBits & Sync::Player::UpdateBits::Level) {
			player->level = packet.get<uint8_t>();
			updateParty = true;
			updateGuild = true;
			updateAlliance = true;
		}
		if (updateBits & Sync::Player::UpdateBits::Map) {
			player->map = packet.get<int32_t>();
			updateParty = true;
		}

		if (updateParty && player->party != 0) {
			getParty(player->party)->silentUpdate();
		}
	}
}

PlayerData * PlayerDataProvider::getPlayerData(int32_t id) {
	return (m_playerData.find(id) != m_playerData.end() ? m_playerData[id].get() : nullptr);
}

// Parties
Party * PlayerDataProvider::getParty(int32_t id) {
	return (m_parties.find(id) == m_parties.end() ? nullptr : m_parties[id].get());
}

void PlayerDataProvider::newParty(int32_t id, int32_t leaderId) {
	Party *party = new Party(id);
	Player *leader = getPlayer(leaderId);
	if (leader == 0) {
		party->addMember(leaderId);
	}
	else {
		party->addMember(leader);
	}
	party->setLeader(leaderId);
	std::shared_ptr<Party> p(party);
	m_parties[id] = p;
}

void PlayerDataProvider::disbandParty(int32_t id) {
	if (Party *party = getParty(id)) {
		party->disband();
		m_parties.erase(id);
	}
}

void PlayerDataProvider::switchPartyLeader(int32_t id, int32_t leaderId) {
	if (Party *party = getParty(id)) {
		party->setLeader(leaderId);
	}
}

void PlayerDataProvider::removePartyMember(int32_t id, int32_t playerId, bool kicked) {
	if (Party *party = getParty(id)) {
		if (Player *member = getPlayer(playerId)) {
			party->deleteMember(member, kicked);
		}
		else {
			party->deleteMember(playerId, kicked);
		}
	}
}

void PlayerDataProvider::addPartyMember(int32_t id, int32_t playerId) {
	if (Party *party = getParty(id)) {
		if (Player *member = getPlayer(playerId)) {
			party->addMember(member);
		}
		else {
			party->addMember(playerId);
		}
	}
}