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
#include "ManagementFunctions.hpp"
#include "ChannelServer.hpp"
#include "Database.hpp"
#include "Inventory.hpp"
#include "ItemDataProvider.hpp"
#include "Maps.hpp"
#include "NpcHandler.hpp"
#include "Player.hpp"
#include "PlayerDataProvider.hpp"
#include "PlayerPacket.hpp"
#include "StringUtilities.hpp"
#include "SyncPacket.hpp"
#include "WorldServerConnectPacket.hpp"

auto ManagementFunctions::map(Player *player, const string_t &args) -> bool {
	match_t matches;
	if (ChatHandlerFunctions::runRegexPattern(args, R"((\w+)? ?(\w+)?)", matches)) {
		string_t rawMap = matches[1];
		string_t rawPortal = matches[2];
		if (rawMap.empty()) {
			ChatHandlerFunctions::showInfo(player, "Current map: " + StringUtilities::lexical_cast<string_t>(player->getMapId()));
			return true;
		}

		int32_t mapId = ChatHandlerFunctions::getMap(rawMap, player);
		if (mapId != -1 && Maps::getMap(mapId) != nullptr) {
			PortalInfo *portal = nullptr;
			if (!rawPortal.empty()) {
				Map *map = Maps::getMap(mapId);
				if (rawPortal == "sp") {
					portal = map->getSpawnPoint();
				}
				else if (rawPortal == "tp") {

				}
				else {
					portal = map->getPortal(rawPortal);
				}

				if (portal == nullptr) {
					ChatHandlerFunctions::showError(player, "Invalid portal: " + rawPortal);
					return true;
				}
			}
			player->setMap(mapId, portal);
		}
		else {
			ChatHandlerFunctions::showError(player, "Invalid map: " + rawMap);
		}

		return true;
	}

	return false;
}

