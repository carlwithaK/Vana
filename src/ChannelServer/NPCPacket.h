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
#ifndef NPCPACKET_H
#define NPCPACKET_H

#include "Types.h"

class Player;
class PacketReader;
struct NPCSpawnInfo;

namespace NPCPacket {
	void showNPC(Player *player, NPCSpawnInfo npc, int32_t i, bool show = true);
	void showNPC(int32_t mapid, NPCSpawnInfo npc, int32_t i, bool show = true);
	void animateNPC(Player *player, PacketReader &pack);
};

#endif
