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
#include "Mobs.h"
#include "Drops.h"
#include "DropsPacket.h"
#include "GameConstants.h"
#include "GameLogicUtilities.h"
#include "Instance.h"
#include "Inventory.h"
#include "Levels.h"
#include "Maps.h"
#include "MobsPacket.h"
#include "Movement.h"
#include "PacketCreator.h"
#include "PacketReader.h"
#include "Randomizer.h"
#include "Skills.h"
#include "SkillsPacket.h"
#include "Summons.h"
#include "Timer/Time.h"
#include "Timer/Timer.h"
#include <functional>

using std::tr1::bind;

// Mob status stuff
const int32_t Mobs::mobstatuses[19] = { // Order by value ascending
	StatusEffects::Mob::Watk,
	StatusEffects::Mob::Wdef,
	StatusEffects::Mob::Matk,
	StatusEffects::Mob::Mdef,
	StatusEffects::Mob::Acc,
	StatusEffects::Mob::Avoid,
	StatusEffects::Mob::Speed,
	StatusEffects::Mob::Stun,
	StatusEffects::Mob::Freeze,
	StatusEffects::Mob::Poison,
	StatusEffects::Mob::Seal,
	StatusEffects::Mob::WeaponAttackUp,
	StatusEffects::Mob::WeaponDefenseUp,
	StatusEffects::Mob::MagicAttackUp,
	StatusEffects::Mob::MagicDefenseUp,
	StatusEffects::Mob::Doom,
	StatusEffects::Mob::ShadowWeb,
	StatusEffects::Mob::WeaponImmunity,
	StatusEffects::Mob::MagicImmunity
};

/* Mob class */
Mob::Mob(int32_t id, int32_t mapid, int32_t mobid, Pos pos, int32_t spawnid, int16_t fh) :
MovableLife(fh, pos, 2),
originfh(fh),
id(id),
mapid(mapid),
spawnid(spawnid),
mobid(mobid),
status(0x8000000),
owner(0),
counter(0),
hasimmunity(false),
info(MobDataProvider::Instance()->getMobInfo(mobid)),
timers(new Timer::Container),
control(0)
{
	this->hp = info.hp;
	this->mp = info.mp;
	Instance *instance = Maps::getMap(mapid)->getInstance();
	if (instance != 0) {
		instance->sendMessage(MobSpawn, mobid, id);
	}
}

void Mob::applyDamage(int32_t playerid, int32_t damage, bool poison) {
	if (damage > hp)
		damage = hp - poison; // Keep HP from hitting 0 for poison and from going below 0

	damages[playerid] += damage;
	hp -= damage;

	if (!poison) { // HP bar packet does nothing for showing damage when poison is damaging for whatever reason
		Player *player = Players::Instance()->getPlayer(playerid);

		if (info.hpcolor > 0) // Boss HP bars - Horntail's damage sponge isn't a boss in the data
			MobsPacket::showBossHp(player, mobid, hp, info);
		else { // Normal/Miniboss HP bars
			uint8_t percent = static_cast<uint8_t>(hp * 100 / info.hp);
			MobsPacket::showHp(player, id, percent, info.boss);
		}
		Mob *htabusetaker = 0;
		switch (getMobId()) {
			case 8810002:
			case 8810003:
			case 8810004:
			case 8810005:
			case 8810006:
			case 8810007:
			case 8810008:
			case 8810009:
				htabusetaker = Maps::getMap(getMapId())->getMob(8810018, false);
				break;
		}
		if (htabusetaker != 0) {
			htabusetaker->applyDamage(playerid, damage, false);
		}
		if (hp == 0) { // Time to die
			if (mobid == 8810018) { // Horntail damage sponge, we want to be sure that his parts have spawned after death before we clean up
				new Timer::Timer(bind(&Mob::cleanHorntail, this, mapid, player),
					Timer::Id(Timer::Types::HorntailTimer, id, 0),
					0, Timer::Time::fromNow(500));
			}
			die(Players::Instance()->getPlayer(playerid));
		}
	}
}

void Mob::addStatus(int32_t playerid, vector<StatusInfo> statusinfo) {
	for (size_t i = 0; i < statusinfo.size(); i++) {
		if (statusinfo[i].status == StatusEffects::Mob::Poison && statuses.find(StatusEffects::Mob::Poison) != statuses.end()) {
			continue; // Already poisoned, so do not poison again
		}
		statuses[statusinfo[i].status] = statusinfo[i];
		MobsPacket::applyStatus(this, statusinfo[i], 300);

		if (statusinfo[i].status == StatusEffects::Mob::Poison) { // Damage timer for poison
			new Timer::Timer(bind(&Mob::applyDamage, this, playerid, statusinfo[i].val, true),
				Timer::Id(Timer::Types::MobStatusTimer, StatusEffects::Mob::Poison, 1),
				getTimers(), 0, 1000);
		}

		new Timer::Timer(bind(&Mob::removeStatus, this, statusinfo[i].status),
			Timer::Id(Timer::Types::MobStatusTimer, statusinfo[i].status, 0),
			getTimers(), Timer::Time::fromNow(statusinfo[i].time * 1000));
	}
	// Calculate new status mask
	this->status = 0x8000000;
	for (unordered_map<int32_t, StatusInfo>::iterator iter = statuses.begin(); iter != statuses.end(); iter++) {
		this->status += iter->first;
	}
}

