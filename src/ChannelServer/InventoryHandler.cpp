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
#include "InventoryHandler.h"
#include "Drop.h"
#include "GameConstants.h"
#include "GameLogicUtilities.h"
#include "Inventory.h"
#include "InventoryPacket.h"
#include "ItemDataProvider.h"
#include "Levels.h"
#include "Maps.h"
#include "MapleTVs.h"
#include "PacketReader.h"
#include "Pets.h"
#include "Player.h"
#include "Players.h"
#include "Randomizer.h"
#include "Reactors.h"
#include "ShopDataProvider.h"
#include "StoragePacket.h"

void InventoryHandler::itemMove(Player *player, PacketReader &packet) {
	packet.skipBytes(4);
	int8_t inv = packet.get<int8_t>();
	int16_t slot1 = packet.get<int16_t>();
	int16_t slot2 = packet.get<int16_t>();
	if (inv < Inventories::EquipInventory || inv > Inventories::CashInventory)
		return;
	if (slot2 == 0) {
		int16_t amount = packet.get<int16_t>();
		Item *item = player->getInventory()->getItem(inv, slot1);
		if (item == 0)
			return;
		if (GameLogicUtilities::isEquip(item->id) || GameLogicUtilities::isRechargeable(item->id))
			amount = item->amount;
		else if (amount <= 0 || amount > item->amount) {
			// hacking
			return;
		}
		Item droppeditem = Item(item);
		droppeditem.amount = amount;
		if (item->amount == amount) {
			InventoryPacket::moveItem(player, inv, slot1, slot2);
			player->getInventory()->deleteItem(inv, slot1);
		}
		else {
			item->amount -= amount;
			player->getInventory()->changeItemAmount(item->id, -amount);
			InventoryPacket::updateItemAmounts(player, inv, slot1, item->amount, 0, 0);
		}
		Drop *drop = new Drop(player->getMap(), droppeditem, player->getPos(), player->getId(), true);
		drop->setTime(0);
		bool istradeable = true;
		if (GameLogicUtilities::isEquip(droppeditem.id)) {
			EquipInfo info = ItemDataProvider::Instance()->getEquipInfo(droppeditem.id);
			istradeable = !(info.notrade || info.quest);
		}
		else {
			ItemInfo info = ItemDataProvider::Instance()->getItemInfo(droppeditem.id);
			istradeable = !(info.notrade || info.quest);
		}
		drop->setTradeable(istradeable);
		drop->doDrop(player->getPos());
		if (istradeable) // Drop is deleted otherwise, avoid like plague
			Reactors::checkDrop(player, drop);
	}
	else {
		Item *item1 = player->getInventory()->getItem(inv, slot1);
		Item *item2 = player->getInventory()->getItem(inv, slot2);

		if (item1 == 0) {
			// Hacking
			return;
		}

		if (item2 != 0 && !GameLogicUtilities::isRechargeable(item1->id) && !GameLogicUtilities::isEquip(item1->id) && !GameLogicUtilities::isPet(item1->id) && item1->id == item2->id) {
			if (item1->amount + item2->amount <= ItemDataProvider::Instance()->getMaxSlot(item1->id)) {
				item2->amount += item1->amount;
				player->getInventory()->deleteItem(inv, slot1, false);
				InventoryPacket::updateItemAmounts(player, inv, slot2, item2->amount, 0, 0);
				InventoryPacket::moveItem(player, inv, slot1, 0);
			}
			else {
				item1->amount -= (ItemDataProvider::Instance()->getMaxSlot(item1->id) - item2->amount);
				item2->amount = ItemDataProvider::Instance()->getMaxSlot(item2->id);
				InventoryPacket::updateItemAmounts(player, inv, slot1, item1->amount, slot2, item2->amount);
			}
		}
		else {
			if (slot2 < 0) {
				Item *remove = 0;
				int16_t oldslot = 0;
				bool weapon = -slot2 == EquipSlots::Weapon;
				bool shield = -slot2 == EquipSlots::Shield;
				bool top = -slot2 == EquipSlots::Top;
				bool bottom = -slot2 == EquipSlots::Bottom;

				if (weapon && GameLogicUtilities::is2hWeapon(item1->id) && player->getInventory()->getEquippedId(EquipSlots::Shield) != 0) {
					oldslot = -EquipSlots::Shield;
				}
				else if (shield && GameLogicUtilities::is2hWeapon(player->getInventory()->getEquippedId(EquipSlots::Weapon))) {
					oldslot = -EquipSlots::Weapon;
				}
				else if (top && GameLogicUtilities::isOverall(item1->id) && player->getInventory()->getEquippedId(EquipSlots::Bottom) != 0) {
					oldslot = -EquipSlots::Bottom;
				}
				else if (bottom && GameLogicUtilities::isOverall(player->getInventory()->getEquippedId(EquipSlots::Top))) {
					oldslot = -EquipSlots::Top;
				}
				if (oldslot != 0) {
					remove = player->getInventory()->getItem(inv, oldslot);
					bool onlyswap = true;
					if ((player->getInventory()->getEquippedId(EquipSlots::Shield) != 0) && (player->getInventory()->getEquippedId(EquipSlots::Weapon) != 0)) {
						onlyswap = false;
					}
					else if ((player->getInventory()->getEquippedId(EquipSlots::Top) != 0) && (player->getInventory()->getEquippedId(EquipSlots::Bottom) != 0)) {
						onlyswap = false;
					}
					if (onlyswap) {
						int16_t swapslot = 0;
						if (weapon) {
							swapslot = -EquipSlots::Shield;
							player->getActiveBuffs()->stopBooster();
							player->getActiveBuffs()->stopCharge();
						}
						else if (shield) {
							swapslot = -EquipSlots::Weapon;
							player->getActiveBuffs()->stopBooster();
							player->getActiveBuffs()->stopCharge();
						}
						else if (top) {
							swapslot = -EquipSlots::Bottom;
						}
						else if (bottom) {
							swapslot = -EquipSlots::Top;
						}
						player->getInventory()->setItem(inv, swapslot, 0);
						player->getInventory()->setItem(inv, slot1, remove);
						player->getInventory()->setItem(inv, slot2, item1);
						InventoryPacket::moveItem(player, inv, slot1, slot2);
						InventoryPacket::moveItem(player, inv, swapslot, slot1);
						InventoryPacket::updatePlayer(player);
						return;
					}
					else {
						if (player->getInventory()->getOpenSlotsNum(inv) == 0) {
							InventoryPacket::blankUpdate(player);
							return;
						}
						int16_t freeslot = 0;
						for (int16_t s = 1; s <= player->getInventory()->getMaxSlots(inv); s++) {
							Item *olditem = player->getInventory()->getItem(inv, s);
							if (olditem == 0) {
								freeslot = s;
								break;
							}
						}
						player->getInventory()->setItem(inv, freeslot, remove);
						player->getInventory()->setItem(inv, oldslot, 0);
						InventoryPacket::moveItem(player, inv, oldslot, freeslot);
					}
				}
			}
			player->getInventory()->setItem(inv, slot1, item2);
			player->getInventory()->setItem(inv, slot2, item1);
			if (item1->petid > 0)
				player->getPets()->getPet(item1->petid)->setInventorySlot((int8_t) slot2);
			if (item2 != 0 && item2->petid > 0)
				player->getPets()->getPet(item2->petid)->setInventorySlot((int8_t) slot1);
			InventoryPacket::moveItem(player, inv, slot1, slot2);
		}
	}
	if ((slot1 < 0 && -slot1 == EquipSlots::Weapon) || (slot2 < 0 && -slot2 == EquipSlots::Weapon)) {
		player->getActiveBuffs()->stopBooster();
		player->getActiveBuffs()->stopCharge();
		player->getActiveBuffs()->stopBulletSkills();
	}
	if (slot1 < 0 || slot2 < 0) {
		InventoryPacket::updatePlayer(player);
	}
}

