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
#pragma once

#ifdef WIN32
# include <winsock2.h> // Prevent conflict with boost::asio
#endif

#include <soci.h>
#include "SociExtensions.h"
#include "Types.h"
#include <memory>
#include <string>

struct DbConfig;

class Database {
public:
	typedef soci::session * tsConn;

	static void connectCharDb();
	static void connectDataDb();
	static soci::session & getCharDb();
	static soci::session & getDataDb();
	template <typename T> static T getLastId(soci::session &sql);
private:
	static soci::session & getConnection(tsConn &conn, std::function<void()> func);
	thread_local static tsConn m_chardb;
	thread_local static tsConn m_datadb;
	static std::string buildConnectionString(const DbConfig &conf);
};

inline
soci::session & Database::getCharDb() {
	return getConnection(m_chardb, &connectCharDb);
}

inline
soci::session & Database::getDataDb() {
	return getConnection(m_datadb, &connectDataDb);
}

inline
soci::session & Database::getConnection(tsConn &conn, std::function<void()> func) {
	if (conn == nullptr) {
		func();
	}
	/* SOCI
	// This will attempt to re-establish a connection if it's lost, but it costs a query every single time
	// Consider re-architecting the system for better SQL failsafes
	else {
		try {
			int32_t i = 0;
			*conn.get() << "SELECT 1", soci::into(i);
		}
		catch (soci::soci_error) {
			conn->reconnect();
		}
	}
	*/
	return *conn;
}

template<typename T>
T Database::getLastId(soci::session &sql) {
	T val;
	sql.once << "SELECT LAST_INSERT_ID()", soci::into(val);
	return val;
}