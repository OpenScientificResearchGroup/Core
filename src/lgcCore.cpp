/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Core/lgcCore.hpp"

#include <string>

#include "Core/Config/lgcConfigManager.hpp"
#include "Core/Plugin/lgcPluginManager.hpp"
#include "Core/Log/lgcLogManager.hpp"
#include "Core/Task/lgcTaskManager.hpp"
#include "Core/Ui/lgcUiManager.hpp"
#include "Core/Service/lgcServiceManager.hpp"
#include "Core/Data/lgcDataManager.hpp"
#include "Core/Event/lgcEventManager.hpp"
#include "Core/I18n/lgcI18nManager.hpp"
#include "Core/Resource/lgcResourceManager.hpp"

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
		//UiManager::get().init(appName);
		UiManager::get().init();
		PluginManager::get().init(corePluginsDir, userPluginsDir);
	}

	void shutdownCore()
	{
		//PluginManager::get().shutdown();
		UiManager::get().shutdown();
		ResourceManager::get().shutdown();
		I18nManager::get().shutdown();
		EventManager::get().shutdown();
		DataManager::get().shutdown();
		ServiceManager::get().shutdown();
		TaskManager::get().shutdown();
		ConfigManager::get().shutdown();
		PluginManager::get().shutdown(); // must put at here
		LogManager::shutdown();
	}
}
