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
#include "MapleClient.h"
#include "AbstractPlayer.h"
#include "PacketReader.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

MapleClient::MapleClient(boost::asio::io_service &io_service,
		uint32_t server, uint16_t port,
		SessionManagerPtr sessionManager,
		AbstractPlayer *player) :
MapleSession(io_service, sessionManager, player, false),
m_server(server),
m_port(port),
m_resolver(io_service)
{
}

void MapleClient::start_connect() {
	// Synchronously connect and process the connect packet

	tcp::endpoint endpoint(boost::asio::ip::address_v4(m_server), m_port);
    boost::system::error_code error;
	m_socket.connect(endpoint, error);

	if (!error) {
		// Now let's process the connect packet
		readConnectPacket();

		// Start the normal MapleSession routine
		start();
	}
	else {
		std::cout << "Error: " << error.message() << std::endl;
	}
}

void MapleClient::readConnectPacket() {
	boost::system::error_code error;

	m_buffer.reset(new unsigned char[connectHeaderLen]);

	// Get the size of the connect packet
	boost::asio::read(m_socket,
		boost::asio::buffer(m_buffer.get(), connectHeaderLen),
		boost::asio::transfer_all(), 
		error);

	if (error) {
		disconnect();
		return;
	}

	uint16_t packetLen = m_buffer[0] + m_buffer[1] * 0x100;

	m_buffer.reset(new unsigned char[packetLen]);

	// Get the rest of the packet
	boost::asio::read(m_socket,
		boost::asio::buffer(m_buffer.get(), packetLen),
		boost::asio::transfer_all(),
		error);

	if (error) {
		disconnect();
		return;
	}

	// Now finally process it
	PacketReader packet(m_buffer.get(), packetLen);

	uint16_t version = packet.get<int16_t>(); // Maple Version, TODO: Verify it
	packet.getString(); // Unknown

	unsigned char *rawpacket = packet.getBuffer();
	m_decoder.setIvSend(rawpacket);
	m_decoder.setIvRecv(rawpacket + 4);
}
