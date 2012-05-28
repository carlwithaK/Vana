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
#include "PlayerHandler.h"
#include "CmsgHeader.h"
#include "Drop.h"
#include "DropHandler.h"
#include "DropsPacket.h"
#include "GameLogicUtilities.h"
#include "InventoryPacket.h"
#include "ItemDataProvider.h"
#include "MapleTvs.h"
#include "Maps.h"
#include "Mist.h"
#include "MobHandler.h"
#include "MonsterBookPacket.h"
#include "MovementHandler.h"
#include "Player.h"
#include "PlayerDataProvider.h"
#include "PlayerPacket.h"
#include "PlayersPacket.h"
#include "Randomizer.h"
#include "PacketReader.h"
#include "SkillConstants.h"
#include "SkillDataProvider.h"
#include "Skills.h"
#include "SkillsPacket.h"
#include "Summon.h"
#include "TimeUtilities.h"
#include "Timer.h"
#include <functional>

using std::bind;

void PlayerHandler::handleDoorUse(Player *player, PacketReader &packet) {
	int32_t doorId = packet.get<int32_t>();
	bool toTown = !packet.getBool();
	//Player *doorHolder = PlayerDataProvider::Instance()->getPlayer(doorId);
	//if (doorHolder == nullptr || (doorHolder->getParty() != player->getParty() && doorHolder != player)) {
	//	// Hacking or lag
	//	return;
	//}
	//doorHolder->getDoor()->warp(player, toTown);
}

