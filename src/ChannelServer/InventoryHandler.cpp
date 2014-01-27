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
#include "InventoryHandler.h"
#include "CurseDataProvider.h"
#include "Drop.h"
#include "EquipDataProvider.h"
#include "GameLogicUtilities.h"
#include "Inventory.h"
#include "InventoryPacket.h"
#include "ItemConstants.h"
#include "ItemDataProvider.h"
#include "MapleTvs.h"
#include "Maps.h"
#include "MobDataProvider.h"
#include "NpcDataProvider.h"
#include "PacketReader.h"
#include "Pet.h"
#include "PetHandler.h"
#include "PetsPacket.h"
#include "Player.h"
#include "PlayerDataProvider.h"
#include "Randomizer.h"
#include "ReactorHandler.h"
#include "ScriptDataProvider.h"
#include "ShopDataProvider.h"
#include "StoragePacket.h"
#include "ValidCharDataProvider.h"

auto InventoryHandler::itemMove(Player *player, PacketReader &packet) -> void {
	uint32_t ticks = packet.get<uint32_t>();
	int8_t inv = packet.get<int8_t>();
	int16_t slot1 = packet.get<int16_t>();
	int16_t slot2 = packet.get<int16_t>();
	bool dropped = (slot2 == 0);
	bool equippedSlot1 = (slot1 < 0);
	bool equippedSlot2 = (slot2 < 0);
	int16_t strippedSlot1 = GameLogicUtilities::stripCashSlot(slot1);
	int16_t strippedSlot2 = GameLogicUtilities::stripCashSlot(slot2);
	auto testSlot = [&slot1, &slot2, &strippedSlot1, &strippedSlot2](int16_t testSlot) -> bool {
		return (slot1 < 0 && strippedSlot1 == testSlot) || (slot2 < 0 && strippedSlot2 == testSlot);
	};

	if (dropped) {
		Item *item1 = player->getInventory()->getItem(inv, slot1);
		if (item1 == nullptr) {
			return;
		}
		InventoryHandler::dropItem(player, packet, item1, slot1, inv);
	}
	else {
		player->getInventory()->swapItems(inv, slot1, slot2);
	}

	if (equippedSlot1 || equippedSlot2) {
		auto testPetSlot = [&player, &testSlot](int16_t equipSlot, int32_t petIndex) {
			if (testSlot(equipSlot)) {
				if (Pet *pet = player->getPets()->getSummoned(petIndex)) {
					PetsPacket::changeName(player, pet);
				}				
			}
		};
		// Check if any label ring changed, so we can update the look of the apropos pet
		testPetSlot(EquipSlots::PetLabelRing1, 0);
		testPetSlot(EquipSlots::PetLabelRing2, 1);
		testPetSlot(EquipSlots::PetLabelRing3, 2);

		InventoryPacket::updatePlayer(player);
	}
}

auto InventoryHandler::dropItem(Player *player, PacketReader &packet, Item *item, int16_t slot, int8_t inv) -> void {
	int16_t amount = packet.get<int16_t>();
	if (!GameLogicUtilities::isStackable(item->getId())) {
		amount = item->getAmount();
	}
	else if (amount <= 0 || amount > item->getAmount()) {
		// Hacking
		return;
	}
	if (GameLogicUtilities::isGmEquip(item->getId()) || item->hasLock()) {
		// We don't allow these to be dropped or traded
		return;
	}

	Item droppedItem(item);
	droppedItem.setAmount(amount);
	if (item->getAmount() == amount) {
		vector_t<InventoryPacketOperation> ops;
		ops.emplace_back(InventoryPacket::OperationTypes::ModifySlot, item, slot);
		InventoryPacket::inventoryOperation(player, true, ops);

		player->getInventory()->deleteItem(inv, slot);
	}
	else {
		item->decAmount(amount);
		player->getInventory()->changeItemAmount(item->getId(), -amount);

		vector_t<InventoryPacketOperation> ops;
		ops.emplace_back(InventoryPacket::OperationTypes::ModifyQuantity, item, slot);
		InventoryPacket::inventoryOperation(player, true, ops);
	}

	bool isTradeable = !droppedItem.hasTradeBlock() && ItemDataProvider::getInstance().isTradeable(droppedItem.getId());
	Drop *drop = new Drop(player->getMapId(), droppedItem, player->getPos(), player->getId(), true);
	drop->setTime(0);
	drop->setTradeable(isTradeable);
	drop->doDrop(player->getPos());

	if (isTradeable) {
		// Drop is deleted otherwise, avoid like plague
		ReactorHandler::checkDrop(player, drop);
	}
}

