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
#include "LoginServerAcceptPlayer.h"
#include "InterHeader.h"
#include "LoginServer.h"
#include "LoginServerAcceptHandler.h"
#include "MapleSession.h"
#include "PacketReader.h"
#include "RankingCalculator.h"
#include "Worlds.h"

void LoginServerAcceptPlayer::realHandleRequest(PacketReader &packet) {
	if (!processAuth(packet, LoginServer::Instance()->getInterPassword())) return;
	switch (packet.get<int16_t>()) {
		case INTER_REGISTER_CHANNEL: LoginServerAcceptHandler::registerChannel(this, packet); break;
		case INTER_UPDATE_CHANNEL_POP: LoginServerAcceptHandler::updateChannelPop(this, packet); break;
		case INTER_REMOVE_CHANNEL: LoginServerAcceptHandler::removeChannel(this, packet); break;
		case INTER_CALCULATE_RANKING: RankingCalculator::runThread(); break;
	}
}

void LoginServerAcceptPlayer::authenticated(int8_t type) {
	if (type == InterWorldServer)
		Worlds::connectWorldServer(this);
	else if (type == InterChannelServer)
		Worlds::connectChannelServer(this);
	else
		getSession()->disconnect();
}
