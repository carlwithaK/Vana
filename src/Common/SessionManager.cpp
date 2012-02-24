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
#include "SessionManager.h"
#include "AbstractSession.h"
#include <algorithm>
#include <functional>

void SessionManager::start(AbstractSessionPtr session) {
	m_sessions.insert(session);
	session->handleStart();
}

void SessionManager::stop(AbstractSessionPtr session) {
	m_sessions.erase(session);
	session->handleStop();
}

void SessionManager::stopAll() {
	std::for_each(m_sessions.begin(), m_sessions.end(), std::bind(&AbstractSession::handleStop, std::placeholders::_1));
	m_sessions.clear();
}