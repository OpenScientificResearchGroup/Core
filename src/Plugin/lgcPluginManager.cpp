/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Plugin/lgcPluginManager.hpp"
#include "Log/lgcLogManager.hpp"

#include <fstream>
#include <iostream>
#include <set>
#include <algorithm>

// 平台相关头文件
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// Minizip-ng Headers
#include <minizip-ng/mz.h>
#include <minizip-ng/mz_zip.h>
#include <minizip-ng/mz_strm.h>
#include <minizip-ng/mz_zip_rw.h>

namespace core
{
	PluginManager& PluginManager::get()
	{
		static PluginManager instance;
		return instance;
	}

	PluginManager::~PluginManager()
	{
		unloadAll();
	}

	// =========================================================================
	// 1. 初始化 (Init)
	// =========================================================================

	bool PluginManager::init(const std::string& coreDir, const std::string& userDir)
	{
		mCoreDir = coreDir;
		mUserDir = userDir;
		mCoreManifestFile = coreDir + "/core_manifest.json";
		mUserManifestFile = userDir + "/user_manifest.json";

		mMissingCorePlugins.clear();

		// 确保用户插件目录存在
		if (!std::filesystem::exists(mUserDir))
			std::filesystem::create_directories(mUserDir);

		// 1. 加载两份清单
		loadCoreManifest();
		loadUserManifest();

		bool userManifestDirty = false;

		// 2. 完整性检查与状态同步
		for (auto it = mPlugins.begin(); it != mPlugins.end(); )
		{
			auto& plugin = it->second;
			bool isIntact = checkIntegrity(plugin);

			if (!isIntact)
			{
				plugin->state = PluginState::Broken;
				APP_LOG_WARN("[Plugin Manager]: Plugin Integrity Check Failed: {} -> {}", plugin->metadata.id, plugin->metadata.integrityStatus);

				if (plugin->metadata.type == PluginType::Builtin)
					// 核心插件损坏：记录严重错误
					mMissingCorePlugins.push_back(plugin->metadata.id + ": " + plugin->metadata.integrityStatus);
				else if (plugin->metadata.installMethod == InstallMethod::Copy)
				{
					// Copy 模式的用户插件损坏：视为已删除，清理清单
					APP_LOG_WARN("[Plugin Manager]: Removing missing user plugin from manifest: {}", plugin->metadata.id);
					it = mPlugins.erase(it);
					userManifestDirty = true;
					continue;
				}
				// Link 模式保留条目，显示为 Broken
			}
			else
			{
				// 状态重置
				if (!plugin->metadata.isEnabled) plugin->state = PluginState::Disabled;
				else plugin->state = PluginState::Unloaded;
			}
			++it;
		}

		if (userManifestDirty) saveUserManifest();

		// 3. 预加载核心插件 (Pre-load Builtin)
		APP_LOG_INFO("[Plugin Manager]: Pre-loading core plugins...");
		for (auto& [id, plugin] : mPlugins)
			if (plugin->metadata.type == PluginType::Builtin)
				// 核心插件即使被标记为 Disabled (理论上不应该) 也会强制检查
				if (plugin->state != PluginState::Broken)
					loadPlugin(id);

		APP_LOG_INFO("[Plugin Manager]: Plugin Manager initialized successfully.");
		APP_LOG_INFO("[Plugin Manager]: Total Plugins: {}, Missing Core: {}", mPlugins.size(), mMissingCorePlugins.size());
		return true;
	}

	void PluginManager::shutdown()
	{
		unloadAll();
	}

	// =========================================================================
	// 2. 清单管理 (Manifest)
	// =========================================================================

	bool PluginManager::loadCoreManifest()
	{
		if (!std::filesystem::exists(mCoreManifestFile))
		{
			APP_LOG_CRITICAL("[Plugin Manager]: Core manifest missing: {}", mCoreManifestFile);
			return false;
		}
		// 如果文件大小为 0，视为“无内容但正常”，直接返回 true
		if (std::filesystem::file_size(mCoreManifestFile) == 0)
		{
			APP_LOG_WARN("[Plugin Manager]: Core manifest is empty, skipping.");
			return true;
		}
		try
		{
			std::ifstream f(mCoreManifestFile);
			nlohmann::json root = nlohmann::json::parse(f);
			if (root.contains("plugins"))
				for (auto& [id, item] : root["plugins"].items())
					parseManifestEntry(id, item, PluginType::Builtin);
			return true;
		}
		catch (const std::exception& e)
		{
			APP_LOG_CRITICAL("[Plugin Manager]: Failed to parse core manifest: {}", e.what());
			return false;
		}
	}

