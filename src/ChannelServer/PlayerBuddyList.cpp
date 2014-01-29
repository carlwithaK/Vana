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
#include "PlayerBuddyList.hpp"
#include "Algorithm.hpp"
#include "BuddyListPacket.hpp"
#include "ChannelServer.hpp"
#include "Database.hpp"
#include "GameConstants.hpp"
#include "MiscUtilities.hpp"
#include "PacketCreator.hpp"
#include "Player.hpp"
#include "StringUtilities.hpp"
#include "SyncPacket.hpp"
#include <algorithm>

PlayerBuddyList::PlayerBuddyList(Player *player) :
	m_player(player)
{
	load();
}

auto PlayerBuddyList::load() -> void {
	soci::session &sql = Database::getCharDb();

	soci::rowset<> rs = (sql.prepare
		<< "SELECT bl.id, bl.buddy_character_id, bl.name AS name_cache, c.name, bl.group_name, CASE WHEN c.online = 1 THEN u.online ELSE 0 END AS `online` "
		<< "FROM buddylist bl "
		<< "LEFT JOIN characters c ON bl.buddy_character_id = c.character_id "
		<< "LEFT JOIN user_accounts u ON c.user_id = u.user_id "
		<< "WHERE bl.character_id = :char",
		soci::use(m_player->getId(), "char"));

	for (const auto &row : rs) {
		addBuddy(sql, row);
	}

	rs = (sql.prepare
		<< "SELECT p.* "
		<< "FROM buddylist_pending p "
		<< "LEFT JOIN characters c ON c.character_id = p.inviter_character_id "
		<< "WHERE c.world_id = :world AND p.character_id = :char ",
		soci::use(m_player->getId(), "char"),
		soci::use(ChannelServer::getInstance().getWorldId(), "world"));

	BuddyInvite invite;
	for (const auto &row : rs) {
		invite = BuddyInvite();
		invite.id = row.get<int32_t>("inviter_character_id");
		invite.name = row.get<string_t>("inviter_name");
		m_pendingBuddies.push_back(invite);
	}
}

auto PlayerBuddyList::addBuddy(const string_t &name, const string_t &group, bool invite) -> uint8_t {
	if (listSize() >= m_player->getBuddyListSize()) {
		// Buddy list full
		return BuddyListPacket::Errors::BuddyListFull;
	}

	if (!ext::in_range_inclusive<size_t>(name.size(), Characters::MinNameSize, Characters::MaxNameSize) || group.size() > Buddies::MaxGroupNameSize) {
		// Invalid name or group length
		return BuddyListPacket::Errors::UserDoesNotExist;
	}

	soci::session &sql = Database::getCharDb();
	soci::row row;

	sql.once
		<< "SELECT c.character_id, c.name, u.gm_level, c.buddylist_size AS buddylist_limit, ("
		<< "	SELECT COUNT(b.id) "
		<< "	FROM buddylist b "
		<< "	WHERE b.character_id = c.character_id"
		<< ") AS buddylist_size "
		<< "FROM characters c "
		<< "INNER JOIN user_accounts u ON c.user_id = u.user_id "
		<< "WHERE c.name = :name AND c.world_id = :world ",
		soci::use(name, "name"),
		soci::use(ChannelServer::getInstance().getWorldId(), "world"),
		soci::into(row);

	if (!sql.got_data()) {
		// Name does not exist
		return BuddyListPacket::Errors::UserDoesNotExist;
	}

	if (row.get<int32_t>("gm_level") > 0 && !m_player->isGm()) {
		// GM cannot be in buddy list unless the player is a GM
		return BuddyListPacket::Errors::NoGms;
	}

	if (row.get<int64_t>("buddylist_size") >= row.get<int32_t>("buddylist_limit")) {
		// Opposite-end buddy list full
		return BuddyListPacket::Errors::TargetListFull;
	}

	int32_t charId = row.get<int32_t>("character_id");

	if (m_buddies.find(charId) != std::end(m_buddies)) {
		if (m_buddies[charId]->groupName == group) {
			// Already in buddy list
			return BuddyListPacket::Errors::AlreadyInList;
		}
		else {
			sql.once
				<< "UPDATE buddylist "
				<< "SET group_name = :name "
				<< "WHERE buddy_character_id = :buddy AND character_id = :owner ",
				soci::use(group, "name"),
				soci::use(charId, "buddy"),
				soci::use(m_player->getId(), "owner");

			m_buddies[charId]->groupName = group;
		}
	}
	else {
		sql.once
			<< "INSERT INTO buddylist (character_id, buddy_character_id, name, group_name) "
			<< "VALUES (:owner, :buddy, :name, :group)",
			soci::use(name, "name"),
			soci::use(group, "group"),
			soci::use(charId, "buddy"),
			soci::use(m_player->getId(), "owner");

		int32_t rowId = Database::getLastId<int32_t>(sql);

		sql.once
			<< "SELECT bl.id, bl.buddy_character_id, bl.name AS name_cache, c.name, bl.group_name, CASE WHEN c.online = 1 THEN u.online ELSE 0 END AS `online` "
			<< "FROM buddylist bl "
			<< "LEFT JOIN characters c ON bl.buddy_character_id = c.character_id "
			<< "LEFT JOIN user_accounts u ON c.user_id = u.user_id "
			<< "WHERE bl.id = :row",
			soci::use(rowId, "row"),
			soci::into(row);

		addBuddy(sql, row);

		sql.once
			<< "SELECT id "
			<< "FROM buddylist "
			<< "WHERE character_id = :char AND buddy_character_id = :buddy",
			soci::use(charId, "char"),
			soci::use(m_player->getId(), "buddy"),
			soci::into(rowId);

		if (!sql.got_data()) {
			if (invite) {
				SyncPacket::BuddyPacket::buddyInvite(m_player->getId(), charId);
			}
		}
		else {
			vector_t<int32_t> idVector;
			idVector.push_back(charId);
			SyncPacket::BuddyPacket::buddyOnline(m_player->getId(), idVector, true);
		}
	}
	BuddyListPacket::update(m_player, BuddyListPacket::ActionTypes::Add);
	return BuddyListPacket::Errors::None;
}

