/*
Copyright (C) 2008 Vana Development Team

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

#include "TradesPacket.h"
#include "Inventory.h"
#include "PacketCreator.h"
#include "Player.h"
#include "PlayerPacketHelper.h"
#include "SendHeader.h"

void TradesPacket::sendOpenTrade(Player *player, const vector<Player *> &players, const vector<unsigned char> &pos) {
	PacketCreator packet;
	packet.addShort(SEND_SHOP_ACTION);
	packet.addByte(0x05);
	packet.addByte(0x03);
	packet.addByte(0x02);
	packet.addShort(players.size() - 1);
	for (unsigned char c = 0; c < players.size(); c++) { // lol	
		packet.addByte(players[c]->getGender());
		packet.addByte(players[c]->getSkin());
		packet.addInt(players[c]->getEyes());
		packet.addByte(0);
		packet.addInt(players[c]->getHair());
		int equips[35][2] = {0};
		iteminventory *playerequips = players[c]->inv->getItems(1);
		for (iteminventory::iterator iter = playerequips->begin(); iter != playerequips->end(); iter++) { //sort equips
			Item *equip = iter->second;
			if (iter->first < 0) {
				if (equips[equip->type][0] > 0) {
					if (Inventory::isCash(equip->id)) {
						equips[equip->type][1] = equips[equip->type][0];
						equips[equip->type][0] = equip->id;
					}
					else {
						equips[equip->type][1] = equip->id;
					}
				}
				else {
					equips[equip->type][0] = equip->id;
				}
			}
		}
		for (int i = 0; i < 35; i++) { //shown items
			if (equips[i][0] > 0) {
				packet.addByte(i);
				if (i == 11 && equips[i][1] > 0) // normal weapons always here
					packet.addInt(equips[i][1]);
				else
					packet.addInt(equips[i][0]);
			}
		}
		packet.addByte(-1);
		for (int i = 0; i < 35; i++) { //covered items
			if (equips[i][1] > 0 && i != 11) {
				packet.addByte(i);
				packet.addInt(equips[i][1]);
			}
		}
		packet.addByte(-1);
		if (equips[11][1] > 0) // cs weapon
			packet.addInt(equips[11][0]);
		else
			packet.addInt(0);
		packet.addInt(0);
		packet.addInt(0);
		packet.addInt(0);
		packet.addString(players[c]->getName());
		packet.addByte(pos[c]); // Location in the window
	}
	packet.send(player);
}

void TradesPacket::sendTradeRequest(Player *player, Player *receiver, int tradeid) {
	PacketCreator packet;
	packet.addShort(SEND_SHOP_ACTION);
	packet.addByte(0x02);
	packet.addByte(0x03);
	packet.addString(player->getName());
	packet.addInt(tradeid);
	packet.send(receiver);
}

void TradesPacket::sendTradeMessage(Player *player, Player *receiver, char type, char message) {
	PacketCreator packet;
	packet.addShort(SEND_SHOP_ACTION);
	packet.addByte(type);
	packet.addByte(message);
	packet.addString(player->getName());
	packet.send(receiver);
}

void TradesPacket::sendTradeMessage(Player *receiver, char type, char message) {
	PacketCreator packet;
	packet.addShort(SEND_SHOP_ACTION);
	packet.addByte(type);
	packet.addByte(0x00);
	packet.addByte(message);
	packet.send(receiver);
}

void TradesPacket::sendTradeChat(Player *player, unsigned char blue, string chat) {
	PacketCreator packet;
	packet.addShort(SEND_SHOP_ACTION);
	packet.addByte(0x06);
	packet.addByte(0x08);
	packet.addByte(blue);
	packet.addString(chat);
	packet.send(player);
}

void TradesPacket::sendAddUser(Player *original, Player *newb, char slot) {
	PacketCreator packet;
	packet.addShort(SEND_SHOP_ACTION);
	packet.addByte(0x04);
	packet.addByte(slot);
	packet.addByte(newb->getGender());
	packet.addByte(newb->getSkin());
	packet.addInt(newb->getEyes());
	packet.addByte(0);
	packet.addInt(newb->getHair());
	int equips[35][2] = {0};
	iteminventory *playerequips = newb->inv->getItems(1);
	for (iteminventory::iterator iter = playerequips->begin(); iter != playerequips->end(); iter++) { //sort equips
		Item *equip = iter->second;
		if (iter->first < 0) {
			if (equips[equip->type][0] > 0) {
				if (Inventory::isCash(equip->id)) {
					equips[equip->type][1] = equips[equip->type][0];
					equips[equip->type][0] = equip->id;
				}
				else {
					equips[equip->type][1] = equip->id;
				}
			}
			else {
				equips[equip->type][0] = equip->id;
			}
		}
	}
	for (int i = 0; i < 35; i++) { //shown items
		if (equips[i][0] > 0) {
			packet.addByte(i);
			if (i == 11 && equips[i][1] > 0) // normal weapons always here
				packet.addInt(equips[i][1]);
			else
				packet.addInt(equips[i][0]);
		}
	}
	packet.addByte(-1);
	for (int i = 0; i < 35; i++) { //covered items
		if (equips[i][1] > 0 && i != 11) {
			packet.addByte(i);
			packet.addInt(equips[i][1]);
		}
	}
	packet.addByte(-1);
	if (equips[11][1] > 0) // cs weapon
		packet.addInt(equips[11][0]);
	else
		packet.addInt(0);
	packet.addInt(0);
	packet.addInt(0);
	packet.addInt(0);
	packet.addString(newb->getName());
	packet.send(original);
}

void TradesPacket::sendLeaveTrade(Player *player) {
	PacketCreator packet;
	packet.addShort(SEND_SHOP_ACTION);
	packet.addByte(0x0A);
	packet.addByte(0x01); // Slot, doesn't matter for trades
	packet.addByte(0x02); // Message, doesn't matter for trades
	packet.send(player);
}

void TradesPacket::sendAddMesos(Player *receiver, unsigned char slot, int amount) {
	PacketCreator packet;
	packet.addShort(SEND_SHOP_ACTION);
	packet.addByte(0x0F);
	packet.addByte(slot);
	packet.addInt(amount);
	packet.send(receiver);
}

void TradesPacket::sendAccepted(Player *destination) {
	PacketCreator packet;
	packet.addShort(SEND_SHOP_ACTION);
	packet.addByte(0x10);
	packet.send(destination);
}

void TradesPacket::sendEndTrade(Player *destination, unsigned char message) {
	PacketCreator packet;
	packet.addShort(SEND_SHOP_ACTION);
	packet.addByte(0x0A);
	packet.addByte(0x00);
	packet.addByte(message);
	// Message:
	//			0x06 = success [tax is automated]
	//			0x07 = unsuccessful
	//			0x08 = "You cannot make the trade because there are some items which you cannot carry more than one."
	//			0x09 = "You cannot make the trade because the other person's on a different map."
	packet.send(destination);
}

void TradesPacket::sendAddItem(Player *destination, unsigned char player, char slot, char inventory, Item *item) {
	PacketCreator packet;
	packet.addShort(SEND_SHOP_ACTION);
	packet.addByte(0x0E);
	packet.addByte(player);
	packet.addByte(slot);
	switch (inventory) {
		case 0x01:
			packet.addByte(0x01);
			break;
		default:
			packet.addByte(0x02);
			break;
	}
	packet.addInt(item->id);
	packet.addShort(0);
	packet.addBytes("8005BB46E61702");
	if (inventory == 1) {
		packet.addByte(item->slots);
		packet.addByte(item->scrolls);
		packet.addShort(item->istr);
		packet.addShort(item->idex);
		packet.addShort(item->iint);
		packet.addShort(item->iluk);
		packet.addShort(item->ihp);
		packet.addShort(item->imp);
		packet.addShort(item->iwatk);
		packet.addShort(item->imatk);
		packet.addShort(item->iwdef);
		packet.addShort(item->imdef);
		packet.addShort(item->iacc);
		packet.addShort(item->iavo);
		packet.addShort(item->ihand);
		packet.addShort(item->ijump);
		packet.addShort(item->ispeed);
		packet.addInt(0);
		packet.addInt(0);
		packet.addInt(0);
		packet.addShort(0);
	}
	else {
		packet.addShort(item->amount);
		packet.addInt(0);
	}
	packet.send(destination);
}