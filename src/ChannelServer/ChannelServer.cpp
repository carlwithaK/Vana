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
#include "ChannelServer.h"
#include "Connection/Connection.h"
#include "WorldServerConnectPlayer.h"
#include "InitializeChannel.h"
#include "PacketCreator.h"
#include "ServerPacket.h"
#include "Player.h"
#include "Players.h"

ChannelServer* ChannelServer::singleton = 0;

void ChannelServer::listen() {
	Connection::Instance()->accept(port, new PlayerFactory());
	Initializing::setUsersOffline(getOnlineId());
}

void ChannelServer::loadData() {
	Initializing::initializeMySQL();
	Initializing::initializeMobs();
	Initializing::initializeItems();
	Initializing::initializeDrops();
	Initializing::initializeEquips();
	Initializing::initializeShops();
	Initializing::initializeQuests();
	Initializing::initializeSkills();
	Initializing::initializeMaps();

	WorldServerConnectPlayer *loginPlayer = dynamic_cast<WorldServerConnectPlayer *>(Connection::Instance()->connect(login_ip.c_str(), login_inter_port, new WorldServerConnectPlayerFactory()));
	loginPlayer->setIP(external_ip.c_str());
	loginPlayer->sendAuth(inter_password.c_str());
}

void ChannelServer::connectWorld() {
	worldPlayer = dynamic_cast<WorldServerConnectPlayer *>(Connection::Instance()->connect(world_ip, world_port, new WorldServerConnectPlayerFactory()));
	worldPlayer->setIP(external_ip.c_str());
	worldPlayer->sendAuth(inter_password.c_str());
}

void ChannelServer::loadConfig() {
	Config config("conf/channelserver.lua");
	login_ip = config.getString("login_ip");
	login_inter_port = config.getInt("login_inter_port");
	external_ip = config.getString("external_ip"); // External IP

	world = -1; // Will get from login server
	port = -1; // Will get from world server
	channel = -1; // Will get from world server
}

void ChannelServer::shutdown() {
	// Note that we are not increasing an iterator because deleting a player would cause the iterator to fail
	while (1) {
		if (Players::players.empty())
			break;
		hash_map<int,Player*>::iterator iter = Players::players.begin();
		iter->second->disconnect();
	}
	exit(0);
}

void ChannelServer::sendToWorld(Packet &packet) {
	packet.packetSend(worldPlayer);
}

void ChannelServer::setScrollingHeader(char *message) {
	string strMsg = string(message);
	if (scrollingHeader != strMsg) {
		scrollingHeader = strMsg;
		if (scrollingHeader.size() == 0) {
			ServerPacket::scrollingHeaderOff();
		}
		else {
			ServerPacket::changeScrollingHeader(scrollingHeader.c_str());
		}
	}
}
