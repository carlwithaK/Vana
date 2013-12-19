/*
Copyright (C) 2008-2013 Vana Development Team

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
#include "MiscUtilities.h"
#include <botan/pipe.h>
#include <botan/filters.h>

string MiscUtilities::hashPassword(const string &password, const string &salt) {
	const string &salted = salt + password;
	Botan::Pipe pipe(
		new Botan::Chain(
			new Botan::Hash_Filter("SHA-512"),
			new Botan::Hex_Encoder()));

	pipe.process_msg(salted);
	return pipe.read_all_as_string();
}

string MiscUtilities::generateSalt(size_t length) {
	string salt(length, 0);
	for (size_t i = 0; i < length; i++) {
		salt[i] = Randomizer::rand<uint8_t>(126, 33);
	}
	return salt;
}

bool MiscUtilities::isBossChannel(const vector<int8_t> &vec, int8_t channelId) {
	for (const auto &channel : vec) {
		if (channel == channelId) {
			return true;
		}
	}
	return false;
}