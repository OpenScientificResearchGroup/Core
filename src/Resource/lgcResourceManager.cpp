/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Resource/lgcResourceManager.hpp"

#include <algorithm>
#include <iostream>

#include "Log/lgcLogManager.hpp"

namespace core
{

	ResourceManager& ResourceManager::get()
	{
		static ResourceManager instance;
		return instance;
	}

	ResourceManager::~ResourceManager()
	{
		clearAll();
	}

	bool ResourceManager::init()
	{
		clearAll();
		return true;
	}

	void ResourceManager::shutdown()
	{
		clearAll();
	}

	void ResourceManager::mount(const std::string& alias, const std::filesystem::path& physicalPath)
	{
		std::unique_lock<std::shared_mutex> lock(mAliasMutex);

		// 规范化别名：去掉末尾的 "://" 或 ":"
		std::string cleanAlias = alias;
		if (cleanAlias.length() >= 3 && cleanAlias.substr(cleanAlias.length() - 3) == "://")
			cleanAlias.resize(cleanAlias.length() - 3);
		else if (!cleanAlias.empty() && cleanAlias.back() == ':')
			cleanAlias.pop_back();

		mAliases[cleanAlias] = std::filesystem::absolute(physicalPath);
		APP_LOG_INFO("[Resource Manager]: Mounted '{}' -> {}", cleanAlias, mAliases[cleanAlias].string());
	}

	void ResourceManager::registerLoader(const std::string& extension, std::function<std::shared_ptr<ResourceBase>(const std::filesystem::path&)> loader)
	{
		std::unique_lock<std::shared_mutex> lock(mLoaderMutex);
		mLoaders[extension] = loader;
	}

	std::optional<std::filesystem::path> ResourceManager::resolvePath(const std::string& logicalPath) const
	{
		std::shared_lock<std::shared_mutex> lock(mAliasMutex);

		// Example: "assets://textures/hero.png" -> alias: "assets", suffix: "textures/hero.png"
		size_t protocolPos = logicalPath.find("://");
		if (protocolPos != std::string::npos)
		{
			std::string prefix = logicalPath.substr(0, protocolPos);
			std::string suffix = logicalPath.substr(protocolPos + 3);

			auto it = mAliases.find(prefix);
			if (it != mAliases.end())
			{
				std::filesystem::path fullPath = it->second / suffix;
				return fullPath.make_preferred();
			}
		}

		std::filesystem::path directPath(logicalPath);
		if (std::filesystem::exists(directPath))
			return std::filesystem::absolute(directPath);

		return std::nullopt;
	}

	std::shared_ptr<ResourceBase> ResourceManager::loadFromDisk(const std::filesystem::path& path)
	{
		std::string ext = path.extension().string();

		auto it = mLoaders.find(ext);
		if (it == mLoaders.end())
		{
			APP_LOG_ERROR("[Resource Manager] No loader registered for extension: {}", ext);
			return nullptr;
		}

		APP_LOG_INFO("[Resource Manager] Loading from disk: {}", path.string());

		try
		{
			return it->second(path);
		}
		catch (const std::exception& e)
		{
			APP_LOG_ERROR("[Resource Manager] Exception loading {}: {}", path.string(), e.what());
			return nullptr;
		}
	}

	void ResourceManager::reload(const std::string& logicalPath)
	{
		auto physicalPath = resolvePath(logicalPath);
		if (!physicalPath) return;

		std::unique_lock<std::shared_mutex> lock(mResourceMutex);
		std::string key = physicalPath->string();

		mResources.erase(key);

		std::shared_ptr<ResourceBase> newRes = loadFromDisk(physicalPath.value());
		if (newRes)
		{
			newRes->setName(logicalPath);
			mResources[key] = newRes;
		}
	}

	void ResourceManager::unloadUnused()
	{
		std::unique_lock<std::shared_mutex> lock(mResourceMutex);

		size_t count = 0;
		for (auto it = mResources.begin(); it != mResources.end();)
		{
			if (it->second.use_count() == 1)
			{
				APP_LOG_INFO("[Resource Manager]: Garbage Collector releasing: {}", it->second->getName());
				it = mResources.erase(it);
				count++;
			}
			else
				++it;
		}
		if (count > 0)
			APP_LOG_INFO("[Resource Manager]: Garbage Collector finished. Released {} resources.", count);
	}

	void ResourceManager::clearAll()
	{
		{
			std::unique_lock<std::shared_mutex> lock(mResourceMutex);
			mResources.clear();
		}
		{
			std::unique_lock<std::shared_mutex> lock(mLoaderMutex);
			mLoaders.clear();
		}
		{
			std::unique_lock<std::shared_mutex> lock(mAliasMutex);
			mAliases.clear();
		}
	}
} // namespace Core