void PlayerHandler::handleDamage(Player *player, PacketReader &packet) {
	const int8_t BumpDamage = -1;
	const int8_t MapDamage = -2;
	const int8_t UnkDamage1 = -3;
	const int8_t UnkDamage2 = -4;
	
	bool damageApplied = false;
	bool deadlyAttack = false;
	uint8_t hit = 0;
	uint8_t stance = 0;
	uint8_t disease = 0;
	uint8_t level = 0;
	uint16_t mpBurn = 0;
	int32_t mapMobId = 0; // Map Mob ID
	int32_t mobId = 0; // Actual Mob ID - i.e. 8800000 for Zakum
	int32_t noDamageId = 0;
	Mob *mob = nullptr;
	ReturnDamageInfo pgmr;

	packet.skipBytes(4); // RAANDOMIZEED
	packet.skipBytes(4); // Ticks
	int8_t type = packet.get<int8_t>();
	packet.skipBytes(1); // Element - 0x00 = elementless, 0x01 = ice, 0x02 = fire, 0x03 = lightning
	int32_t damage = packet.get<int32_t>();
	packet.skipBytes(1); // 0 ?
	packet.skipBytes(1); // ?

	if (type != MapDamage && type != UnkDamage1) {
		mobId = packet.get<int32_t>();

		mapMobId = packet.get<int32_t>();
		mob = Maps::getMap(player->getMap())->getMob(mapMobId);
		if (mob == nullptr || mob->getMobId() != mobId) {
			// Hacking
			return;
		}

		if (type != BumpDamage) {
			int32_t attackerId = (mob->hasLink() ? mob->getLink() : mobId);
			MobAttackInfo *attack = MobDataProvider::Instance()->getMobAttack(attackerId, type);
			if (attack == nullptr) {
				// Hacking, I think
				return;
			}
			disease = attack->disease;
			level = attack->level;
			mpBurn = attack->mpBurn;
			deadlyAttack = attack->deadlyAttack;
		}
		hit = packet.get<uint8_t>(); // Knock direction
		pgmr.reduction = packet.get<uint8_t>();
		packet.skipBytes(3); // I think reduction is a short, but it's a byte in the S -> C packet, so..
		// NOW ITS AN INT OMG
		if (pgmr.reduction != 0) {
			pgmr.isPhysical = packet.getBool();
			pgmr.mapMobId = packet.get<int32_t>();
			packet.skipBytes(1); // 0x06 for Power Guard, 0x00 for Mana Reflection?
			packet.skipBytes(4); // Mob position garbage
			pgmr.pos = packet.getPos();
			pgmr.damage = damage;
			if (pgmr.isPhysical) {
				// Only Power Guard decreases damage
				damage = (damage - (damage * pgmr.reduction / 100));
			}
			mob->applyDamage(player->getId(), (pgmr.damage * pgmr.reduction / 100));
		}
	}

	if (type == MapDamage) {
		level = packet.get<uint8_t>();
		disease = packet.get<uint8_t>();
	}
	else {
		stance = packet.get<int8_t>(); // Power Stance
		if (stance > 0 && !player->getActiveBuffs()->hasPowerStance()) {
			// Hacking
			return;
		}
	}

	if (damage == -1) {
		if (!player->getSkills()->hasNoDamageSkill()) {
			// Hacking
			return;
		}
		noDamageId = player->getSkills()->getNoDamageSkill();
	}

	if (disease > 0 && damage != 0) {
		// Fake/Guardian don't prevent disease
		player->getActiveBuffs()->addDebuff(disease, level);
	}

	int16_t mp = player->getStats()->getMp();
	int16_t hp = player->getStats()->getHp();

	auto deadlyAttackFunc = [&player, &mp](bool setHp) {
		if (mp > 0) {
			player->getStats()->setMp(1);
		}
		if (setHp) {
			player->getStats()->setHp(1);
		}
	};

	if (damage > 0 && !player->hasGmEquip()) {
		if (player->getActiveBuffs()->hasMesoGuard() && player->getInventory()->getMesos() > 0) {
			int32_t skillId = player->getActiveBuffs()->getMesoGuard();
			int16_t mesoRate = player->getActiveBuffs()->getActiveSkillInfo(skillId)->x; // Meso Guard meso %
			int16_t mesoLoss = static_cast<int16_t>(mesoRate * damage / 2 / 100);
			int32_t mesos = player->getInventory()->getMesos();
			int32_t newMesos = mesos - mesoLoss;

			if (newMesos < 0) {
				// Special damage calculation for not having enough mesos
				double reduction = 2.0 - ((double)(mesos / mesoLoss)) / 2.0;
				damage = static_cast<uint16_t>(damage / reduction);
				// This puts us pretty close to the damage observed clientskillIde, needs improvement
				// TODO: Improve formula
			}
			else {
				damage /= 2;
				// Usually displays 1 below the actual damage but is sometimes accurate - no clue why
			}

			player->getInventory()->setMesos(newMesos);
			player->getStats()->damageHp(static_cast<uint16_t>(damage));

			if (deadlyAttack) {
				deadlyAttackFunc(false);
			}
			else if (mpBurn > 0) {
				player->getStats()->damageMp(mpBurn);
			}
			damageApplied = true;

			SkillsPacket::showSkillEffect(player, skillId);
		}

		if (player->getActiveBuffs()->hasMagicGuard()) {
			if (deadlyAttack) {
				deadlyAttackFunc(true);
			}
			else if (mpBurn > 0) {
				player->getStats()->damageMp(mpBurn);
				player->getStats()->damageHp(static_cast<uint16_t>(damage));
			}
			else {
				int32_t skillId = player->getActiveBuffs()->getMagicGuard();
				int16_t reduc = player->getActiveBuffs()->getActiveSkillInfo(skillId)->x;
				uint16_t mpDamage = static_cast<uint16_t>((damage * reduc) / 100);
				uint16_t hpDamage = static_cast<uint16_t>(damage - mpDamage);

				if (mpDamage < mp || player->getActiveBuffs()->hasInfinity()) {
					player->getStats()->damageMp(mpDamage);
					player->getStats()->damageHp(hpDamage);
				}
				else if (mpDamage >= mp) {
					player->getStats()->setMp(0);
					player->getStats()->damageHp(hpDamage + (mpDamage - mp));
				}
			}
			damageApplied = true;
		}

		if (player->getSkills()->hasAchilles()) {
			int32_t skillId = player->getSkills()->getAchilles();
			double red = (2.0 - player->getSkills()->getSkillInfo(skillId)->x / 1000.0);

			player->getStats()->damageHp(static_cast<uint16_t>(damage / red));

			if (deadlyAttack) {
				deadlyAttackFunc(false);
			}
			else if (mpBurn > 0) {
				player->getStats()->damageMp(mpBurn);
			}

			damageApplied = true;
		}

		if (!damageApplied) {
			if (deadlyAttack) {
				deadlyAttackFunc(true);
			}
			else {
				player->getStats()->damageHp(static_cast<uint16_t>(damage));
			}

			if (mpBurn > 0) {
				player->getStats()->damageMp(mpBurn);
			}

			if (player->getActiveBuffs()->getActiveSkillLevel(Jobs::Corsair::Battleship) > 0) {
				player->getActiveBuffs()->reduceBattleshipHp(static_cast<uint16_t>(damage));
			}
		}
		int32_t morph = player->getActiveBuffs()->getCurrentMorph();
		if (morph < 0 || (morph != 0 && player->getStats()->isDead())) {
			player->getActiveBuffs()->endMorph();
		}
	}
	PlayersPacket::damagePlayer(player, damage, mobId, hit, type, stance, noDamageId, pgmr);
}

void PlayerHandler::handleFacialExpression(Player *player, PacketReader &packet) {
	int32_t face = packet.get<int32_t>();
	int32_t unk = packet.get<int32_t>();
	int8_t unk2 = packet.get<int8_t>();
	PlayersPacket::faceExpression(player, face, unk, unk2);
}

