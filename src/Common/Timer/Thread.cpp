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
#include "Thread.h"
#include "Container.h"
#include "Timer.h"
#include <functional>

namespace Timer {

using std::tr1::bind;

Thread * Thread::singleton = 0;

Thread::Thread() :
m_resort_timer(false),
m_terminate(false),
m_container(new Container)
{
	m_thread.reset(new boost::thread(bind(&Thread::runThread, this)));
}

Thread::~Thread() {
	m_terminate = true;
}

void Thread::registerTimer(Timer *timer) {
	boost::mutex::scoped_lock l(m_timers_mutex);
	m_resort_timer = true;
	m_timers.push_back(timer);
	m_main_loop_condition.notify_one();
}

void Thread::removeTimer(Timer *timer) {
	boost::mutex::scoped_lock l(m_timers_mutex);
	m_timers.remove(timer);
	m_main_loop_condition.notify_one();
}

Timer * Thread::findMin() {
	// Unsynchronized
	if (m_timers.size() == 0) {
		return 0;
	}

	if (m_resort_timer) {
		struct {
			bool operator()(const Timer *t1, const Timer *t2) {
				return (t1->getRunAt() < t2->getRunAt());
			}
		} less_than;

		m_timers.sort(less_than);
		m_resort_timer = false;
	}

	return *m_timers.begin();
}

void Thread::forceReSort() {
	boost::mutex::scoped_lock l(m_timers_mutex);
	m_resort_timer = true;
	m_main_loop_condition.notify_one();
}

void Thread::runThread() {
	boost::mutex::scoped_lock l(m_timers_mutex);
	while (!m_terminate) {
		// Find minimum wakeup time
		Timer *minThread = findMin();
		clock_t msec = (minThread == 0) ? msec = 1000000000 : minThread->getRunAt() - clock();
		if (msec <= 0) {
			minThread->run();
			continue;
		}
		
		if (m_main_loop_condition.timed_wait(m_timers_mutex,
			boost::get_system_time() + boost::posix_time::milliseconds(msec))) {
				continue;
		}

		if (minThread != 0) {
			l.unlock();
			minThread->run();
			l.lock();
		}
	}
}

}