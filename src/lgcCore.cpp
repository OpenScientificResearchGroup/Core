/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "lgcCore.hpp"

#include <string>

#include "Config/lgcConfigManager.hpp"
#include "Plugin/lgcPluginManager.hpp"
#include "Log/lgcLogManager.hpp"
#include "Task/lgcTaskManager.hpp"
#include "Ui/lgcUiManager.hpp"
#include "Service/lgcServiceManager.hpp"
#include "Data/lgcDataManager.hpp"
#include "Event/lgcEventManager.hpp"
#include "I18n/lgcI18nManager.hpp"
#include "Resource/lgcResourceManager.hpp"

namespace core
{
	void initializeCore(
		const std::string& appName,
		const std::string& confPath,
		const std::string& coreConfPath,
		const std::string& corePluginsDir,
		const std::string& userPluginsDir
	)
	{
		LogManager::init();
		ConfigManager::get().init(confPath, coreConfPath);
		LogManager::init(
			ConfigManager::get().getValue<std::string>("/Core/log/path"),
			ConfigManager::get().getValue<size_t>("/Core/log/max_file_size"),
			ConfigManager::get().getValue<size_t>("/Core/log/max_files"),
			ConfigManager::get().getValue<bool>("/Core/log/console_out")
		);
		TaskManager::get().init(ConfigManager::get().getValue<size_t>("/Core/task/num_threads"));
		ServiceManager::get().init();
		DataManager::get().init();
		EventManager::get().init();
		I18nManager::get().init(ConfigManager::get().getValue<std::string>("/Core/i18n/locale"));
		ResourceManager::get().init();
		UiManager::get().init(appName);
		PluginManager::get().init(corePluginsDir, userPluginsDir);

	}

	void shutdownCore()
	{
		PluginManager::get().shutdown();
		UiManager::get().shutdown();
		ResourceManager::get().shutdown();
		I18nManager::get().shutdown();
		EventManager::get().shutdown();
		DataManager::get().shutdown();
		ServiceManager::get().shutdown();
		TaskManager::get().shutdown();
		LogManager::shutdown();
	}
}
