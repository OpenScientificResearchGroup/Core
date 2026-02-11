#pragma once
#include "defCoreApi.hpp"

#include <string>
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <filesystem>
#include <functional>
#include <typeindex>
#include <optional>
#include <iostream>

#include "Resource/virResourceBase.hpp"

namespace core
{
	class CORE_API ResourceManager
	{
	public:
		static ResourceManager& get();

		ResourceManager(const ResourceManager&) = delete;
		ResourceManager& operator=(const ResourceManager&) = delete;

		void mount(const std::string& alias, const std::filesystem::path& physicalPath);

		void registerLoader(const std::string& extension, std::function<std::shared_ptr<ResourceBase>(const std::filesystem::path&)> loader);

		template <typename T>
		std::shared_ptr<T> load(const std::string& logicalPath)
		{
			auto physicalPath = resolvePath(logicalPath);
			if (!physicalPath)
			{
				std::cerr << "[ResourceManager] Error: Path resolve failed for " << logicalPath << std::endl;
				return nullptr;
			}

			std::string cacheKey = physicalPath->string();

			{
				std::shared_lock lock(mMutex);
				auto it = mResources.find(cacheKey);
				if (it != mResources.end())
					return std::dynamic_pointer_cast<T>(it->second);
			}

			std::unique_lock lock(mMutex);

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
		std::unordered_map<std::string, std::filesystem::path> mAliases;

		std::unordered_map<std::string, std::shared_ptr<ResourceBase>> mResources;

		std::unordered_map<std::string, std::function<std::shared_ptr<ResourceBase>(const std::filesystem::path&)>> mLoaders;

		mutable std::shared_mutex mMutex;
		mutable std::shared_mutex mPathMutex;
	};

} // namespace core