auto PlayerBuddyList::removeBuddy(int32_t charId) -> void {
	if (m_pendingBuddies.size() != 0 && m_sentRequest) {
		BuddyInvite invite = m_pendingBuddies.front();
		if (invite.id == charId) {
			removePendingBuddy(charId, false);
		}
		return;
	}

	if (m_buddies.find(charId) == std::end(m_buddies)) {
		// Hacking
		return;
	}
	if (m_buddies[charId]->channel != -1) {
		vector_t<int32_t> idVector;
		idVector.push_back(charId);
		SyncPacket::BuddyPacket::buddyOnline(m_player->getId(), idVector, false);
	}

	m_buddies.erase(charId);

	Database::getCharDb().once
		<< "DELETE FROM buddylist "
		<< "WHERE character_id = :char AND buddy_character_id = :buddy",
		soci::use(m_player->getId(), "char"),
		soci::use(charId, "buddy");

	BuddyListPacket::update(m_player, BuddyListPacket::ActionTypes::Remove);
}

auto PlayerBuddyList::addBuddy(soci::session &sql, const soci::row &row) -> void {
	int32_t charId = row.get<int32_t>("buddy_character_id");
	int32_t rowId = row.get<int32_t>("id");
	opt_string_t name = row.get<opt_string_t>("name");
	opt_string_t group = row.get<opt_string_t>("group_name");
	string_t cache = row.get<string_t>("name_cache");

	if (name.is_initialized() && name.get() != cache) {
		// Outdated name cache, i.e. character renamed
		sql.once
			<< "UPDATE buddylist "
			<< "SET name = :name "
			<< "WHERE id = :id ",
			soci::use(name.get(), "name"),
			soci::use(rowId, "id");
	}

	ref_ptr_t<Buddy> buddy = make_ref_ptr<Buddy>();
	buddy->charId = charId;

	// Note that the cache is for displaying the character name when the
	// character in question is deleted.
	if (!name.is_initialized()) {
		// Buddy's character deleted
		buddy->name = cache;
	}
	else {
		buddy->name = name.get();
	}

	int32_t channelId = -1;
	int64_t online = row.get<int64_t>("online");
	if (online >= 20000) {
		online -= 20000;
		channelId = online % 100;
	}

	buddy->channel = channelId;
	if (!group.is_initialized()) {
		buddy->groupName = "Default Group";
		sql.once
			<< "UPDATE buddylist "
			<< "SET group_name = :name "
			<< "WHERE buddy_character_id = :buddy AND character_id = :owner ",
			soci::use(buddy->groupName, "name"),
			soci::use(charId, "buddy"),
			soci::use(m_player->getId(), "owner");
	}
	else {
		buddy->groupName = group.get();
	}

	sql.once
		<< "SELECT id "
		<< "FROM buddylist "
		<< "WHERE character_id = :char AND buddy_character_id = :buddy ",
		soci::use(charId, "char"),
		soci::use(m_player->getId(), "buddy"),
		soci::into(rowId);

	if (sql.got_data()) {
		buddy->oppositeStatus = BuddyListPacket::OppositeStatus::Registered;
	}
	else {
		buddy->oppositeStatus = BuddyListPacket::OppositeStatus::Unregistered;
	}

	m_buddies[charId] = buddy;
}

