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
#ifndef PLAYERACTIVEBUFF_H
#define PLAYERACTIVEBUFF_H

#include "Buffs.h"
#include "Types.h"
#include <boost/tr1/memory.hpp>
#include <boost/tr1/unordered_map.hpp>
#include <list>
#include <vector>

using std::list;
using std::tr1::shared_ptr;
using std::tr1::unordered_map;
using std::vector;

class PacketCreator;
class PacketReader;
class Player;

namespace Timer {
	class Container;
};

struct BuffContainer {

};

class PlayerActiveBuffs {
public:
	PlayerActiveBuffs(Player *player) :
		m_player(player),
		m_combo(0),
		m_energycharge(0),
		m_activecharge(0),
		m_activebooster(0),
		m_pickpocketcounter(0),
		m_battleshiphp(0),
		m_debuffmask(0),
		m_berserk(false)
		{ }

	// Buff handling
	void addBuff(int32_t skill, int32_t time);
	void removeBuff(int32_t skill, bool fromTimer = false);
	void removeBuff();
	int32_t buffTimeLeft(int32_t skill);
	list<int32_t> getBuffs() const { return m_buffs; }
	void dispelBuffs();

	// Buff info
	void setActiveBuffsByType(ActiveBuffsByType &buffs);
	void addBuffInfo(int32_t skillid, const vector<Buff> &buffs);
	void setActiveSkillLevel(int32_t skillid, uint8_t level);
	uint8_t getActiveSkillLevel(int32_t skillid);
	ActiveBuff removeBuffInfo(int32_t skillid, const vector<Buff> &buffs);
	ActiveBuffsByType getBuffTypes() const { return m_activebuffsbytype; }

	// Buff map info
	void addMapEntryBuffInfo(ActiveMapBuff &buff);
	void deleteMapEntryBuffInfo(ActiveMapBuff &buff);
	void setMountInfo(int32_t skillid, int32_t mountid);
	void setMapEntryBuffs(MapEntryBuffs &buffs);
	MapEntryBuffs getMapEntryBuffs();

	// Skill "acts"
	void addAct(int32_t skill, Act act, int16_t value, int32_t time);
	Timer::Container * getActTimer(int32_t skill);
	void removeAct(int32_t skill);

	// Debuffs
	void addDebuff(uint8_t skill, uint8_t level);
	void useDispel();
	void useDebuffHealingItem(int32_t mask);
	void removeDebuff(uint8_t skill, bool fromTimer = false);
	int32_t getDebuffMask() const { return m_debuffmask; }
	void setDebuffMask(int32_t newmask) { m_debuffmask = newmask; }

	// Combo Attack
	void setCombo(uint8_t combo, bool sendPacket);
	void addCombo();
	uint8_t getCombo() const { return m_combo; }

	// Berserk
	bool getBerserk() const { return m_berserk; }
	void checkBerserk(bool display = false);

	// Energy Charge
	int16_t getEnergyChargeLevel() const { return m_energycharge; }
	void increaseEnergyChargeLevel(int8_t targets = 1);
	void decreaseEnergyChargeLevel();
	void setEnergyChargeLevel(int16_t chargelevel, bool startTimer = false);
	void resetEnergyChargeLevel();
	void startEnergyChargeTimer();
	void stopEnergyChargeTimer();

	// Boosters
	void setBooster(int32_t skillid) { m_activebooster = skillid; }
	void stopBooster();
	int32_t getBooster() const { return m_activebooster; }
	
	// White Knight/Paladin charges
	void setCharge(int32_t skillid) { m_activecharge = skillid; }
	void stopCharge();
	bool hasIceCharge() const;
	int32_t getCharge() const { return m_activecharge; }

	// Pickpocket
	int32_t getPickpocketCounter() { return ++m_pickpocketcounter; }

	// Battleship
	int32_t getBattleshipHp() const { return m_battleshiphp; }
	void setBattleshipHp(int32_t amount) { m_battleshiphp = amount; }
	void reduceBattleshipHp(uint16_t amount);
	void resetBattleshipHp();

	// Commonly referred to buffs on the server end
	const bool hasInfinity();
	const bool hasMesoUp();
	const bool hasHolySymbol();
	const bool hasPowerStance();
	const bool hasMagicGuard();
	const bool hasMesoGuard();
	const bool hasHyperBody();
	const bool hasHolyShield();
	const bool hasShadowPartner();
	const bool hasShadowStars();
	const bool hasSoulArrow();
	const bool isUsingHide();
	const bool isCursed();
	const int32_t getHolySymbol();
	const int32_t getPowerStance();
	const int32_t getHyperBody();
	const int32_t getCurrentMorph();
	const int32_t getMagicGuard();
	const int32_t getMesoGuard();
	const int16_t getMapleWarriorValue();

	// Packet marshaling
	void getBuffTransferPacket(PacketCreator &packet);
	void parseBuffTransferPacket(PacketReader &packet);
private:
	Player *m_player;
	uint8_t m_combo;
	int16_t m_energycharge;
	int32_t m_activecharge;
	int32_t m_activebooster;
	int32_t m_pickpocketcounter;
	int32_t m_battleshiphp;
	uint32_t m_timeseed;
	uint32_t m_debuffmask;
	bool m_berserk;
	list<int32_t> m_buffs;
	ActiveBuffsByType m_activebuffsbytype;
	MapEntryBuffs m_mapbuffs;
	unordered_map<int32_t, uint8_t> m_activelevels;
	unordered_map<int32_t, shared_ptr<Timer::Container> > m_skill_acts;

	int32_t calculateDebuffMaskBit(uint8_t skill);
};

#endif
