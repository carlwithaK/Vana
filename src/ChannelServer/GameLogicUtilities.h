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
#ifndef GAMELOGICUTILITIES_H
#define GAMELOGICUTILITIES_H

#include "GameConstants.h"
#include "Pos.h"

namespace GameLogicUtilities {
	inline uint8_t getInventory(int32_t itemid) { return static_cast<uint8_t>(itemid / 1000000); }
	inline int32_t getItemType(int32_t itemid) { return (itemid / 10000); }
	inline bool isArrow(int32_t itemid) { return (getItemType(itemid) == ItemArrow); }
	inline bool isStar(int32_t itemid) { return (getItemType(itemid) == ItemStar); }
	inline bool isBullet(int32_t itemid) { return (getItemType(itemid) == ItemBullet); }
	inline bool isRechargeable(int32_t itemid) { return (isBullet(itemid) || isStar(itemid)); }
	inline bool isEquip(int32_t itemid) { return (getInventory(itemid) == 1); }
	inline bool isPet(int32_t itemid) {	return ((itemid / 100 * 100) == 5000000); }
	inline bool isOverall(int32_t itemid) { return (getItemType(itemid) == ArmorOverall); }
	inline bool isTop(int32_t itemid) { return (getItemType(itemid) == ArmorTop); }
	inline bool isBottom(int32_t itemid) { return (getItemType(itemid) == ArmorBottom); }
	inline bool isShield(int32_t itemid) { return (getItemType(itemid) == ArmorShield); }
	inline bool is2hWeapon(int32_t itemid) { return (getItemType(itemid) / 10 == 14); }
	inline bool is1hWeapon(int32_t itemid) { return (getItemType(itemid) / 10 == 13); }
	inline bool isBeginnerSkill(int32_t skillid) { return (skillid / 1000000 == 0); }
	inline bool isFourthJobSkill(int32_t skillid) { return ((skillid / 10000) % 10 == 2); }
	inline bool isSummon(int32_t skillid) { return (skillid == Jobs::Sniper::Puppet || skillid == Jobs::Ranger::Puppet || skillid == Jobs::Priest::SummonDragon || skillid == Jobs::Ranger::SilverHawk || skillid == Jobs::Sniper::GoldenEagle || skillid == Jobs::DarkKnight::Beholder || skillid == Jobs::FPArchMage::Elquines || skillid == Jobs::ILArchMage::Ifrit || skillid == Jobs::Bishop::Bahamut || skillid == Jobs::Bowmaster::Phoenix || skillid == Jobs::Marksman::Frostprey || skillid == Jobs::Outlaw::Octopus || skillid == Jobs::Outlaw::Gaviota); }
	inline bool isPuppet(int32_t skillid) { return (skillid == Jobs::Sniper::Puppet || skillid == Jobs::Ranger::Puppet); }
	inline bool isInBox(const Pos &start, const Pos &lt, const Pos &rb, const Pos &test) { return (((test.x >= start.x + lt.x) && (test.x <= start.x + rb.x)) && ((test.y >= start.y + lt.y) && (test.y <= start.y + rb.y))); }
	inline int8_t getPartyMember1(int8_t totalmembers) { return (totalmembers >= 1 ? (0x40 >> totalmembers) : 0xFF); }
	inline int8_t getPartyMember2(int8_t totalmembers) { return (totalmembers >= 2 ? (0x80 >> totalmembers) : 0xFF); }
	inline int8_t getPartyMember3(int8_t totalmembers) { return (totalmembers >= 3 ? (0x100 >> totalmembers) : 0xFF); }
	inline int8_t getPartyMember4(int8_t totalmembers) { return (totalmembers >= 4 ? (0x200 >> totalmembers) : 0xFF); }
	inline int8_t getPartyMember5(int8_t totalmembers) { return (totalmembers >= 5 ? (0x400 >> totalmembers) : 0xFF); }
	inline int8_t getPartyMember6(int8_t totalmembers) { return (totalmembers >= 6 ? (0x800 >> totalmembers) : 0xFF); }
};

#endif