auto InventoryHandler::useItem(Player *player, PacketReader &packet) -> void {
	uint32_t ticks = packet.get<uint32_t>();
	int16_t slot = packet.get<int16_t>();
	int32_t itemId = packet.get<int32_t>();
	const int16_t amountUsed = 1;
	if (player->getStats()->isDead() || player->getInventory()->getItemAmountBySlot(Inventories::UseInventory, slot) < amountUsed) {
		// Hacking
		return;
	}

	Item *item = player->getInventory()->getItem(Inventories::UseInventory, slot);
	if (item == nullptr || item->getId() != itemId) {
		// Hacking
		return;
	}

	if (!player->hasGmBenefits()) {
		Inventory::takeItemSlot(player, Inventories::UseInventory, slot, amountUsed);
	}

	Inventory::useItem(player, itemId);
}

auto InventoryHandler::cancelItem(Player *player, PacketReader &packet) -> void {
	int32_t itemId = packet.get<int32_t>();
	Buffs::endBuff(player, itemId);
}

auto InventoryHandler::useSkillbook(Player *player, PacketReader &packet) -> void {
	uint32_t ticks = packet.get<uint32_t>();
	int16_t slot = packet.get<int16_t>();
	int32_t itemId = packet.get<int32_t>();

	Item *item = player->getInventory()->getItem(Inventories::UseInventory, slot);
	if (item == nullptr || item->getId() != itemId) {
		// Hacking
		return;
	}
	if (!ItemDataProvider::getInstance().skillItemExists(itemId)) {
		// Hacking
		return;
	}

	int32_t skillId = 0;
	uint8_t newMaxLevel = 0;
	bool use = false;
	bool succeed = false;

	const vector_t<Skillbook> &skillbookItems = *ItemDataProvider::getInstance().getItemSkills(itemId);

	for (const auto &s : skillbookItems) {
		skillId = s.skillId;
		newMaxLevel = s.maxLevel;
		if (GameLogicUtilities::itemSkillMatchesJob(skillId, player->getStats()->getJob())) {
			if (player->getSkills()->getSkillLevel(skillId) >= s.reqLevel) {
				if (player->getSkills()->getMaxSkillLevel(skillId) < newMaxLevel) {
					if (Randomizer::rand<int8_t>(99) < s.chance) {
						player->getSkills()->setMaxSkillLevel(skillId, newMaxLevel);
						succeed = true;
					}

					Inventory::takeItemSlot(player, Inventories::UseInventory, slot, 1);
					break;
				}
			}
		}
	}

	if (skillId != 0) {
		InventoryPacket::useSkillbook(player, skillId, newMaxLevel, true, succeed);
	}
}

auto InventoryHandler::useChair(Player *player, PacketReader &packet) -> void {
	int32_t chairId = packet.get<int32_t>();
	player->setChair(chairId);
	InventoryPacket::sitChair(player, chairId);
}

auto InventoryHandler::handleChair(Player *player, PacketReader &packet) -> void {
	int16_t chair = packet.get<int16_t>();
	Map *map = player->getMap();
	if (chair == -1) {
		if (player->getChair() != 0) {
			player->setChair(0);
		}
		else {
			map->playerSeated(player->getMapChair(), nullptr);
			player->setMapChair(0);
		}
		InventoryPacket::stopChair(player);
	}
	else {
		// Map chair
		if (map->seatOccupied(chair)) {
			InventoryPacket::stopChair(player, false);
		}
		else {
			map->playerSeated(chair, player);
			player->setMapChair(chair);
			InventoryPacket::sitMapChair(player, chair);
		}
	}
}