void InventoryHandler::useShop(Player *player, PacketReader &packet) {
	int8_t type = packet.get<int8_t>();
	switch (type) {
		case 0: { // Buy
			int16_t itemindex = packet.get<int16_t>();
			packet.skipBytes(4); // Item ID, no reason to trust this
			int16_t quantity = packet.get<int16_t>();
			packet.skipBytes(4); // Price, don't want to trust this
			int16_t amount = ShopDataProvider::Instance()->getAmount(player->getShop(), itemindex);
			int32_t itemid = ShopDataProvider::Instance()->getItemId(player->getShop(), itemindex);
			int32_t price = ShopDataProvider::Instance()->getPrice(player->getShop(), itemindex);
			if (quantity < 0 || price == 0 || player->getInventory()->getMesos() < (price * quantity)) {
				// Hacking
				return;
			}
			bool haveslot = player->getInventory()->hasOpenSlotsFor(itemid, quantity * amount, true);
			if (haveslot) {
				Inventory::addNewItem(player, itemid, quantity * amount);
				player->getInventory()->modifyMesos(-(quantity * price));
			}
			InventoryPacket::bought(player, haveslot ? 0 : 3);
			break;
		}
		case 1: { // Sell
			int16_t slot = packet.get<int16_t>();
			int32_t itemid = packet.get<int32_t>();
			int16_t amount = packet.get<int16_t>();
			int8_t inv = GameLogicUtilities::getInventory(itemid);
			Item *item = player->getInventory()->getItem(inv, slot);
			if (item == 0 || (!GameLogicUtilities::isRechargeable(itemid) && amount > item->amount)) {
				InventoryPacket::bought(player, 1); // Hacking
				return;
			}
			int32_t price = ItemDataProvider::Instance()->getPrice(itemid);

			player->getInventory()->modifyMesos(price * amount);
			if (GameLogicUtilities::isRechargeable(itemid))
				Inventory::takeItemSlot(player, inv, slot, item->amount, true);
			else
				Inventory::takeItemSlot(player, inv, slot, amount, true);
			InventoryPacket::bought(player, 0);
			break;
		}
		case 2: { // Recharge
			int16_t slot = packet.get<int16_t>();
			Item *item = player->getInventory()->getItem(Inventories::UseInventory, slot);
			if (item == 0 || GameLogicUtilities::isRechargeable(item->id) == false) {
				// Hacking
				return;
			}
			int16_t maxslot = ItemDataProvider::Instance()->getMaxSlot(item->id);
			if (GameLogicUtilities::isRechargeable(item->id))
				maxslot += player->getSkills()->getRechargeableBonus();

			int32_t modifiedmesos = ShopDataProvider::Instance()->getRechargeCost(player->getShop(), item->id, maxslot - item->amount);
			if ((modifiedmesos < 0) && (player->getInventory()->getMesos() > -modifiedmesos)) {
				player->getInventory()->modifyMesos(modifiedmesos);
				InventoryPacket::updateItemAmounts(player, Inventories::UseInventory, slot, maxslot, 0, 0);
				item->amount = maxslot;
				InventoryPacket::bought(player, 0);
			}
			break;
		}
		case 3:
			// 3 is close shop. For now we have no reason to handle this.
			break;
	}
}

