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
#include "SkillsPacket.h"
#include "Player.h"
#include "PacketCreator.h"
#include "Skills.h"
#include "SendHeader.h"

void SkillsPacket::addSkill(Player *player, int skillid, int level, int maxlevel) {
	Packet packet;
	packet.addHeader(SEND_ADD_SKILL);
	packet.addByte(1);
	packet.addShort(1);
	packet.addInt(skillid);
	packet.addInt(level); // Level
	packet.addInt(maxlevel); // Master Level
	packet.addByte(1);
	packet.send(player);
	packet.send(player);
}

void SkillsPacket::showSkill(Player *player, vector <Player*> players, int skillid, unsigned char level, unsigned char direction) {
 	Packet packet;
 	packet.addHeader(SEND_SHOW_SKILL);
 	packet.addInt(player->getPlayerid());
 	packet.addByte(1);
 	packet.addInt(skillid);
	packet.addByte(level); //TODO
	if (direction != 0xFF)
		packet.addByte(direction);
	packet.sendTo(player, players, false);
}

void SkillsPacket::useSkill(Player *player, vector <Player*> players, int skillid, int time, SkillActiveInfo pskill, SkillActiveInfo mskill) {
	Packet packet;
	packet.addHeader(SEND_USE_SKILL);
	packet.addInt64(0);
	packet.addByte(pskill.types[0]);
	packet.addByte(pskill.types[1]);
	packet.addByte(pskill.types[2]);
	packet.addByte(pskill.types[3]);
	packet.addByte(pskill.types[4]);
	packet.addByte(pskill.types[5]);
	packet.addByte(pskill.types[6]);
	packet.addByte(pskill.types[7]);
	for (size_t i = 0; i < pskill.vals.size(); i++) {
		packet.addShort(pskill.vals[i]);
		packet.addInt(skillid);
		packet.addInt(time);
	}
	packet.addShort(0);
	packet.addByte(0);
	packet.addShort(0);
	packet.addByte(0);
	packet.send(player);
	if (mskill.vals.size() > 0) {
		packet = Packet();
		packet.addHeader(SEND_SHOW_OTHERS_SKILL);
		packet.addInt(player->getPlayerid());
		packet.addInt64(0);
		packet.addByte(mskill.types[0]);
		packet.addByte(mskill.types[1]);
		packet.addByte(mskill.types[2]);
		packet.addByte(mskill.types[3]);
		packet.addByte(mskill.types[4]);
		packet.addByte(mskill.types[5]);
		packet.addByte(mskill.types[6]);
		packet.addByte(mskill.types[7]);
		for (size_t i = 0; i < mskill.vals.size(); i++) {
			packet.addShort(mskill.vals[i]);
		}
		packet.addByte(0);
		packet.sendTo(player, players, false);
	}
}

void SkillsPacket::healHP(Player *player, short hp) {
	Packet packet;
	packet.addHeader(SEND_GAIN_ITEM);
	packet.addByte(0xA);
	packet.addShort(hp);
	packet.send(player);
}

void SkillsPacket::endSkill(Player *player, vector <Player*> players, SkillActiveInfo pskill, SkillActiveInfo mskill) {
	Packet packet;
	packet.addHeader(SEND_CANCEL_SKILL);
	packet.addInt64(0);
	packet.addByte(pskill.types[0]);
	packet.addByte(pskill.types[1]);
	packet.addByte(pskill.types[2]);
	packet.addByte(pskill.types[3]);
	packet.addByte(pskill.types[4]);
	packet.addByte(pskill.types[5]);
	packet.addByte(pskill.types[6]);
	packet.addByte(pskill.types[7]);
	packet.addByte(0);
	packet.send(player);
	if (mskill.vals.size() > 0) {
		packet = Packet();
		packet.addHeader(SEND_CANCEL_OTHERS_BUFF);
		packet.addInt(player->getPlayerid());
		packet.addInt64(0);
		packet.addByte(mskill.types[0]);
		packet.addByte(mskill.types[1]);
		packet.addByte(mskill.types[2]);
		packet.addByte(mskill.types[3]);
		packet.addByte(mskill.types[4]);
		packet.addByte(mskill.types[5]);
		packet.addByte(mskill.types[6]);
		packet.addByte(mskill.types[7]);
		packet.sendTo(player, players, false);
	}
}
// For Combo Attack
void SkillsPacket::showCombo(Player *player, vector <Player*> players, int time) { // Show combos to everyone on map 
	Packet packet;
	packet.addHeader(SEND_USE_SKILL);
	packet.addByte(0);
	packet.addByte(0);
	packet.addByte((char)0x20);
	packet.addByte(0);
	packet.addByte(0);
	packet.addByte(0);
	packet.addByte(0);
	packet.addByte(0);
	packet.addShort(player->getCombo()+1);
	packet.addInt(1111002);
	packet.addInt(time);
	packet.addShort(0);
	packet.addByte(0);
	packet.send(player);
	packet = Packet();
	packet.addHeader(SEND_SHOW_OTHERS_SKILL);
	packet.addInt(player->getPlayerid());
	packet.addByte(0);
	packet.addByte(0);
	packet.addByte((char)0x20);
	packet.addByte(0);
	packet.addByte(0);
	packet.addByte(0);
	packet.addByte(0);
	packet.addByte(0);
	packet.addShort(player->getCombo()+1);
	packet.addByte(0);
	packet.sendTo(player, players, false);
}

void SkillsPacket::showSkillEffect(Player *player, vector <Player*> players,int skillid) {
	Packet packet;
	packet.addHeader(SEND_GAIN_ITEM); // For the player themselves
	bool sendto = false;
	switch (skillid) { 
		case 2100000:
		case 2200000:
		case 2300000: // MP Eater
			packet.addByte(1);
			packet.addInt(skillid);
			packet.addByte(1);
			sendto = true;
			break;
	}
	if (sendto)
		packet.send(player);
	packet = Packet();
	sendto = false;
	packet.addHeader(SEND_SHOW_SKILL);  // For others
	packet.addInt(player->getPlayerid());
	switch (skillid) {
		case 2100000:
		case 2200000:
		case 2300000: // MP Eater
			packet.addByte(1);
			packet.addInt(skillid);
			packet.addByte(1);
			sendto = true;
			break;
		case 4211005: // Meso Guard
			packet.addByte(5);
			packet.addInt(skillid);
			sendto = true;
	}
	if (sendto)
		packet.sendTo(player, players, false);
}

void SkillsPacket::showSpecialSkill(Player *player, vector <Player*> players, SpecialSkillInfo info) { // Hurricane, Pierce, Big Bang
	Packet packet;
	packet.addHeader(SEND_SPECIAL_SKILL);
	packet.addInt(player->getPlayerid());
	packet.addInt(info.skillid);
	packet.addByte(info.level);
	packet.addByte(info.direction);
	packet.addByte(info.w_speed);
	packet.sendTo(player, players, false);
}

void SkillsPacket::endSpecialSkill(Player *player, vector <Player*> players, SpecialSkillInfo info) {
	Packet packet;
	packet.addHeader(SEND_SPECIAL_SKILL_END);
	packet.addInt(player->getPlayerid());
	packet.addInt(info.skillid);
	packet.sendTo(player, players, false);
}

void SkillsPacket::showMagnet(Player *player, vector <Player*> players, int mobid, unsigned char success) {  // Monster Magnet
	Packet packet;
	packet.addHeader(SEND_SHOW_MAGNET);
	packet.addInt(mobid);
	packet.addByte(success);
	packet.sendTo(player, players, false);
}
