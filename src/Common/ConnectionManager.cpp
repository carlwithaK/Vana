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
#include "ConnectionManager.h"
#include "AbstractConnection.h"
#include "ServerClient.h"
#include <algorithm>
#include <boost/bind.hpp>

ConnectionManager * ConnectionManager::singleton = nullptr;

ConnectionManager::ConnectionManager() :
	m_clients(new SessionManager),
	m_work(new boost::asio::io_service::work(m_ioService))
{
}

void ConnectionManager::accept(port_t port, AbstractConnectionFactory *acf, bool encrypted, const string &patchLocation) {
	tcp::endpoint endpoint(tcp::v4(), port);
	m_servers.push_back(ConnectionAcceptorPtr(new ConnectionAcceptor(m_ioService, endpoint, acf, encrypted, patchLocation)));
}

void ConnectionManager::connect(ip_t serverIp, port_t serverPort, AbstractConnection *connection) {
	ServerClientPtr c = ServerClientPtr(new ServerClient(m_ioService, serverIp, serverPort, m_clients, connection));
	c->startConnect();
}

void ConnectionManager::stop() {
	// Post a call to io_service so it is safe to call from all threads
	m_ioService.post(boost::bind(&ConnectionManager::handleStop, this));
}

void ConnectionManager::run() {
	m_thread.reset(new boost::thread(boost::bind(&ConnectionManager::handleRun, this)));
}

void ConnectionManager::join() {
	m_thread->join();
}

void ConnectionManager::handleRun() {
	m_ioService.run();
}

void ConnectionManager::handleStop() {
	std::for_each(m_servers.begin(), m_servers.end(), boost::bind(&ConnectionAcceptor::stop, _1));

	m_clients->stopAll();

	// Destroy the "work" so io_service would return
	m_work.reset();
}