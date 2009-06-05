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
#include "PlayerPacketHelper.h"
#include "GameConstants.h"
#include "GameLogicUtilities.h"
#include "Inventory.h"
#include "PacketCreator.h"
#include "Pets.h"
#include "Player.h"

void PlayerPacketHelper::addItemInfo(PacketCreator &packet, int16_t slot, Item *item, bool shortSlot) {
	if (slot != 0) {
		if (shortSlot)
			packet.add<int16_t>(slot);
		else {
			slot = abs(slot);
			if (slot > 100) slot -= 100;
			packet.add<int8_t>((int8_t) slot);
		}
	}
	packet.add<int8_t>(!GameLogicUtilities::isEquip(item->id) + 1);
	packet.add<int32_t>(item->id);
	packet.add<int8_t>(0);
	packet.add<int8_t>(0);
	packet.addBytes("8005BB46E61702");
	if (GameLogicUtilities::isEquip(item->id)) {
		packet.add<int8_t>(item->slots); // Slots
		packet.add<int8_t>(item->scrolls); // Scrolls
		packet.add<int16_t>(item->istr); // STR
		packet.add<int16_t>(item->idex); // DEX
		packet.add<int16_t>(item->iint); // INT
		packet.add<int16_t>(item->iluk); // LUK
		packet.add<int16_t>(item->ihp); // HP
		packet.add<int16_t>(item->imp); // MP
		packet.add<int16_t>(item->iwatk); // W.Atk
		packet.add<int16_t>(item->imatk); // M.Atk
		packet.add<int16_t>(item->iwdef); // W.Def
		packet.add<int16_t>(item->imdef); // M.Def
		packet.add<int16_t>(item->iacc); // Acc
		packet.add<int16_t>(item->iavo); // Avo
		packet.add<int16_t>(item->ihand); // Hands
		packet.add<int16_t>(item->ispeed); // Speed
		packet.add<int16_t>(item->ijump); // Jump
		packet.addString(item->name); // Owner string
		packet.add<int8_t>(item->flags); // Lock, shoe spikes, cape cold protection, etc.
		packet.add<int64_t>(0); // Expiration time
		packet.add<int8_t>(0); // No clue
	}
	else {
		packet.add<int16_t>(item->amount); // Amount
		packet.add<int8_t>(0);
		packet.add<int8_t>(0);
		packet.add<int8_t>(item->flags);
		packet.add<int8_t>(0);
		if (GameLogicUtilities::isRechargeable(item->id)) {
			packet.add<int32_t>(2);
			packet.add<int16_t>(0x54);
			packet.add<int8_t>(0);
			packet.add<int8_t>(0x34);
		}
	}
}

void PlayerPacketHelper::addPlayerDisplay(PacketCreator &packet, Player *player) {
	packet.add<int8_t>(player->getGender());
	packet.add<int8_t>(player->getSkin());
	packet.add<int32_t>(player->getEyes());
	packet.add<int8_t>(1);
	packet.add<int32_t>(player->getHair());
	player->getInventory()->addEquippedPacket(packet);
	for (int8_t i = 0; i < 3; i++) {
		if (Pet *pet = player->getPets()->getSummoned(i)) {
			packet.add<int32_t>(pet->getType());
		}
		else {
			packet.add<int32_t>(0);
		}
	}
}
