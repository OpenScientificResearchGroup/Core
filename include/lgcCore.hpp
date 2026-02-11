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