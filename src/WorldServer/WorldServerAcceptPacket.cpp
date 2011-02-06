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
#include "WorldServerAcceptPacket.h"
#include "Channels.h"
#include "InterHeader.h"
#include "MapleSession.h"
#include "PacketCreator.h"
#include "PacketReader.h"
#include "Parties.h"
#include "PartyHandler.h"
#include "Rates.h"
#include "SendHeader.h"
#include "TimeUtilities.h"
#include "WorldServer.h"
#include "WorldServerAcceptConnection.h"
#include <boost/tr1/unordered_map.hpp>
#include <map>

using std::map;
using std::tr1::unordered_map;

void WorldServerAcceptPacket::groupChat(WorldServerAcceptConnection *player, int32_t playerid, int8_t type, const string &message, const string &sender) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_FORWARD_TO);
	packet.add<int32_t>(playerid);
	packet.add<int16_t>(SMSG_MESSAGE_GROUP);
	packet.add<int8_t>(type);
	packet.addString(sender);
	packet.addString(message);
	player->getSession()->send(packet);
}

void WorldServerAcceptPacket::connect(WorldServerAcceptConnection *player, uint16_t channel, uint16_t port) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_CHANNEL_CONNECT);
	packet.add<int16_t>(channel);
	packet.add<int16_t>(port);
	packet.add<int8_t>(WorldServer::Instance()->getMaxMultiLevel());
	packet.add<int16_t>(WorldServer::Instance()->getMaxStats());
	packet.add<int32_t>(WorldServer::Instance()->getMaxChars());
	// Boss junk
	packet.add<int16_t>(WorldServer::Instance()->getPianusAttempts());
	packet.add<int16_t>(WorldServer::Instance()->getPapAttempts());
	packet.add<int16_t>(WorldServer::Instance()->getZakumAttempts());
	packet.add<int16_t>(WorldServer::Instance()->getHorntailAttempts());
	packet.add<int16_t>(WorldServer::Instance()->getPinkBeanAttempts());
	packet.addVector(WorldServer::Instance()->getPianusChannels());
	packet.addVector(WorldServer::Instance()->getPapChannels());
	packet.addVector(WorldServer::Instance()->getZakumChannels());
	packet.addVector(WorldServer::Instance()->getHorntailChannels());
	packet.addVector(WorldServer::Instance()->getPinkBeanChannels());
	player->getSession()->send(packet);
}

void WorldServerAcceptPacket::sendPacketToChannelForHolding(uint16_t channel, int32_t playerid, PacketReader &buffer) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_TRANSFER_PLAYER_PACKET);
	packet.add<int32_t>(playerid);
	packet.addBuffer(buffer);
	Channels::Instance()->getChannel(channel)->player->getSession()->send(packet);
}

void WorldServerAcceptPacket::sendHeldPacketRemoval(uint16_t channel, int32_t playerid) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_TRANSFER_PLAYER_PACKET_DISCONNECT);
	packet.add<int32_t>(playerid);
	Channels::Instance()->getChannel(channel)->player->getSession()->send(packet);
}

void WorldServerAcceptPacket::playerChangeChannel(WorldServerAcceptConnection *player, int32_t playerid, uint32_t ip, int16_t port) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_PLAYER_CHANGE_CHANNEL);
	packet.add<int32_t>(playerid);
	packet.add<uint32_t>(ip);
	packet.add<int16_t>(port);
	player->getSession()->send(packet);
}

void WorldServerAcceptPacket::sendToChannels(unsigned char *data, int32_t len) {
	PacketCreator packet;
	packet.addBuffer(data, len);
	Channels::Instance()->sendToAll(packet);
}

void WorldServerAcceptPacket::sendToLogin(unsigned char *data, int32_t len) {
	PacketCreator packet;
	packet.addBuffer(data, len);
	WorldServer::Instance()->getLoginConnection()->getSession()->send(packet);
}

void WorldServerAcceptPacket::findPlayer(WorldServerAcceptConnection *player, int32_t finder, uint16_t channel, const string &findee, uint8_t is) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_FIND);
	packet.add<int32_t>(finder);
	packet.add<int16_t>(channel);
	packet.addString(findee);
	packet.add<int8_t>(is);

	player->getSession()->send(packet);
}