auto ManagementFunctions::warp(Player *player, const string_t &args) -> bool {
	match_t matches;
	if (ChatHandlerFunctions::runRegexPattern(args, R"((\w+) (\w+) (\w+) ?(\w+)? ?(\w+)?)", matches)) {
		string_t sourceType = matches[1];
		string_t destinationType = matches[2];
		string_t rawMap = matches[3];
		string_t optional = matches[4];
		string_t moreOptional = matches[5];
		int32_t sourceMapId = -1;
		int32_t destinationMapId = -1;
		string_t portal;
		Player *sourcePlayer = nullptr;
		bool validCombo = true;
		bool onlySource = true;
		bool singleArgumentDestination = false;
		bool portalSpecified = false;

		auto resolvePlayer = [](const string_t &playerArg) { return PlayerDataProvider::getInstance().getPlayer(playerArg); };
		auto resolveMapArg = [player](const string_t &mapArg) { return ChatHandlerFunctions::getMap(mapArg, player); };
		auto resolveMapPortal = [&portal, player](const string_t &portalArg) {
			if (!portalArg.empty()) {
				portal = portalArg;
				return true;
			}
			return false;
		};
		auto resolveMapCurrent = [player]() { return player->getMapId(); };
		auto resolveMapPlayer = [player](const string_t &playerArg) {
			if (Player *target = PlayerDataProvider::getInstance().getPlayer(playerArg)) {
				return target->getMapId();
			}
			return -2;
		};

		if (sourceType == "map") {
			onlySource = false;

			sourceMapId = resolveMapArg(rawMap);
			if (sourceMapId == -1 || Maps::getMap(sourceMapId) == nullptr) {
				ChatHandlerFunctions::showError(player, "Invalid source map: " + rawMap);
				return true;
			}

			if (destinationType == "map") {
				destinationMapId = resolveMapArg(optional);
				portalSpecified = resolveMapPortal(moreOptional);
			}
			else if (destinationType == "current" || destinationType == "self") {
				destinationMapId = resolveMapCurrent();
				portalSpecified = resolveMapPortal(optional);
			}
			else if (destinationType == "player") {
				destinationMapId = resolveMapPlayer(optional);
				portalSpecified = resolveMapPortal(moreOptional);
			}
			else {
				validCombo = false;
			}
		}
		else if (sourceType == "player") {
			onlySource = false;

			sourcePlayer = PlayerDataProvider::getInstance().getPlayer(rawMap);
			if (sourcePlayer == nullptr) {
				ChatHandlerFunctions::showError(player, "Invalid source player: " + rawMap);
				return true;
			}

			if (destinationType == "map") {
				destinationMapId = resolveMapArg(optional);
				portalSpecified = resolveMapPortal(moreOptional);
			}
			else if (destinationType == "current" || destinationType == "self") {
				destinationMapId = resolveMapCurrent();
				portalSpecified = resolveMapPortal(optional);
			}
			else if (destinationType == "player") {
				destinationMapId = resolveMapPlayer(optional);
				portalSpecified = resolveMapPortal(moreOptional);
			}
			else {
				validCombo = false;
			}
		}
		else if (sourceType == "current" || sourceType == "self") {
			sourceMapId = resolveMapCurrent();
			if (destinationType == "map") {
				destinationMapId = resolveMapArg(rawMap);
				portalSpecified = resolveMapPortal(optional);
			}
			else if (destinationType == "player") {
				destinationMapId = resolveMapPlayer(rawMap);
				portalSpecified = resolveMapPortal(optional);
			}
			else {
				validCombo = false;
			}
		}
		else if (sourceType == "channel") {
			if (destinationType == "map") {
				destinationMapId = resolveMapArg(rawMap);
				portalSpecified = resolveMapPortal(optional);
			}
			else if (destinationType == "current" || destinationType == "self") {
				destinationMapId = resolveMapCurrent();
				portalSpecified = resolveMapPortal(rawMap);
				singleArgumentDestination = true;
			}
			else if (destinationType == "player") {
				destinationMapId = resolveMapPlayer(rawMap);
				portalSpecified = resolveMapPortal(optional);
			}
			else {
				validCombo = false;
			}
		}
		else {
			ChatHandlerFunctions::showError(player, "Invalid source type: " + sourceType);
			return true;
		}

		if (!validCombo) {
			ChatHandlerFunctions::showError(player, "Invalid destination type for source type \"" + sourceType + "\": " + destinationType);
			return true;
		}

		if (destinationMapId == -2) {
			ChatHandlerFunctions::showError(player, "Invalid destination player: " + (onlySource ? rawMap : optional));
			return true;
		}

		if (destinationMapId == -1 || Maps::getMap(destinationMapId) == nullptr) {
			ChatHandlerFunctions::showError(player, "Invalid destination map: " + (onlySource ? rawMap : optional));
			return true;
		}

		PortalInfo *destinationPortal = nullptr;
		Map *destination = Maps::getMap(destinationMapId);

		if (portalSpecified) {
			if (portal == "sp") {
				destinationPortal = destination->getSpawnPoint();
			}
			else if (portal == "tp") {

			}
			else {
				destinationPortal = destination->getPortal(portal);
			}

			if (destinationPortal == nullptr) {
				ChatHandlerFunctions::showError(player, "Invalid destination portal: " + (singleArgumentDestination ? rawMap : (onlySource ? optional : moreOptional)));
				return true;
			}
		}

		auto warpToMap = [destinationMapId, destinationPortal](Player *target) {
			if (target->getMapId() != destinationMapId) {
				target->setMap(destinationMapId, destinationPortal);
			}
		};

		if (sourceType == "map" || sourceType == "current") {
			out_stream_t message;
			message << "Warped all players on map ID " << sourceMapId << " to map ID " << destinationMapId;
			if (portal.length() != 0) {
				message << " (portal " << portal << ")";
			}

			Maps::getMap(sourceMapId)->runFunctionPlayers(warpToMap);
			ChatHandlerFunctions::showInfo(player, message.str());
		}
		else if (sourceType == "player") {
			out_stream_t message;
			message << "Warped player " << rawMap << " to map ID " << destinationMapId;
			if (portal.length() != 0) {
				message << " (portal " << portal << ")";
			}

			warpToMap(sourcePlayer);
			ChatHandlerFunctions::showInfo(player, message.str());
		}
		else if (sourceType == "self") {
			warpToMap(player);
		}
		else if (sourceType == "channel") {
			out_stream_t message;
			message << "Warped everyone in the channel to map ID " << destinationMapId;
			if (portal.length() != 0) {
				message << " (portal " << portal << ")";
			}

			PlayerDataProvider::getInstance().run(warpToMap);
			ChatHandlerFunctions::showInfo(player, message.str());
		}

		return true;
	}
	return false;
}

