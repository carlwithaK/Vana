/*
Copyright (C) 2008-2012 Vana Development Team

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
#include "StringUtilities.h"
#include "Database.h"
#include "tokenizer.hpp"

int32_t StringUtilities::noCaseCompare(const string &s1, const string &s2) {
	string::const_iterator iter1 = s1.begin();
	string::const_iterator iter2 = s2.begin();

	while ((iter1 != s1.end()) && (iter2 != s2.end())) {
		if (toupper(*iter1) != toupper(*iter2)) {
			return (toupper(*iter1) < toupper(*iter2)) ? -1 : 1;
		}
		++iter1;
		++iter2;
	}

	// The letters are the same, so let's return based on size
	size_t l1 = s1.size(), l2 = s2.size();
	if (l1 == l2) {
		return 0;
	}
	return (l1 < l2) ? -1 : 1;
}

void StringUtilities::runFlags(const opt_string &flags, function<void (const string &)> func) {
	if (flags.is_initialized()) {
		runFlags(flags.get(), func);
	}
}

void StringUtilities::runFlags(const string &flags, function<void (const string &)> func) {
	if (flags.length() > 0) {
		std::tokenizer tokens(flags, ",");
		for (std::tokenizer::iterator iter = tokens.begin(); iter != tokens.end(); ++iter) {
			func(*iter);
		}
	}
}

int64_t StringUtilities::atoli(const char *str) {
	int64_t result = 0;
	while (*str >= '0' && *str <= '9') {
		result = (result * 10) + (*str++ - '0');
	}
	return result;
}

string StringUtilities::replace(const string &input, const string &what, const string &replacement) {
	string ret = input;
	size_t searchLen = what.length();
	size_t foundPos = ret.find(what);
	while (foundPos != string::npos) {
		ret.replace(foundPos, searchLen, replacement);
		foundPos = ret.find(what); // Search the next one
	}
	return ret;
}

string StringUtilities::bytesToHex(const unsigned char *input, size_t inputSize, bool uppercase) {
	string ret;
	if (inputSize > 0) {
		std::ostringstream out;
		size_t bufLen = inputSize - 1;

		for (size_t i = 0; i <= bufLen; i++) {
			out << std::hex;
			if (uppercase) {
				out << std::uppercase;
			}
			else {
				out << std::nouppercase;
			}
			out << std::setw(2) << std::setfill('0') << static_cast<int16_t>(input[i]);
			if (i < bufLen) {
				out << " ";
			}
		}

		ret = out.str();
	}
	return ret;
}