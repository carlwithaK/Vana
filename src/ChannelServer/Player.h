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
#ifndef PLAYER_H
#define PLAYER_H

#include "AbstractPlayer.h"
#include "ChannelServer.h"
#include "MovableLife.h"
#include "PlayerActiveBuffs.h"
#include "PlayerBuddyList.h"
#include "PlayerInventory.h"
#include "PlayerMounts.h"
#include "PlayerPets.h"
#include "PlayerQuests.h"
#include "PlayerSkills.h"
#include "PlayerStorage.h"
#include "PlayerSummons.h"
#include "PlayerVariables.h"
#include "Pos.h"
#include "Quests.h"
#include "Skills.h"
#include <boost/scoped_ptr.hpp>
#include <boost/tr1/unordered_set.hpp>
#include <string>
#include <vector>

using boost::scoped_ptr;
using std::string;
using std::vector;
using std::tr1::unordered_set;

class Instance;
class NPC;
class PacketReader;
class Party;
struct PortalInfo;

class Player : public AbstractPlayer, public MovableLife {
public:
	Player();

	~Player();

	void realHandleRequest(PacketReader &packet);

	void setSaveOnDc(bool save) { save_on_dc = save; }
	void setTrading(bool state) { trade_state = state; }
	void setSkin(int8_t id);
	void setFallCounter(int8_t falls) { fall_counter = falls; }
	void setLevel(uint8_t level);
	void setAp(int16_t ap);
	void setSp(int16_t sp);
	void setFame(int16_t fame);
	void setJob(int16_t job);
	void setStr(int16_t str);
	void setDex(int16_t dex);
	void setInt(int16_t intt);
	void setLuk(int16_t luk);
	void modifyHp(int16_t hp, bool is = true); // Bases its calculations on current HP/MP
	void modifyMp(int16_t mp, bool is = false);
	void damageHp(uint16_t dhp); // Calculations done based on the fact that damage can range from 0 to ~55k
	void damageMp(uint16_t dmp);
	void setHp(int16_t hp, bool is = true); // Only use setHp/Mp if you're going to do checking, they fall easily to datatype issues
	void setMp(int16_t mp, bool is = false); // For example, Power Elixir at 30k HP = 30k + 30k = 60k, but wait! That's > 32767, so it's negative
	void setMHp(int16_t mhp);
	void setMMp(int16_t mmp);
	void modifyRMHp(int16_t mod);
	void modifyRMMp(int16_t mod);
	void setRMHp(int16_t rmhp);
	void setRMMp(int16_t rmmp);
	void setHyperBody(int16_t modx, int16_t mody);
	void setHpMpAp(uint16_t ap) { hpmp_ap = ap; }
	void setEyes(int32_t id);
	void setHair(int32_t id);
	void setExp(int32_t exp);
	void setMap(int32_t mapid, PortalInfo *portal = 0);
	void setBuddyListSize(uint8_t size);
	void setTradeId(int32_t id) { this->trade_id = id; }
	void setShop(int32_t shopid) { shop = shopid; }
	void setNPC(NPC *npc) { this->npc = npc; }
	void setParty(Party *party) { this->party = party; }
	void setInstance(Instance *instance) { this->instance = instance; }
	void setChair(int32_t chair) { this->chair = chair; }
	void setItemEffect(int32_t effect) { this->item_effect = effect; }
	void setSpecialSkill(SpecialSkillInfo info) { this->info = info; }