	bool PluginManager::loadUserManifest()
	{
		if (!std::filesystem::exists(mUserManifestFile))
		{
			APP_LOG_WARN("[Plugin Manager]: User manifest missing: {}", mUserManifestFile);
			return false;
		}
		// 如果文件大小为 0，视为“无内容但正常”，直接返回 true
		if (std::filesystem::file_size(mUserManifestFile) == 0)
		{
			APP_LOG_WARN("[Plugin Manager]: User manifest is empty, skipping.");
			return true;
		}
		try
		{
			std::ifstream f(mUserManifestFile);
			nlohmann::json root = nlohmann::json::parse(f);
			if (root.contains("plugins"))
				for (auto& [id, item] : root["plugins"].items())
					parseManifestEntry(id, item, PluginType::ThirdParty);
			return true;
		}
		catch (const std::exception& e)
		{
			APP_LOG_ERROR("[Plugin Manager]: Failed to parse user manifest: {}", e.what());
			return false;
		}
	}

	void PluginManager::saveUserManifest()
	{
		nlohmann::json pluginsObj;
		for (const auto& [id, plugin] : mPlugins)
		{
			// 只保存第三方插件
			if (plugin->metadata.type == PluginType::ThirdParty)
			{
				nlohmann::json item;

				// 路径标准化为 Generic String (Forward Slash)
				std::string pathStr = plugin->metadata.rootPath.string();
				std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

				item["path"] = pathStr;
				item["method"] = (plugin->metadata.installMethod == InstallMethod::Link) ? "Link" : "Copy";
				item["enabled"] = plugin->metadata.isEnabled;

				pluginsObj[id] = item;
			}
		}

		nlohmann::json root;
		root["plugins"] = pluginsObj;

		// 确保目录存在
		std::filesystem::path p(mUserManifestFile);
		if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());