void InventoryHandler::useStorage(Player *player, PacketReader &packet) {
	int8_t type = packet.get<int8_t>();
	switch (type) {
		case 4: { // Take item out
			int8_t inv = packet.get<int8_t>(); // Inventory, as in equip, use, etc
			int8_t slot = packet.get<int8_t>(); // Slot within the inventory
			Item *item = player->getStorage()->getItem(slot);
			if (item == 0) { // It's a trap
				// hacking
				return;
			}
			Inventory::addItem(player, new Item(item));
			player->getStorage()->takeItem(slot);
			StoragePacket::takeItem(player, inv);
			break;
		}
		case 5: { // Store item
			int16_t slot = packet.get<int16_t>();
			int32_t itemid = packet.get<int32_t>();
			int16_t amount = packet.get<int16_t>();
			if (player->getInventory()->getMesos() < 100) {
				StoragePacket::noMesos(player); // We don't have enough mesos to store this item
				return;
			}
			if (player->getStorage()->isFull()) { // Storage is full, so tell the player and abort the mission.
				StoragePacket::storageFull(player);
				return;
			}
			int8_t inv = GameLogicUtilities::getInventory(itemid);
			Item *item = player->getInventory()->getItem(inv, slot);
			if (item == 0) {
				// hacking
				return;
			}
			if (GameLogicUtilities::isRechargeable(itemid) || GameLogicUtilities::isEquip(itemid))
				amount = 1;
			else if (amount <= 0 || amount > item->amount) {
				// hacking
				return;
			}
			player->getStorage()->addItem((inv == Inventories::EquipInventory || GameLogicUtilities::isRechargeable(itemid)) ? new Item(item) : new Item(itemid, amount));
			// For equips or rechargeable items (stars/bullets) we create a
			// new object for storage with the inventory object, and allow
			// the one in the inventory to go bye bye.
			// Else: For items we just create a new item based on the ID and amount.
			Inventory::takeItemSlot(player, inv, slot, GameLogicUtilities::isRechargeable(itemid) ? item->amount : amount, true);
			player->getInventory()->modifyMesos(-100); // Take 100 mesos for storage cost
			StoragePacket::addItem(player, inv);
			break;
		}
		case 7: { // Take out/store mesos
			int32_t mesos = packet.get<int32_t>(); // Amount of mesos to remove. Deposits are negative, and withdrawls are positive.
			bool success = player->getInventory()->modifyMesos(mesos);
			if (success)
				player->getStorage()->changeMesos(mesos);
			break;
		}
		case 8:
			// 8 is close storage. For now we have no reason to handle this.
			break;
	}
}