void Mob::statusPacket(PacketCreator &packet) {
	packet.add<int32_t>(status);
	if (status != 0x8000000) {
		for (uint8_t i = 0; i < 19; i++) { // Val/skillid pairs must be ordered in the packet by status value ascending
			int32_t status = Mobs::mobstatuses[i];
			if (statuses.find(status) != statuses.end()) {
				packet.add<int16_t>(statuses[status].val);
				if (statuses[status].skillid >= 0) {
					packet.add<int32_t>(statuses[status].skillid);
				}
				else {
					packet.add<int16_t>(statuses[status].mobskill);
					packet.add<int16_t>(statuses[status].level);
				}
				packet.add<int16_t>(1);
			}
		}
	}
	packet.add<int32_t>(0);
}

bool Mob::hasStatus(int32_t status) {
	return (statuses.find(status) != statuses.end());
}

void Mob::removeStatus(int32_t status) {
	if (status == StatusEffects::Mob::WeaponImmunity || status == StatusEffects::Mob::MagicImmunity)
		setImmunity(false);
	this->status -= status;
	statuses.erase(status);
	if (status == StatusEffects::Mob::Poison) // Stop poison damage timer
		getTimers()->removeTimer(Timer::Id(Timer::Types::MobStatusTimer, status, 1));
	MobsPacket::removeStatus(this, status);
}

void Mob::setControl(Player *control) {
	/*if (this->control != 0)
		MobsPacket::endControlMob(this->control, this);*/
	this->control = control;
	if (control != 0)
		MobsPacket::requestControl(control, this, false);
}

void Mob::endControl() {
	if (control != 0 && control->getMap() == getMapId())
		MobsPacket::endControlMob(control, this);
}

void Mob::die(Player *player, bool fromexplosion) {
	endControl();
	int32_t highestdamager = 0;
	uint32_t highestdamage = 0;
	if (spawns.size() > 0) {
		for (unordered_map<int32_t, Mob *>::iterator spawniter = spawns.begin(); spawniter != spawns.end(); spawniter++) {
			spawniter->second->setOwner(0);
		}
	}
	if (getOwner() != 0) {
		owner->removeSpawn(getId());
	}
	for (unordered_map<int32_t, uint32_t>::iterator iter = damages.begin(); iter != damages.end(); iter++) {
		if (iter->second > highestdamage) { // Find the highest damager to give drop ownership
			highestdamager = iter->first;
			highestdamage = iter->second;
		}
		Player *damager = Players::Instance()->getPlayer(iter->first);
		if (damager == 0 || damager->getMap() != this->mapid || damager->getHp() == 0) // Only give EXP if the damager is in the same channel, on the same map and is alive
			continue;

		uint8_t multiplier = damager == player ? 10 : 8; // Multiplier for player to give the finishing blow is 1 and .8 for others. We therefore set this to 10 or 8 and divide the result in the formula found later on by 10.
		// Account for Holy Symbol
		int16_t hsrate = 0;
		if (damager->getActiveBuffs()->hasHolySymbol()) {
			int32_t hsid = damager->getActiveBuffs()->getHolySymbol();
			hsrate = Skills::skills[hsid][damager->getActiveBuffs()->getActiveSkillLevel(hsid)].x;
		}
		uint32_t exp = (info.exp * (multiplier * iter->second / info.hp)) / 10;
		Levels::giveEXP(damager, (exp + ((exp * hsrate) / 100)) * ChannelServer::Instance()->getExprate(), false, (damager == player));
	}

	// Spawn mob(s) the mob is supposed to spawn when it dies
	for (size_t i = 0; i < info.summon.size(); i++) {
		Maps::getMap(mapid)->spawnMob(info.summon[i], m_pos, -1, getFh(), this);
	}

	if (fromexplosion)
		MobsPacket::dieMob(this, 4);
	else
		MobsPacket::dieMob(this, 1);

	Drops::doDrops(highestdamager, mapid, mobid, getPos());

	if (player != 0)
		player->getQuests()->updateQuestMob(mobid);

	Instance *instance = Maps::getMap(mapid)->getInstance();
	if (instance != 0) {
		instance->sendMessage(MobDeath, mobid, id);
	}
	Maps::getMap(mapid)->removeMob(id, spawnid);

	delete this;
}

void Mob::explode() {
	die(0, true);
}

void Mob::die(bool showpacket) {
	if (showpacket) {
		endControl();
		MobsPacket::dieMob(this);
		Instance *instance = Maps::getMap(mapid)->getInstance();
		if (instance != 0) {
			instance->sendMessage(MobDeath, mobid, id);
		}
	}
	Maps::getMap(mapid)->removeMob(id, spawnid);
	delete this;
}

void Mob::cleanHorntail(int32_t mapid, Player *player) {
	for (int8_t q = 0; q < 8; q++) {
		Maps::getMap(mapid)->killMobs(player, (8810010 + q)); // Dead Horntail's parts
	}
}

void Mob::skillHeal(int32_t healhp, int32_t healmp) {
	int32_t minamount, maxamount, range, amount;
	minamount = healhp * 10 / 15;
	maxamount = healhp * 15 / 10;
	range = maxamount - minamount;
	amount = Randomizer::Instance()->randInt(range) + minamount;
	hp += amount;
	minamount = healmp * 10 / 15;
	maxamount = healmp * 15 / 10;
	range = maxamount - minamount;
	mp += Randomizer::Instance()->randInt(range) + minamount;
	if (hp > getMHp())
		hp = getMHp();
	if (mp > getMMp())
		mp = getMMp();
	MobsPacket::healMob(this, amount);
}