auto InventoryHandler::useSummonBag(Player *player, PacketReader &packet) -> void {
	uint32_t ticks = packet.get<uint32_t>();
	int16_t slot = packet.get<int16_t>();
	int32_t itemId = packet.get<int32_t>();

	if (!ItemDataProvider::getInstance().summonBagExists(itemId)) {
		// Most likely hacking
		return;
	}

	Item *inventoryItem = player->getInventory()->getItem(Inventories::UseInventory, slot);
	if (inventoryItem == nullptr || inventoryItem->getId() != itemId) {
		// Hacking
		return;
	}

	Inventory::takeItemSlot(player, Inventories::UseInventory, slot, 1);

	vector_t<SummonBag> *item = ItemDataProvider::getInstance().getItemSummons(itemId);
	for (size_t i = 0; i < item->size(); i++) {
		const SummonBag &s = (*item)[i];
		if (Randomizer::rand<uint32_t>(99) < s.chance) {
			if (MobDataProvider::getInstance().mobExists(s.mobId)) {
				player->getMap()->spawnMob(s.mobId, player->getPos());
			}
		}
	}
}

auto InventoryHandler::useReturnScroll(Player *player, PacketReader &packet) -> void {
	uint32_t ticks = packet.get<uint32_t>();
	int16_t slot = packet.get<int16_t>();
	int32_t itemId = packet.get<int32_t>();

	Item *item = player->getInventory()->getItem(Inventories::UseInventory, slot);
	if (item == nullptr || item->getId() != itemId) {
		// Hacking
		return;
	}
	ConsumeInfo *info = ItemDataProvider::getInstance().getConsumeInfo(itemId);
	if (info == nullptr) {
		// Probably hacking
		return;
	}

	Inventory::takeItemSlot(player, Inventories::UseInventory, slot, 1);

	int32_t map = info->moveTo;
	player->setMap(map == Maps::NoMap ? player->getMap()->getReturnMap() : map);
}

auto InventoryHandler::useScroll(Player *player, PacketReader &packet) -> void {
	uint32_t ticks = packet.get<uint32_t>();
	int16_t slot = packet.get<int16_t>();
	int16_t equipSlot = packet.get<int16_t>();
	bool whiteScroll = (packet.get<int16_t>() == 2);
	bool legendarySpirit = packet.get<bool>();

	Item *item = player->getInventory()->getItem(Inventories::UseInventory, slot);
	Item *equip = player->getInventory()->getItem(Inventories::EquipInventory, equipSlot);
	if (item == nullptr || equip == nullptr) {
		// Most likely hacking
		return;
	}

	int32_t itemId = item->getId();
	int8_t succeed = -1;
	bool cursed = false;
	ItemDataProvider::getInstance().scrollItem(itemId, equip, whiteScroll, player->hasGmBenefits(), succeed, cursed);

	if (succeed != -1) {
		if (whiteScroll) {
			Inventory::takeItem(player, Items::WhiteScroll, 1);
		}

		Inventory::takeItemSlot(player, Inventories::UseInventory, slot, 1);
		InventoryPacket::useScroll(player, succeed, cursed, legendarySpirit);

		if (!cursed) {
			player->getStats()->setEquip(equipSlot, equip);

			vector_t<InventoryPacketOperation> ops;
			ops.emplace_back(InventoryPacket::OperationTypes::AddItem, equip, equipSlot);
			InventoryPacket::inventoryOperation(player, true, ops);
		}
		else {
			vector_t<InventoryPacketOperation> ops;
			ops.emplace_back(InventoryPacket::OperationTypes::ModifySlot, equip, equipSlot);
			InventoryPacket::inventoryOperation(player, true, ops);

			player->getInventory()->deleteItem(Inventories::EquipInventory, equipSlot);
		}
		InventoryPacket::updatePlayer(player);
	}
	else {
		if (legendarySpirit) {
			InventoryPacket::useScroll(player, succeed, cursed, legendarySpirit);
		}
		InventoryPacket::blankUpdate(player);
	}
}

