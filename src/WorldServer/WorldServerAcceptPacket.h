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
#ifndef WORLDSERVERACCEPTPACKET_H
#define WORLDSERVERACCEPTPACKET_H

#include "Types.h"
#include <string>

using std::string;

class WorldServerAcceptConnection;
class PacketReader;

namespace WorldServerAcceptPacket {
	void connect(WorldServerAcceptConnection *player, uint16_t channel, uint16_t port);
	void sendPacketToChannelForHolding(uint16_t channel, int32_t playerid, PacketReader &buffer);
	void sendHeldPacketRemoval(uint16_t channel, int32_t playerid);
	void playerChangeChannel(WorldServerAcceptConnection *player, int32_t playerid, uint32_t ip, int16_t port);
	void sendToChannels(unsigned char *data, int32_t len);
	void sendToLogin(unsigned char *data, int32_t len);
	void findPlayer(WorldServerAcceptConnection *player, int32_t finder, uint16_t channel, const string &findee, uint8_t is = 0);
	void whisperPlayer(WorldServerAcceptConnection *player, int32_t whisperee, const string &whisperer, uint16_t channel, const string &message);
	void scrollingHeader(const string &message);
	void newConnectable(uint16_t channel, int32_t playerid);
	void groupChat(WorldServerAcceptConnection *player, int32_t playerid, int8_t type, const string &message, const string &sender);
	void sendRates(WorldServerAcceptConnection *player, int32_t setBit);
	void sendParties(WorldServerAcceptConnection *player);
	void sendRemovePartyPlayer(int32_t playerid, int32_t partyid);
	void sendAddPartyPlayer(int32_t playerid, int32_t partyid);
	void sendSwitchPartyLeader(int32_t playerid, int32_t partyid);
	void sendCreateParty(int32_t playerid, int32_t partyid);
	void sendDisbandParty(int32_t partyid);
};

#endif
