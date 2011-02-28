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
#include "Fame.h"
#include "FamePacket.h"
#include "Player.h"
#include "PlayerDataProvider.h"
#include "Database.h"
#include "PacketReader.h"

void Fame::handleFame(Player *player, PacketReader &packet) {
	int32_t playerid = packet.get<int32_t>();
	uint8_t type = (uint8_t) packet.get<int32_t>();
	if (player->getId() > 0) {
		if (player->getId() != playerid) {
			int32_t checkResult = canFame(player, playerid);
			if (checkResult >= 1 && checkResult <= 4)
				FamePacket::sendError(player, checkResult);
			else {
				Player *famee = PlayerDataProvider::Instance()->getPlayer(playerid);
				int16_t newFame = famee->getStats()->getFame() + (type == 1 ? 1 : -1); // Increase if type = 1, else decrease
				famee->getStats()->setFame(newFame);
				addFameLog(player->getId(), playerid);
				FamePacket::sendFame(player, famee, type, newFame);
			}
		}
		else {
			// Hacking
			return;
		}
	}
	else
		FamePacket::sendError(player, 1);
}

int32_t Fame::canFame(Player *player, int32_t to) {
	int32_t from = player->getId();
	if (player->getStats()->getLevel() < 15)
		return 2;
	if (getLastFameLog(from))
		return 3;
	if (getLastFameSPLog(from, to))
		return 4;
	return 0;
}

void Fame::addFameLog(int32_t from, int32_t to) {
	mysqlpp::Query query = Database::getCharDB().query();
	query << "INSERT INTO fame_log (`from`, `to`, `time`) VALUES (" 
			<< from << ","
			<< to << ", NOW())";
	query.exec();
}

bool Fame::getLastFameLog(int32_t from) { // Last fame from that char
	mysqlpp::Query query = Database::getCharDB().query();
	query << "SELECT `time` FROM `fame_log` WHERE `from`=" << from << " AND UNIX_TIMESTAMP(`time`) > UNIX_TIMESTAMP()-86400 ORDER BY `time` DESC LIMIT 1";
	mysqlpp::StoreQueryResult res = query.store();
	if (!res.empty())
		return (res.num_rows() == 0) ? false : true;
	return false;
}

bool Fame::getLastFameSPLog(int32_t from, int32_t to) {
	mysqlpp::Query query = Database::getCharDB().query();
	query << "SELECT `time` FROM `fame_log` WHERE `from`=" << from << " AND `to`=" << to << " AND UNIX_TIMESTAMP(`time`) > UNIX_TIMESTAMP()-2592000 ORDER BY `time` DESC LIMIT 1";
	mysqlpp::StoreQueryResult res = query.store();
	if (!res.empty())
		return (res.num_rows() == 0) ? false : true;
	return false;
}
