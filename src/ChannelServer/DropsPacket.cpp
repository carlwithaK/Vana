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
#include "DropsPacket.h"
#include "Drops.h"
#include "GameConstants.h"
#include "GameLogicUtilities.h"
#include "MapleSession.h"
#include "Maps.h"
#include "PacketCreator.h"
#include "Player.h"
#include "SendHeader.h"

void DropsPacket::showDrop(Player *player, Drop *drop, int8_t type, bool newdrop, Pos origin) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_DROP_ITEM);
	packet.add<int8_t>(type); // 3 = disappear during drop animation, 2 = show existing, 1 then 0 = show new
	packet.add<int32_t>(drop->getId());
	packet.add<int8_t>(drop->isMesos());
	packet.add<int32_t>(drop->getObjectId());
	packet.add<int32_t>(drop->getOwner()); // Owner of drop
	packet.add<int8_t>(0);
	packet.addPos(drop->getPos());
	packet.add<int32_t>(drop->getTime());
	if (type == 0 || type == 1 || type == 3) { // Give the point of origin for things that are just being dropped
		packet.addPos(origin);
		packet.add<int16_t>(0);
	}
	if (!drop->isMesos()) {
		packet.addBytes("008005BB46E61702");
	}
	packet.add<int8_t>(!drop->isplayerDrop()); // Determines whether pets can pick item up or not
	if (player != 0)
		player->getSession()->send(packet);
	else
		Maps::getMap(drop->getMap())->sendPacket(packet);

	if (newdrop) {
		showDrop(player, drop, 0, false, origin);
	}
}

void DropsPacket::takeNote(Player *player, int32_t id, bool ismesos, int16_t amount) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_NOTE);
	packet.add<int8_t>(0);
	if (id == 0)
		packet.add<int8_t>(-1);
	else {
		packet.add<int8_t>(ismesos);
		packet.add<int32_t>(id);
		if (ismesos) {
			packet.add<int16_t>(0); // Internet Cafe Bonus
		}
		else if (GameLogicUtilities::getInventory(id) != Inventories::EquipInventory)
			packet.add<int16_t>(amount);
	}
	if (!ismesos) {
		packet.add<int32_t>(0);
		packet.add<int32_t>(0);
	}
	player->getSession()->send(packet);
}

void DropsPacket::takeDrop(Player *player, Drop *drop, int8_t pet_index) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_TAKE_DROP);
	packet.add<int8_t>(pet_index != -1 ? 5 : 2);
	packet.add<int32_t>(drop->getId());
	packet.add<int32_t>(player->getId());
	if (pet_index != -1)
		packet.add<int8_t>(pet_index);
	if (!drop->isQuest())
		Maps::getMap(drop->getMap())->sendPacket(packet);
	else
		player->getSession()->send(packet);
}

void DropsPacket::dontTake(Player *player) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_MOVE_ITEM);
	packet.add<int16_t>(1);
	player->getSession()->send(packet);
}

void DropsPacket::removeDrop(Drop *drop) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_TAKE_DROP);
	packet.add<int8_t>(0);
	packet.add<int32_t>(drop->getId());
	Maps::getMap(drop->getMap())->sendPacket(packet);
}

void DropsPacket::explodeDrop(Drop *drop) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_TAKE_DROP);
	packet.add<int8_t>(4);
	packet.add<int32_t>(drop->getId());
	packet.add<int16_t>(655);
	Maps::getMap(drop->getMap())->sendPacket(packet);
}