	int8_t getWorldId() const { return world_id; }
	int8_t getGender() const { return gender; }
	int8_t getSkin() const { return skin; }
	int8_t getMappos() const { return map_pos; }
	int8_t getFallCounter() const { return fall_counter; }
	uint8_t getLevel() const { return level; }
	uint8_t getBuddyListSize() const { return buddylist_size; }
	int16_t getJob() const { return job; }
	int16_t getStr() const { return str; }
	int16_t getDex() const { return dex; }
	int16_t getInt() const { return intt; }
	int16_t getLuk() const { return luk; }
	int16_t getAp() const { return ap; }
	int16_t getSp() const { return sp; }
	int16_t getFame() const { return fame; }
	int16_t getHp() const { return hp; }
	int16_t getMp() const { return mp; }
	int16_t getMHp() const { return mhp; }
	int16_t getRMHp() const { return rmhp; }
	int16_t getMMp() const { return mmp; }
	int16_t getRMMp() const { return rmmp; }
	uint16_t getHpMpAp() const { return hpmp_ap; }
	int32_t getId() const { return id; }
	int32_t getUserId() const { return user_id; }
	int32_t getEyes() const { return eyes; }
	int32_t getHair() const { return hair; }
	int32_t getExp() const { return exp; }
	int32_t getMap() const { return map; }
	int32_t getShop() const { return shop; }
	int32_t getChair() const { return chair; }
	int32_t getItemEffect() const { return item_effect; }
	int32_t getGmLevel() const { return gm_level; }
	int32_t getSpecialSkill() const { return info.skillid; }
	int32_t getTradeId() const { return trade_id; }
	string getName() const { return name; }
	NPC * getNPC() const { return npc; }
	Party * getParty() const { return party; }
	Instance * getInstance() const { return instance; }
	bool isGm() const { return gm_level > 0; }
	bool isTrading() const { return trade_state; }
	SpecialSkillInfo getSpecialSkillInfo() const { return info; }

	bool hasGmEquip();

	PlayerActiveBuffs * getActiveBuffs() const { return activeBuffs.get(); }
	PlayerBuddyList * getBuddyList() const { return buddyList.get(); }
	PlayerInventory * getInventory() const { return inv.get(); }
	PlayerMounts * getMounts() const { return mounts.get(); }
	PlayerPets * getPets() const { return pets.get(); }
	PlayerQuests * getQuests() const { return quests.get(); }
	PlayerSkills * getSkills() const { return skills.get(); }
	PlayerStorage * getStorage() const { return storage.get(); }
	PlayerSummons * getSummons() const { return summons.get(); }
	PlayerVariables * getVariables() const { return variables.get(); }

	// For "onlyOnce" portals
	void addUsedPortal(int8_t portalId) { used_portals.insert(portalId); }
	bool usedPortal(int8_t portalId) const { return used_portals.find(portalId) != used_portals.end(); }

	bool addWarning();
	void addWishlistItem(int32_t id) { wishlist.push_back(id); }
	vector<int32_t> getWishlist() const { return wishlist; }
	void changeChannel(int8_t channel);
	void saveStats();
	void saveAll(bool savecooldowns = false);
	void setOnline(bool online);
	void setLevelDate();
	void acceptDeath();
	void loseExp();
private:
	void playerConnect(PacketReader &packet);
	void changeKey(PacketReader &packet);
	void changeSkillMacros(PacketReader &packet);

	int8_t world_id;
	int8_t map_pos;
	int8_t gender;
	int8_t skin;
	int8_t fall_counter;
	uint8_t level;
	uint8_t buddylist_size;
	int16_t job;
	int16_t fame;
	int16_t str;
	int16_t dex;
	int16_t intt;
	int16_t luk;
	int16_t ap;
	int16_t sp;
	int16_t hp;
	int16_t mhp;
	int16_t rmhp;
	int16_t mp;
	int16_t mmp;
	int16_t rmmp;
	uint16_t hpmp_ap;
	int32_t id;
	int32_t user_id;
	int32_t eyes;
	int32_t hair;
	int32_t exp;
	int32_t map;
	int32_t shop;
	int32_t item_effect;
	int32_t chair;
	int32_t gm_level;
	int32_t trade_id;
	bool trade_state;
	bool save_on_dc;
	bool is_connect;
	string name;
	NPC *npc;
	Instance *instance;
	Party *party;
	unordered_set<int8_t> used_portals;
	vector<int32_t> warnings;
	vector<int32_t> wishlist;
	SpecialSkillInfo info; // Hurricane/Pierce/Big Bang/Monster Magnet/etc.

	scoped_ptr<PlayerActiveBuffs> activeBuffs;
	scoped_ptr<PlayerBuddyList> buddyList;
	scoped_ptr<PlayerInventory> inv;
	scoped_ptr<PlayerMounts> mounts;
	scoped_ptr<PlayerPets> pets;
	scoped_ptr<PlayerQuests> quests;
	scoped_ptr<PlayerSkills> skills;
	scoped_ptr<PlayerStorage> storage;
	scoped_ptr<PlayerSummons> summons;
	scoped_ptr<PlayerVariables> variables;
};

class PlayerFactory : public AbstractPlayerFactory {
public:
	AbstractPlayer * createPlayer() {
		return new Player();
	}
};

#endif
