/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once

#ifdef CORE_EXPORTS
#define CORE_API __declspec(dllexport)
#else
#define CORE_API __declspec(dllimport)
#endif
