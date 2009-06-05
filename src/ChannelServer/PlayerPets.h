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
#ifndef PLAYERPETS_H
#define PLAYERPETS_H

#include <boost/tr1/unordered_map.hpp>

using std::tr1::unordered_map;

class Pet;
class Player;


class PlayerPets {
public:
	PlayerPets(Player *player) : player(player) { }
	void addPet(Pet *pet);
	Pet * getPet(int32_t petid);
	void setSummoned(int8_t index, int32_t petid);
	Pet * getSummoned(int8_t index);
	void save();
private:
	unordered_map<int32_t, Pet *> playerpets;
	unordered_map<int8_t, int32_t> summoned;
	Player *player;
};

#endif