void PlayerHandler::handleGetInfo(Player *player, PacketReader &packet) {
	packet.skipBytes(4);
	if (Player *info = PlayerDataProvider::Instance()->getPlayer(packet.get<int32_t>())) {
		PlayersPacket::showInfo(player, info, packet.getBool());
	}
}

void PlayerHandler::handleHeal(Player *player, PacketReader &packet) {
	packet.skipBytes(4);

	int16_t hp = packet.get<int16_t>();
	int16_t mp = packet.get<int16_t>();
	if (player->getStats()->isDead() || hp > 400 || mp > 1000 || (hp > 0 && mp > 0)) {
		// Hacking
		return;
	}
	player->getStats()->modifyHp(hp);
	player->getStats()->modifyMp(mp);
}

void PlayerHandler::handleMoving(Player *player, PacketReader &packet) {
	if (packet.get<uint8_t>() != player->getPortalCount()) {
		// Portal count doesn't match, usually an indication of hacking
		return;
	}
	packet.reset(19); // Contains position?
	MovementHandler::parseMovement(player, packet);
	packet.reset(11);
	PlayersPacket::showMoving(player, packet.getBuffer(), packet.getBufferLength());

	if (player->getFh() == 0) {
		// Player is floating in the air
		int32_t mapId = player->getMap();
		const Pos &playerPos = player->getPos();
		Map *map = Maps::getMap(mapId);

		const Pos &floor = map->findFloor(playerPos);
		if (floor.y == playerPos.y) {
			// There are no footholds below the player
			int8_t count = player->getFallCounter();
			if (count > 3) {
				player->setMap(mapId);
			}
			else {
				player->setFallCounter(++count);
			}
		}
	}
	else if (player->getFallCounter() > 0) {
		player->setFallCounter(0);
	}
}

void PlayerHandler::handleSpecialSkills(Player *player, PacketReader &packet) {
	int32_t skillId = packet.get<int32_t>();
	switch (skillId) {
		case Jobs::Hero::MonsterMagnet:
		case Jobs::Paladin::MonsterMagnet:
		case Jobs::DarkKnight::MonsterMagnet:
		case Jobs::Marksman::PiercingArrow:
		case Jobs::FpArchMage::BigBang:
		case Jobs::IlArchMage::BigBang:
		case Jobs::Bishop::BigBang: {
			SpecialSkillInfo info;
			info.skillId = skillId;
			info.level = packet.get<uint8_t>();
			info.direction = packet.get<uint8_t>();
			info.weaponSpeed = packet.get<uint8_t>();
			player->setSpecialSkill(info);
			SkillsPacket::showSpecialSkill(player, info);
			break;
		}
		case Jobs::ChiefBandit::Chakra: {
			int16_t dex = player->getStats()->getDex(true);
			int16_t luk = player->getStats()->getLuk(true);
			int16_t recovery = player->getSkills()->getSkillInfo(skillId)->y;
			int16_t maximum = (luk * 66 / 10 + dex) * 2 / 10 * (recovery / 100 + 1);
			int16_t minimum = (luk * 33 / 10 + dex) * 2 / 10 * (recovery / 100 + 1);
			// Maximum = (luk * 6.6 + dex) * 0.2 * (recovery% / 100 + 1)
			// Minimum = (luk * 3.3 + dex) * 0.2 * (recovery% / 100 + 1)
			// I used 66 / 10 and 2 / 10 respectively to get 6.6 and 0.2 without using floating points
			player->getStats()->modifyHp(Randomizer::Instance()->randShort(maximum, minimum));
			break;
		}
	}
}

void PlayerHandler::handleMonsterBook(Player *player, PacketReader &packet) {
	int32_t cardId = packet.get<int32_t>();
	if (cardId != 0 && player->getMonsterBook()->getCard(cardId) == 0) {
		// Hacking
		return;
	}
	int32_t newCover = 0;
	if (cardId != 0) {
		newCover = ItemDataProvider::Instance()->getMobId(cardId);
	}
	player->getMonsterBook()->setCover(newCover);
	MonsterBookPacket::changeCover(player, cardId);
}