auto InventoryHandler::useCashItem(Player *player, PacketReader &packet) -> void {
	packet.get<int8_t>(); // Type
	packet.skipBytes(1);
	int32_t itemId = packet.get<int32_t>();

	if (player->getInventory()->getItemAmount(itemId) == 0) {
		// Hacking
		return;
	}

	bool used = false;
	if (GameLogicUtilities::getItemType(itemId) == Items::Types::WeatherCash) {
		string_t message = packet.getString();
		uint32_t ticks = packet.get<uint32_t>();
		if (message.length() <= 35) {
			Map *map = player->getMap();
			message = player->getName() + " 's message : " + message;
			used = map->createWeather(player, false, Items::WeatherTime, itemId, message);
		}
	}
	else if (GameLogicUtilities::getItemType(itemId) == Items::Types::CashPetFood) {
		Pet *pet = player->getPets()->getSummoned(0);
		if (pet != nullptr) {
			if (pet->getFullness() < Stats::MaxFullness) {
				PetsPacket::showAnimation(player, pet, 1);
				pet->modifyFullness(Stats::MaxFullness, false);
				pet->addCloseness(100); // All cash pet food gives 100 closeness
				used = true;
			}
		}
	}
	else {
		switch (itemId) {
			case Items::TeleportRock:
			case Items::TeleportCoke:
			case Items::VipRock: // Only occurs when you actually try to move somewhere
				used = handleRockTeleport(player, itemId, packet);
				break;
			case Items::FirstJobSpReset:
			case Items::SecondJobSpReset:
			case Items::ThirdJobSpReset:
			case Items::FourthJobSpReset: {
				int32_t toSkill = packet.get<int32_t>();
				int32_t fromSkill = packet.get<int32_t>();
				if (!player->getSkills()->addSkillLevel(fromSkill, -1, true)) {
					// Hacking
					return;
				}
				if (!player->getSkills()->addSkillLevel(toSkill, 1, true)) {
					// Hacking
					return;
				}
				used = true;
				break;
			}
			case Items::ApReset: {
				int32_t toStat = packet.get<int32_t>();
				int32_t fromStat = packet.get<int32_t>();
				player->getStats()->addStat(toStat, 1, true);
				player->getStats()->addStat(fromStat, -1, true);
				used = true;
				break;
			}
			case Items::Megaphone: {
				const string_t &msg = player->getMedalName() + " : " + packet.getString();
				InventoryPacket::showMegaphone(player, msg);
				used = true;
				break;
			}
			case Items::SuperMegaphone: {
				const string_t &msg = player->getMedalName() + " : " + packet.getString();
				bool whisper = packet.get<bool>();
				InventoryPacket::showSuperMegaphone(player, msg, whisper);
				used = true;
				break;
			}
			case Items::DiabloMessenger:
			case Items::Cloud9Messenger:
			case Items::LoveholicMessenger: {
				const string_t &msg = packet.getString();
				const string_t &msg2 = packet.getString();
				const string_t &msg3 = packet.getString();
				const string_t &msg4 = packet.getString();

				InventoryPacket::showMessenger(player, msg, msg2, msg3, msg4, packet.getBuffer(), packet.getBufferLength(), itemId);
				used = true;
				break;
			}
			case Items::ItemMegaphone: {
				const string_t &msg = player->getMedalName() + " : " + packet.getString();
				bool whisper = packet.get<bool>();
				Item *item = nullptr;
				if (packet.get<bool>()) {
					int8_t inv = static_cast<int8_t>(packet.get<int32_t>());
					int16_t slot = static_cast<int16_t>(packet.get<int32_t>());
					item = player->getInventory()->getItem(inv, slot);
					if (item == nullptr) {
						// Hacking
						return;
					}
				}
				InventoryPacket::showItemMegaphone(player, msg, whisper, item);
				used = true;
				break;
			}
			case Items::ArtMegaphone: {
				int8_t lines = packet.get<int8_t>();
				if (lines < 1 || lines > 3) {
					// Hacking
					return;
				}
				string_t text[3];
				for (int8_t i = 0; i < lines; i++) {
					text[i] = player->getMedalName() + " : " + packet.getString();
				}
				bool whisper = packet.get<bool>();
				InventoryPacket::showTripleMegaphone(player, lines, text[0], text[1], text[2], whisper);
				used = true;
				break;
			}
			case Items::PetNameTag: {
				const string_t &name = packet.getString();
				if (ValidCharDataProvider::getInstance().isForbiddenName(name) || CurseDataProvider::getInstance().isCurseWord(name)) {
					// Don't think it's hacking, but it should be forbidden
					return;
				}
				else {
					PetHandler::changeName(player, name);
					used = true;
				}
				break;
			}
			case Items::ItemNameTag: {
				int16_t slot = packet.get<int16_t>();
				if (slot != 0) {
					Item *item = player->getInventory()->getItem(Inventories::EquipInventory, slot);
					if (item == nullptr) {
						// Hacking or failure, dunno
						return;
					}
					item->setName(player->getName());

					vector_t<InventoryPacketOperation> ops;
					ops.emplace_back(InventoryPacket::OperationTypes::AddItem, item, slot);
					InventoryPacket::inventoryOperation(player, true, ops);
					used = true;
				}
				break;
			}
			case Items::ScissorsOfKarma:
			case Items::ItemLock: {
				int8_t inv = static_cast<int8_t>(packet.get<int32_t>());
				int16_t slot = static_cast<int16_t>(packet.get<int32_t>());
				if (slot != 0) {
					Item *item = player->getInventory()->getItem(inv, slot);
					if (item == nullptr) {
						// Hacking or failure, dunno
						return;
					}
					switch (itemId) {
						case Items::ItemLock:
							if (item->hasLock()) {
								// Hacking
								return;
							}
							item->setLock(true);
							break;
						case Items::ScissorsOfKarma:
							if (!ItemDataProvider::getInstance().canKarma(item->getId())) {
								// Hacking
								return;
							}
							if (item->hasTradeBlock() || item->hasKarma()) {
								// Hacking
								return;
							}
							item->setKarma(true);
							break;
					}

					vector_t<InventoryPacketOperation> ops;
					ops.emplace_back(InventoryPacket::OperationTypes::AddItem, item, slot);
					InventoryPacket::inventoryOperation(player, true, ops);
					used = true;
				}
				break;
			}
			case Items::MapleTvMessenger:
			case Items::Megassenger: {
				bool hasReceiver = (packet.get<int8_t>() == 3);
				bool showWhisper = (itemId == Items::Megassenger ? packet.get<bool>() : false);
				Player *receiver = PlayerDataProvider::getInstance().getPlayer(packet.getString());
				int32_t time = 15;

				if ((hasReceiver && receiver != nullptr) || (!hasReceiver && receiver == nullptr)) {
					const string_t &msg = packet.getString();
					const string_t &msg2 = packet.getString();
					const string_t &msg3 = packet.getString();
					const string_t &msg4 = packet.getString();
					const string_t &msg5 = packet.getString();
					uint32_t ticks = packet.get<uint32_t>();

					MapleTvs::getInstance().addMessage(player, receiver, msg, msg2, msg3, msg4, msg5, itemId - (itemId == Items::Megassenger ? 3 : 0), time);

					if (itemId == Items::Megassenger) {
						InventoryPacket::showSuperMegaphone(player, player->getMedalName() + " : " + msg + msg2 + msg3 + msg4 + msg5, showWhisper);
					}
					used = true;
				}
				break;
			}
			case Items::MapleTvStarMessenger:
			case Items::StarMegassenger: {
				int32_t time = 30;
				bool showWhisper = (itemId == Items::StarMegassenger ? packet.get<bool>() : false);
				const string_t &msg = packet.getString();
				const string_t &msg2 = packet.getString();
				const string_t &msg3 = packet.getString();
				const string_t &msg4 = packet.getString();
				const string_t &msg5 = packet.getString();
				uint32_t ticks = packet.get<uint32_t>();

				MapleTvs::getInstance().addMessage(player, nullptr, msg, msg2, msg3, msg4, msg5, itemId - (itemId == Items::StarMegassenger ? 3 : 0), time);

				if (itemId == Items::StarMegassenger) {
					InventoryPacket::showSuperMegaphone(player, player->getMedalName() + " : " + msg + msg2 + msg3 + msg4 + msg5, showWhisper);
				}
				used = true;
				break;
			}
			case Items::MapleTvHeartMessenger:
			case Items::HeartMegassenger: {
				bool showWhisper = (itemId == Items::HeartMegassenger ? packet.get<bool>() : false);
				const string_t &name = packet.getString();
				Player *receiver = PlayerDataProvider::getInstance().getPlayer(name);
				int32_t time = 60;

				if (receiver != nullptr) {
					const string_t &msg = packet.getString();
					const string_t &msg2 = packet.getString();
					const string_t &msg3 = packet.getString();
					const string_t &msg4 = packet.getString();
					const string_t &msg5 = packet.getString();
					uint32_t ticks = packet.get<uint32_t>();

					MapleTvs::getInstance().addMessage(player, receiver, msg, msg2, msg3, msg4, msg5, itemId - (itemId == Items::HeartMegassenger ? 3 : 0), time);

					if (itemId == Items::HeartMegassenger) {
						InventoryPacket::showSuperMegaphone(player, player->getMedalName() + " : " + msg + msg2 + msg3 + msg4 + msg5, showWhisper);
					}
					used = true;
				}
				break;
			}
			case Items::BronzeSackOfMesos:
			case Items::SilverSackOfMesos:
			case Items::GoldSackOfMesos: {
				int32_t mesos = ItemDataProvider::getInstance().getMesoBonus(itemId);
				if (!player->getInventory()->modifyMesos(mesos)) {
					InventoryPacket::sendMesobagFailed(player);
				}
				else {
					InventoryPacket::sendMesobagSucceed(player, mesos);
					used = true;
				}
				break;
			}
			case Items::Chalkboard:
			case Items::Chalkboard2: {
				const string_t &msg = packet.getString();
				player->setChalkboard(msg);
				InventoryPacket::sendChalkboardUpdate(player, msg);
				break;
			}
			case Items::FungusScroll:
			case Items::OinkerDelight:
			case Items::ZetaNightmare:
				Inventory::useItem(player, itemId);
				used = true;
				break;
			case Items::ViciousHammer: {
				int8_t inv = static_cast<int8_t>(packet.get<int32_t>());
				int16_t slot = static_cast<int16_t>(packet.get<int32_t>());
				Item *item = player->getInventory()->getItem(inv, slot);
				if (item == nullptr || item->getHammers() == Items::MaxHammers || EquipDataProvider::getInstance().getSlots(item->getId()) == 0) {
					// Hacking, probably
					return;
				}
				item->incHammers();
				item->incSlots();
				InventoryPacket::sendHammerSlots(player, item->getHammers());
				player->getInventory()->setHammerSlot(slot);
				used = true;
				break;
			}
			case Items::CongratulatorySong:
				InventoryPacket::playCashSong(player->getMapId(), itemId, player->getName());
				used = true;
				break;
		}
	}
	if (used) {
		Inventory::takeItem(player, itemId, 1);
	}
	else {
		InventoryPacket::blankUpdate(player);
	}
}

