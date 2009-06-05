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
#include "WorldServerAcceptPlayer.h"
#include "Channels.h"
#include "InterHeader.h"
#include "LoginServerConnectPacket.h"
#include "MapleSession.h"
#include "MiscUtilities.h"
#include "Rates.h"
#include "PacketReader.h"
#include "Players.h"
#include "WorldServer.h"
#include "WorldServerAcceptHandler.h"
#include "WorldServerAcceptPacket.h"
#include <iostream>

WorldServerAcceptPlayer::~WorldServerAcceptPlayer() {
	if (isAuthenticated()) {
		Channels::Instance()->removeChannel(channel);
		Players::Instance()->removeChannelPlayers(channel);
		LoginServerConnectPacket::removeChannel(WorldServer::Instance()->getLoginPlayer(), channel);
		std::cout << "Channel " << channel << " disconnected." << std::endl;
	}
}

void WorldServerAcceptPlayer::realHandleRequest(PacketReader &packet) {
	if (!processAuth(packet, WorldServer::Instance()->getInterPassword())) return;
	switch (packet.get<int16_t>()) {
		case INTER_PLAYER_CHANGE_CHANNEL: WorldServerAcceptHandler::playerChangeChannel(this, packet); break;
		case INTER_TO_PLAYERS: packet.reset(); WorldServerAcceptPacket::sendToChannels(packet.getBuffer(), packet.getBufferLength()); break;
		case INTER_REGISTER_PLAYER: WorldServerAcceptHandler::registerPlayer(this, packet); break;
		case INTER_REMOVE_PLAYER: WorldServerAcceptHandler::removePlayer(this, packet); break;
		case INTER_FIND: WorldServerAcceptHandler::findPlayer(this, packet); break;
		case INTER_WHISPER: WorldServerAcceptHandler::whisperPlayer(this, packet); break;
		case INTER_SCROLLING_HEADER: WorldServerAcceptHandler::scrollingHeader(this, packet); break;
		case INTER_PARTY_OPERATION: WorldServerAcceptHandler::partyOperation(this, packet); break;
		case INTER_UPDATE_LEVEL: WorldServerAcceptHandler::updateLevel(this, packet); break;
		case INTER_UPDATE_JOB: WorldServerAcceptHandler::updateJob(this, packet); break;
		case INTER_UPDATE_MAP: WorldServerAcceptHandler::updateMap(this, packet); break;
		case INTER_GROUP_CHAT: WorldServerAcceptHandler::groupChat(this, packet); break;
		case INTER_TO_LOGIN: WorldServerAcceptPacket::sendToLogin(packet.getBuffer(), packet.getBufferLength()); break;
		case INTER_TRANSFER_PLAYER_PACKET: WorldServerAcceptHandler::handleChangeChannel(this, packet); break;
	}
}

void WorldServerAcceptPlayer::authenticated(int8_t type) {
	if (Channels::Instance()->size() < WorldServer::Instance()->getMaxChannels()) {
		channel = Channels::Instance()->size();
		uint16_t port = WorldServer::Instance()->getInterPort() + channel + 1;
		Channels::Instance()->registerChannel(this, channel, ip, getExternalIp(), port);
		WorldServerAcceptPacket::connect(this, channel, port, WorldServer::Instance()->getMaxMultiLevel(), WorldServer::Instance()->getMaxStats());
		WorldServerAcceptPacket::sendRates(this, Rates::SetBits::all);
		WorldServerAcceptPacket::scrollingHeader(WorldServer::Instance()->getScrollingHeader());
		WorldServerAcceptPacket::sendParties(this);
		LoginServerConnectPacket::registerChannel(WorldServer::Instance()->getLoginPlayer(), channel, ip, getExternalIp(), port);
		std::cout << "Assigned channel " << channel << " to channel server." << std::endl;
	}
	else {
		WorldServerAcceptPacket::connect(this, -1, 0, 0, 0);
		std::cout << "Error: No more channel to assign." << std::endl;
		getSession()->disconnect();
	}
}
