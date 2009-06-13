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

#ifndef PLAYERMONSTERBOOK_H
#define PLAYERMONSTERBOOK_H

#include "Types.h"
#include <boost/tr1/unordered_map.hpp>

using std::tr1::unordered_map;

class Player;
class PacketCreator;

struct MonsterCard {
	MonsterCard() : id(0), level(0) { };
	MonsterCard(int32_t id, uint8_t level) : id(id), level(level) { };
	int32_t id; // Card ID
	uint8_t level; // 1 - 5
};

class PlayerMonsterBook {
public:
	PlayerMonsterBook(Player *player);

	void connectData(PacketCreator &packet);
	void infoData(PacketCreator &packet);

	int32_t getSpecials() const { return m_specialcount; }
	int32_t getNormals() const { return m_normalcount; }
	int32_t getSize() const { return (int32_t) m_cards.size(); }
	int32_t getLevel() const { return m_level; }
	int32_t getCover() const { return m_cover; }
private:
	int32_t m_specialcount;
	int32_t m_normalcount;
	int32_t m_level;
	int32_t m_cover;
	Player *m_player;
	unordered_map<int32_t, MonsterCard> m_cards;
};

#endif