auto ManagementFunctions::changeChannel(Player *player, const string_t &args) -> bool {
	match_t matches;
	if (ChatHandlerFunctions::runRegexPattern(args, R"((\d+))", matches)) {
		string_t targetChannel = matches[1];
		int8_t channel = atoi(targetChannel.c_str()) - 1;
		player->changeChannel(channel);
		return true;
	}
	return false;
}

auto ManagementFunctions::lag(Player *player, const string_t &args) -> bool {
	match_t matches;
	if (ChatHandlerFunctions::runRegexPattern(args, R"((\w+))", matches)) {
		string_t target = matches[1];
		if (Player *p = PlayerDataProvider::getInstance().getPlayer(target)) {
			ChatHandlerFunctions::showInfo(player, p->getName() + "'s lag: " + StringUtilities::lexical_cast<string_t>(p->getLatency().count()) + "ms");
		}
		else {
			ChatHandlerFunctions::showError(player, "Invalid player: " + target);
		}
		return true;
	}
	return false;
}

auto ManagementFunctions::header(Player *player, const string_t &args) -> bool {
	SyncPacket::ConfigPacket::scrollingHeader(args);
	return true;
}

auto ManagementFunctions::shutdown(Player *player, const string_t &args) -> bool {
	ChatHandlerFunctions::showInfo(player, "Shutting down the server");
	ChannelServer::getInstance().log(LogType::GmCommand, "GM shutdown the server. GM: " + player->getName());
	ChannelServer::getInstance().shutdown();
	return true;
}

auto ManagementFunctions::kick(Player *player, const string_t &args) -> bool {
	if (args.length() != 0) {
		if (Player *target = PlayerDataProvider::getInstance().getPlayer(args)) {
			if (player->getGmLevel() > target->getGmLevel()) {
				target->getSession()->disconnect();
				ChatHandlerFunctions::showInfo(player, "Kicked " + args + " from the server");
			}
			else {
				ChatHandlerFunctions::showError(player, "Player " + args + " is your peer or outranks you");
			}
		}
		else {
			ChatHandlerFunctions::showError(player, "Invalid player: " + args);
		}
		return true;
	}
	return false;
}

auto ManagementFunctions::relog(Player *player, const string_t &args) -> bool {
	player->changeChannel(static_cast<int8_t>(ChannelServer::getInstance().getChannelId()));
	return true;
}

auto ManagementFunctions::calculateRanks(Player *player, const string_t &args) -> bool {
	WorldServerConnectPacket::rankingCalculation();
	ChatHandlerFunctions::showInfo(player, "Sent a signal to force the calculation of rankings");
	return true;
}

auto ManagementFunctions::item(Player *player, const string_t &args) -> bool {
	match_t matches;
	if (ChatHandlerFunctions::runRegexPattern(args, R"((\d+) ?(\d*)?)", matches)) {
		string_t rawItem = matches[1];
		int32_t itemId = atoi(rawItem.c_str());
		if (ItemDataProvider::getInstance().getItemInfo(itemId) != nullptr) {
			string_t countString = matches[2];
			uint16_t count = countString.length() > 0 ? atoi(countString.c_str()) : 1;
			Inventory::addNewItem(player, itemId, count, true);
		}
		else {
			ChatHandlerFunctions::showError(player, "Invalid item: " + rawItem);
		}
		return true;
	}
	return false;
}

auto ManagementFunctions::storage(Player *player, const string_t &args) -> bool {
	NpcHandler::showStorage(player, 1012009);
	return true;
}