auto InventoryHandler::useItemEffect(Player *player, PacketReader &packet) -> void {
	int32_t itemId = packet.get<int32_t>();
	if (player->getInventory()->getItemAmount(itemId) == 0) {
		// Hacking
		return;
	}
	player->setItemEffect(itemId);
	InventoryPacket::useItemEffect(player, itemId);
}

auto InventoryHandler::handleRockFunctions(Player *player, PacketReader &packet) -> void {
	int8_t mode = packet.get<int8_t>();
	int8_t type = packet.get<int8_t>();

	enum Modes : int8_t {
		Remove = 0x00,
		Add = 0x01
	};

	if (mode == Remove) {
		int32_t map = packet.get<int32_t>();
		player->getInventory()->delRockMap(map, type);
	}
	else if (mode == Add) {
		int32_t map = player->getMapId();
		Map *m = Maps::getMap(map);
		if (m->canVip() && m->getContinent() != 0) {
			player->getInventory()->addRockMap(map, type);
		}
		else {
			// Hacking, the client doesn't allow this to occur
			InventoryPacket::sendRockError(player, InventoryPacket::RockErrors::CannotSaveMap, type);
		}
	}
}

auto InventoryHandler::handleRockTeleport(Player *player, int32_t itemId, PacketReader &packet) -> bool {
	if (itemId == Items::SpecialTeleportRock) {
		packet.skipBytes(5);
	}
	int8_t type = InventoryPacket::RockTypes::Regular;
	switch (itemId) {
		case Items::VipRock: type = InventoryPacket::RockTypes::Vip; break;
		case Items::SpecialTeleportRock: type = packet.get<int8_t>(); break;
	}
	bool used = false;
	int8_t mode = packet.get<int8_t>();
	int32_t targetMapId = -1;

	enum Modes : int8_t {
		PresetMap = 0x00,
		Ign = 0x01
	};
	if (mode == PresetMap) {
		targetMapId = packet.get<int32_t>();
		if (!player->getInventory()->ensureRockDestination(targetMapId)) {
			// Hacking
			return false;
		}
	}
	else if (mode == Ign) {
		const string_t &targetName = packet.getString();
		Player *target = PlayerDataProvider::getInstance().getPlayer(targetName);
		if (target != nullptr && target != player) {
			targetMapId = target->getMapId();
		}
		else if (target == nullptr) {
			InventoryPacket::sendRockError(player, InventoryPacket::RockErrors::DifficultToLocate, type);
		}
		else if (target == player) {
			// Hacking
			return false;
		}
	}
	if (targetMapId != -1) {
		Map *destination = Maps::getMap(targetMapId);
		Map *origin = player->getMap();
		if (!destination->canVip()) {
			InventoryPacket::sendRockError(player, InventoryPacket::RockErrors::CannotGo, type);
		}
		else if (!origin->canVip()) {
			InventoryPacket::sendRockError(player, InventoryPacket::RockErrors::CannotGo, type);
		}
		else if (player->getMapId() == targetMapId) {
			InventoryPacket::sendRockError(player, InventoryPacket::RockErrors::AlreadyThere, type);
		}
		else if (type == 0 && destination->getContinent()!= origin->getContinent()) {
			InventoryPacket::sendRockError(player, InventoryPacket::RockErrors::CannotGo, type);
		}
		else if (player->getStats()->getLevel() < 7 && origin->getContinent() == 0 && destination->getContinent() != 0) {
			InventoryPacket::sendRockError(player, InventoryPacket::RockErrors::NoobsCannotLeaveMapleIsland, type);
		}
		else {
			player->setMap(targetMapId);
			used = true;
		}
	}
	if (itemId == Items::SpecialTeleportRock) {
		if (used) {
			Inventory::takeItem(player, itemId, 1);
		}
		else {
			InventoryPacket::blankUpdate(player);
		}
	}
	return used;
}

