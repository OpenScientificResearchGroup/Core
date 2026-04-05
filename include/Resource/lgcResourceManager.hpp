/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "defCoreApi.hpp"

#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <typeindex>
#include <unordered_map>

#include "Resource/virResourceBase.hpp"
#include "Log/lgcLogManager.hpp"

namespace core
{
	class CORE_API ResourceManager
	{
	public:
		static ResourceManager& get();

		ResourceManager(const ResourceManager&) = delete;
		ResourceManager& operator=(const ResourceManager&) = delete;

		bool init();
		void shutdown();

		void mount(const std::string& alias, const std::filesystem::path& physicalPath);
		void registerLoader(const std::string& extension, std::function<std::shared_ptr<ResourceBase>(const std::filesystem::path&)> loader);
		template <typename T>
		std::shared_ptr<T> getRes(const std::string& logicalPath)
		{
			auto physicalPath = resolvePath(logicalPath);
			if (!physicalPath)
			{
				APP_LOG_ERROR("[Resource Manager]: Error: Path resolve failed for {}", logicalPath);
				return nullptr;
			}

			std::string cacheKey = physicalPath->string();

			{
				std::shared_lock lock(mResourceMutex);
				auto it = mResources.find(cacheKey);
				if (it != mResources.end())
					return std::dynamic_pointer_cast<T>(it->second);
			}

			std::unique_lock lock(mResourceMutex);

			auto it = mResources.find(cacheKey);
			if (it != mResources.end())
				return std::dynamic_pointer_cast<T>(it->second);

			std::shared_ptr<ResourceBase> rawRes = loadFromDisk(physicalPath.value());

			if (rawRes)
			{
				rawRes->setName(logicalPath);
				mResources[cacheKey] = rawRes;
				return std::dynamic_pointer_cast<T>(rawRes);
			}

			return nullptr;
		}
		void reload(const std::string& logicalPath);
		void unloadUnused();
		void clearAll();
		std::optional<std::filesystem::path> resolvePath(const std::string& logicalPath) const;

	private:
		ResourceManager() = default;
		~ResourceManager();

		std::shared_ptr<ResourceBase> loadFromDisk(const std::filesystem::path& path);

	private:
		mutable std::shared_mutex mResourceMutex;
		mutable std::shared_mutex mAliasMutex;
		mutable std::shared_mutex mLoaderMutex;
		std::unordered_map<std::string, std::filesystem::path> mAliases;
		std::unordered_map<std::string, std::shared_ptr<ResourceBase>> mResources;
		std::unordered_map<std::string, std::function<std::shared_ptr<ResourceBase>(const std::filesystem::path&)>> mLoaders;

	};

} // namespace core
