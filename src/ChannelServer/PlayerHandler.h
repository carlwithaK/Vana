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
#ifndef PLAYERHANDLER_H
#define PLAYERHANDLER_H

#include "Types.h"

class PacketReader;
class Player;
struct MpEaterInfo;

namespace PlayerHandler {
	void handleDamage(Player *player, PacketReader &packet);
	void handleFacialExpression(Player *player, PacketReader &packet);
	void handleGetInfo(Player *player, PacketReader &packet);
	void handleHeal(Player *player, PacketReader &packet);
	void handleMoving(Player *player, PacketReader &packet);
	void handleSpecialSkills(Player *player, PacketReader &packet);

	void useMeleeAttack(Player *player, PacketReader &packet);
	void useRangedAttack(Player *player, PacketReader &packet);
	void useSpellAttack(Player *player, PacketReader &packet);
	void useEnergyChargeAttack(Player *player, PacketReader &packet);
	void useSummonAttack(Player *player, PacketReader &packet);
	uint32_t damageMobs(Player *player, PacketReader &packet, int8_t targets, int8_t hits, int32_t skillid, int32_t &extra, MpEaterInfo *eater = 0);

};

#endif