void PlayerHandler::handleAdminMessenger(Player *player, PacketReader &packet) {
	if (!player->isAdmin()) {
		// Hacking
		return;
	}
	Player *receiver = nullptr;
	bool hasTarget = packet.get<int8_t>() == 2;
	int8_t sort = packet.get<int8_t>();
	bool useWhisper = packet.getBool();
	int8_t type = packet.get<int8_t>();
	int32_t characterId = packet.get<int32_t>();

	if (player->getId() != characterId) {
		return;
	}

	const string &line1 = packet.getString();
	const string &line2 = packet.getString();
	const string &line3 = packet.getString();
	const string &line4 = packet.getString();
	const string &line5 = packet.getString();
	if (hasTarget) {
		receiver = PlayerDataProvider::Instance()->getPlayer(packet.getString());
	}

	int32_t time = 15;
	switch (type) {
		case 1: time = 30; break;
		case 2: time = 60; break;
	}

	MapleTvs::Instance()->addMessage(player, receiver, line1, line2, line3, line4, line5, 5075000 + type, time);
	if (sort == 1) {
		std::ostringstream output;
		output << player->getMedalName() << " : " << line1 << line2 << line3 << line4 << line5;
		InventoryPacket::showSuperMegaphone(player, output.str(), useWhisper);
	}
}

void PlayerHandler::useMeleeAttack(Player *player, PacketReader &packet) {
	const Attack &attack = compileAttack(player, packet, SkillTypes::Melee);
	if (attack.portals != player->getPortalCount()) {
		// Usually evidence of hacking
		return;
	}
	PlayersPacket::useMeleeAttack(player, attack);
	int8_t damagedTargets = 0;
	int32_t skillId = attack.skillId;
	uint8_t level = attack.skillLevel;

	if (skillId != Jobs::All::RegularAttack) {
		Skills::useAttackSkill(player, skillId);
	}

	int32_t map = player->getMap();
	uint8_t ppLevel = player->getActiveBuffs()->getActiveSkillLevel(Jobs::ChiefBandit::Pickpocket); // Check for active pickpocket level
	bool ppok = !attack.isMesoExplosion && ppLevel > 0;
	SkillLevelInfo *picking = SkillDataProvider::Instance()->getSkill(Jobs::ChiefBandit::Pickpocket, ppLevel);
	Pos origin;
	vector<int32_t> ppDamages;

	for (Attack::iterator i = attack.damages.begin(); i != attack.damages.end(); ++i) {
		int32_t targetTotal = 0;
		int8_t connectedHits = 0;
		Mob *mob = Maps::getMap(map)->getMob(i->first);
		if (mob == nullptr) {
			continue;
		}
		origin = mob->getPos(); // Info for pickpocket before mob is set to 0 (in the case that mob dies)
		for (Attack::diterator k = i->second.begin(); k != i->second.end(); ++k) {
			int32_t damage = *k;
			if (damage != 0) {
				connectedHits++;
				targetTotal += damage;
			}
			if (ppok && Randomizer::Instance()->randInt(99) < picking->prop) {
				 // Make sure this is a melee attack and not meso explosion, plus pickpocket being active
				ppDamages.push_back(damage);
			}
			if (mob == nullptr) {
				if (ppok) {
					// Roll along after the mob is dead to finish getting damage values for pickpocket
					continue;
				}
				break;
			}
			if (skillId == Jobs::Paladin::HeavensHammer) {
				damage = (mob->isBoss() ? Stats::MaxDamage : (mob->getHp() - 1)); // If a Paladin wants to prove that it does something else, feel free
			}
			else if (skillId == Jobs::Bandit::Steal && !mob->isBoss()) {
				DropHandler::doDrops(player->getId(), map, mob->getInfo()->level, mob->getMobId(), mob->getPos(), false, false, mob->getTauntEffect(), true);
			}
			int32_t tempHp = mob->getHp();
			mob->applyDamage(player->getId(), damage);
			if (tempHp <= damage) {
				// Mob was killed, so set the Mob pointer to 0
				mob = nullptr;
			}
		}
		if (targetTotal > 0) {
			if (mob != nullptr && mob->getHp() > 0) {
				MobHandler::handleMobStatus(player->getId(), mob, skillId, level, player->getInventory()->getEquippedId(EquipSlots::Weapon), connectedHits); // Mob status handler (freeze, stun, etc)
				if (mob->getHp() < mob->getSelfDestructHp()) {
					mob->explode();
				}
			}
			damagedTargets++;
		}
		uint8_t ppSize = ppDamages.size();
		for (uint8_t pickpocket = 0; pickpocket < ppSize; ++pickpocket) {
			// Drop stuff for Pickpocket
			Pos ppPos = origin;
			ppPos.x += (ppSize % 2 == 0 ? 5 : 0) + (ppSize / 2) - 20 * ((ppSize / 2) - pickpocket);

			clock_t ppTime = 175 * pickpocket;
			int32_t ppMesos = ((ppDamages[pickpocket] * picking->x) / 10000); // TODO: Check on this formula in different situations
			Drop *ppDrop = new Drop(player->getMap(), ppMesos, ppPos, player->getId(), true);
			ppDrop->setTime(100);
			new Timer::Timer(bind(&Drop::doDrop, ppDrop, origin),
				Timer::Id(Timer::Types::PickpocketTimer, player->getId(), player->getActiveBuffs()->getPickpocketCounter()),
				nullptr, TimeUtilities::fromNow(ppTime));
		}
		ppDamages.clear();
	}

	if (player->getSkills()->hasEnergyCharge()) {
		player->getActiveBuffs()->increaseEnergyChargeLevel(damagedTargets);
	}

	switch (skillId) {
		case Jobs::ChiefBandit::MesoExplosion: {
			uint8_t items = packet.get<int8_t>();
			int32_t map = player->getMap();
			for (uint8_t i = 0; i < items; i++) {
				int32_t objId = packet.get<int32_t>();
				packet.skipBytes(1); // Some value
				if (Drop *drop = Maps::getMap(map)->getDrop(objId)) {
					if (!drop->isMesos()) {
						// Hacking
						return;
					}
					DropsPacket::explodeDrop(drop);
					Maps::getMap(map)->removeDrop(drop->getId());
					delete drop;
				}
			}
			break;
		}
		case Jobs::Marauder::EnergyDrain:
		case Jobs::ThunderBreaker::EnergyDrain: {
			int32_t hpRecover = static_cast<int32_t>(attack.totalDamage * player->getSkills()->getSkillInfo(skillId)->x / 100);
			if (hpRecover > player->getStats()->getMaxHp()) {
				player->getStats()->setHp(player->getStats()->getMaxHp());
			}
			else {
				player->getStats()->modifyHp((int16_t) hpRecover);
			}
			break;
		}
		case Jobs::Crusader::SwordPanic: // Crusader finishers
		case Jobs::Crusader::SwordComa:
		case Jobs::Crusader::AxePanic:
		case Jobs::Crusader::AxeComa:
		case Jobs::DawnWarrior::Panic:
		case Jobs::DawnWarrior::Coma:
			player->getActiveBuffs()->setCombo(0, true);
			break;
		case Jobs::Crusader::Shout:
		case Jobs::Gm::SuperDragonRoar:
		case Jobs::SuperGm::SuperDragonRoar:
			break;
		case Jobs::DragonKnight::DragonRoar: {
			int16_t xProperty = SkillDataProvider::Instance()->getSkill(skillId, level)->x;
			uint16_t reduction = (player->getStats()->getMaxHp() / 100) * xProperty;
			if (reduction < player->getStats()->getHp()) {
				player->getStats()->damageHp(reduction);
			}
			else {
				// Hacking
				return;
			}
			Buffs::addBuff(player, Jobs::DragonKnight::DragonRoar, level, 0);
			break;
		}
		case Jobs::DragonKnight::Sacrifice: {
			int16_t xProperty = player->getSkills()->getSkillInfo(skillId)->x;
			uint16_t hpDamage = static_cast<uint16_t>(attack.totalDamage * xProperty / 100);
			player->getStats()->setHp(hpDamage > player->getStats()->getHp() ? 1 : hpDamage);
			break;
		}
		case Jobs::WhiteKnight::ChargeBlow: {
			int8_t skillLevel = player->getSkills()->getSkillLevel(Jobs::Paladin::AdvancedCharge);
			int16_t xProperty = 0;
			if (skillLevel > 0) {
				xProperty = SkillDataProvider::Instance()->getSkill(Jobs::Paladin::AdvancedCharge, skillLevel)->x;
			}
			if ((xProperty != 100) && (xProperty == 0 || Randomizer::Instance()->randShort(99) > (xProperty - 1))) {
				player->getActiveBuffs()->stopCharge();
			}
			break;
		}
		default:
			if (attack.totalDamage > 0) {
				player->getActiveBuffs()->addCombo();
			}
	}
}

