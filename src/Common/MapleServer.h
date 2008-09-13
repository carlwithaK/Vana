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
#ifndef MAPLESERVER_H
#define MAPLESERVER_H

#include "MapleSession.h"
#include "SessionManager.h"
#include <memory>
#include <string>
#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>

using std::string;
using boost::asio::ip::tcp;

class AbstractPlayerFactory;

class MapleServer {
public:
	MapleServer(boost::asio::io_service &io_service,
		const tcp::endpoint &endpoint,
		AbstractPlayerFactory *apf,
		string connectPacketUnknown);
private:
	void start_accept();
	void handle_accept(MapleSessionPtr new_session,
		const boost::system::error_code &error);

	tcp::acceptor m_acceptor;
	boost::scoped_ptr<AbstractPlayerFactory> m_apf;
	SessionManagerPtr m_session_manager;
	string m_connect_packet_unknown;
};

typedef std::tr1::shared_ptr<MapleServer> MapleServerPtr;

#endif