void Mob::dispelBuffs() {
	if (hasStatus(StatusEffects::Mob::Watk))
		removeStatus(StatusEffects::Mob::Watk);
	if (hasStatus(StatusEffects::Mob::Wdef))
		removeStatus(StatusEffects::Mob::Wdef);
	if (hasStatus(StatusEffects::Mob::Matk))
		removeStatus(StatusEffects::Mob::Matk);
	if (hasStatus(StatusEffects::Mob::Mdef))
		removeStatus(StatusEffects::Mob::Mdef);
	if (hasStatus(StatusEffects::Mob::Acc))
		removeStatus(StatusEffects::Mob::Acc);
	if (hasStatus(StatusEffects::Mob::Avoid))
		removeStatus(StatusEffects::Mob::Avoid);
	if (hasStatus(StatusEffects::Mob::Speed))
		removeStatus(StatusEffects::Mob::Speed);
}

void Mob::doCrashSkill(int32_t skillid) {
	switch (skillid) {
		case Jobs::Crusader::ArmorCrash:
			if (hasStatus(StatusEffects::Mob::Wdef))
				removeStatus(StatusEffects::Mob::Wdef);
			break;
		case Jobs::WhiteKnight::MagicCrash:
			if (hasStatus(StatusEffects::Mob::Matk))
				removeStatus(StatusEffects::Mob::Matk);
			break;
		case Jobs::DragonKnight::PowerCrash:
			if (hasStatus(StatusEffects::Mob::Watk))
				removeStatus(StatusEffects::Mob::Watk);
			break;
	}
}

/* Mobs namespace */
void Mobs::handleBomb(Player *player, PacketReader &packet) {
	int32_t mobid = packet.get<int32_t>();
	Mob *mob = Maps::getMap(player->getMap())->getMob(mobid);
	if (player->getHp() == 0 || mob == 0) {
		return;
	}
	if (mob->getSelfDestructHp() == 0) {
		// Hacking, I think
		return;
	}
	mob->explode();
}

void Mobs::monsterControl(Player *player, PacketReader &packet) {
	int32_t mobid = packet.get<int32_t>();

	Mob *mob = Maps::getMap(player->getMap())->getMob(mobid);

	if (mob == 0) {
		return;
	}

	int16_t moveid = packet.get<int16_t>();
	bool useskill = (packet.get<int8_t>() != 0);
	int8_t skill = packet.get<int8_t>();
	uint8_t realskill = 0;
	uint8_t level = 0;
	Pos target = packet.getPos();

	packet.skipBytes(9);
	Pos cpos = Movement::parseMovement(mob, packet);
	if (cpos - mob->getPos() > 300) {
		if (player->addWarning())
			return;
	}
	if (useskill && skill == -1 || useskill && skill == 0) {
		if (!mob->hasStatus(StatusEffects::Mob::Freeze) && !mob->hasStatus(StatusEffects::Mob::Stun)) {
			vector<MobSkillInfo> hurf = mob->getSkills();
			bool used = false;
			if (hurf.size()) {
				bool keepgoing = true;
				uint8_t rand = (uint8_t)(Randomizer::Instance()->randInt() % hurf.size());
				realskill = hurf[rand].skillid;
				level = hurf[rand].level;
				MobSkillLevelInfo mobskill = Skills::mobskills[realskill][level];
				switch (realskill) {
					case 100:
					case 110:
						if (mob->hasStatus(StatusEffects::Mob::Watk))
							keepgoing = false;
						break;
					case 101:
					case 111:
						if (mob->hasStatus(StatusEffects::Mob::Matk))
							keepgoing = false;
						break;
					case 102:
					case 112:
						if (mob->hasStatus(StatusEffects::Mob::Wdef))
							keepgoing = false;
						break;
					case 103:
					case 113:
						if (mob->hasStatus(StatusEffects::Mob::Mdef))
							keepgoing = false;
						break;					
					case 140:
					case 141:
						if (mob->hasImmunity())
							keepgoing = false;
						break;
					case 200: {
						int16_t spawns = (int16_t)(mob->getSpawns().size());
						int16_t limit = mobskill.limit;
						if (limit == 5000) // Custom limit based on number of players on map
							limit = 30 + Maps::getMap(mob->getMapId())->getNumPlayers() * 2;
						if (spawns >= limit)
							keepgoing = false;
						break;
					}
				}
				if (keepgoing) {
					time_t now = time(0);
					time_t ls = mob->getLastSkillUse(realskill);
					if (ls == 0 || ((int16_t)(now - ls) > mobskill.interval)) {
						mob->setLastSkillUse(realskill, now);
						int64_t reqhp = mob->getHp() * 100;
						reqhp /= mob->getMHp();
						if ((uint8_t)(reqhp) <= mobskill.hp) {
							if (hurf[rand].effectAfter == 0) {
								handleMobSkill(mob, realskill, level, mobskill);
							}
							else {
								new Timer::Timer(bind(&Mobs::handleMobSkill, mob, realskill, level, mobskill),
									Timer::Id(Timer::Types::MobSkillTimer, mob->getMobId(), mob->getCounter()),
									mob->getTimers(), Timer::Time::fromNow(hurf[rand].effectAfter));
							}
							used = true;
						}
					}
				}
			}
			if (!used) { 
				realskill = 0;
				level = 0;
			}
		}
	}
	MobsPacket::moveMobResponse(player, mobid, moveid, useskill, mob->getMp(), realskill, level);
	packet.reset(19);
	MobsPacket::moveMob(player, mobid, useskill, skill, target, packet.getBuffer(), packet.getBufferLength());
}