void PlayerHandler::useRangedAttack(Player *player, PacketReader &packet) {
	const Attack &attack = compileAttack(player, packet, SkillTypes::Ranged);
	if (attack.portals != player->getPortalCount()) {
		// Usually evidence of hacking
		return;
	}
	PlayersPacket::useRangedAttack(player, attack);
	int32_t skillId = attack.skillId;
	uint8_t level = attack.skillLevel;

	switch (skillId) {
		case Jobs::Bowmaster::Hurricane:
		case Jobs::WindArcher::Hurricane:
		case Jobs::Corsair::RapidFire:
			if (player->getSpecialSkill() == 0) {
				SpecialSkillInfo info;
				info.skillId = skillId;
				info.direction = 0;//attack.animation;
				info.weaponSpeed = 0;//attack.weaponSpeed;
				info.level = level;
				player->setSpecialSkill(info);
				SkillsPacket::showSpecialSkill(player, info);
			}
			break;
	}

	Skills::useAttackSkillRanged(player, skillId, attack.starPos);

	int32_t maxHp = 0;
	int32_t firstHit = 0;

	for (Attack::iterator i = attack.damages.begin(); i != attack.damages.end(); ++i) {
		int32_t mapMobId = i->first;
		Mob *mob = Maps::getMap(player->getMap())->getMob(mapMobId);
		if (mob == nullptr) {
			continue;
		}
		int32_t targetTotal = 0;
		int8_t connectedHits = 0;

		for (Attack::diterator k = i->second.begin(); k != i->second.end(); ++k) {
			int32_t damage = *k;
			if (damage != 0) {
				connectedHits++;
				targetTotal += damage;
			}
			if (firstHit == 0) {
				firstHit = damage;
			}
			if (mob == nullptr) {
				continue;
			}
			maxHp = mob->getMaxHp();
			if (skillId == Jobs::Ranger::MortalBlow || skillId == Jobs::Sniper::MortalBlow) {
				SkillLevelInfo *sk = player->getSkills()->getSkillInfo(skillId);
				int32_t hpPercentage = maxHp * sk->x / 100; // Percentage of HP required for Mortal Blow activation
				if ((mob->getHp() < hpPercentage) && (Randomizer::Instance()->randShort(99) < sk->y)) {
					damage = mob->getHp();
				}
			}
			else if (skillId == Jobs::Outlaw::HomingBeacon || skillId == Jobs::Corsair::Bullseye) {
				Buffs::addBuff(player, skillId, level, 0, mapMobId);
			}
			int32_t tempHp = mob->getHp();
			mob->applyDamage(player->getId(), damage);
			if (tempHp <= damage) {
				mob = nullptr;
			}
		}
		if (mob != nullptr && targetTotal > 0 && mob->getHp() > 0) {
			MobHandler::handleMobStatus(player->getId(), mob, skillId, level, player->getInventory()->getEquippedId(EquipSlots::Weapon), connectedHits, firstHit); // Mob status handler (freeze, stun, etc)
			if (mob->getHp() < mob->getSelfDestructHp()) {
				mob->explode();
			}
		}
	}

	switch (skillId) {
		case Jobs::Assassin::Drain: {
			int16_t xProperty = player->getSkills()->getSkillInfo(skillId)->x;
			int32_t hpRecover = static_cast<int32_t>(attack.totalDamage * xProperty / 100);
			int16_t playerMaxHp = player->getStats()->getMaxHp();
			if (hpRecover > maxHp) {
				hpRecover = maxHp;
			}
			if (hpRecover > (playerMaxHp / 2)) {
				hpRecover = playerMaxHp / 2;
			}
			if (hpRecover > playerMaxHp) {
				player->getStats()->setHp(playerMaxHp);
			}
			else {
				player->getStats()->modifyHp(static_cast<int16_t>(hpRecover));
			}
			break;
		}
		case Jobs::DawnWarrior::SoulBlade:
			if (attack.totalDamage > 0) {
				player->getActiveBuffs()->addCombo();
			}
			break;
	}
}