auto InventoryHandler::handleHammerTime(Player *player) -> void {
	if (!player->getInventory()->isHammering()) {
		// Hacking
		return;
	}
	int16_t hammerSlot = player->getInventory()->getHammerSlot();
	Item *item = player->getInventory()->getItem(Inventories::EquipInventory, hammerSlot);
	InventoryPacket::sendHammerUpdate(player);
	InventoryPacket::sendHulkSmash(player, hammerSlot, item);
	player->getInventory()->setHammerSlot(-1);
}

auto InventoryHandler::handleRewardItem(Player *player, PacketReader &packet) -> void {
	int16_t slot = packet.get<int16_t>();
	int32_t itemId = packet.get<int32_t>();
	Item *item = player->getInventory()->getItem(Inventories::UseInventory, slot);
	if (item == nullptr || item->getId() != itemId) {
		// Hacking or hacking failure
		InventoryPacket::blankUpdate(player); // We don't want stuck players, do we?
		return;
	}

	ItemRewardInfo *reward = ItemDataProvider::getInstance().getRandomReward(itemId);
	if (reward == nullptr) {
		// Hacking or no information in the database
		InventoryPacket::blankUpdate(player); // We don't want stuck players, do we?
		return;
	}

	Inventory::takeItem(player, itemId, 1);
	Item *rewardItem = new Item(reward->rewardId, reward->quantity);
	Inventory::addItem(player, rewardItem, true);
	InventoryPacket::sendRewardItemAnimation(player, itemId, reward->effect);
}