auto ManagementFunctions::shop(Player *player, const string_t &args) -> bool {
	if (args.length() != 0) {
		int32_t shopId = -1;
		if (args == "gear") shopId = 9999999;
		else if (args == "scrolls") shopId = 9999998;
		else if (args == "nx") shopId = 9999997;
		else if (args == "face") shopId = 9999996;
		else if (args == "ring") shopId = 9999995;
		else if (args == "chair") shopId = 9999994;
		else if (args == "mega") shopId = 9999993;
		else if (args == "pet") shopId = 9999992;
		else shopId = atoi(args.c_str());

		if (NpcHandler::showShop(player, shopId)) {
			return true;
		}
	}
	return false;
}

auto ManagementFunctions::reload(Player *player, const string_t &args) -> bool {
	if (args.length() != 0) {
		if (args == "items" || args == "drops" || args == "shops" ||
			args == "mobs" || args == "beauty" || args == "scripts" ||
			args == "skills" || args == "reactors" || args == "pets" ||
			args == "quests" || args == "all") {
			WorldServerConnectPacket::reloadMcdb(args);
			ChatHandlerFunctions::showInfo(player, "Reloading message for " + args + " sent to all channels");
		}
		else {
			ChatHandlerFunctions::showError(player, "Invalid reload type: " + args);
		}
		return true;
	}
	return false;
}

auto ManagementFunctions::npc(Player *player, const string_t &args) -> bool {
	if (args.length() != 0) {
		int32_t npcId = atoi(args.c_str());
		Npc *npc = new Npc(npcId, player);
		npc->run();
		return true;
	}
	return false;
}

auto ManagementFunctions::addNpc(Player *player, const string_t &args) -> bool {
	if (args.length() != 0) {
		NpcSpawnInfo npc;
		npc.id = atoi(args.c_str());
		npc.foothold = 0;
		npc.pos = player->getPos();
		npc.rx0 = npc.pos.x - 50;
		npc.rx1 = npc.pos.x + 50;
		int32_t id = player->getMap()->addNpc(npc);
		ChatHandlerFunctions::showInfo(player, "Spawned NPC with object ID " + StringUtilities::lexical_cast<string_t>(id));
		return true;
	}
	return false;
}

auto ManagementFunctions::killNpc(Player *player, const string_t &args) -> bool {
	player->setNpc(nullptr);
	return true;
}

auto ManagementFunctions::kill(Player *player, const string_t &args) -> bool {
	if (player->getGmLevel() == 1) {
		player->getStats()->setHp(0);
	}
	else {
		bool proceed = true;
		auto iterate = [&player](function_t<bool(Player *p)> func) -> int {
			int32_t kills = 0;
			Map *map = player->getMap();
			for (size_t i = 0; i < map->getNumPlayers(); ++i) {
				Player *t = map->getPlayer(i);
				if (t != player) {
					if (func(t)) kills++;
				}
			}
			return kills;
		};
		if (args == "all") {
			proceed = false;
			int32_t kills = iterate([](Player *p) -> bool {
				p->getStats()->setHp(0);
				return true;
			});
			ChatHandlerFunctions::showInfo(player, "Killed " + StringUtilities::lexical_cast<string_t>(kills) + " players in the current map");
		}
		else if (args == "gm" || args == "players") {
			proceed = false;
			int32_t kills = iterate([&args](Player *p) -> bool {
				if ((args == "gm" && p->isGm()) || (args == "players" && !p->isGm())) {
					p->getStats()->setHp(0);
					return true;
				}
				return false;
			});
			ChatHandlerFunctions::showInfo(player, "Killed " + StringUtilities::lexical_cast<string_t>(kills)+" " + (args == "gm" ? "GMs" : "players") + " in the current map");
		}
		if (proceed) {
			if (args == "me") {
				player->getStats()->setHp(0);
				ChatHandlerFunctions::showInfo(player, "Killed yourself");
			}
			else if (Player *kill = PlayerDataProvider::getInstance().getPlayer(args)) {
				// Kill by name
				kill->getStats()->setHp(0);
				ChatHandlerFunctions::showInfo(player, "Killed " + args);
			}
			else {
				// Nothing valid
				return false;
			}
		}
	}
	return true;
}