void PlayerHandler::useSpellAttack(Player *player, PacketReader &packet) {
	const Attack &attack = compileAttack(player, packet, SkillTypes::Magic);
	if (attack.portals != player->getPortalCount()) {
		// Usually evidence of hacking
		return;
	}
	PlayersPacket::useSpellAttack(player, attack);

	int32_t skillId = attack.skillId;
	uint8_t level = attack.skillLevel;

	MpEaterInfo eater;
	eater.skillId = player->getSkills()->getMpEater();
	eater.level = player->getSkills()->getSkillLevel(eater.skillId);
	if (eater.level > 0) {
		SkillLevelInfo *eaaat = SkillDataProvider::Instance()->getSkill(eater.skillId, eater.level);
		eater.prop = eaaat->prop;
		eater.x = eaaat->x;
	}

	if (!attack.isHeal) {
		// Heal is sent as both an attack and as a used skill - it's only sometimes an attack
		Skills::useAttackSkill(player, skillId);
	}

	for (Attack::iterator i = attack.damages.begin(); i != attack.damages.end(); ++i) {
		int32_t targetTotal = 0;
		int32_t mapMobId = i->first;
		int8_t connectedHits = 0;
		Mob *mob = Maps::getMap(player->getMap())->getMob(mapMobId);
		if (mob == nullptr) {
			continue;
		}
		if (attack.isHeal && !mob->isUndead()) {
			// Hacking
			return;
		}

		for (Attack::diterator k = i->second.begin(); k != i->second.end(); ++k) {
			int32_t damage = *k;
			if (damage != 0) {
				connectedHits++;
				targetTotal += damage;
			}
			if (damage != 0 && eater.level != 0 && !eater.used) {
				// MP Eater
				mob->mpEat(player, &eater);
			}
			int32_t tempHp = mob->getHp();
			mob->applyDamage(player->getId(), damage);
			if (tempHp <= damage) {
				// Mob was killed, so set the Mob pointer to 0
				mob = nullptr;
				break;
			}
		}
		if (mob != nullptr && targetTotal > 0 && mob->getHp() > 0) {
			MobHandler::handleMobStatus(player->getId(), mob, skillId, level, player->getInventory()->getEquippedId(EquipSlots::Weapon), connectedHits); // Mob status handler (freeze, stun, etc)
			if (mob->getHp() < mob->getSelfDestructHp()) {
				mob->explode();
			}
		}
	}

	switch (skillId) {
		case Jobs::FpMage::PoisonMist:
		case Jobs::BlazeWizard::FlameGear: {
			Mist *mist = new Mist(player->getMap(), player, player->getPos(), SkillDataProvider::Instance()->getSkill(skillId, level), skillId, level, true);
			break;
		}
	}
}

