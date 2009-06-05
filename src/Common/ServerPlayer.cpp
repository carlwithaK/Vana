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
#include "ServerPlayer.h"
#include "AuthenticationPacket.h"
#include "InterHeader.h"
#include "IpUtilities.h"
#include "MapleSession.h"
#include "PacketReader.h"
#include <iostream>

void AbstractServerConnectPlayer::sendAuth(const string &pass, vector<vector<uint32_t> > extIp) {
	AuthenticationPacket::sendPassword(this, pass, extIp);
}

bool AbstractServerAcceptPlayer::processAuth(PacketReader &packet, const string &pass) {
	if (packet.get<int16_t>() == INTER_PASSWORD) {
		if (packet.getString() == pass) {
			std::cout << "Server successfully authenticated." << std::endl;
			is_authenticated = true;

			IpUtilities::extractExternalIp(packet, external_ip);

			authenticated(packet.get<int8_t>());
		}
		else {
			getSession()->disconnect();
			return false;
		}
	}
	else if (is_authenticated == false) {
		// Trying to do something while unauthenticated? DC!
		getSession()->disconnect();
		return false;
	}
	packet.reset();
	return true;
}
