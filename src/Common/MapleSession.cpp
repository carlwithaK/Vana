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
#include "MapleSession.h"
#include "AbstractPlayer.h"
#include "Decoder.h"
#include "IpUtilities.h"
#include "PacketCreator.h"
#include "PacketReader.h"
#include "SessionManager.h"
#include <boost/bind.hpp>

MapleSession::MapleSession(boost::asio::io_service &io_service,
						   SessionManagerPtr sessionManager,
						   AbstractPlayer *player, bool isServer,
						   string connectPacketUnknown) :
AbstractSession(sessionManager),
m_socket(io_service),
m_player(player),
m_is_server(isServer),
m_connect_packet_unknown(connectPacketUnknown)
{
}

void MapleSession::start() {
	m_session_manager->start(shared_from_this());
}

void MapleSession::handle_start() {
	m_player->setSession(this);
	m_player->setIp(m_socket.remote_endpoint().address().to_v4().to_ulong());

	if (m_is_server) {
		std::cout << "Accepted connection from " << IpUtilities::ipToString(m_player->getIp()) << std::endl;

		PacketCreator connectPacket = m_decoder.getConnectPacket(m_connect_packet_unknown);
		send(connectPacket, false);
	}

	start_read_header();
}

void MapleSession::stop() {
	m_session_manager->stop(shared_from_this());
}

void MapleSession::disconnect() {
	stop();
}

void MapleSession::handle_stop() {
	boost::system::error_code ec;
	m_socket.close(ec);
	if (ec) {
		std::cout << "FAILURE TO CLOSE SESSION (" << ec.value() << "): " << ec.message() << std::endl;
	}
}

void MapleSession::send(const unsigned char *buf, int32_t len, bool encrypt) {
	boost::mutex::scoped_lock l(m_send_mutex);
	size_t realLength = encrypt ? len + headerLen : len;
	unsigned char *buffer = new unsigned char[realLength];

	if (encrypt) {
		memcpy(buffer + headerLen, buf, len);

		// Encrypt packet
		m_decoder.createHeader(buffer, (int16_t) len);
		m_decoder.encrypt(buffer + headerLen, len);
		m_decoder.next();
	}
	else {
		memcpy(buffer, buf, len);
	}

	bool isWriting = !m_send_packet_queue.empty();

	m_send_packet_queue.push(shared_array<unsigned char>(buffer));
	m_send_size_queue.push(realLength);

	if (!isWriting) { // No write operation active
		send_next_packet();
	}
}

void MapleSession::send(const PacketCreator &packet, bool encrypt) {
	return send(packet.getBuffer(), packet.getSize(), encrypt);
}

void MapleSession::start_read_header() {
	m_buffer.reset(new unsigned char[headerLen]);

	boost::asio::async_read(m_socket,
		boost::asio::buffer(m_buffer.get(), headerLen),
		boost::bind(
			&MapleSession::handle_read_header, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void MapleSession::handle_write(const boost::system::error_code &error,
								size_t bytes_transferred) {
	boost::mutex::scoped_lock l(m_send_mutex);
	if (!error) {
		if (!m_send_packet_queue.empty()) { // More packet(s) to send
			send_next_packet();
		}
	}
	else {
		disconnect();
	}
}

void MapleSession::handle_read_header(const boost::system::error_code &error,
									  size_t bytes_transferred) {
	if (!error) {
		size_t len = Decoder::getLength(m_buffer.get());

		if (len < 2 || len > maxBufferLen) {
			// Hacking or trying to crash server
			disconnect();
		}

		m_buffer.reset(new unsigned char[len]);

		boost::asio::async_read(m_socket,
			boost::asio::buffer(m_buffer.get(), len),
			boost::bind(
				&MapleSession::handle_read_body, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
	else {
		disconnect();
	}
}

void MapleSession::handle_read_body(const boost::system::error_code &error,
									size_t bytes_transferred) {
	if (!error) {
		m_decoder.decrypt(m_buffer.get(), bytes_transferred);

		PacketReader packet(m_buffer.get(), bytes_transferred);
		m_player->handleRequest(packet);

		start_read_header();
	}
	else {
		disconnect();
	}
}

void MapleSession::send_next_packet() {
	m_send_packet = m_send_packet_queue.front();
	m_send_packet_queue.pop();
	uint32_t len = m_send_size_queue.front();
	m_send_size_queue.pop();

	boost::asio::async_write(m_socket, boost::asio::buffer(m_send_packet.get(), len),
		boost::bind(&MapleSession::handle_write, shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}