		std::ofstream f(mUserManifestFile);
		f << root.dump(4);
	}

	void PluginManager::parseManifestEntry(const std::string& id, const nlohmann::json& item, PluginType type)
	{
		// 安全保护：防止 User 覆盖 Builtin
		if (mPlugins.count(id))
			if (mPlugins[id]->metadata.type == PluginType::Builtin && type == PluginType::ThirdParty)
			{
				APP_LOG_WARN("[Plugin Manager]: Ignored attempt to override builtin plugin: {}", id);
				return;
			}

		auto p = std::make_shared<PluginInstance>();
		p->metadata.id = id;
		p->metadata.type = type;
		
		std::filesystem::path pluginPath(item.value("path", ""));

		// 如果 JSON 里写的是相对路径 (如 "./Essential")，则将其拼接到 basePath 后面
		if (pluginPath.is_relative())
		{
			if (type == PluginType::Builtin) pluginPath = mCoreDir / pluginPath;
			else pluginPath = mUserDir / pluginPath;
		}
		p->metadata.rootPath = std::filesystem::weakly_canonical(pluginPath).string();

		std::string method = item.value("method", "Copy");
		p->metadata.installMethod = (method == "Link") ? InstallMethod::Link : InstallMethod::Copy;

		// 核心插件强制启用
		if (type == PluginType::Builtin) p->metadata.isEnabled = true;
		else p->metadata.isEnabled = item.value("enabled", true);

		mPlugins[id] = p;
	}

	// =========================================================================
	// 3. 完整性检查与本地解析
	// =========================================================================

	bool PluginManager::checkIntegrity(std::shared_ptr<PluginInstance> plugin)
	{
		if (!std::filesystem::exists(plugin->metadata.rootPath))
		{
			plugin->metadata.integrityStatus = "RootDirectoryMissing";
			return false;
		}

		// 解析本地 plugin.json
		auto freshInfo = parseLocalPluginJson(plugin->metadata.rootPath);
		if (!freshInfo)
		{
			plugin->metadata.integrityStatus = "InvalidPluginJson";
			return false;
		}

		if (freshInfo->metadata.id != plugin->metadata.id)
		{
			plugin->metadata.integrityStatus = "IdMismatch";
			return false;
		}

		// 刷新元数据
		plugin->metadata.name = freshInfo->metadata.name;
		plugin->metadata.version = freshInfo->metadata.version;
		plugin->metadata.binaryName = freshInfo->metadata.binaryName;
		plugin->metadata.dependencies = freshInfo->metadata.dependencies;
		plugin->metadata.resources = freshInfo->metadata.resources;
		plugin->metadata.defaultValuePath = freshInfo->metadata.defaultValuePath;

		// 检查 Binary
		auto mainDllPath = plugin->metadata.rootPath / plugin->metadata.binaryName;
		if (!std::filesystem::exists(mainDllPath))
		{
			plugin->metadata.integrityStatus = "MainBinaryMissing: " + plugin->metadata.binaryName;
			return false;
		}

		// 2. 【新增】检查依赖 DLL (Libraries)
		// 这些文件丢失虽然不影响 PluginManager 逻辑，但会导致 LoadLibrary 失败
		for (const auto& lib : plugin->metadata.libraries)
		{
			auto libPath = plugin->metadata.rootPath / lib;
			if (!std::filesystem::exists(libPath))
			{
				plugin->metadata.integrityStatus = "LibraryMissing: " + lib;
				return false;
			}
		}

		// 检查 Resources
		for (const auto& res : plugin->metadata.resources)
		{
			auto resPath = plugin->metadata.rootPath / res;
			if (!std::filesystem::exists(resPath))
			{
				plugin->metadata.integrityStatus = "ResourceMissing: " + res;
				return false;
			}
		}

		plugin->metadata.integrityStatus = "OK";
		return true;
	}

	std::shared_ptr<PluginInstance> PluginManager::parseLocalPluginJson(const std::filesystem::path& dir)
	{
		auto path = dir / "plugin.json";
		if (!std::filesystem::exists(path)) return nullptr;

		try
		{
			std::ifstream f(path);
			nlohmann::json j = nlohmann::json::parse(f);

			auto p = std::make_shared<PluginInstance>();
			p->metadata.id = j.value("id", "");
			p->metadata.name = j.value("name", "Unknown");
			p->metadata.version = j.value("version", "0.0.0");
			p->metadata.binaryName = j.value("binary", "");
			p->metadata.defaultValuePath = j.value("default_value_path", "");

			if (j.contains("libraries"))
				p->metadata.libraries = j["libraries"].get<std::vector<std::string>>();

			if (j.contains("dependencies"))
				p->metadata.dependencies = j["dependencies"].get<std::vector<std::string>>();

			if (j.contains("resources"))
				p->metadata.resources = j["resources"].get<std::vector<std::string>>();

			if (p->metadata.id.empty()) return nullptr;
			return p;
		}
		catch (...)
		{
			return nullptr;
		}
	}

	// =========================================================================
	// 4. 加载与卸载 (含懒加载)
	// =========================================================================

	bool PluginManager::loadPlugin(const std::string& id)
	{
		auto it = mPlugins.find(id);
		if (it == mPlugins.end()) return false;
		auto plugin = it->second;

		// 快速状态检查
		if (plugin->state == PluginState::Loaded) return true;
		if (plugin->state == PluginState::Disabled)
		{
			APP_LOG_WARN("Attempted to load disabled plugin: {}", id);
			return false;
		}
		if (plugin->state == PluginState::Broken) return false;

		std::set<std::string> visited;
		return loadPluginRecursive(id, visited);
	}

	bool PluginManager::loadPluginRecursive(const std::string& id, std::set<std::string>& visited)
	{
		// 循环依赖检测
		if (visited.count(id))
		{
			APP_LOG_ERROR("Circular dependency detected: {}", id);
			return false;
		}

		auto it = mPlugins.find(id);
		if (it == mPlugins.end()) return false;
		auto plugin = it->second;

		if (plugin->state == PluginState::Loaded) return true;
		if (plugin->state != PluginState::Unloaded) return false; // Broken or Disabled

		visited.insert(id);

		// 递归加载依赖
		for (const auto& depId : plugin->metadata.dependencies)
		{
			// APP_LOG_INFO("Resolving dependency: {} -> {}", id, depId);
			if (!loadPluginRecursive(depId, visited))
			{
				APP_LOG_ERROR("Failed to load dependency {} for plugin {}", depId, id);
				return false;
			}
		}

		// 加载 DLL
		if (loadDll(*plugin))
		{
			mLoadOrder.push_back(id);
			return true;
		}
		return false;
	}

	void PluginManager::loadAll()
	{
		for (auto& [id, plugin] : mPlugins)
			if (plugin->state == PluginState::Unloaded && plugin->metadata.isEnabled)
				loadPlugin(id);
	}

	bool PluginManager::loadDll(PluginInstance& plugin)
	{
		auto path = plugin.metadata.rootPath / plugin.metadata.binaryName;
		APP_LOG_INFO("[Plugin Manager]: Loading DLL: {}", path.string());

#ifdef _WIN32
		// LOAD_WITH_ALTERED_SEARCH_PATH 允许 DLL 查找同目录下的依赖
		HMODULE h = LoadLibraryExA(path.string().c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
#else
		void* h = dlopen(path.c_str(), RTLD_LAZY);
#endif

		if (!h)
		{
			APP_LOG_ERROR("[Plugin Manager]: Failed to load binary: {}", path.string());
#ifdef _WIN32
			APP_LOG_ERROR("[Plugin Manager]: Error code from OS: {}", GetLastError());
#else

#endif
			plugin.state = PluginState::Error;
			return false;
		}
		plugin.dllHandle = h;

#ifdef _WIN32
#define GET_SYM(x) GetProcAddress(h, x)
#else
#define GET_SYM(x) dlsym(h, x)
#endif

		// 绑定导出函数
		auto createFunc = (core::PluginBase * (*)())GET_SYM("createPlugin");
		auto destroyFunc = (void (*)(core::PluginBase*))GET_SYM("destroyPlugin");

		if (!createFunc || !destroyFunc)
		{
			APP_LOG_ERROR("[Plugin Manager]: Failed to resolve symbols in: {}", path.string());
			// 记得在这里释放 handle，否则这次加载失败的 DLL 会一直占用内存
#ifdef _WIN32
			APP_LOG_ERROR("[Plugin Manager]: Error code from OS: {}", GetLastError());
			FreeLibrary(h);
#else
			dlclose(h);
#endif
			plugin.dllHandle = nullptr;
			plugin.state = PluginState::Error;
			return false;
		}

//		// 调用工厂函数创建裸指针
//		core::PluginBase* rawPtr = createFunc();
//		if (!rawPtr) {
//			APP_LOG_ERROR("[Plugin Manager]: createPlugin returned null.");
//#ifdef _WIN32
//			FreeLibrary(h);
//#else
//			dlclose(h);
//#endif
//			plugin.dllHandle = nullptr;
//			plugin.state = PluginState::Error;
//			return false;
//		}

		std::unique_ptr<core::PluginBase, void (*)(core::PluginBase*)> guard(createFunc(), destroyFunc);

		if (!guard)
		{
			APP_LOG_ERROR("[Plugin Manager]: createPlugin returned null.");
#ifdef _WIN32
			FreeLibrary(h);
#else
			dlclose(h);
#endif
			plugin.dllHandle = nullptr;
			plugin.state = PluginState::Error;

			return false;
			//APP_LOG_ERROR("[Plugin Manager]: Factory returned null.");
			//return false;
		}

		plugin.pluginObject = std::shared_ptr<core::PluginBase>(std::move(guard));

		// 混合生命周期：RAII 管理对象生存期，init() 负责业务初始化。
		if (!plugin.pluginObject->init())
		{
			APP_LOG_ERROR("[Plugin Manager]: InitPlugin returned false: {}", plugin.metadata.id);
			unloadDll(plugin);
			return false;
		}

		plugin.state = PluginState::Loaded;
		return true;
	}

	void PluginManager::unloadAll()
	{
		// 按加载顺序反向卸载
		for (auto it = mLoadOrder.rbegin(); it != mLoadOrder.rend(); ++it)
			if (mPlugins.count(*it))
				unloadDll(*mPlugins[*it]);
		mLoadOrder.clear();
	}

	void PluginManager::unloadDll(PluginInstance& plugin)
	{
		// 如果还没加载，直接返回
		if (plugin.state == PluginState::Unloaded) return;

		// 混合生命周期：RAII 管理对象生存期，shutdown() 负责业务反注册/反初始化。
		if (plugin.pluginObject)
		{
			try
			{
				plugin.pluginObject->shutdown();
			}
			catch (const std::exception& e)
			{
				APP_LOG_ERROR("[Plugin Manager]: Exception during plugin shutdown: {}", e.what());
			}
			catch (...)
			{
				APP_LOG_ERROR("[Plugin Manager]: Unknown exception during plugin shutdown");
			}
		}

		// 1. 销毁 C++ 对象
		// 必须在 FreeLibrary 之前完成！
		// 置空 shared_ptr 会触发 deleter (destroyPlugin)，此时 DLL 还在内存中，是安全的。
		APP_LOG_INFO("[Plugin Manager]: Releasing plugin object: {}", plugin.metadata.id);
		plugin.pluginObject = nullptr;
		APP_LOG_INFO("[Plugin Manager]: Plugin object released: {}", plugin.metadata.id);

		// 2. 最后卸载 DLL
		if (plugin.dllHandle)
		{
			APP_LOG_INFO("[Plugin Manager]: Freeing Library: {}", plugin.metadata.id);
#ifdef _WIN32
			BOOL ok = FreeLibrary((HMODULE)plugin.dllHandle);
			if (!ok)
				APP_LOG_ERROR("[Plugin Manager]: FreeLibrary failed for {}. GetLastError={}", plugin.metadata.id, GetLastError());
#else
			dlclose(plugin.dllHandle);
#endif
			plugin.dllHandle = nullptr;
			APP_LOG_INFO("[Plugin Manager]: Library freed: {}", plugin.metadata.id);
		}

		plugin.state = PluginState::Unloaded;
	}

	// =========================================================================
	// 5. 状态控制 (级联禁用)
	// =========================================================================

	std::vector<std::string> PluginManager::setPluginEnabled(const std::string& id, bool enabled)
	{
		std::vector<std::string> affectedList;
		auto it = mPlugins.find(id);
		if (it == mPlugins.end()) return affectedList;

		auto targetPlugin = it->second;

		// 启用逻辑
		if (enabled)
		{
			if (targetPlugin->metadata.type == PluginType::Builtin) return affectedList; // Core always enabled
			if (targetPlugin->state == PluginState::Broken) return affectedList;

			if (!targetPlugin->metadata.isEnabled)
			{
				targetPlugin->metadata.isEnabled = true;
				targetPlugin->state = PluginState::Unloaded;
				affectedList.push_back(id);
				// 尝试懒加载
				loadPlugin(id);
				saveUserManifest();
			}
		}
		// 禁用逻辑 (级联)
		else
		{
			if (targetPlugin->metadata.type == PluginType::Builtin)
			{
				APP_LOG_WARN("Cannot disable Built-in plugin: {}", id);
				return affectedList;
			}

			std::set<std::string> visited;
			disablePluginRecursive(id, affectedList, visited);

			if (!affectedList.empty())
			{
				APP_LOG_INFO("Cascading disable affected {} plugins.", affectedList.size());
				saveUserManifest();
			}
		}
		return affectedList;
	}

	void PluginManager::disablePluginRecursive(const std::string& targetId, std::vector<std::string>& affected, std::set<std::string>& visited)
	{
		if (visited.count(targetId)) return;
		visited.insert(targetId);

		// 反向依赖查找：查找所有依赖 targetId 的插件
		for (auto& [otherId, plugin] : mPlugins)
		{
			if (plugin->metadata.isEnabled)
			{
				const auto& deps = plugin->metadata.dependencies;
				if (std::find(deps.begin(), deps.end(), targetId) != deps.end())
				{
					// otherId 依赖 targetId，必须先禁用 otherId
					disablePluginRecursive(otherId, affected, visited);
				}
			}
		}

		// 禁用当前插件
		auto& plugin = mPlugins[targetId];
		if (plugin->metadata.isEnabled)
		{
			if (plugin->state == PluginState::Loaded)
				unloadDll(*plugin);
			plugin->metadata.isEnabled = false;
			plugin->state = PluginState::Disabled;
			affected.push_back(targetId);
		}
	}

	// =========================================================================
	// 6. 安装与卸载 (Copy / Link)
	// =========================================================================

	bool PluginManager::installFromPath(const std::string& externalPath)
	{
		auto info = parseLocalPluginJson(externalPath);
		if (!info)
		{
			APP_LOG_ERROR("Invalid plugin directory: {}", externalPath);
			return false;
		}

		std::string id = info->metadata.id;
		if (mPlugins.count(id))
		{
			APP_LOG_ERROR("Plugin ID collision: {}", id);
			return false;
		}

		info->metadata.type = PluginType::ThirdParty;
		info->metadata.installMethod = InstallMethod::Link;
		info->metadata.isEnabled = true;

		mPlugins[id] = info;
		saveUserManifest();

		if (checkIntegrity(info))
		{
			info->state = PluginState::Unloaded;
			loadPlugin(id); // 尝试加载
		}
		return true;
	}

	bool PluginManager::installFromPackage(const std::string& zipPath)
	{
		// 1. 解压到临时目录
		std::string tempDir = "temp_install";
		if (std::filesystem::exists(tempDir)) std::filesystem::remove_all(tempDir);

		if (!extractZip(zipPath, tempDir)) return false;

		// 2. 读取 ID
		auto info = parseLocalPluginJson(tempDir);
		if (!info || info->metadata.id.empty())
		{
			std::filesystem::remove_all(tempDir);
			return false;
		}

		std::string id = info->metadata.id;

		// 允许覆盖旧的 ThirdParty 插件
		if (mPlugins.count(id))
		{
			if (mPlugins[id]->metadata.type == PluginType::Builtin)
			{
				std::filesystem::remove_all(tempDir);
				return false;
			}
			uninstallPlugin(id);
		}

		// 3. 移动到 plugins/ID
		std::filesystem::path dest = mUserDir / id;
		try
		{
			std::filesystem::create_directories(dest);
			// 递归复制
			for (const auto& entry : std::filesystem::recursive_directory_iterator(tempDir))
			{
				auto rel = std::filesystem::relative(entry.path(), tempDir);
				auto target = dest / rel;
				if (entry.is_directory()) std::filesystem::create_directories(target);
				else std::filesystem::copy_file(entry.path(), target, std::filesystem::copy_options::overwrite_existing);
			}
			std::filesystem::remove_all(tempDir);
		}
		catch (...)
		{
			return false;
		}

		// 4. 注册
		auto newPlugin = parseLocalPluginJson(dest);
		newPlugin->metadata.type = PluginType::ThirdParty;
		newPlugin->metadata.installMethod = InstallMethod::Copy;
		newPlugin->metadata.isEnabled = true;

		mPlugins[id] = newPlugin;
		checkIntegrity(newPlugin);
		saveUserManifest();

		if (newPlugin->metadata.integrityStatus == "OK")
		{
			newPlugin->state = PluginState::Unloaded;
			loadPlugin(id);
		}

		return true;
	}

	bool PluginManager::uninstallPlugin(const std::string& id)
	{
		auto it = mPlugins.find(id);
		if (it == mPlugins.end()) return false;
		auto plugin = it->second;

		if (plugin->metadata.type == PluginType::Builtin) return false;

		if (plugin->state == PluginState::Loaded) unloadDll(*plugin);

		if (plugin->metadata.installMethod == InstallMethod::Copy)
		{
			try
			{
				std::filesystem::remove_all(plugin->metadata.rootPath);
			}
			catch (...)
			{
				APP_LOG_ERROR("Failed to remove plugin files: {}", plugin->metadata.rootPath.string());
				return false;
			}
		}

		mPlugins.erase(it);
		saveUserManifest();
		return true;
	}

	// =========================================================================
	// 7. 工具函数 (Minizip)
	// =========================================================================

	bool PluginManager::extractZip(const std::string& zipPath, const std::string& destDir)
	{
		void* reader = mz_zip_reader_create();

		if (mz_zip_reader_open_file(reader, zipPath.c_str()) != MZ_OK)
		{
			mz_zip_reader_delete(&reader);
			return false;
		}

		// 解压所有文件
		mz_zip_reader_set_pattern(reader, "*", 1);

		int err = mz_zip_reader_goto_first_entry(reader);
		bool success = true;

		while (err == MZ_OK && success)
		{
			mz_zip_file* file_info = NULL;
			mz_zip_reader_entry_get_info(reader, &file_info);

			// 路径拼接
			std::filesystem::path outPath = std::filesystem::path(destDir) / file_info->filename;

			if (mz_zip_reader_entry_is_dir(reader) == MZ_OK)
				std::filesystem::create_directories(outPath);
			else
			{
				if (outPath.has_parent_path()) std::filesystem::create_directories(outPath.parent_path());

				if (mz_zip_reader_entry_save_file(reader, outPath.string().c_str()) != MZ_OK)
				{
					success = false;
					APP_LOG_ERROR("Failed to extract file: {}", file_info->filename);
				}
			}
			err = mz_zip_reader_goto_next_entry(reader);
		}

		mz_zip_reader_close(reader);
		mz_zip_reader_delete(&reader);
		return success;
	}

	// 查询接口
	std::shared_ptr<PluginInstance> PluginManager::getPlugin(const std::string& id)
	{
		if (mPlugins.count(id)) return mPlugins[id];
		return nullptr;
	}

	std::vector<std::shared_ptr<PluginInstance>> PluginManager::getAllPlugins() const
	{
		std::vector<std::shared_ptr<PluginInstance>> list;
		for (auto& [k, v] : mPlugins) list.push_back(v);
		return list;
	}

	std::vector<std::string> PluginManager::getMissingCorePlugins() const
	{
		return mMissingCorePlugins;
	}
}
