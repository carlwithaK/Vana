/*
Copyright (C) 2008-2011 Vana Development Team

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
#include "Configuration.h"
#include "InterHeader.h"
#include "LoginServerAcceptConnection.h"
#include "PacketCreator.h"
#include "Rates.h"
#include "Session.h"
#include "World.h"
#include "Worlds.h"

void LoginServerAcceptPacket::connect(LoginServerAcceptConnection *connection, World *world) {
	PacketCreator packet;
	packet.add<int16_t>(IMSG_WORLD_CONNECT);
	packet.add<int8_t>(world->getId());
	packet.add<port_t>(world->getPort());

	packet.addClass<Configuration>(world->getConfig());

	connection->getSession()->send(packet);
}

void LoginServerAcceptPacket::noMoreWorld(LoginServerAcceptConnection *connection) {
	PacketCreator packet;
	packet.add<int16_t>(IMSG_WORLD_CONNECT);
	packet.add<int8_t>(-1);
	connection->getSession()->send(packet);
}

void LoginServerAcceptPacket::connectChannel(LoginServerAcceptConnection *connection, int8_t worldId, ip_t ip, port_t port) {
	PacketCreator packet;
	packet.add<int16_t>(IMSG_LOGIN_CHANNEL_CONNECT);
	packet.add<int8_t>(worldId);
	packet.add<ip_t>(ip);
	packet.add<port_t>(port);
	connection->getSession()->send(packet);
}

void LoginServerAcceptPacket::newPlayer(LoginServerAcceptConnection *connection, uint16_t channel, int32_t charId, ip_t ip) {
	PacketCreator packet;
	packet.add<int16_t>(IMSG_NEW_PLAYER);
	packet.add<int16_t>(channel);
	packet.add<int32_t>(charId);
	packet.add<ip_t>(ip);
	connection->getSession()->send(packet);
}