void PlayerHandler::useEnergyChargeAttack(Player *player, PacketReader &packet) {
	const Attack &attack = compileAttack(player, packet, SkillTypes::EnergyCharge);
	PlayersPacket::useEnergyChargeAttack(player, attack);

	int32_t skillId = attack.skillId;
	int8_t level = attack.skillLevel;

	for (Attack::iterator i = attack.damages.begin(); i != attack.damages.end(); ++i) {
		int32_t targetTotal = 0;
		int32_t mapMobId = i->first;
		int8_t connectedHits = 0;
		Mob *mob = Maps::getMap(player->getMap())->getMob(mapMobId);
		if (mob == nullptr) {
			continue;
		}

		for (Attack::diterator k = i->second.begin(); k != i->second.end(); ++k) {
			int32_t damage = *k;
			if (damage != 0) {
				connectedHits++;
				targetTotal += damage;
			}
			int32_t tempHp = mob->getHp();
			mob->applyDamage(player->getId(), damage);
			if (tempHp <= damage) {
				// Mob was killed, so set the Mob pointer to 0
				mob = nullptr;
				break;
			}
		}
		if (mob != nullptr && targetTotal > 0 && mob->getHp() > 0) {
			MobHandler::handleMobStatus(player->getId(), mob, skillId, level, player->getInventory()->getEquippedId(EquipSlots::Weapon), connectedHits); // Mob status handler (freeze, stun, etc)
			if (mob->getHp() < mob->getSelfDestructHp()) {
				mob->explode();
			}
		}
	}
}

void PlayerHandler::useSummonAttack(Player *player, PacketReader &packet) {
	const Attack &attack = compileAttack(player, packet, SkillTypes::Summon);
	Summon *summon = player->getSummons()->getSummon();
	if (summon == nullptr) {
		// Hacking or some other form of tomfoolery
		return;
	}
	PlayersPacket::useSummonAttack(player, attack);
	int32_t skillId = summon->getSummonId();
	for (Attack::iterator i = attack.damages.begin(); i != attack.damages.end(); ++i) {
		int32_t targetTotal = 0;
		int32_t mapMobId = i->first;
		int8_t connectedHits = 0;
		Mob *mob = Maps::getMap(player->getMap())->getMob(mapMobId);
		if (mob == nullptr) {
			continue;
		}
		for (Attack::diterator k = i->second.begin(); k != i->second.end(); ++k) {
			int32_t damage = *k;
			if (damage != 0) {
				connectedHits++;
				targetTotal += damage;
			}
			int32_t tempHp = mob->getHp();
			mob->applyDamage(player->getId(), damage);
			if (tempHp <= damage) {
				// Mob was killed, so set the Mob pointer to 0
				mob = nullptr;
				break;
			}
		}
		if (mob != nullptr && targetTotal > 0 && mob->getHp() > 0) {
			MobHandler::handleMobStatus(player->getId(), mob, skillId, player->getSkills()->getSkillLevel(skillId), player->getInventory()->getEquippedId(EquipSlots::Weapon), connectedHits); // Mob status handler (freeze, stun, etc)
			if (mob->getHp() < mob->getSelfDestructHp()) {
				mob->explode();
			}
		}
	}
}

