#include "lgcCore.hpp"

#include <string>

#include "Config/lgcConfigManager.hpp"
#include "Plugin/lgcPluginManager.hpp"
#include "Log/lgcLogManager.hpp"
#include "Task/lgcTaskManager.hpp"
#include "Ui/lgcUiManager.hpp"
#include "Service/lgcServiceManager.hpp"

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
		TaskManager::init(ConfigManager::get().getValue<size_t>("/Core/task/num_threads"));
		UiManager::get().init(appName);
		PluginManager::get().init(corePluginsDir, userPluginsDir);
		ServiceManager::get();
	}

	void shutdownCore()
	{
		ServiceManager::get().shutdownAll();
		PluginManager::get().shutdown();
		UiManager::get().shutdown();
		TaskManager::shutdown();
		LogManager::shutdown();
	}
}
