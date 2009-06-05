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
#ifndef PETS_H
#define PETS_H

#include "MovableLife.h"
#include <boost/tr1/unordered_map.hpp>
#include <string>

using std::string;
using std::tr1::unordered_map;

class Pet;
class Player;
class PacketReader;
struct Item;

struct PetInfo {
	string name;
	int32_t hunger;
};

struct PetInteractInfo {
	uint32_t prob;
	int16_t increase;
};

namespace Pets {
	extern unordered_map<int32_t, PetInfo> petsInfo;
	extern unordered_map<int32_t, unordered_map<int32_t, PetInteractInfo> > petsInteractInfo;
	extern int16_t exps[29];
	void showPets(Player *player);
	void handleChat(Player *player, PacketReader &packet);
	void handleFeed(Player *player, PacketReader &packet);
	void handleMovement(Player *player, PacketReader &packet);
	void handleSummon(Player *player, PacketReader &packet);
	void handleCommand(Player *player, PacketReader &packet);
	void changeName(Player *player, const string &name);
};

class Pet : public MovableLife {
public:
	Pet(Player *player, Item *item);
	Pet(Player *player, Item *item, int8_t index, string name, int8_t level, int16_t closeness, int8_t fullness, int8_t inventorySlot);

	void setIndex(int8_t index) { this->index = index; }
	void setInventorySlot(int8_t slot) { this->inventorySlot = slot; }

	void setName(const string &name);
	void modifyFullness(int8_t offset, bool sendPacket = true);
	void addCloseness(int16_t amount);

	int8_t getIndex() const { return this->index; }
	int8_t getLevel() const { return this->level; }
	int8_t getInventorySlot() const { return this->inventorySlot; }
	int8_t getFullness() const { return this->fullness; }
	int16_t getCloseness() const { return this->closeness; }
	int16_t getPosX() const { return m_pos.x; }
	int16_t getPosY() const { return m_pos.y - 1; }
	int32_t getId() const { return this->id; }
	int32_t getType() const { return this->type; }
	bool isSummoned() const { return this->index != -1; }
	string getName() { return this->name; }
	Pos getPos() const { return Pos(getPosX(), getPosY()); }

	void startTimer();
private:
	Player *player;
	int32_t id;
	int32_t type;
	int8_t index;
	string name;
	int8_t level;
	int8_t fullness;
	int16_t closeness;
	int8_t inventorySlot;

	void levelUp();
};

#endif