void InventoryHandler::useItem(Player *player, PacketReader &packet) {
	packet.skipBytes(4);
	int16_t slot = packet.get<int16_t>();
	int32_t itemid = packet.get<int32_t>();
	if (player->getStats()->getHp() == 0 || player->getInventory()->getItemAmountBySlot(Inventories::UseInventory, slot) == 0) {
		// hacking
		return;
	}
	Inventory::takeItemSlot(player, Inventories::UseInventory, slot, 1);
	Inventory::useItem(player, itemid);
}

void InventoryHandler::cancelItem(Player *player, PacketReader &packet) {
	int32_t itemid = packet.get<int32_t>();
	Buffs::Instance()->endBuff(player, itemid);
}

void InventoryHandler::useSkillbook(Player *player, PacketReader &packet) {
	packet.skipBytes(4);
	int16_t slot = packet.get<int16_t>();
	int32_t itemid = packet.get<int32_t>();

	if (player->getInventory()->getItemAmountBySlot(Inventories::UseInventory, slot) == 0) {
		// hacking
		return;
	}

	int32_t skillid = 0;
	uint8_t newMaxLevel = 0;
	bool use = false;
	bool succeed = false;

	ItemInfo item = ItemDataProvider::Instance()->getItemInfo(itemid);
	for (size_t i = 0; i < item.cons.skills.size(); i++) {
		skillid = item.cons.skills[i].skillid;
		newMaxLevel = item.cons.skills[i].maxlevel;
		if (player->getStats()->getJob() == item.cons.skills[i].skillid / 10000) { // Make sure the skill is for the person's job
			if (player->getSkills()->getSkillLevel(skillid) >= item.cons.skills[i].reqlevel && player->getSkills()->getMaxSkillLevel(skillid) < newMaxLevel)
				use = true;
		}
		if (use) {
			if ((int16_t) Randomizer::Instance()->randShort(100) <= item.cons.success) {
				player->getSkills()->setMaxSkillLevel(skillid, newMaxLevel);
				succeed = true;
			}
			Inventory::takeItemSlot(player, Inventories::UseInventory, slot, 1);
			break;
		}
	}

	if (skillid == 0)
		return;

	InventoryPacket::useSkillbook(player, skillid, newMaxLevel, use, succeed);
}
void InventoryHandler::useChair(Player *player, PacketReader &packet) {
	int32_t chairid = packet.get<int32_t>();
	player->setChair(chairid);
	InventoryPacket::sitChair(player, chairid);
}

