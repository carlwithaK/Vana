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
#include "Pets.h"
#include "MapleSession.h"
#include "Maps.h"
#include "PacketCreator.h"
#include "PetsPacket.h"
#include "Player.h"
#include "PacketReader.h"
#include "SendHeader.h"

void PetsPacket::petSummoned(Player *player, Pet *pet, bool kick, bool onlyPlayer, int8_t index) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_PET_SUMMONED);
	packet.add<int32_t>(player->getId());
	packet.add<int8_t>(index != -1 ? index : pet->getIndex());
	packet.add<int8_t>(pet->isSummoned());
	packet.add<int8_t>(kick); // Kick existing pet (only when player doesn't have follow the lead)
	if (pet->isSummoned()) {
		packet.add<int32_t>(pet->getItemId());
		packet.addString(pet->getName());
		packet.add<int32_t>(pet->getId());
		packet.add<int32_t>(0);
		packet.addPos(pet->getPos());
		packet.add<int8_t>(pet->getStance());
		packet.add<int32_t>(pet->getFh());
	}
	onlyPlayer ? player->getSession()->send(packet) : Maps::getMap(player->getMap())->sendPacket(packet);
}

void PetsPacket::showChat(Player *player, Pet *pet, const string &message, int8_t act) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_PET_SHOW_CHAT);
	packet.add<int32_t>(player->getId());
	packet.add<int8_t>(pet->getIndex());
	packet.add<int8_t>(0);
	packet.add<int8_t>(act);
	packet.addString(message);
	packet.add<int8_t>(0);
	Maps::getMap(player->getMap())->sendPacket(packet, player);
}

void PetsPacket::showMovement(Player *player, Pet *pet, unsigned char *buf, int32_t buflen) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_PET_SHOW_MOVING);
	packet.add<int32_t>(player->getId());
	packet.add<int8_t>(pet->getIndex());
	packet.addBuffer(buf, buflen);
	Maps::getMap(player->getMap())->sendPacket(packet, player);
}

void PetsPacket::showAnimation(Player *player, Pet *pet, int8_t animation, bool success) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_PET_ANIMATION);
	packet.add<int32_t>(player->getId());
	packet.add<int8_t>(pet->getIndex()); // Index for multiple pets
	packet.add<int8_t>(animation == 1 && success);
	packet.add<int8_t>(animation);
	animation == 1 ? packet.add<int8_t>(0) : packet.add<int16_t>(success);

	player->getSession()->send(packet);
}

void PetsPacket::updatePet(Player *player, Pet *pet) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_MOVE_ITEM);
	packet.add<int8_t>(0);
	packet.add<int8_t>(2);
	packet.add<int8_t>(3);
	packet.add<int8_t>(5);
	packet.add<int8_t>(pet->getInventorySlot());
	packet.add<int16_t>(0);
	packet.add<int8_t>(5);
	packet.add<int8_t>(pet->getInventorySlot());
	packet.add<int8_t>(0);
	packet.add<int8_t>(3);
	packet.add<int32_t>(pet->getItemId());
	packet.add<int8_t>(1);
	packet.add<int32_t>(pet->getId());
	packet.add<int32_t>(0);
	packet.addBytes("008005BB46E61702");
	packet.addString(pet->getName(), 13);
	packet.add<int8_t>(pet->getLevel());
	packet.add<int16_t>(pet->getCloseness());
	packet.add<int8_t>(pet->getFullness());
	packet.add<int8_t>(0);
	packet.addBytes("B8D56000CEC8"); // TODO: Expire date
	packet.add<int8_t>(1); // Alive or dead
	packet.add<int32_t>(0);
	player->getSession()->send(packet);
}

void PetsPacket::levelUp(Player *player, Pet *pet) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_GAIN_ITEM);
	packet.add<int8_t>(4);
	packet.add<int8_t>(0);
	packet.add<int8_t>(pet->getIndex());
	player->getSession()->send(packet);

	packet = PacketCreator();
	packet.add<int16_t>(SEND_SHOW_SKILL);
	packet.add<int32_t>(player->getId());
	packet.add<int16_t>(4);
	packet.add<int8_t>(pet->getIndex());
	Maps::getMap(player->getMap())->sendPacket(packet);
}

void PetsPacket::changeName(Player *player, Pet *pet) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_PET_NAME_CHANGE);
	packet.add<int32_t>(player->getId());
	packet.add<int8_t>(pet->getIndex());
	packet.addString(pet->getName());
	packet.add<int8_t>(0);
	Maps::getMap(player->getMap())->sendPacket(packet);
}

void PetsPacket::showPet(Player *player, Pet *pet) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_PET_SHOW);
	packet.add<int32_t>(player->getId());
	packet.add<int8_t>(pet->getIndex());
	packet.add<int32_t>(pet->getId());
	packet.add<int32_t>(0);
	packet.add<int8_t>(0);
	player->getSession()->send(packet);
}

void PetsPacket::updateSummonedPets(Player *player) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_UPDATE_STAT);
	packet.add<int8_t>(0);
	packet.add<int16_t>(0x8);
	packet.add<int16_t>(0x18);
	for (int8_t i = 1; i <= Inventories::MaxPetCount; i++) {
		if (Pet *pet = player->getPets()->getSummoned(i)) {
			packet.add<int32_t>(pet->getId());
		}
		else {
			packet.add<int32_t>(0);
		}
		packet.add<int32_t>(0);
	}
	packet.add<int8_t>(0);
	player->getSession()->send(packet);
}

void PetsPacket::blankUpdate(Player *player) {
	PacketCreator packet;
	packet.add<int16_t>(SEND_UPDATE_STAT);
	packet.add<int8_t>(1);
	packet.add<int32_t>(0);
	player->getSession()->send(packet);
}