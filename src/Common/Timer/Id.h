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
#ifndef TIMER_ID
#define TIMER_ID

#include "Types.h"

namespace Timer {

struct Id {
	Id(unsigned int type, unsigned int id, unsigned int id2);
	unsigned int type;
	unsigned int id;
	unsigned int id2;

	bool operator==(Id const &other) const;
	friend size_t hash_value(Id const &id);
};

inline
Id::Id(unsigned int type, unsigned int id, unsigned int id2) :
type(type),
id(id),
id2(id2)
{
}

inline
bool Id::operator==(Id const &other) const {
	return type == other.type && id == other.id && id2 == other.id2;
}

}
#endif