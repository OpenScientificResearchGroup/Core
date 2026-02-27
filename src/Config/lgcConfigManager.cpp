#include "Config/lgcConfigManager.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

#include <nlohmann/json.hpp>

#include "Log/lgcLogManager.hpp"
#include "Utility/lgcString.hpp"

namespace core
{
	ConfigManager& ConfigManager::get()
	{
		static ConfigManager instance;
		return instance;
	}

	bool ConfigManager::init(const std::string& userConfigPath, const std::string& coreConfigPath)
	{
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			mUserConfigPath = userConfigPath;

			// 确保目录存在
			if (mUserConfigPath.has_parent_path())
			{
				try
				{
					std::filesystem::create_directories(mUserConfigPath.parent_path());
				}
				catch (...)
				{
					APP_LOG_ERROR("[Configure Manager]: Failed to create user configuration directory: {}.", mUserConfigPath.parent_path().string());
					return false;
				}
			}
		}

		load();
		if (!readDefaultValue(coreConfigPath))
		{
			APP_LOG_CRITICAL("[Configure Manager]: Failed to read Core default configuration. Core initialization failed.");
			return false;
		}

		APP_LOG_INFO("[Configure Manager]: Configure Manager initialized successfully.");
	}

	void ConfigManager::shutdown()
	{
		std::unique_lock<std::shared_mutex> lock(mMutex);
		mUserConfig = nlohmann::json::object();
		mSchema.clear();
	}

	bool ConfigManager::readDefaultValue(const std::string& path)
	{
		std::ifstream i(path);

		if (!i.is_open())
		{
			APP_LOG_ERROR("[Configure Manager]: Could not open default configuration file: {}", path);
			return false;
		}

		try
		{
			nlohmann::json data;
			// 使用输入操作符 (>>) 直接从文件流读取
			i >> data;

			// 定义递归 lambda 函数来遍历 JSON 树
			// 参数: j = 当前节点, currentKey = 当前累积的键名路径 (如 "Core.log")
			std::function<void(const nlohmann::json&, std::string)> traverseAndRegister;

			traverseAndRegister = [&](const nlohmann::json& j, std::string currentKey)
				{
				// 检查当前节点是否是配置项的“叶子节点”
				// 特征：它是一个对象，且必须包含 "value" 键
				if (j.is_object() && j.contains("value"))
				{
					// 获取值
					const auto& value = j["value"];

					// 获取描述 (如果不存在则默认为空字符串)
					std::string description = j.value("description", "");

					// 调用你的注册函数
					// currentKey 此时类似于 "/Core/log/path"
					// 注意：这里 value 是 nlohmann::json 类型，registerOption 需要能接受它
					registerOption(currentKey, value, description);

					return; // 找到配置项后，不再继续深入该分支
				}

				// 如果不是叶子节点，且是对象，说明是中间层级（如 "Core" 或 "log"）
				// 继续向下递归
				if (j.is_object())
				{
					for (const auto& element : j.items())
					{
						std::string nextKey;
						if (currentKey.empty())
							nextKey = "/" + element.key();
						else
							nextKey = currentKey + "/" + element.key(); // 使用"/"连接

						traverseAndRegister(element.value(), nextKey);
					}
				}
				};

			// 从根节点开始遍历
			traverseAndRegister(data, "");

			return true;
		}
		catch (const nlohmann::json::parse_error& e)
		{
			APP_LOG_ERROR("[Configure Manager]: Failed to parse default configuration: {}", e.what());
			return false;
		}
		catch (const nlohmann::json::exception& e)
		{
			APP_LOG_ERROR("[Configure Manager]: Failed to access default configuration: {}", e.what());
			return false;
		}
	}

	bool ConfigManager::load()
	{
		std::unique_lock<std::shared_mutex> lock(mMutex);
		if (!std::filesystem::exists(mUserConfigPath))
		{
			APP_LOG_ERROR("[Configure Manager]: Can not open user configure.");
			mUserConfig = nlohmann::json::object();
			return false;
		}

		try
		{
			std::ifstream file(mUserConfigPath);
			if (file.is_open())
			{
				file >> mUserConfig;
				return true;
			}
		}
		catch (const std::exception& e)
		{
			APP_LOG_WARN("[Configure Manager]: Failed to load user configuration: {}.", e.what());
			APP_LOG_INFO("[Configure Manager]: Reset to empty configuration.");
			mUserConfig = nlohmann::json::object(); // 容错：解析失败则置空
		}
		return false;
	}

	bool ConfigManager::save()
	{
		std::shared_lock<std::shared_mutex> readLock(mMutex); // 读取内存需要读锁
		// 但是写入文件需要防止其他线程同时修改内存吗？
		// 严格来说，traverse 修改 JSON 时加了写锁。我们 dump 时只需要读锁。

		try
		{
			std::ofstream file(mUserConfigPath);
			if (file.is_open())
			{
				file << mUserConfig.dump(4); // 缩进4空格
				return true;
			}
		}
		catch (const std::exception& e)
		{
			APP_LOG_ERROR("[Configure Manager]: Failed to save user configuration: {}.", e.what());
		}
		return false;
	}

	const std::string& ConfigManager::getDescription(const std::string& key)
	{
		std::shared_lock<std::shared_mutex> lock(mMutex);

		// 尝试从注册表(Schema Layer)读取值
		auto it = mSchema.find(key);
		if (it != mSchema.end())
		{
			try
			{
				return it->second.description;
			}
			catch (...)
			{
				APP_LOG_ERROR("[Configure Manager]: Failed to get description for key: {}.", key);
			}
		}

		// 默认构造
		return "";
	}

	void ConfigManager::reset(const std::string& key)
	{
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);

			auto parts = util::string::split(key, SEPARATOR);
			if (parts.empty()) return;

			nlohmann::json* current = &mUserConfig;

			// 找到父节点
			for (size_t i = 0; i < parts.size() - 1; ++i)
			{
				if (current->contains(parts[i]))
					current = &((*current)[parts[i]]);
				else
					return; // 路径本来就不存在，无需删除
			}

			// 删除目标 Key
			std::string lastKey = parts.back();
			if (current->contains(lastKey))
				current->erase(lastKey);
		}

		save();
	}

	std::map<std::string, ConfigOption> ConfigManager::getSchema() const
	{
		std::shared_lock<std::shared_mutex> lock(mMutex);
		return mSchema;
	}

	std::string ConfigManager::dump() const
	{
		std::shared_lock<std::shared_mutex> lock(mMutex);
		return mUserConfig.dump(4);
	}

	// const 版本 (用于 get)
	const nlohmann::json* ConfigManager::traverse(const nlohmann::json& root, const std::string& key, bool /*create*/) const
	{
		auto parts = util::string::split(key, SEPARATOR);
		const nlohmann::json* current = &root;

		for (const auto& part : parts)
		{
			if (current->contains(part))
				current = &((*current)[part]);
			else
				return nullptr;
		}
		return current;
	}

	// 非 const 版本 (用于 set)
	nlohmann::json* ConfigManager::traverse(nlohmann::json& root, const std::string& key, bool create)
	{
		auto parts = util::string::split(key, SEPARATOR);
		nlohmann::json* current = &root;

		for (const auto& part : parts)
		{
			if (current->contains(part))
				current = &((*current)[part]);
			else
			{
				if (create)
				{
					(*current)[part] = nlohmann::json::object();
					current = &((*current)[part]);
				}
				else
					return nullptr;
			}
		}
		return current;
	}
}
