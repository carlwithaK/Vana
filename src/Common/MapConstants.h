/*
Copyright (C) 2008-2011 Vana Development Team

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

#include "Types.h"

namespace Maps {
	enum {
		AdventurerStartMap = 0,
		NoblesseStartMap = 130030000,
		GmMap = 180000000,
		OriginOfClockTower = 220080001,
		SorcerersRoom = 270020211,
		LegendStartMap = 914000000,
		NoMap = 999999999
	};
}

namespace PlayerNpcs {
	const int8_t MaxPlayerNpcsAdventurer = 20;
	const int8_t MaxPlayerNpcsKoc = 52; // 3 Floors!
	enum {
		WarriorRankNpcStart = 9901000,
		MagicianRankNpcStart = 9901100,
		BowmanRankNpcStart = 9901200,
		ThiefRankNpcStart = 9901300,
		// Pirates doesn't have Player NPC's
		KoCRankNpcStart = 9901500
	};
}