auto InventoryHandler::handleScriptItem(Player *player, PacketReader &packet) -> void {
	if (player->getNpc() != nullptr || player->getShop() != 0 || player->getTradeId() != 0) {
		// Hacking
		InventoryPacket::blankUpdate(player);
		return;
	}

	uint32_t ticks = packet.get<uint32_t>();
	int16_t slot = packet.get<int16_t>();
	int32_t itemId = packet.get<int32_t>();

	Item *item = player->getInventory()->getItem(Inventories::UseInventory, slot);
	if (item == nullptr || item->getId() != itemId) {
		// Hacking or hacking failure
		InventoryPacket::blankUpdate(player);
		return;
	}

	const string_t &scriptName = ScriptDataProvider::getInstance().getScript(itemId, ScriptTypes::Item);
	if (scriptName == "") {
		// Hacking or no script for item found
		InventoryPacket::blankUpdate(player);
		return;
	}

	int32_t npcId = ItemDataProvider::getInstance().getItemNpc(itemId);

	// Let's run the NPC
	Npc *npc = new Npc(npcId, player, scriptName);
	if (!npc->checkEnd()) {
		// NPC is running/script found
		// Delete the item used
		Inventory::takeItem(player, itemId, 1);
		npc->run();
	}
	else {
		// NPC didn't run/no script found
		InventoryPacket::blankUpdate(player);
	}
}