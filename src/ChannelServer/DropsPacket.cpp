/*
Copyright (C) 2008-2013 Vana Development Team

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
#include "Drop.h"
#include "GameLogicUtilities.h"
#include "ItemConstants.h"
#include "Maps.h"
#include "PacketCreator.h"
#include "Player.h"
#include "Session.h"
#include "SmsgHeader.h"

void DropsPacket::showDrop(Player *player, Drop *drop, int8_t type, bool newDrop, const Pos &origin) {
	PacketCreator packet;
	packet.add<header_t>(SMSG_DROP_ITEM);
	packet.add<int8_t>(type);
	packet.add<int32_t>(drop->getId());
	packet.add<bool>(drop->isMesos());
	packet.add<int32_t>(drop->getObjectId());
	packet.add<int32_t>(drop->getOwner()); // Owner of drop
	packet.add<int8_t>(drop->getType());
	packet.addClass<Pos>(drop->getPos());
	packet.add<int32_t>(drop->getTime());
	if (type != DropTypes::ShowExisting) {
		// Give the point of origin for things that are just being dropped
		packet.addClass<Pos>(origin);
		packet.add<int16_t>(0);
	}
	if (!drop->isMesos()) {
		packet.add<int64_t>(Items::NoExpiration);
	}
	packet.add<bool>(!drop->isplayerDrop()); // Determines whether pets can pick item up or not

	if (player != nullptr) {
		player->getSession()->send(packet);
	}
	else {
		Maps::getMap(drop->getMap())->sendPacket(packet);
	}
	if (newDrop) {
		showDrop(player, drop, DropTypes::ShowDrop, false, origin);
	}
}

void DropsPacket::takeDrop(Player *player, Drop *drop, int8_t petIndex) {
	PacketCreator packet;
	packet.add<header_t>(SMSG_DROP_PICKUP);
	packet.add<int8_t>(petIndex != -1 ? 5 : 2);
	packet.add<int32_t>(drop->getId());
	packet.add<int32_t>(player->getId());
	if (petIndex != -1) {
		packet.add<int8_t>(petIndex);
	}
	if (!drop->isQuest()) {
		Maps::getMap(drop->getMap())->sendPacket(packet);
	}
	else {
		player->getSession()->send(packet);
	}
}

void DropsPacket::dontTake(Player *player) {
	PacketCreator packet;
	packet.add<header_t>(SMSG_INVENTORY_ITEM_MOVE);
	packet.add<int16_t>(1);
	player->getSession()->send(packet);
}

void DropsPacket::removeDrop(Drop *drop) {
	PacketCreator packet;
	packet.add<header_t>(SMSG_DROP_PICKUP);
	packet.add<int8_t>(0);
	packet.add<int32_t>(drop->getId());
	Maps::getMap(drop->getMap())->sendPacket(packet);
}

void DropsPacket::explodeDrop(Drop *drop) {
	PacketCreator packet;
	packet.add<header_t>(SMSG_DROP_PICKUP);
	packet.add<int8_t>(4);
	packet.add<int32_t>(drop->getId());
	packet.add<int16_t>(655);
	Maps::getMap(drop->getMap())->sendPacket(packet);
}

void DropsPacket::dropNotAvailableForPickup(Player *player) {
	PacketCreator packet;
	packet.add<header_t>(SMSG_NOTICE);
	packet.add<int8_t>(0);
	packet.add<int8_t>(-2);
	player->getSession()->send(packet);
}

void DropsPacket::cantGetAnymoreItems(Player *player) {
	PacketCreator packet;
	packet.add<header_t>(SMSG_NOTICE);
	packet.add<int8_t>(0);
	packet.add<int8_t>(-1);
	player->getSession()->send(packet);
}

void DropsPacket::pickupDrop(Player *player, int32_t id, int32_t amount, bool isMesos, int16_t cafeBonus) {
	PacketCreator packet;
	packet.add<header_t>(SMSG_NOTICE);
	packet.add<int8_t>(0);
	packet.add<bool>(isMesos);
	packet.add<int32_t>(id);

	if (isMesos) {
		packet.add<int16_t>(cafeBonus);
	}
	else if (GameLogicUtilities::getInventory(id) != Inventories::EquipInventory) {
		packet.add<int16_t>(static_cast<int16_t>(amount));
	}
	if (!isMesos) {
		packet.add<int32_t>(0);
		packet.add<int32_t>(0);
	}

	player->getSession()->send(packet);
}

void DropsPacket::pickupDropSpecial(Player *player, int32_t id) {
	// This packet is used for PQ drops (maybe, got it from the Wing of the Wind item) and monster cards
	PacketCreator packet;
	packet.add<header_t>(SMSG_NOTICE);
	packet.add<int8_t>(0);
	packet.add<int8_t>(2);
	packet.add<int32_t>(id);
	player->getSession()->send(packet);
}