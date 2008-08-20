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
#ifndef TIMER_TIMER_H
#define TIMER_TIMER_H

#include "Id.h"
#include <ctime>
#include <functional>

namespace Timer {

using std::tr1::function;

class Container;

class Timer {
public:
	Timer(function<void ()> func, Id id, Container *container,
		clock_t length, bool persistent);
	~Timer();

	Id getId() const { return m_id; }
	clock_t getRunAt() const { return m_run_at; }
	int getTimeLeft() const { return m_run_at - clock(); }
	bool getPersistent() const { return m_persistent; }
	void setPersistent(bool val) { m_persistent = val; }

	void run();
	void reset();
private:
	Id m_id;
	Container *m_container;
	clock_t m_run_at; // The time that this timer will run
	clock_t m_length; // Difference between the time the timer is set and the time the timer is ran
	bool m_persistent;
	function<void ()> m_function;
};

}

#endif