void Mobs::handleMobSkill(Mob *mob, uint8_t skillid, uint8_t level, const MobSkillLevelInfo &skillinfo) {
	Pos mobpos = mob->getPos();
	Map *map = Maps::getMap(mob->getMapId());
	vector<StatusInfo> statuses;
	bool pushed = false;
	switch (skillid) {
		case MobSkills::WeaponAttackUp:
		case MobSkills::WeaponAttackUpAoe:
			statuses.push_back(StatusInfo(StatusEffects::Mob::Watk, skillinfo.x, skillid, level, skillinfo.time));
			pushed = true;
			break;
		case MobSkills::MagicAttackUp:
		case MobSkills::MagicAttackUpAoe:
			statuses.push_back(StatusInfo(StatusEffects::Mob::Matk, skillinfo.x, skillid, level, skillinfo.time));
			pushed = true;
			break;
		case MobSkills::WeaponDefenseUp:
		case MobSkills::WeaponDefenseUpAoe:
			statuses.push_back(StatusInfo(StatusEffects::Mob::Wdef, skillinfo.x, skillid, level, skillinfo.time));
			pushed = true;
			break;
		case MobSkills::MagicDefenseUp:
		case MobSkills::MagicDefenseUpAoe:
			statuses.push_back(StatusInfo(StatusEffects::Mob::Mdef, skillinfo.x, skillid, level, skillinfo.time));
			pushed = true;
			break;
		case MobSkills::Heal:
			map->healMobs(skillinfo.x, skillinfo.y, mob->getPos(), skillinfo.lt, skillinfo.rb);
			break;
		case MobSkills::Seal:
		case MobSkills::Darkness:
		case MobSkills::Weakness:
		case MobSkills::Stun:
		case MobSkills::Curse:
		case MobSkills::Poison:
		case MobSkills::Slow:
		case MobSkills::Seduce:
			map->statusPlayers(skillid, level, skillinfo.count, skillinfo.prop, mob->getPos(), skillinfo.lt, skillinfo.rb);
			break;
		case MobSkills::Dispel:
			map->dispelPlayers(skillinfo.prop, mob->getPos(), skillinfo.lt, skillinfo.rb);
			break;
		case MobSkills::NoClue:
			// ??
			break;
		case MobSkills::PoisonMist:
			// TODO
			break;
		case MobSkills::NoClue2:
			// ???
			break;
		case MobSkills::WeaponImmunity:
			statuses.push_back(StatusInfo(StatusEffects::Mob::WeaponImmunity, skillinfo.x, skillid, level, skillinfo.time));
			mob->addStatus(0, statuses);
			mob->setImmunity(true);
			break;
		case MobSkills::MagicImmunity:
			statuses.push_back(StatusInfo(StatusEffects::Mob::MagicImmunity, skillinfo.x, skillid, level, skillinfo.time));
			mob->addStatus(0, statuses);
			mob->setImmunity(true);
			break;
		case MobSkills::Summon: {
			int16_t minx, maxx;
			int16_t miny = mobpos.y + skillinfo.lt.y;
			int16_t maxy = mobpos.y + skillinfo.rb.y;
			if (mob->isFacingRight()) {
				minx = mobpos.x + skillinfo.rb.x * -1;
				maxx = mobpos.x + skillinfo.lt.x * -1;
			}
			else {
				minx = mobpos.x + skillinfo.lt.x;
				maxx = mobpos.x + skillinfo.rb.x;
			}
			int16_t rangex = maxx - minx;
			int16_t rangey = maxy - miny;
			if (rangex < 0)
				rangex *= -1;
			if (rangey < 0)
				rangey *= -1;
			for (size_t summonsize = 0; summonsize < skillinfo.summons.size(); summonsize++) {
				int32_t spawnid = skillinfo.summons[summonsize];
				int16_t mobx = Randomizer::Instance()->randShort(rangex) + minx;
				int16_t moby = Randomizer::Instance()->randShort(rangey) + miny;
				Pos floor;
				if (mob->getMapId() == 220080001) { // Papulatus' map
					if (spawnid == 8500003) { // Keep High Darkstars high
						while ((floor.y > -538 || floor.y == moby) || !GameLogicUtilities::isInBox(mob->getPos(), skillinfo.lt, skillinfo.rb, floor)) { // Mobs spawn on the ground, we need them up top
							mobx = Randomizer::Instance()->randShort(rangex) + minx;
							moby = -590;
							floor = map->findFloor(Pos(mobx, moby));
						}
					}
					else if (spawnid == 8500004) { // Keep Low Darkstars low
						floor = map->findFloor(Pos(mobx, mobpos.y));
					}
				}
				else {
					floor = map->findFloor(Pos(mobx, moby));
					if (floor.y == moby) {
						floor.y = mobpos.y;
					}
				}
				map->spawnMob(spawnid, floor, -1, 0, mob, skillinfo.summoneffect);
			}
			break;
		}
	}
	if (pushed) {
		skillid = (skillid / 10) * 10 % 100;
		if (skillid == 0)
			mob->addStatus(0, statuses);
		if (skillid == 10)
			map->statusMobs(statuses, mob->getPos(), skillinfo.lt, skillinfo.rb);
	}
}

