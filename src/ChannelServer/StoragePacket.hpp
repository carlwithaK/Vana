/*
Copyright (C) 2008-2014 Vana Development Team

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

#include "PacketBuilder.hpp"
#include "Types.hpp"

class Player;

namespace StoragePacket {
	PACKET(showStorage, Player *player, npc_id_t npcId);
	PACKET(addItem, Player *player, inventory_t inv);
	PACKET(takeItem, Player *player, inventory_t inv);
	PACKET(changeMesos, storage_slot_t slotCount, mesos_t mesos);
	PACKET(storageFull);
	PACKET(noMesos);
}