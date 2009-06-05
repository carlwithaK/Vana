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
#ifndef PACKETCREATOR_H
#define PACKETCREATOR_H

#include "Types.h"
#include <cstring>
#include <string>
#include <boost/shared_array.hpp>

using std::string;
using boost::shared_array;

class PacketReader;
struct Pos;

class PacketCreator {
public:
	PacketCreator();

	template <typename T>
	void add(T value);
	template <typename T>
	void set(T value, size_t pos);
	void addString(const string &str); // Dynamically-lengthed strings
	void addString(const string &str, size_t len); // Static-lengthed strings
	void addPos(Pos pos); // Positions
	void addBytes(const char *hex);
	void addBuffer(const unsigned char *bytes, size_t len);
	void addBuffer(PacketCreator &packet);
	void addBuffer(PacketReader &packet);

	const unsigned char * getBuffer() const;
	size_t getSize() const;
private:
	static const size_t bufferLen = 1000; // Initial buffer length

	unsigned char * getBuffer(size_t pos, size_t len);

	size_t pos;
	shared_array<unsigned char> packet;
	size_t packetCapacity;
};

template <typename T>
void PacketCreator::add(T value) {
	(*(T *) getBuffer(pos, sizeof(T))) = value;
	pos += sizeof(T);
}

template <typename T>
void PacketCreator::set(T value, size_t pos) {
	(*(T *) getBuffer(pos, sizeof(T))) = value;
}

inline
const unsigned char * PacketCreator::getBuffer() const {
	return packet.get();
}

inline
void PacketCreator::addBuffer(const unsigned char *bytes, size_t len) {
	memcpy(getBuffer(pos, len), bytes, len);
	pos += len;
}

inline
size_t PacketCreator::getSize() const {
	return pos;
}

#endif
