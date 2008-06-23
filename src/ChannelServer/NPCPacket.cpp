/*
Copyright (C) 2008 Vana Development Team

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
#include "NPCPacket.h"
#include "PacketCreator.h"
#include "Player.h"
#include "NPCs.h"
#include "SendHeader.h"

void NPCPacket::showNPC(Player *player, NPCInfo npc, int i) {
	Packet packet;
	packet.addHeader(SEND_SHOW_NPC);
	packet.addInt(i+0x64);
	packet.addInt(npc.id);
	packet.addShort(npc.x);
	packet.addShort(npc.cy);
	packet.addByte(1);
	packet.addShort(npc.fh);
	packet.addShort(npc.rx0);
	packet.addShort(npc.rx1);
	packet.addByte(1);
	packet.send(player);
	packet = Packet();
	packet.addHeader(SEND_SHOW_NPC2);
	packet.addByte(1);
	packet.addInt(i+0x64);
	packet.addInt(npc.id);
	packet.addShort(npc.x);
	packet.addShort(npc.cy);
	packet.addByte(1);
	packet.addShort(npc.fh);
	packet.addShort(npc.rx0);
	packet.addShort(npc.rx1);
	packet.addByte(1);
	packet.send(player);
}