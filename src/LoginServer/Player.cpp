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
#include "Player.hpp"
#include "Characters.hpp"
#include "CmsgHeader.hpp"
#include "Database.hpp"
#include "Login.hpp"
#include "LoginPacket.hpp"
#include "LoginServer.hpp"
#include "PacketReader.hpp"
#include "Worlds.hpp"
#include <iostream>
#include <stdexcept>

Player::~Player() {
	setOnline(false);
}

auto Player::handleRequest(PacketReader &packet) -> void {
	try {
		switch (packet.getHeader()) {
			case CMSG_AUTHENTICATION: Login::loginUser(this, packet); break;
			case CMSG_PLAYER_LIST: Worlds::getInstance().channelSelect(this, packet); break;
			case CMSG_WORLD_STATUS: Worlds::getInstance().selectWorld(this, packet); break;
			case CMSG_PIN: Login::handleLogin(this, packet); break;
			case CMSG_WORLD_LIST:
			case CMSG_WORLD_LIST_REFRESH: Worlds::getInstance().showWorld(this); break;
			case CMSG_CHANNEL_CONNECT: Characters::connectGame(this, packet); break;
			case CMSG_CLIENT_ERROR: LoginServer::getInstance().log(LogType::ClientError, packet.getString()); break;
			case CMSG_CLIENT_STARTED: LoginServer::getInstance().log(LogType::Info, [&](out_stream_t &log) { log << "Client connected and started from " << getIp(); }); break;
			case CMSG_PLAYER_GLOBAL_LIST: Characters::showAllCharacters(this); break;
			case CMSG_PLAYER_GLOBAL_LIST_CHANNEL_CONNECT: Characters::connectGameWorldFromViewAllCharacters(this, packet); break;
			case CMSG_PLAYER_NAME_CHECK: Characters::checkCharacterName(this, packet); break;
			case CMSG_PLAYER_CREATE: Characters::createCharacter(this, packet); break;
			case CMSG_PLAYER_DELETE: Characters::deleteCharacter(this, packet); break;
			case CMSG_ACCOUNT_GENDER: Login::setGender(this, packet); break;
			case CMSG_REGISTER_PIN: Login::registerPin(this, packet); break;
			case CMSG_LOGIN_RETURN: LoginPacket::relogResponse(this); break;
		}
	}
	catch (const PacketContentException &e) {
		// Packet data didn't match the packet length somewhere
		// This isn't always evidence of tampering with packets
		// We may not process the structure properly

		packet.reset();
		LoginServer::getInstance().log(LogType::MalformedPacket, [&](out_stream_t &log) {
			log << "User ID: " << getUserId()
				<< "; Packet: " << packet
				<< "; Error: " << e.what();
		});
		getSession()->disconnect();
	}
}

auto Player::setOnline(bool online) -> void {
	Database::getCharDb()
		<< "UPDATE user_accounts u "
		<< "SET "
		<< "	u.online = :online,"
		<< "	u.last_login = NOW() "
		<< "WHERE u.user_id = :id",
		soci::use((online ? 1 : 0), "online"),
		soci::use(m_userId, "id");
}