void Mobs::damageMob(Player *player, PacketReader &packet) {
	MobsPacket::damageMob(player, packet);
	packet.reset(2);
	packet.skipBytes(1); // Useless
	uint8_t tbyte = packet.get<int8_t>();
	int8_t targets = tbyte / 0x10;
	if (player->getSkills()->getSkillLevel(Jobs::Marauder::EnergyCharge) > 0)
		player->getActiveBuffs()->increaseEnergyChargeLevel(targets);
	int8_t hits = tbyte % 0x10;
	int32_t skillid = packet.get<int32_t>();
	switch (skillid) {
		case Jobs::Gunslinger::Grenade:
		case Jobs::Infighter::CorkscrewBlow:
			packet.skipBytes(4); // Charge
			break;
	}
	packet.skipBytes(8); // In order: Display [1], Animation [1], Weapon subclass [1], Weapon speed [1], Tick count [4]
	if (skillid > 0)
		Skills::useAttackSkill(player, skillid);
	int32_t map = player->getMap();
	uint32_t totaldmg = 0;
	uint8_t pplevel = player->getActiveBuffs()->getActiveSkillLevel(Jobs::ChiefBandit::Pickpocket); // Check for active pickpocket level
	for (int8_t i = 0; i < targets; i++) {
		int32_t targettotal = 0;
		int32_t mapmobid = packet.get<int32_t>();
		Mob *mob = Maps::getMap(map)->getMob(mapmobid);
		if (mob == 0)
			continue;
		packet.skipBytes(3); // Useless
		packet.skipBytes(1); // State
		packet.skipBytes(8); // Useless
		if (skillid != Jobs::ChiefBandit::MesoExplosion)
			packet.skipBytes(1); // Distance, first half for non-Meso Explosion
		int8_t num = packet.get<int8_t>(); // Distance, second half for non-Meso Explosion OR hits for Meso Explosion
		hits = skillid == Jobs::ChiefBandit::MesoExplosion ? num : hits;
		vector<int32_t> ppdamages; // Pickpocket
		Pos origin = mob->getPos(); // Info for pickpocket before mob is set to 0 (in the case that mob dies)
		for (int8_t k = 0; k < hits; k++) {
			int32_t damage = packet.get<int32_t>();
			targettotal += damage;
			if (skillid != Jobs::ChiefBandit::MesoExplosion && pplevel > 0) { // Make sure this is a melee attack and not meso explosion, plus pickpocket being active
				if (Randomizer::Instance()->randInt(99) < Skills::skills[Jobs::ChiefBandit::Pickpocket][pplevel].prop) {
					ppdamages.push_back(damage);
				}
			}
			if (mob == 0) {
				if (skillid != Jobs::ChiefBandit::MesoExplosion && pplevel > 0) // Roll along after the mob is dead to finish getting damage values for pickpocket
					continue;
				else {
					packet.skipBytes(4 * (hits - 1 - k));
					break;
				}
			}
			if (skillid == Jobs::Paladin::HeavensHammer && mob->isBoss()) {
				// Damage calculation goes in here, I think? Hearing conflicted views.
			}
			else {
				if (skillid == Jobs::Paladin::HeavensHammer)
					damage = mob->getHp() - 1;

				int32_t temphp = mob->getHp();
				mob->applyDamage(player->getId(), damage);
				if (temphp - damage <= 0) // Mob was killed, so set the Mob pointer to 0
					mob = 0;
			}
		}
		if (mob != 0 && targettotal > 0 && mob->getHp() > 0) {
			uint8_t weapontype = (uint8_t) GameLogicUtilities::getItemType(player->getInventory()->getEquippedId(EquipSlots::Weapon));
			handleMobStatus(player, mob, skillid, weapontype); // Mob status handler (freeze, stun, etc)
			if (mob->getHp() < mob->getSelfDestructHp()) {
				mob->explode();
			}
		}
		totaldmg += targettotal;
		uint8_t ppdamagesize = (uint8_t)(ppdamages.size());
		for (uint8_t pickpocket = 0; pickpocket < ppdamagesize; pickpocket++) { // Drop stuff for Pickpocket
			Pos pppos;
			pppos.x = origin.x + (ppdamagesize % 2 == 0 ? 5 : 0) + (ppdamagesize / 2) - 20 * ((ppdamagesize / 2) - pickpocket);
			pppos.y = origin.y;
			clock_t pptime = 175 * pickpocket;
			int32_t ppmesos = ((ppdamages[pickpocket] * Skills::skills[Jobs::ChiefBandit::Pickpocket][pplevel].x) / 10000); // TODO: Check on this formula in different situations
			Drop *ppdrop = new Drop(player->getMap(), ppmesos, pppos, player->getId(), true);
			ppdrop->setTime(100);
			new Timer::Timer(bind(&Drop::doDrop, ppdrop, origin),
				Timer::Id(Timer::Types::PickpocketTimer, player->getId(), player->getActiveBuffs()->getPickpocketCounter()),
				0, Timer::Time::fromNow(pptime));
		}
		if (!GameLogicUtilities::isSummon(skillid))
			packet.skipBytes(4); // 4 bytes of unknown purpose, new in .56
	}
	packet.skipBytes(4); // Character positioning, end of packet, might eventually be useful for hacking detection
	switch (skillid) {
		case Jobs::ChiefBandit::MesoExplosion: { // Meso Explosion
			uint8_t items = packet.get<int8_t>();
			int32_t map = player->getMap();
			for (uint8_t i = 0; i < items; i++) {
				int32_t objId = packet.get<int32_t>();
				packet.skipBytes(1); // Boolean for hit a monster
				Drop *drop = Maps::getMap(map)->getDrop(objId);
				if (drop != 0) {
					DropsPacket::explodeDrop(drop);
					Maps::getMap(map)->removeDrop(drop->getId());
					delete drop;
				}
			}
			break;
		}
		case Jobs::Crusader::SwordPanic: // Crusader finishers
		case Jobs::Crusader::SwordComa:
		case Jobs::Crusader::AxePanic:
		case Jobs::Crusader::AxeComa:
			player->getActiveBuffs()->setCombo(0, true);
			break;
		case Jobs::Crusader::Shout: // Shout
		case Jobs::Gm::SuperDragonRoar: // Super Dragon Roar
			break;
		case Jobs::DragonKnight::DragonRoar: { // Dragon Roar
			int8_t roarlv = player->getSkills()->getSkillLevel(skillid);
			int16_t x_value = Skills::skills[skillid][roarlv].x;
			uint16_t reduction = (player->getMHp() / 100) * x_value;
			if ((player->getHp() - reduction) > 0)
				player->damageHp(reduction);
			else {
				// Hacking
				return;
			}
			Buffs::Instance()->addBuff(player, Jobs::DragonKnight::DragonRoar, roarlv, 0);
			break;
		}
		case Jobs::DragonKnight::Sacrifice: { // Sacrifice
			int16_t hp_damage_x = Skills::skills[skillid][player->getSkills()->getSkillLevel(skillid)].x;
			uint16_t hp_damage = (uint16_t) totaldmg * hp_damage_x / 100;
			if ((player->getHp() - hp_damage) < 1)
				player->setHp(1);
			else
				player->damageHp(hp_damage);
			break;
		}
		case Jobs::WhiteKnight::ChargeBlow: { // Charge Blow
			int8_t acb_level = player->getSkills()->getSkillLevel(Jobs::Paladin::AdvancedCharge);
			int16_t acb_x = 0;
			if (acb_level > 0)
				acb_x = Skills::skills[Jobs::Paladin::AdvancedCharge][acb_level].x;
			if ((acb_x != 100) && (acb_x == 0 || Randomizer::Instance()->randShort(99) > (acb_x - 1)))
				player->getActiveBuffs()->stopCharge();
			break;
		}
		default:
			if (totaldmg > 0)
				player->getActiveBuffs()->addCombo();
			break;
	}
}