void WorldServerAcceptPacket::whisperPlayer(WorldServerAcceptConnection *player, int32_t whisperee, const string &whisperer, uint16_t channel, const string &message) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_WHISPER);
	packet.add<int32_t>(whisperee);
	packet.addString(whisperer);
	packet.add<int16_t>(channel);
	packet.addString(message);

	player->getSession()->send(packet);
}

void WorldServerAcceptPacket::scrollingHeader(const string &message) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_SCROLLING_HEADER);
	packet.addString(message);

	Channels::Instance()->sendToAll(packet);
}

void WorldServerAcceptPacket::newConnectable(uint16_t channel, int32_t playerid) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_NEW_CONNECTABLE);
	packet.add<int32_t>(playerid);

	Channels::Instance()->getChannel(channel)->player->getSession()->send(packet);
}

void WorldServerAcceptPacket::sendRates(WorldServerAcceptConnection *player, int32_t setBit) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_SET_RATES);
	packet.add<int32_t>(setBit);

	if (setBit & Rates::SetBits::exp) {
		packet.add<int32_t>(WorldServer::Instance()->getExprate());
	}
	if (setBit & Rates::SetBits::questExp) {
		packet.add<int32_t>(WorldServer::Instance()->getQuestExprate());
	}
	if (setBit & Rates::SetBits::meso) {
		packet.add<int32_t>(WorldServer::Instance()->getMesorate());
	}
	if (setBit & Rates::SetBits::drop) {
		packet.add<int32_t>(WorldServer::Instance()->getDroprate());
	}

	player->getSession()->send(packet);
}

void WorldServerAcceptPacket::sendParties(WorldServerAcceptConnection *player) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_PARTY_SYNC);
	packet.add<int8_t>(PARTY_SYNC_CHANNEL_START);

	unordered_map<int32_t, Party *> parties = Parties::Instance()->getParties();
	map<int32_t, Player *> players;

	packet.add<int32_t>((int32_t)(parties.size()));
	for (unordered_map<int32_t, Party *>::iterator iter = parties.begin(); iter != parties.end(); iter++) {
		packet.add<int32_t>(iter->first);
		players = iter->second->members;
		packet.add<int8_t>((int8_t)(players.size()));
		for (map<int32_t, Player *>::iterator playeriter = players.begin(); playeriter != players.end(); playeriter++) {
			packet.add<int32_t>(playeriter->first);
		}
		packet.add<int32_t>(iter->second->getLeader());
	}

	player->getSession()->send(packet);
}

void WorldServerAcceptPacket::sendRemovePartyPlayer(int32_t playerid, int32_t partyid) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_PARTY_SYNC);
	packet.add<int8_t>(PARTY_SYNC_REMOVE_MEMBER);
	packet.add<int32_t>(partyid);
	packet.add<int32_t>(playerid);
	Channels::Instance()->sendToAll(packet);
}

void WorldServerAcceptPacket::sendAddPartyPlayer(int32_t playerid, int32_t partyid) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_PARTY_SYNC);
	packet.add<int8_t>(PARTY_SYNC_ADD_MEMBER);
	packet.add<int32_t>(partyid);
	packet.add<int32_t>(playerid);
	Channels::Instance()->sendToAll(packet);
}

void WorldServerAcceptPacket::sendSwitchPartyLeader(int32_t playerid, int32_t partyid) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_PARTY_SYNC);
	packet.add<int8_t>(PARTY_SYNC_SWITCH_LEADER);
	packet.add<int32_t>(partyid);
	packet.add<int32_t>(playerid);
	Channels::Instance()->sendToAll(packet);
}

void WorldServerAcceptPacket::sendCreateParty(int32_t playerid, int32_t partyid) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_PARTY_SYNC);
	packet.add<int8_t>(PARTY_SYNC_CREATE);
	packet.add<int32_t>(partyid);
	packet.add<int32_t>(playerid);
	Channels::Instance()->sendToAll(packet);
}

void WorldServerAcceptPacket::sendDisbandParty(int32_t partyid) {
	PacketCreator packet;
	packet.add<int16_t>(INTER_PARTY_SYNC);
	packet.add<int8_t>(PARTY_SYNC_DISBAND);
	packet.add<int32_t>(partyid);
	Channels::Instance()->sendToAll(packet);
}