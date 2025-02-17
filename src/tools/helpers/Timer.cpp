/*
Copyright (c) 2020 Cedric Jimenez
This file is part of OpenOCPP.

OpenOCPP is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenOCPP is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with OpenOCPP. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Timer.h"
#include "TimerPool.h"

namespace ocpp
{
namespace helpers
{

/** @brief Constructor */
Timer::Timer(TimerPool& pool, const char* name)
    : m_pool(pool),
      m_name(name),
      m_single_shot(false),
      m_interval(std::chrono::milliseconds(0)),
      m_wake_up_time_point(std::chrono::time_point<std::chrono::system_clock>::min()),
      m_started(false),
      m_callback()
{
}

/** @brief Copy constructor */
Timer::Timer(const Timer& timer)
    : m_pool(timer.m_pool),
      m_name(timer.m_name),
      m_single_shot(timer.m_single_shot),
      m_interval(timer.m_interval),
      m_wake_up_time_point(timer.m_wake_up_time_point),
      m_started(timer.m_started),
      m_callback(timer.m_callback)
{
}

/** @brief Destructor */
Timer::~Timer()
{
    stop();
}

/** @brief Start the timer with the specified interval */
bool Timer::start(std::chrono::milliseconds interval, bool single_shot)
{
    bool ret = false;

    // Lock timers
    m_pool.lock();

    // Check if the timer is already started
    if (!m_started)
    {
        // Configure timer
        m_interval           = interval;
        m_single_shot        = single_shot;
        m_wake_up_time_point = std::chrono::system_clock::now() + m_interval;

        // Add timer to the list
        m_pool.addTimer(this);

        ret = true;
    }

    // Unlock timers
    m_pool.unlock();

    return ret;
}

/** @brief Restart the timer with the specified interval */
bool Timer::restart(std::chrono::milliseconds interval, bool single_shot)
{
    bool ret = false;

    // Lock timers
    m_pool.lock();

    // Check if the timer is already started
    if (m_started)
    {
        // Remove timer from the list
        m_pool.removeTimer(this);
    }

    // Configure timer
    m_interval           = interval;
    m_single_shot        = single_shot;
    m_wake_up_time_point = std::chrono::system_clock::now() + m_interval;

    // Add timer to the list
    m_pool.addTimer(this);

    ret = true;

    // Unlock timers
    m_pool.unlock();

    return ret;
}

/** @brief Stop the timer */
bool Timer::stop()
{
    bool ret = false;

    // Lock timers
    m_pool.lock();

    // Check if the timer is started
    if (m_started)
    {
        // Remove timer from the list
        m_pool.removeTimer(this);

        ret = true;
    }

    // Unlock timers
    m_pool.unlock();

    return ret;
}

/** @brief Indicate if the timer is started */
bool Timer::isStarted() const
{
    return m_started;
}

/** @brief Set the timer's callback */
void Timer::setCallback(std::function<void()> callback)
{
    // Lock timers
    m_pool.lock();

    // Save callback
    m_callback = callback;

    // Unlock timers
    m_pool.unlock();
}

/** @brief Get the timer's interval */
std::chrono::milliseconds Timer::getInterval() const
{
    return m_interval;
}

} // namespace helpers
} // namespace ocpp