void InventoryHandler::handleChair(Player *player, PacketReader &packet) {
	int16_t chair = packet.get<int16_t>();
	Map *map = Maps::getMap(player->getMap());
	if (chair == -1) {
		if (player->getChair() != 0) {
			player->setChair(0);
		}
		else {
			map->playerSeated(player->getMapChair(), 0);
			player->setMapChair(0);
		}
		InventoryPacket::stopChair(player);
	}
	else { // Map chair
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

void InventoryHandler::useSummonBag(Player *player, PacketReader &packet) {
	packet.skipBytes(4);
	int16_t slot = packet.get<int16_t>();
	int32_t itemid = packet.get<int32_t>();
	Item *inventoryitem = player->getInventory()->getItem(Inventories::UseInventory, slot);
	if (!ItemDataProvider::Instance()->itemExists(itemid))
		return;
	if (inventoryitem == 0 || inventoryitem->id != itemid) {
		// hacking
		return;
	}
	Inventory::takeItemSlot(player, Inventories::UseInventory, slot, 1);

	ItemInfo item = ItemDataProvider::Instance()->getItemInfo(itemid);
	for (size_t i = 0; i < item.cons.mobs.size(); i++) {
		if (Randomizer::Instance()->randInt(100) <= item.cons.mobs[i].chance) {
			Maps::getMap(player->getMap())->spawnMob(item.cons.mobs[i].mobid, player->getPos());
		}
	}
}

void InventoryHandler::useReturnScroll(Player *player, PacketReader &packet) {
	packet.skipBytes(4);
	int16_t slot = packet.get<int16_t>();
	int32_t itemid = packet.get<int32_t>();
	if (player->getInventory()->getItemAmountBySlot(Inventories::UseInventory, slot) == 0) {
		// hacking
		return;
	}
	if (!ItemDataProvider::Instance()->itemExists(itemid))
		return;
	Inventory::takeItemSlot(player, Inventories::UseInventory, slot, 1);
	int32_t map = ItemDataProvider::Instance()->getItemInfo(itemid).cons.moveTo;
	player->setMap(map == Maps::NoMap ? Maps::getMap(player->getMap())->getInfo()->rm : map);
}

void InventoryHandler::useScroll(Player *player, PacketReader &packet) {
	packet.skipBytes(4);
	int16_t slot = packet.get<int16_t>();
	int16_t eslot = packet.get<int16_t>();
	bool wscroll = (packet.get<int16_t>() == 2);
	bool legendary_spirit = (packet.get<int8_t>() != 0);

	Item *item = player->getInventory()->getItem(Inventories::UseInventory, slot);
	Item *equip = player->getInventory()->getItem(Inventories::EquipInventory, eslot);
	if (item == 0 || equip == 0)
		return;

	int32_t itemid = item->id;
	int8_t succeed = -1;
	bool cursed = false;
	if (!ItemDataProvider::Instance()->itemExists(itemid))
		return;

	ItemInfo iteminfo = ItemDataProvider::Instance()->getItemInfo(itemid);

	if (iteminfo.cons.randstat) {
		if (equip->slots > 0) {
			succeed = 0;
			if (wscroll)
				Inventory::takeItem(player, Items::WhiteScroll, 1);
			if (Randomizer::Instance()->randShort(99) < iteminfo.cons.success) { // Add stats
				int8_t n = -1; // Default - Decrease stats
				if (Randomizer::Instance()->randShort(99) < 50U) // Increase
					n = 1;
				// Gives/takes 0-5 stats on every stat on the item
				if (equip->istr > 0)
					equip->istr += Randomizer::Instance()->randShort(5) * n;
				if (equip->idex > 0)
					equip->idex += Randomizer::Instance()->randShort(5) * n;
				if (equip->iint > 0)
					equip->iint += Randomizer::Instance()->randShort(5) * n;
				if (equip->iluk > 0)
					equip->iluk += Randomizer::Instance()->randShort(5) * n;
				if (equip->iavo > 0)
					equip->iavo += Randomizer::Instance()->randShort(5) * n;
				if (equip->iacc > 0)
					equip->iacc += Randomizer::Instance()->randShort(5) * n;
				if (equip->ihand > 0)
					equip->ihand += Randomizer::Instance()->randShort(5) * n;
				if (equip->ijump > 0)
					equip->ijump += Randomizer::Instance()->randShort(5) * n;
				if (equip->ispeed > 0)
					equip->ispeed += Randomizer::Instance()->randShort(5) * n;
				if (equip->imatk > 0)
					equip->imatk += Randomizer::Instance()->randShort(5) * n;
				if (equip->iwatk > 0)
					equip->iwatk += Randomizer::Instance()->randShort(5) * n;
				if (equip->imdef > 0)
					equip->imdef += Randomizer::Instance()->randShort(5) * n;
				if (equip->iwdef > 0)
					equip->iwdef += Randomizer::Instance()->randShort(5) * n;
				if (equip->ihp > 0)
					equip->ihp += Randomizer::Instance()->randShort(5) * n;
				if (equip->imp > 0)
					equip->imp += Randomizer::Instance()->randShort(5) * n;
				equip->scrolls++;
				equip->slots--;
				succeed = 1;
			}
			else if (!wscroll)
				equip->slots--;
		}
	}
	else if (iteminfo.cons.recover) {
		if ((ItemDataProvider::Instance()->getEquipInfo(equip->id).slots - equip->scrolls) > equip->slots) {
			if (Randomizer::Instance()->randShort(99) < iteminfo.cons.success) { // Give back a slot
				equip->slots++;
				succeed = 1;
			}
			else {
				if (Randomizer::Instance()->randShort(99) < iteminfo.cons.cursed)
					cursed = true;
				succeed = 0;
			}
		}
	}
	else {
		switch (itemid) {
			case Items::ShoeSpikes: // 10%
			case Items::CapeColdProtection: // 10%
				succeed = 0;
				if (Randomizer::Instance()->randShort(99) < iteminfo.cons.success) {
					// These do not take slots and can be used even after success
					switch (itemid) {
						case Items::ShoeSpikes:
							equip->flags |= FlagSpikes;
							break;
						case Items::CapeColdProtection:
							equip->flags |= FlagCold;
							break;
					}
					succeed = 1;
				}
				break;
			default: // Most scrolls
				if (equip->slots > 0) {
					if (wscroll)
						Inventory::takeItem(player, Items::WhiteScroll, 1);
					if (Randomizer::Instance()->randShort(99) < iteminfo.cons.success) {
						succeed = 1;
						equip->istr += iteminfo.cons.istr;
						equip->idex += iteminfo.cons.idex;
						equip->iint += iteminfo.cons.iint;
						equip->iluk += iteminfo.cons.iluk;
						equip->ihp += iteminfo.cons.ihp;
						equip->imp += iteminfo.cons.imp;
						equip->iwatk += iteminfo.cons.iwatk;
						equip->imatk += iteminfo.cons.imatk;
						equip->iwdef += iteminfo.cons.iwdef;
						equip->imdef += iteminfo.cons.imdef;
						equip->iacc += iteminfo.cons.iacc;
						equip->iavo += iteminfo.cons.iavo;
						equip->ihand += iteminfo.cons.ihand;
						equip->ijump += iteminfo.cons.ijump;
						equip->ispeed += iteminfo.cons.ispeed;
						equip->scrolls++;
						equip->slots--;
					}
					else {
						succeed = 0;
						if (Randomizer::Instance()->randShort(99) < iteminfo.cons.cursed)
							cursed = true;
						else if (!wscroll)
							equip->slots--;
					}
				}
				break;
		}
	}
	if (succeed != -1) {
		Inventory::takeItemSlot(player, Inventories::UseInventory, slot, 1);
		InventoryPacket::useScroll(player, succeed, cursed, legendary_spirit);
		if (!cursed)
			InventoryPacket::addNewItem(player, Inventories::EquipInventory, eslot, equip, true);
		else {
			InventoryPacket::moveItem(player, Inventories::EquipInventory, eslot, 0);
			player->getInventory()->deleteItem(Inventories::EquipInventory, eslot);
		}
		InventoryPacket::updatePlayer(player);
	}
	else {
		if (legendary_spirit)
			InventoryPacket::useScroll(player, succeed, cursed, legendary_spirit);
		InventoryPacket::blankUpdate(player);
	}
}

void InventoryHandler::useCashItem(Player *player, PacketReader &packet) {
	int8_t type = packet.get<int8_t>();
	packet.skipBytes(1);
	int32_t itemid = packet.get<int32_t>();

	if (!player->getInventory()->getItemAmount(itemid)) {
		// Hacking
		return;
	}

	bool used = false;
	switch (itemid) {
		case Items::TeleportRock:
		case Items::TeleportCoke:
		case Items::VipRock: // Only occurs when you actually try to move somewhere
			used = handleRockTeleport(player, (itemid == Items::VipRock ? 1 : 0), itemid, packet);
			break;
		case Items::FirstJobSpReset:
		case Items::SecondJobSpReset:
		case Items::ThirdJobSpReset:
		case Items::FourthJobSpReset: {
			int32_t toskill = packet.get<int32_t>();
			int32_t fromskill = packet.get<int32_t>();
			if (!player->getSkills()->addSkillLevel(fromskill, -1, true)) {
				// Hacking
				return;
			}
			if (!player->getSkills()->addSkillLevel(toskill, 1, true)) {
				// Hacking
				return;
			}
			used = true;
			break;
		}
		case Items::ApReset: {
			int32_t tostat = packet.get<int32_t>();
			int32_t fromstat = packet.get<int32_t>();
			Levels::addStat(player, tostat, 1, true);
			Levels::addStat(player, fromstat, -1, true);
			used = true;
			break;
		}
		case Items::Megaphone: {
			string msg = player->getName() + " : " + packet.getString();
			InventoryPacket::showMegaphone(player, msg);
			used = true;
			break;
		}
		case Items::SuperMegaphone: {
			string msg = player->getName() + " : " + packet.getString();
			uint8_t whisper = packet.get<int8_t>();
			InventoryPacket::showSuperMegaphone(player, msg, whisper);
			used = true;
			break;
		}
		case Items::DiabloMessenger:
		case Items::Cloud9Messenger:
		case Items::LoveholicMessenger: {
			string msg = packet.getString();
			string msg2 = packet.getString();
			string msg3 = packet.getString();
			string msg4 = packet.getString();

			InventoryPacket::showMessenger(player, msg, msg2, msg3, msg4, packet.getBuffer(), packet.getBufferLength(), itemid);
			used = true;
			break;
		}
		case Items::PetNameTag: {
			string name = packet.getString();
			Pets::changeName(player, name);
			used = true;
			break;
		}
		case Items::ItemNameTag: {
			int16_t slot = packet.get<int16_t>();
			if (slot != 0) {
				Item *item = player->getInventory()->getItem(Inventories::EquipInventory, slot);
				if (item == 0) {
					// Hacking or failure, dunno
					return;
				}
				item->name = player->getName();
				InventoryPacket::addNewItem(player, Inventories::EquipInventory, slot, item, true);
				used = true;
			}
			break;
		}
		case Items::ItemLock: {
			int8_t inventory = (int8_t) packet.get<int32_t>();
			int16_t slot = (int16_t) packet.get<int32_t>();
			if (slot != 0) {
				Item *item = player->getInventory()->getItem(inventory, slot);
				if (item == 0) {
					// Hacking or failure, dunno
					return;
				}
				item->flags |= FlagLock;
				InventoryPacket::addNewItem(player, inventory, slot, item, true);
				used = true;
			}
			break;
		}
		case Items::MapleTvMessenger: {
			bool hasreceiver = (packet.get<int8_t>() == 3);
			Player *receiver = Players::Instance()->getPlayer(packet.getString());
			int32_t time = 15;
			if ((hasreceiver && receiver != 0) || (!hasreceiver && receiver == 0)) {
				string msg = packet.getString();
				string msg2 = packet.getString();
				string msg3 = packet.getString();
				string msg4 = packet.getString();
				string msg5 = packet.getString();
				int32_t ticks = packet.get<int32_t>();
				MapleTVs::Instance()->addMessage(player, receiver, msg, msg2, msg3, msg4, msg5, itemid, time);
				used = true;
			}
			break;
		}
		case Items::MapleTvStarMessenger: {
			int32_t time = 30;
			string msg = packet.getString();
			string msg2 = packet.getString();
			string msg3 = packet.getString();
			string msg4 = packet.getString();
			string msg5 = packet.getString();
			int32_t ticks = packet.get<int32_t>();
			MapleTVs::Instance()->addMessage(player, 0, msg, msg2, msg3, msg4, msg5, itemid, time);
			used = true;
			break;
		}
		case Items::MapleTvHeartMessenger: {
			string name = packet.getString();
			Player *receiver = Players::Instance()->getPlayer(name);
			int32_t time = 60;
			if (receiver != 0) {
				string msg = packet.getString();
				string msg2 = packet.getString();
				string msg3 = packet.getString();
				string msg4 = packet.getString();
				string msg5 = packet.getString();
				int32_t ticks = packet.get<int32_t>();
				MapleTVs::Instance()->addMessage(player, receiver, msg, msg2, msg3, msg4, msg5, itemid, time);
				used = true;
			}
			break;
		}
		case Items::FungusScroll:
		case Items::OinkerDelight:
		case Items::ZetaNightmare:
			Inventory::useItem(player, itemid);
			used = true;
			break;
		default:
			break;
	}
	if (used) {
		Inventory::takeItem(player, itemid, 1);
	}
	else {
		InventoryPacket::blankUpdate(player);
	}
}

void InventoryHandler::useItemEffect(Player *player, PacketReader &packet) {
	int32_t itemid = packet.get<int32_t>();
	if (player->getInventory()->getItemAmount(itemid) == 0) {
		// hacking
		return;
	}
	player->setItemEffect(itemid);
	InventoryPacket::useItemEffect(player, itemid);
}

void InventoryHandler::handleRockFunctions(Player *player, PacketReader &packet) {
	uint8_t mode = packet.get<int8_t>();
	uint8_t type = packet.get<int8_t>();
	if (mode == 0) { // Remove
		int32_t map = packet.get<int32_t>();
		player->getInventory()->delRockMap(map, type);
	}
	else if (mode == 1) { // Add
		int32_t map = player->getMap();
		player->getInventory()->addRockMap(map, type);
	}
}

bool InventoryHandler::handleRockTeleport(Player *player, int8_t type, int32_t itemid, PacketReader &packet) {
	bool used = false;
	uint8_t mode = packet.get<uint8_t>();
	int32_t targetmapid = -1;
	if (mode == 0) { // Preset map
		targetmapid = packet.get<int32_t>();
		if (!player->getInventory()->ensureRockDestination(targetmapid)) {
			// Hacking
			return false;
		}
	}
	else if (mode == 1) { // IGN
		string tname = packet.getString();
		Player *target = Players::Instance()->getPlayer(tname);
		if (target != 0 && target != player) {
			targetmapid = target->getMap();
		}
		else if (target == 0) {
			InventoryPacket::sendRockError(player, 0x06, type);
		}
		else if (target == player) {
			// Hacking
			return false;
		}
	}
	if (targetmapid != -1) {
		MapInfoPtr destination = Maps::getMap(targetmapid)->getInfo();
		MapInfoPtr origin = Maps::getMap(player->getMap())->getInfo();
		if ((destination->fieldLimit & FieldLimitBits::VipRock) != 0) {
			InventoryPacket::sendRockError(player, 0x08, type);
		}
		else if ((origin->fieldLimit & FieldLimitBits::VipRock) != 0) {
			InventoryPacket::sendRockError(player, 0x08, type);
		}
		else if (type == 0 && destination->continent != origin->continent) {
			InventoryPacket::sendRockError(player, 0x08, type);
		}
		else {
			player->setMap(targetmapid);
			used = true;
		}
	}
	if (itemid == Items::SpecialTeleportRock) {
		if (used) {
			Inventory::takeItem(player, itemid, 1);
		}
		else {
			InventoryPacket::blankUpdate(player);
		}
	}
	return used;
}