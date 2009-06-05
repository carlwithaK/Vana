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
#include "LoginServerAcceptPacket.h"
#include "InterHeader.h"
#include "LoginServerAcceptPlayer.h"
#include "MapleSession.h"
#include "PacketCreator.h"
#include "Rates.h"
#include "Worlds.h"

void LoginServerAcceptPacket::connect(LoginServerAcceptPlayer *player, World *world) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_WORLD_CONNECT);
	packet.add<int8_t>(world->id);
	packet.add<int16_t>(world->port);
	packet.add<int32_t>(world->maxChannels);
	packet.add<int8_t>(world->maxMultiLevel);
	packet.add<int16_t>(world->maxStats);
	packet.addString(world->scrollingHeader);
	
	packet.add<int32_t>(Rates::SetBits::all);
	packet.add<int32_t>(world->exprate);
	packet.add<int32_t>(world->questexprate);
	packet.add<int32_t>(world->mesorate);
	packet.add<int32_t>(world->droprate);
	player->getSession()->send(packet);
}

void LoginServerAcceptPacket::noMoreWorld(LoginServerAcceptPlayer *player) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_WORLD_CONNECT);
	packet.add<int8_t>(-1);
	player->getSession()->send(packet);
}

void LoginServerAcceptPacket::connectChannel(LoginServerAcceptPlayer *player, int8_t worldid, uint32_t ip, int16_t port) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_LOGIN_CHANNEL_CONNECT);
	packet.add<int8_t>(worldid);
	packet.add<uint32_t>(ip);
	packet.add<int16_t>(port);
	player->getSession()->send(packet);
}

void LoginServerAcceptPacket::newPlayer(LoginServerAcceptPlayer *player, uint16_t channel, int32_t charid) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_NEW_PLAYER);
	packet.add<int16_t>(channel);
	packet.add<int32_t>(charid);
	player->getSession()->send(packet);
}
