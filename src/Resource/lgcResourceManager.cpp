#include "Resource/lgcResourceManager.hpp"

#include <algorithm>
#include <iostream>

namespace core
{

    ResourceManager &ResourceManager::get()
    {
        static ResourceManager instance;
        return instance;
    }

    ResourceManager::~ResourceManager()
    {
        clearAll();
    }

    void ResourceManager::mount(const std::string &alias, const std::filesystem::path &physicalPath)
    {
        std::unique_lock lock(mPathMutex);

        std::string cleanAlias = alias;
        if (cleanAlias.length() >= 3 && cleanAlias.substr(cleanAlias.length() - 3) == "://")
            cleanAlias.resize(cleanAlias.length() - 3);
        else if (!cleanAlias.empty() && cleanAlias.back() == ':')
            cleanAlias.pop_back();

        mAliases[cleanAlias] = std::filesystem::absolute(physicalPath);
        std::cout << "[ResourceManager] Mounted '" << cleanAlias << "' -> " << physicalPath << std::endl;
    }

    void ResourceManager::registerLoader(const std::string &extension, std::function<std::shared_ptr<ResourceBase>(const std::filesystem::path &)> loader)
    {
        std::unique_lock lock(mMutex);
        mLoaders[extension] = loader;
    }

    std::optional<std::filesystem::path> ResourceManager::resolvePath(const std::string &logicalPath) const
    {
        std::shared_lock lock(mPathMutex);

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

    std::shared_ptr<ResourceBase> ResourceManager::loadFromDisk(const std::filesystem::path &path)
    {
        std::string ext = path.extension().string();

        auto it = mLoaders.find(ext);
        if (it == mLoaders.end())
        {
            std::cerr << "[ResourceManager] No loader registered for extension: " << ext << std::endl;
            return nullptr;
        }

        std::cout << "[ResourceManager] Loading from disk: " << path << std::endl;

        try
        {
            return it->second(path);
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ResourceManager] Exception loading " << path << ": " << e.what() << std::endl;
            return nullptr;
        }
    }

    void ResourceManager::reload(const std::string &logicalPath)
    {
        auto physicalPath = resolvePath(logicalPath);
        if (!physicalPath)
            return;

        std::unique_lock lock(mMutex);
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
        std::unique_lock lock(mMutex);

        size_t count = 0;
        for (auto it = mResources.begin(); it != mResources.end();)
        {
            if (it->second.use_count() == 1)
            {
                std::cout << "[ResourceManager] GC releasing: " << it->second->getName() << std::endl;
                it = mResources.erase(it);
                count++;
            }
            else
                ++it;
        }
        if (count > 0)
            std::cout << "[ResourceManager] GC finished. Released " << count << " resources." << std::endl;
    }

    void ResourceManager::clearAll()
    {
        std::unique_lock lock(mMutex);
        mResources.clear();
    }
} // namespace Core
