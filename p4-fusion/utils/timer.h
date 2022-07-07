/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#pragma once

#include <chrono>

typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

class Timer2
{
	static const std::chrono::high_resolution_clock s_Clock;

	TimePoint m_StartTime;

public:
	static TimePoint Now() { return s_Clock.now(); }

	Timer2();

	/// Get time spent since construction of this object in seconds
	float GetTimeS() const { return (float)(s_Clock.now() - m_StartTime).count() * 1e-9; }
};
