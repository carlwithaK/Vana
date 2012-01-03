/*
Copyright (C) 2008-2012 Vana Development Team

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
#include "PlayerMonsterBook.h"
#include "Database.h"
#include "GameConstants.h"
#include "GameLogicUtilities.h"
#include "ItemDataProvider.h"
#include "MonsterBookPacket.h"
#include "PacketCreator.h"
#include "Player.h"

PlayerMonsterBook::PlayerMonsterBook(Player *player) : m_player(player),  m_specialCount(0),  m_normalCount(0), m_level(1) {
	load();
}

void PlayerMonsterBook::load() {
	soci::session &sql = Database::getCharDb();
	int32_t charId = m_player->getId();

	soci::rowset<> rs = (sql.prepare << "SELECT b.card_id, b.level FROM monster_book b " <<
										"WHERE b.character_id = :char " <<
										"ORDER BY b.card_id ASC",
										soci::use(charId, "char"));

	for (soci::rowset<>::const_iterator i = rs.begin(); i != rs.end(); ++i) {
		soci::row const &row = *i;

		addCard(row.get<int32_t>("card_id"), row.get<uint8_t>("level"), true);
	}

	calculateLevel();
}

void PlayerMonsterBook::save() {
	soci::session &sql = Database::getCharDb();
	int32_t charId = m_player->getId();

	sql.once << "DELETE FROM monster_book WHERE character_id = :char", soci::use(charId, "char");

	if (m_cards.size() > 0) {
		int32_t cardId = 0;
		uint8_t level = 0;

		soci::statement st = (sql.prepare << "INSERT INTO monster_book " <<
												"VALUES (:char, :card, :level) ",
												soci::use(charId, "char"),
												soci::use(cardId, "card"),
												soci::use(level, "level"));
	

		for (unordered_map<int32_t, MonsterCard>::iterator iter = m_cards.begin(); iter != m_cards.end(); ++iter) {
			MonsterCard &c = iter->second;
			cardId = c.id;
			level = c.level;
			st.execute(true);
		}
	}
}

uint8_t PlayerMonsterBook::getCardLevel(int32_t cardId) {
	return m_cards[cardId].level;
}

bool PlayerMonsterBook::addCard(int32_t cardId, uint8_t level, bool initialLoad) {
	if (m_cards.find(cardId) == m_cards.end()) {
		if (GameLogicUtilities::isSpecialCard(cardId)) {
			++m_specialCount;
		}
		else {
			++m_normalCount;
		}
	}

	if (initialLoad) {
		MonsterCard card(cardId, level);
		m_cards[cardId] = card;
	}
	else {
		MonsterCard card = (m_cards.find(cardId) != m_cards.end() ? m_cards[cardId] : MonsterCard(cardId, 0));
		if (isFull(cardId)) {
			return true;
		}
		++card.level;
		m_cards[cardId] = card;
		if (card.level == 1) {
			calculateLevel();
		}
		return false;
	}
	return false;
}

void PlayerMonsterBook::connectData(PacketCreator &packet) {
	packet.add<int32_t>(getCover() != 0 ? ItemDataProvider::Instance()->getCardId(getCover()) : 0);
	packet.add<int8_t>(0);

	packet.add<uint16_t>(m_cards.size());
	for (unordered_map<int32_t, MonsterCard>::iterator iter = m_cards.begin(); iter != m_cards.end(); ++iter) {
		packet.add<int16_t>(GameLogicUtilities::getCardShortId(iter->second.id));
		packet.add<int8_t>(iter->second.level);
	}
}

void PlayerMonsterBook::calculateLevel() {
	int32_t size = getSize();
	m_level = MonsterCards::MaxPlayerLevel;
	for (int32_t i = 1; i < MonsterCards::MaxPlayerLevel; i++) {
		// We don't calculate for the last level because that includes all values above the second to last level
		if (size < MonsterCards::PlayerLevels[i - 1]) {
			m_level = i;
			break;
		}
	}
}

void PlayerMonsterBook::infoData(PacketCreator &packet) {
	packet.add<int32_t>(getLevel());
	packet.add<int32_t>(getNormals());
	packet.add<int32_t>(getSpecials());
	packet.add<int32_t>(getSize());
	packet.add<int32_t>(getCover());
}

MonsterCard * PlayerMonsterBook::getCard(int32_t cardId) {
	return (m_cards.find(cardId) != m_cards.end() ? &m_cards[cardId] : nullptr);
}

bool PlayerMonsterBook::isFull(int32_t cardId) {
	return (m_cards.find(cardId) != m_cards.end() ? (m_cards[cardId].level == MonsterCards::MaxCardLevel) : false);
}