void Mobs::damageMobRanged(Player *player, PacketReader &packet) {
	MobsPacket::damageMobRanged(player, packet);
	packet.reset(2); // Passing to the display function causes the buffer to be eaten, we need it
	packet.skipBytes(1); // Number of portals taken (not kidding)
	uint8_t tbyte = packet.get<int8_t>();
	int8_t targets = tbyte / 0x10;
	int8_t hits = tbyte % 0x10;
	int32_t skillid = packet.get<int32_t>();
	uint8_t display = 0;
	switch (skillid) {
		case Jobs::Bowmaster::Hurricane:
		case Jobs::Marksman::PiercingArrow:
		case Jobs::Corsair::RapidFire:
			packet.skipBytes(4); // Charge time
			display = packet.get<int8_t>();
			if ((skillid == Jobs::Bowmaster::Hurricane || skillid == Jobs::Corsair::RapidFire) && player->getSpecialSkill() == 0) { // Only Hurricane constantly does damage and display it if not displayed
				SpecialSkillInfo info;
				info.skillid = skillid;
				info.direction = packet.get<int8_t>();
				packet.skipBytes(1); // Weapon subclass
				info.w_speed = packet.get<int8_t>();
				info.level = player->getSkills()->getSkillLevel(info.skillid);
				player->setSpecialSkill(info);
				SkillsPacket::showSpecialSkill(player, info);
			}
			else
				packet.skipBytes(3);
			break;
		default:
			display = packet.get<int8_t>(); // Projectile display
			packet.skipBytes(1); // Direction/animation
			packet.skipBytes(1); // Weapon subclass
			packet.skipBytes(1); // Weapon speed
			break;
	}
	packet.skipBytes(4); // Ticks
	int16_t pos = packet.get<int16_t>();
	packet.skipBytes(2); // Cash Shop star cover
	packet.skipBytes(1); // 0x00 = AoE, 0x41 = other
	if (skillid != Jobs::Hermit::ShadowMeso && ((display & 0x40) > 0))
		packet.skipBytes(4); // Star ID added by Shadow Claw
	Skills::useAttackSkillRanged(player, skillid, pos, display);
	int32_t mhp = 0;
	uint32_t totaldmg = damageMobInternal(player, packet, targets, hits, skillid, mhp);
	if (skillid == Jobs::Assassin::Drain) { // Drain
		int16_t drain_x = Skills::skills[4101005][player->getSkills()->getSkillLevel(4101005)].x;
		int32_t hpRecover = totaldmg * drain_x / 100;
		if (hpRecover > mhp)
			hpRecover = mhp;
		if (hpRecover > (player->getMHp() / 2))
			hpRecover = player->getMHp() / 2;
		if (hpRecover > player->getMHp())
			player->setHp(player->getMHp());
		else
			player->modifyHp((int16_t) hpRecover);
	}
}

void Mobs::damageMobSpell(Player *player, PacketReader &packet) {
	MobsPacket::damageMobSpell(player, packet);
	packet.reset(2);
	packet.skipBytes(1);
	uint8_t tbyte = packet.get<int8_t>();
	int8_t targets = tbyte / 0x10;
	int8_t hits = tbyte % 0x10;
	int32_t skillid = packet.get<int32_t>();
	switch (skillid) {
		case Jobs::FPArchMage::BigBang: // Big Bang has a 4 byte charge time after skillid
		case Jobs::ILArchMage::BigBang:
		case Jobs::Bishop::BigBang:
			packet.skipBytes(4);
			break;
	}
	MpEaterInfo eater;
	eater.id = (player->getJob() / 10) * 100000;
	eater.level = player->getSkills()->getSkillLevel(eater.id);
	if (eater.level > 0) {
		eater.prop = Skills::skills[eater.id][eater.level].prop;
		eater.x = Skills::skills[eater.id][eater.level].x;
	}
	packet.skipBytes(2); // Display, direction/animation
	packet.skipBytes(2); // Weapon subclass, casting speed
	packet.skipBytes(4); // Ticks
	if (skillid != Jobs::Cleric::Heal) // Heal is sent as both an attack and as a used skill - always used, sometimes attack
		Skills::useAttackSkill(player, skillid);
	int32_t useless = 0;
	uint32_t totaldmg = damageMobInternal(player, packet, targets, hits, skillid, useless, &eater);
}