auto ManagementFunctions::ban(Player *player, const string_t &args) -> bool {
	match_t matches;
	if (ChatHandlerFunctions::runRegexPattern(args, R"((\w+) ?(\d+)?)", matches)) {
		string_t targetName = matches[1];
		if (Player *target = PlayerDataProvider::getInstance().getPlayer(targetName)) {
			target->getSession()->disconnect();
		}
		string_t reasonString = matches[2];
		int8_t reason = reasonString.length() > 0 ? atoi(reasonString.c_str()) : 1;

		// Ban account
		string_t expire("2130-00-00 00:00:00");

		soci::session &sql = Database::getCharDb();
		soci::statement st = (sql.prepare
			<< "UPDATE user_accounts u "
			<< "INNER JOIN characters c ON u.user_id = c.user_id "
			<< "SET "
			<< "	u.banned = 1, "
			<< "	u.ban_expire = :expire, "
			<< "	u.ban_reason = :reason "
			<< "WHERE c.name = :name ",
			soci::use(targetName, "name"),
			soci::use(expire, "expire"),
			soci::use(reason, "reason"));

		st.execute();

		if (st.get_affected_rows() > 0) {
			const string_t &banMessage = targetName + " has been banned" + ChatHandlerFunctions::getBanString(reason);
			PlayerPacket::showMessageChannel(banMessage, PlayerPacket::NoticeTypes::Notice);
			ChannelServer::getInstance().log(LogType::GmCommand, [&](out_stream_t &log) {
				log << "GM " << player->getName()
					<< " banned a player with reason " << reason
					<< ", player: " << targetName;
			});
		}
		else {
			ChatHandlerFunctions::showError(player, "Invalid player: " + targetName);
		}

		return true;
	}
	return false;
}

auto ManagementFunctions::tempBan(Player *player, const string_t &args) -> bool {
	match_t matches;
	if (ChatHandlerFunctions::runRegexPattern(args, R"((\w+) (\d+) (\d+))", matches)) {
		string_t targetName = matches[1];
		if (Player *target = PlayerDataProvider::getInstance().getPlayer(targetName)) {
			target->getSession()->disconnect();
		}
		string_t reasonString = matches[2];
		string_t length = matches[3];
		int8_t reason = reasonString.length() > 0 ? atoi(reasonString.c_str()) : 1;

		// Ban account
		soci::session &sql = Database::getCharDb();
		soci::statement st = (sql.prepare
			<< "UPDATE user_accounts u "
			<< "INNER JOIN characters c ON u.user_id = c.user_id "
			<< "SET "
			<< "	u.banned = 1, "
			<< "	u.ban_expire = DATE_ADD(NOW(), INTERVAL :expire DAY), "
			<< "	u.ban_reason = :reason "
			<< "WHERE c.name = :name ",
			soci::use(targetName, "name"),
			soci::use(length, "expire"),
			soci::use(reason, "reason"));

		st.execute();

		if (st.get_affected_rows() > 0) {
			const string_t &banMessage = targetName + " has been banned" + ChatHandlerFunctions::getBanString(reason);
			PlayerPacket::showMessageChannel(banMessage, PlayerPacket::NoticeTypes::Notice);
			ChannelServer::getInstance().log(LogType::GmCommand, [&](out_stream_t &log) {
				log << "GM " << player->getName()
					<< " temporary banned a player with reason " << reason
					<< " for " << length
					<< " days, player: " << targetName;
			});
		}
		else {
			ChatHandlerFunctions::showError(player, "Invalid player: " + targetName);
		}

		return true;
	}
	return false;
}

