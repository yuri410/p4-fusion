/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#include "timer.h"

const std::chrono::high_resolution_clock Timer2::s_Clock;

Timer2::Timer2()
    : m_StartTime(Now())
{
}