void Mobs::damageMobEnergyCharge(Player *player, PacketReader &packet) {
	MobsPacket::damageMobEnergyCharge(player, packet);
	packet.reset(2);
	packet.skipBytes(1);
	uint8_t tbyte = packet.get<int8_t>();
	int8_t targets = tbyte / 0x10;
	int8_t hits = tbyte % 0x10;
	int32_t skillid = packet.get<int32_t>();
	packet.skipBytes(2); // Display, direction/animation
	packet.skipBytes(2); // Weapon subclass, casting speed
	packet.skipBytes(4); // Ticks
	int32_t mapmobid = packet.get<int32_t>();
	Mob *mob = Maps::getMap(player->getMap())->getMob(mapmobid);
	if (mob == 0)
		return;
	packet.skipBytes(14); // ???
	int32_t damage = packet.get<int32_t>();
	mob->applyDamage(player->getId(), damage);
	packet.skipBytes(8); // End of packet	
}

void Mobs::damageMobSummon(Player *player, PacketReader &packet) {
	MobsPacket::damageMobSummon(player, packet);
	packet.reset(2);
	packet.skipBytes(4); // Summon ID
	Summon *summon = player->getSummons()->getSummon();
	if (summon == 0)
		// Hacking or some other form of tomfoolery
		return;
	packet.skipBytes(5);
	int8_t targets = packet.get<int8_t>();
	int32_t useless = 0;
	damageMobInternal(player, packet, targets, 1, summon->getSummonId(), useless);
}

uint32_t Mobs::damageMobInternal(Player *player, PacketReader &packet, int8_t targets, int8_t hits, int32_t skillid, int32_t &extra, MpEaterInfo *eater) {
	int32_t map = player->getMap();
	uint32_t total = 0;
	for (int8_t i = 0; i < targets; i++) {
		int32_t targettotal = 0;
		int32_t mapmobid = packet.get<int32_t>();
		Mob *mob = Maps::getMap(map)->getMob(mapmobid);
		if (mob == 0)
			return 0;
		if (skillid == Jobs::Cleric::Heal && !mob->isUndead()) {
			// hacking
			return 0;
		}
		packet.skipBytes(3); // Useless
		packet.skipBytes(1); // State
		packet.skipBytes(8); // Useless
		packet.skipBytes(2); // Distance
		for (int8_t k = 0; k < hits; k++) {
			int32_t damage = packet.get<int32_t>();
			targettotal += damage;
			if (mob == 0) {
				packet.skipBytes(4 * (hits - 1 - k));
				break;
			}
			extra = mob->getMHp();
			if (eater != 0) { // MP Eater
				int32_t cmp = mob->getMp();
				if ((!eater->onlyonce) && (damage != 0) && (cmp > 0) && (Randomizer::Instance()->randInt(99) < eater->prop)) {
					eater->onlyonce = true;
					int32_t mp = mob->getMMp() * eater->x / 100;
					if (mp > cmp)
						mp = cmp;
					mob->setMp(cmp - mp);
					player->modifyMp((int16_t) mp);
					SkillsPacket::showSkillEffect(player, eater->id);
				}
			}
			int32_t temphp = mob->getHp();
			mob->applyDamage(player->getId(), damage);
			if (temphp - damage <= 0) // Mob was killed, so set the Mob pointer to 0
				mob = 0;
		}
		if (mob != 0 && targettotal > 0 && mob->getHp() > 0) {
			handleMobStatus(player, mob, skillid, 0); // Mob status handler (freeze, stun, etc)
			if (mob->getHp() < mob->getSelfDestructHp()) {
				mob->explode();
			}
		}
		total += targettotal;
		if (!GameLogicUtilities::isSummon(skillid))
			packet.skipBytes(4); // 4 bytes of unknown purpose, new in .56
	}
	packet.skipBytes(4); // Character positioning, end of packet, might eventually be useful for hacking detection
	return total;
}