Attack PlayerHandler::compileAttack(Player *player, PacketReader &packet, int8_t skillType) {
	Attack attack;
	int8_t targets = 0;
	int8_t hits = 0;
	int32_t skillId = 0;
	bool mesoExplosion = false;
	bool shadowMeso = false;
	packet.reset(0);
	attack.isRanged = packet.get<uint16_t>() == CMSG_ATTACK_RANGED;

	if (skillType != SkillTypes::Summon) {
		//  [4F 00] [01] [01] [E8 03 00 00] [00 29 62 85] [65 29 62 85] [65] [00] [2C] [00] [3C F3 E8 75] 01 06 DA 12 A9 02 00 00 00 00 00 00 5F 00 00
		attack.portals = packet.get<uint8_t>();
		uint8_t tByte = packet.get<uint8_t>();
		targets = tByte / 0x10;
		hits = tByte % 0x10;

		skillId = packet.get<int32_t>();
		if (skillId == 31121010) {
			uint8_t infernalConcussionLevel = player->getSkills()->getSkillLevel(31121000);

			attack.skillLevel = infernalConcussionLevel * 3;
		}
		else if (skillId != Jobs::All::RegularAttack) {
			attack.skillLevel = player->getSkills()->getSkillLevel(skillId);
		}

		attack.unk_val = packet.get<int8_t>();

		packet.skipBytes(4); // Skill CRC's
		packet.skipBytes(4);

		switch (skillId) {
			case Jobs::Hermit::ShadowMeso:
				attack.isShadowMeso = true;
				shadowMeso = true;
				break;
			case Jobs::ChiefBandit::MesoExplosion:
				attack.isMesoExplosion = true;
				mesoExplosion = true;
				break;
			case Jobs::Cleric::Heal:
				attack.isHeal = true;
				break;
			case Jobs::Gunslinger::Grenade:
			case Jobs::Brawler::CorkscrewBlow:
			case Jobs::Bowmaster::Hurricane:
			case Jobs::WindArcher::Hurricane:
			case Jobs::Marksman::PiercingArrow:
			case Jobs::NightWalker::PoisonBomb:
			case Jobs::Corsair::RapidFire:
			case Jobs::FpArchMage::BigBang:
			case Jobs::IlArchMage::BigBang:
			case Jobs::Bishop::BigBang:
				attack.isChargeSkill = true;
				attack.charge = packet.get<int32_t>();
				break;
		}

		attack.display = packet.get<uint8_t>();
		attack.animationAndSpeed = packet.get<uint16_t>();

		packet.skipBytes(4); // More CRC

		attack.weaponClass = packet.get<uint8_t>();
		packet.skipBytes(1); // Temp stat + ?

		attack.ticks = packet.get<int32_t>();
		packet.skipBytes(4); // Unk
	}
	else {
		attack.summonId = packet.get<int32_t>(); // Summon ID, not to be confused with summon skill ID
		attack.ticks = packet.get<int32_t>();
		attack.display = packet.get<uint8_t>();
		targets = packet.get<int8_t>();
		hits = 1;
	}

	if (skillType == SkillTypes::Ranged) {
		int16_t starSlot = packet.get<int16_t>();
		int16_t csStar = packet.get<int16_t>();
		attack.starPos = starSlot;
		attack.cashStarPos = csStar;
		packet.skipBytes(1); // 0x00 = AoE?
		if (!shadowMeso) {
			if (player->getActiveBuffs()->hasShadowStars() && skillId != Jobs::NightLord::Taunt) {
				attack.starId = packet.get<int32_t>();
			}
			else if (csStar > 0) {
				if (Item *item = player->getInventory()->getItem(Inventories::CashInventory, csStar)) {
					attack.starId = item->getId();
				}
			}
			else if (starSlot > 0) {
				if (Item *item = player->getInventory()->getItem(Inventories::UseInventory, starSlot)) {
					attack.starId = item->getId();
				}
			}
		}
	}

	attack.targets = targets;
	attack.hits = hits;
	attack.skillId = skillId;

	for (int8_t i = 0; i < targets; ++i) {
		int32_t mapMobId = packet.get<int32_t>();
		packet.skipBytes(4); // Always 0x06, <two bytes of some kind>, 0x01
		packet.skipBytes(4); // Mob pos
		packet.skipBytes(4); // Damage pos
		if (!mesoExplosion) {
			packet.skipBytes(2); // Distance
		}
		else {
			hits = packet.get<int8_t>(); // Hits for Meso Explosion
		}
		for (int8_t k = 0; k < hits; ++k) {
			int32_t damage = packet.get<int32_t>();
			attack.damages[mapMobId].push_back(damage);
			attack.totalDamage += damage;
		}
		if (skillType != SkillTypes::Summon) {
			packet.skipBytes(4); // Mob CRC
		}
	}

	if (skillType == SkillTypes::Ranged) {
		attack.projectilePos = packet.getPos();
	}
	attack.playerPos = packet.getPos();

	return attack;
}