auto PlayerBuddyList::addBuddies(PacketCreator &packet) -> void {
	for (const auto &kvp : m_buddies) {
		const ref_ptr_t<Buddy> &buddy = kvp.second;
		packet.add<int32_t>(buddy->charId);
		packet.addString(buddy->name, 13);
		packet.add<uint8_t>(buddy->oppositeStatus);
		if (buddy->oppositeStatus == BuddyListPacket::OppositeStatus::Unregistered) {
			packet.add<uint16_t>(0x00);
			packet.add<uint8_t>(0xF0);
			packet.add<uint8_t>(0xB2);
		}
		else {
			packet.add<int32_t>(buddy->channel);
		}
		packet.addString(buddy->groupName, 13);
		packet.add<int8_t>(0x00);
		packet.add<int8_t>(20); // Seems to be the amount of buddy slots for the character...
		packet.add<uint8_t>(0xFD);
		packet.add<uint8_t>(0xBA);
	}
}

auto PlayerBuddyList::checkForPendingBuddy() -> void {
	if (m_pendingBuddies.size() == 0 || m_sentRequest) {
		// No buddies pending or request sent (didn't receive answer yet)
		return;
	}

	BuddyListPacket::invitation(m_player, m_pendingBuddies.front());
	m_sentRequest = true;
}

auto PlayerBuddyList::removePendingBuddy(int32_t id, bool accepted) -> void {
	if (m_pendingBuddies.size() == 0 || !m_sentRequest) {
		// Hacking
		return;
	}

	BuddyInvite invite = m_pendingBuddies.front();
	if (invite.id != id) {
		// Hacking
		ChannelServer::getInstance().log(LogType::Warning, [&](out_stream_t &log) {
			log << "Player tried to accept a player with player ID " << id
				<< " but the sent player ID was " << invite.id
				<< ". Player: " << m_player->getName();
		});
		return;
	}

	if (accepted) {
		int8_t error = addBuddy(invite.name, "Default Group", false);
		if (error != BuddyListPacket::Errors::None) {
			BuddyListPacket::error(m_player, error);
		}
		else {
			vector_t<int32_t> idVector;
			idVector.push_back(id);
			SyncPacket::BuddyPacket::buddyOnline(m_player->getId(), idVector, true);
		}

		Database::getCharDb().once
			<< "DELETE FROM buddylist_pending "
			<< "WHERE character_id = :char AND inviter_character_id = :buddy",
			soci::use(m_player->getId(), "char"),
			soci::use(id, "buddy");
	}

	BuddyListPacket::update(m_player, BuddyListPacket::ActionTypes::First);

	m_pendingBuddies.pop_front();
	m_sentRequest = false;
	checkForPendingBuddy();
}

auto PlayerBuddyList::getBuddyIds() -> vector_t<int32_t> {
	vector_t<int32_t> ids;
	for (const auto &kvp : m_buddies) {
		ids.push_back(kvp.second->charId);
	}

	return ids;
}