void Mobs::handleMobStatus(Player *player, Mob *mob, int32_t skillid, uint8_t weapon_type) {
	uint8_t level = skillid > 0 ? player->getSkills()->getSkillLevel(skillid) : 0;
	vector<StatusInfo> statuses;
	if (mob->canFreeze()) { // Freezing stuff
		switch (skillid) {
			case Jobs::ILWizard::ColdBeam:
			case Jobs::ILMage::IceStrike:
			case Jobs::ILMage::ElementComposition:
			case Jobs::Sniper::Blizzard:
			case Jobs::ILArchMage::Blizzard:
				statuses.push_back(StatusInfo(StatusEffects::Mob::Freeze, StatusEffects::Mob::Freeze, skillid, Skills::skills[skillid][level].time));
				break;
			case Jobs::Outlaw::IceSplitter: {
				int16_t y = 0;
				if (player->getSkills()->getSkillLevel(Jobs::Corsair::ElementalBoost) > 0)
					y = Skills::skills[Jobs::Corsair::ElementalBoost][player->getSkills()->getSkillLevel(Jobs::Corsair::ElementalBoost)].y;
				statuses.push_back(StatusInfo(StatusEffects::Mob::Freeze, StatusEffects::Mob::Freeze, skillid, Skills::skills[skillid][level].time + y));
				break;
			}
			case Jobs::FPArchMage::Elquines:
			case Jobs::Marksman::Frostprey:
				statuses.push_back(StatusInfo(StatusEffects::Mob::Freeze, StatusEffects::Mob::Freeze, skillid, Skills::skills[skillid][level].x));
				break;
		}
		if ((weapon_type == Weapon1hSword || weapon_type == Weapon2hSword || weapon_type == Weapon1hMace || weapon_type == Weapon2hMace) && player->getActiveBuffs()->hasIceCharge()) { // Ice Charges
			int32_t charge = player->getActiveBuffs()->getCharge();
			statuses.push_back(StatusInfo(StatusEffects::Mob::Freeze, StatusEffects::Mob::Freeze, charge, Skills::skills[charge][player->getActiveBuffs()->getActiveSkillLevel(charge)].y));
		}
	}
	if (mob->canPoison() && mob->getHp() > 1) { // Poisoning stuff
		if ((skillid == Jobs::FPWizard::PoisonBreath || skillid == Jobs::FPMage::PoisonMist || skillid == Jobs::FPMage::ElementComposition) && Randomizer::Instance()->randInt(99) < Skills::skills[skillid][level].prop) { // Poison brace, Element composition, and Poison mist
			int16_t pdamage = (int16_t)(mob->getMHp() / (70 - level));
			statuses.push_back(StatusInfo(StatusEffects::Mob::Poison, pdamage, skillid, Skills::skills[skillid][level].time));
		}
	}
	if (!mob->isBoss()) { // Seal, Stun, etc
		switch (skillid) {
			case Jobs::Hunter::ArrowBomb:
			case Jobs::Crusader::SwordComa:
			case Jobs::Crusader::AxeComa:
			case Jobs::Crusader::Shout:
			case Jobs::WhiteKnight::ChargeBlow:
			case Jobs::ChiefBandit::Assaulter:
			case Jobs::Shadower::BoomerangStep:
			case Jobs::Gunslinger::BlankShot:
				if (Randomizer::Instance()->randInt(99) < Skills::skills[skillid][level].prop) {
					statuses.push_back(StatusInfo(StatusEffects::Mob::Stun, StatusEffects::Mob::Stun, skillid, Skills::skills[skillid][level].time));
				}
				break;
			case Jobs::Infighter::BackspinBlow:
			case Jobs::Infighter::DoubleUppercut:
			case Jobs::Buccaneer::Demolition:
			case Jobs::Buccaneer::Snatch:
				statuses.push_back(StatusInfo(StatusEffects::Mob::Stun, StatusEffects::Mob::Stun, skillid, Skills::skills[skillid][level].time));
				break;
			case Jobs::Ranger::SilverHawk:
			case Jobs::Sniper::GoldenEagle:
				if (Randomizer::Instance()->randInt(99) < Skills::skills[skillid][level].prop) {
					statuses.push_back(StatusInfo(StatusEffects::Mob::Stun, StatusEffects::Mob::Stun, skillid, Skills::skills[skillid][level].x));
				}
				break;
			case Jobs::FPMage::Seal:
			case Jobs::ILMage::Seal:
				if (Randomizer::Instance()->randInt(99) < Skills::skills[skillid][level].prop) {
					statuses.push_back(StatusInfo(StatusEffects::Mob::Stun, StatusEffects::Mob::Stun, skillid, Skills::skills[skillid][level].time));
				}
				break;
			case Jobs::Priest::Doom:
				if (Randomizer::Instance()->randInt(99) < Skills::skills[skillid][level].prop) {
					statuses.push_back(StatusInfo(StatusEffects::Mob::Doom, 0x100, skillid, Skills::skills[skillid][level].time));
				}
				break;
			case Jobs::Hermit::ShadowWeb:
				if (Randomizer::Instance()->randInt(99) < Skills::skills[skillid][level].prop) {
					statuses.push_back(StatusInfo(StatusEffects::Mob::ShadowWeb, 0x100, skillid, Skills::skills[skillid][level].time));
				}
				break;
		}
	}
	if (skillid == Jobs::Rogue::Disorder) {
		clock_t time = Skills::skills[skillid][level].time;
		statuses.push_back(StatusInfo(StatusEffects::Mob::Watk, Skills::skills[skillid][level].x, skillid, time));
		statuses.push_back(StatusInfo(StatusEffects::Mob::Wdef, Skills::skills[skillid][level].y, skillid, time));
	}
	else if (skillid == Jobs::Page::Threaten) {
		clock_t time = Skills::skills[skillid][level].time;
		statuses.push_back(StatusInfo(StatusEffects::Mob::Watk, Skills::skills[skillid][level].x, skillid, time));
		statuses.push_back(StatusInfo(StatusEffects::Mob::Wdef, Skills::skills[skillid][level].y, skillid, time));
	}
	else if (skillid == Jobs::FPWizard::Slow || skillid == Jobs::ILWizard::Slow) {
		statuses.push_back(StatusInfo(StatusEffects::Mob::Speed, Skills::skills[skillid][level].x, skillid, Skills::skills[skillid][level].time));
	}
	if (weapon_type == WeaponBow && player->getActiveBuffs()->getActiveSkillLevel(Jobs::Bowmaster::Hamstring) > 0) {
		uint8_t hamlevel = player->getActiveBuffs()->getActiveSkillLevel(Jobs::Bowmaster::Hamstring);
		statuses.push_back(StatusInfo(StatusEffects::Mob::Speed, Skills::skills[Jobs::Bowmaster::Hamstring][hamlevel].x, Jobs::Bowmaster::Hamstring, Skills::skills[Jobs::Bowmaster::Hamstring][hamlevel].y));
	}
	if (weapon_type == WeaponCrossbow && player->getActiveBuffs()->getActiveSkillLevel(Jobs::Marksman::Blind) > 0) {
		uint8_t blindlevel = player->getActiveBuffs()->getActiveSkillLevel(Jobs::Marksman::Blind);
		statuses.push_back(StatusInfo(StatusEffects::Mob::Acc, -Skills::skills[Jobs::Marksman::Blind][blindlevel].x, Jobs::Marksman::Blind, Skills::skills[Jobs::Marksman::Blind][blindlevel].y));
	}

	if (statuses.size() > 0)
		mob->addStatus(player->getId(), statuses);
}
