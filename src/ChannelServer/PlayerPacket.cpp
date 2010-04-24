/*
Copyright (C) 2008-2010 Vana Development Team

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
#include "PlayerPacket.h"
#include "ChannelServer.h"
#include "InterHeader.h"
#include "KeyMaps.h"
#include "MapleSession.h"
#include "PacketCreator.h"
#include "Pet.h"
#include "Player.h"
#include "PlayerDataProvider.h"
#include "PlayerPacketHelper.h"
#include "SendHeader.h"
#include "SkillMacros.h"
#include "Skills.h"
#include "TimeUtilities.h"
#include <boost/tr1/unordered_map.hpp>

using std::tr1::unordered_map;

void PlayerPacket::connectData(Player *player) {
	PacketCreator packet;
	packet.add<int16_t>(SMSG_CHANGE_MAP);
	packet.add<int32_t>(ChannelServer::Instance()->getChannel()); // Channel
	packet.add<uint32_t>(player->getPortalCount(true));

	player->getRandStream()->connectData(packet); // Seeding RNG

	packet.add<int64_t>(-1);
	packet.add<int8_t>(0);
	packet.add<int32_t>(player->getId());
	packet.addString(player->getName(), 13);
	packet.add<int8_t>(player->getGender());
	packet.add<int8_t>(player->getSkin());
	packet.add<int32_t>(player->getEyes());
	packet.add<int32_t>(player->getHair());

	packet.add<int64_t>(0); // Pet ID's (Used in the cashshop window)
	packet.add<int64_t>(0);
	packet.add<int64_t>(0);

	player->getStats()->connectData(packet); // Stats

	packet.add<int32_t>(0); // Gachapon EXP

	packet.add<int32_t>(player->getMap());
	packet.add<int8_t>(player->getMappos());

	packet.add<int32_t>(0); // Unknown int32 added in .62

	packet.add<uint8_t>(player->getBuddyListSize());

	if (!player->getSkills()->getBoFName().empty()) {
		packet.addBool(true);
		packet.addString(player->getSkills()->getBoFName());
	}
	else {
		packet.addBool(false);
	}

	player->getInventory()->connectData(packet); // Inventory data
	player->getSkills()->connectData(packet); // Skills - levels and cooldowns
	player->getQuests()->connectData(packet); // Quests

	packet.add<int32_t>(0);
	packet.add<int32_t>(0);

	player->getInventory()->rockPacket(packet); // Teleport Rock/VIP Rock maps
	player->getMonsterBook()->connectData(packet);

	packet.add<int16_t>(0);

	// Party Quest data (quest needs to be added in the quests list)
	packet.add<int16_t>(0); // Amount of pquests
	// for every pquest: int16_t questid, string questdata

	packet.add<int16_t>(0);

	packet.add<int64_t>(TimeUtilities::getServerTime());
	player->getSession()->send(packet);
}

void PlayerPacket::showKeys(Player *player, KeyMaps *keyMaps) {
	PacketCreator packet;
	packet.add<int16_t>(SMSG_KEYMAP);
	packet.add<int8_t>(0);
	for (size_t i = 0; i < KeyMaps::size; i++) {
		KeyMaps::KeyMap *keyMap = keyMaps->getKeyMap(i);
		if (keyMap != nullptr) {
			packet.add<int8_t>(keyMap->type);
			packet.add<int32_t>(keyMap->action);
		}
		else {
			packet.add<int8_t>(0);
			packet.add<int32_t>(0);
		}
	}
	player->getSession()->send(packet);
}

void PlayerPacket::showSkillMacros(Player *player, SkillMacros *macros) {
	PacketCreator packet;
	packet.add<int16_t>(SMSG_MACRO_LIST);
	packet.add<int8_t>(macros->getMax() + 1);
	for (int8_t i = 0; i <= macros->getMax(); i++) {
		SkillMacros::SkillMacro *macro = macros->getSkillMacro(i);
		if (macro != nullptr) {
			packet.addString(macro->name);
			packet.addBool(macro->shout);
			packet.add<int32_t>(macro->skill1);
			packet.add<int32_t>(macro->skill2);
			packet.add<int32_t>(macro->skill3);
		}
		else {
			packet.addString("");
			packet.addBool(false);
			packet.add<int32_t>(0);
			packet.add<int32_t>(0);
			packet.add<int32_t>(0);
		}
	}

	player->getSession()->send(packet);
}

void PlayerPacket::updateStatInt(Player *player, int32_t id, int32_t value, bool is) {
	PacketCreator packet;
	packet.add<int16_t>(SMSG_PLAYER_UPDATE);
	packet.addBool(is);
	packet.add<int32_t>(id);
	packet.add<int32_t>(value);
	player->getSession()->send(packet);
}

void PlayerPacket::updateStatShort(Player *player, int32_t id, int16_t value, bool is) {
	PacketCreator packet;
	packet.add<int16_t>(SMSG_PLAYER_UPDATE);
	packet.addBool(is);
	packet.add<int32_t>(id);
	packet.add<int16_t>(value);
	player->getSession()->send(packet);
}

void PlayerPacket::updateStatChar(Player *player, int32_t id, int8_t value, bool is) {
	PacketCreator packet;
	packet.add<int16_t>(SMSG_PLAYER_UPDATE);
	packet.addBool(is);
	packet.add<int32_t>(id);
	packet.add<int8_t>(value);
	player->getSession()->send(packet);
}

void PlayerPacket::changeChannel(Player *player, uint32_t ip, int16_t port) {
	PacketCreator packet;
	packet.add<int16_t>(SMSG_CHANNEL_CHANGE);
	packet.add<int8_t>(1);
	packet.add<uint32_t>(htonl(ip)); // MapleStory accepts IP addresses in big-endian
	packet.add<int16_t>(port);
	player->getSession()->send(packet);
}

void PlayerPacket::showMessage(Player *player, const string &msg, int8_t type) {
	PacketCreator packet;
	showMessagePacket(packet, msg, type);
	player->getSession()->send(packet);
}

void PlayerPacket::showMessageChannel(const string &msg, int8_t type) {
	PacketCreator packet;
	showMessagePacket(packet, msg, type);
	PlayerDataProvider::Instance()->sendPacket(packet);
}

void PlayerPacket::showMessageWorld(const string &msg, int8_t type) {
	PacketCreator packet;
	packet.add<int16_t>(IMSG_TO_CHANNELS);
	packet.add<int16_t>(IMSG_TO_PLAYERS);
	showMessagePacket(packet, msg, type);
	ChannelServer::Instance()->sendToWorld(packet);
}

void PlayerPacket::showMessageGlobal(const string &msg, int8_t type) {
	PacketCreator packet;
	packet.add<int16_t>(IMSG_TO_LOGIN);
	packet.add<int16_t>(IMSG_TO_WORLDS);
	packet.add<int16_t>(IMSG_TO_CHANNELS);
	packet.add<int16_t>(IMSG_TO_PLAYERS);
	showMessagePacket(packet, msg, type);
	ChannelServer::Instance()->sendToWorld(packet);
}

void PlayerPacket::showMessagePacket(PacketCreator &packet, const string &msg, int8_t type) {
	packet.add<int16_t>(SMSG_MESSAGE);
	packet.add<int8_t>(type);
	packet.addString(msg);
	if (type == NoticeTypes::Blue) {
		packet.add<int32_t>(0);
	}
}

void PlayerPacket::instructionBubble(Player *player, const string &msg, int16_t width, int16_t height) {
	if (width == -1) {
		width = msg.size() * 10;
		if (width < 40) {
			width = 40; // Anything lower crashes client/doesn't look good
		}
	}

	PacketCreator packet;
	packet.add<int16_t>(SMSG_BUBBLE);
	packet.addString(msg);
	packet.add<int16_t>(width);
	packet.add<int16_t>(height);
	packet.add<int8_t>(1);

	player->getSession()->send(packet);
}

void PlayerPacket::showHpBar(Player *player, Player *target) {
	PacketCreator packet;
	packet.add<int16_t>(SMSG_PARTY_HP_DISPLAY);
	packet.add<int32_t>(player->getId());
	packet.add<int32_t>(player->getStats()->getHp());
	packet.add<int32_t>(player->getStats()->getMaxHp());
	target->getSession()->send(packet);
}

void PlayerPacket::sendBlockedMessage(Player *player, int8_t type) {
	PacketCreator packet;
	packet.add<int16_t>(SMSG_CHANNEL_BLOCKED);
	packet.add<int8_t>(type);
	player->getSession()->send(packet);
}
