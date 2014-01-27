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
#pragma once

#include "Types.hpp"

class Player;
struct PlayerSkillInfo;
struct ChargeOrStationarySkillInfo;

namespace SkillsPacket {
	auto addSkill(Player *player, int32_t skillId, const PlayerSkillInfo &skillInfo) -> void;
	auto showSkill(Player *player, int32_t skillId, uint8_t level, uint8_t direction, bool party = false, bool self = false) -> void;
	auto healHp(Player *player, int16_t hp) -> void;
	auto showSkillEffect(Player *player, int32_t skillId, uint8_t level = 0) -> void;
	auto showChargeOrStationarySkill(Player *player, const ChargeOrStationarySkillInfo &info) -> void;
	auto endChargeOrStationarySkill(Player *player, const ChargeOrStationarySkillInfo &info) -> void;
	auto showMagnetSuccess(Player *player, int32_t mapMobId, uint8_t success) -> void;
	auto sendCooldown(Player *player, int32_t skillId, int16_t time) -> void;
	auto showBerserk(Player *player, uint8_t level, bool on) -> void;
}