auto ManagementFunctions::ipBan(Player *player, const string_t &args) -> bool {
	match_t matches;
	if (ChatHandlerFunctions::runRegexPattern(args, R"((\w+) ?(\d+)?)", matches)) {
		string_t targetName = matches[1];
		if (Player *target = PlayerDataProvider::getInstance().getPlayer(targetName)) {
			const string_t &targetIp = target->getIp().toString();
			target->getSession()->disconnect();

			string_t reasonString = matches[2];
			int8_t reason = reasonString.length() > 0 ? atoi(reasonString.c_str()) : 1;

			// IP ban
			soci::session &sql = Database::getCharDb();
			soci::statement st = (sql.prepare << "INSERT INTO ip_bans (ip) VALUES (:ip)", soci::use(targetIp, "ip"));

			st.execute();

			if (st.get_affected_rows() > 0) {
				const string_t &banMessage = targetName + " has been banned" + ChatHandlerFunctions::getBanString(reason);
				PlayerPacket::showMessageChannel(banMessage, PlayerPacket::NoticeTypes::Notice);

				ChannelServer::getInstance().log(LogType::GmCommand, [&](out_stream_t &log) {
					log << "GM " << player->getName()
						<< " IP banned a player with reason " << reason
						<< ", player: " << targetName;
				});
			}
			else {
				ChatHandlerFunctions::showError(player, "Unknown error, couldn't ban " + targetIp);
			}
		}
		else {
			// TODO FIXME add raw IP banning
			ChatHandlerFunctions::showError(player, "Invalid player: " + targetName);
		}
		return true;
	}
	return false;
}

auto ManagementFunctions::unban(Player *player, const string_t &args) -> bool {
	if (args.length() != 0) {
		// Unban account
		soci::session &sql = Database::getCharDb();
		soci::statement st = (sql.prepare
			<< "UPDATE user_accounts u "
			<< "INNER JOIN characters c ON u.user_id = c.user_id "
			<< "SET "
			<< "	u.banned = 0, "
			<< "	u.ban_reason = NULL, "
			<< "	u.ban_expire = NULL "
			<< "WHERE c.name = :name ",
			soci::use(args, "name"));

		st.execute();

		if (st.get_affected_rows() > 0) {
			const string_t &banMessage = args + " has been unbanned";
			ChatHandlerFunctions::showInfo(player, banMessage);
			ChannelServer::getInstance().log(LogType::GmCommand, [&](out_stream_t &log) {
				log << "GM " << player->getName()
					<< " unbanned a player: " << args;
			});
		}
		else {
			ChatHandlerFunctions::showError(player, "Invalid player: " + args);
		}
		return true;
	}
	return false;
}

auto ManagementFunctions::rehash(Player *player, const string_t &args) -> bool {
	WorldServerConnectPacket::rehashConfig();
	ChatHandlerFunctions::showInfo(player, "Sent a signal to force rehashing world configurations");
	return true;
}

auto ManagementFunctions::rates(Player *player, const string_t &args) -> bool {
	match_t matches;
	if (args.length() > 0) {
		if (!ChatHandlerFunctions::runRegexPattern(args, R"((\w+) ?(\d+)?)", matches)) {
			return false;
		}
		string_t type = matches[1];
		if (type == "view") {
			auto display = [player](const string_t &type, int32_t rate) {
				ChatHandlerFunctions::showInfo(player, type + " rate: " + StringUtilities::lexical_cast<string_t>(rate) + "x");
			};
			ChatHandlerFunctions::showInfo(player, "Current rates");
			auto &config = ChannelServer::getInstance().getConfig();
			display("Mob EXP", config.rates.mobExpRate);
			display("Mob meso", config.rates.mobMesoRate);
			display("Quest EXP", config.rates.questExpRate);
			display("Drop", config.rates.dropRate);
		}
		else if (ChatHandlerFunctions::runRegexPattern(args, R"((\w+) (\d+))", matches)) {
			string_t value = matches[2];
			int32_t rateType = 0;
			int32_t newAmount = value.length() > 0 ? atoi(value.c_str()) : 1;

			if (type == "mobexp") rateType = Rates::Types::MobExpRate;
			if (type == "mobmeso") rateType = Rates::Types::MobMesoRate;
			if (type == "questexp") rateType = Rates::Types::QuestExpRate;
			if (type == "drop") rateType = Rates::Types::DropRate;
			ChannelServer::getInstance().modifyRate(rateType, newAmount);
			ChatHandlerFunctions::showInfo(player, "Sent request to modify rate");
		}
		else {
			return false;
		}
	}
	else {
		ChatHandlerFunctions::showInfo(player, "Sent request to reset rates");
		SyncPacket::ConfigPacket::resetRates();
	}
	return true;
}