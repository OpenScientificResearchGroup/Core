/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "defCoreApi.hpp"

#include <string>

namespace core
{
	CORE_API void initializeCore(
		const std::string& appName,
		const std::string& confPath,
		const std::string& coreConfPath,
		const std::string& corePluginsDir,
		const std::string& userPluginsDir
	);

	CORE_API void shutdownCore();
}