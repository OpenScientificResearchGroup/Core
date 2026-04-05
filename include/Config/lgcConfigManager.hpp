/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "defCoreApi.hpp"

#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <typeinfo>
#include <vector>

#include <nlohmann/json.hpp>

#include "Log/lgcLogManager.hpp"

namespace core
{
	// 配置项元数据（用于 UI 显示和默认值回退）
	struct ConfigOption
	{
		std::string key;				// e.g. "/plugins/git/timeout"
		nlohmann::json defaultValue;	// 默认值 (Schema)
		std::string description;		// 描述文本
		std::string typeName;			// 类型名称
	};

	class CORE_API ConfigManager
	{
		// 使用 Windows 资源管理器风格的分隔符
		static constexpr char SEPARATOR = '/';

	public:
		static ConfigManager& get();

		// 禁止复制和赋值
		ConfigManager(const ConfigManager&) = delete;
		ConfigManager& operator=(const ConfigManager&) = delete;

		/// <summary>
		/// 初始化并加载配置文件
		/// </summary>
		/// <param name="filePath">用户修改的配置文件路径</param>
		/// <param name="coreFilePath">Core默认配置文件路径</param>
		/// <returns></returns>
		bool init(const std::string& filePath, const std::string& coreFilePath);
		void shutdown();

		/// <summary>
		/// 读取默认配置文件 (Schema)
		/// </summary>
		/// <param name="path">JSON路径</param>
		/// <returns></returns>
		bool readDefaultValue(const std::string& path);

		/// <summary>
		/// 注册配置项 (Schema)
		/// 通常由插件在加载时调用。只更新内存元数据，不写入磁盘。
		/// </summary>
		template <typename T>
		void registerOption(const std::string& key, const T& defaultValue, const std::string& description)
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);

			ConfigOption option;
			option.key = key;
			option.defaultValue = defaultValue; // 自动转换为 nlohmann::json
			option.description = description;
			option.typeName = typeid(T).name();

			mSchema[key] = std::move(option);
		}

		/// <summary>
		/// 获取配置值 (核心逻辑)
		/// 优先级：用户设置(JSON) > 插件注册默认值(Schema) > 硬编码兜底(Fallback)
		/// </summary>
		template <typename T>
		T getValue(const std::string& key, const std::optional<T>& hardFallback = std::nullopt)
		{
			std::shared_lock<std::shared_mutex> lock(mMutex);

			// 1. 尝试从用户配置文件(User Layer)读取
			const nlohmann::json* node = traverse(mUserConfig, key, false);
			if (node && !node->is_null())
			{
				try
				{
					return node->get<T>();
				}
				catch (...)
				{

				}
			}

			// 2. 尝试从注册表(Schema Layer)读取默认值
			auto it = mSchema.find(key);
			if (it != mSchema.end())
			{
				try
				{
					return it->second.defaultValue.get<T>();
				}
				catch (...)
				{

				}
			}

			// 3. 硬编码兜底
			if (hardFallback.has_value()) return hardFallback.value();

			// 4. 类型默认构造
			return T();
		}

		/// <summary>
		/// 获取描述 (核心逻辑)
		/// </summary>
		const std::string& getDescription(const std::string& key);

		/// <summary>
		/// 修改配置值 (User Layer)
		/// 写入 mUserConfig 并立即落盘
		/// </summary>
		template <typename T>
		void setValue(const std::string& key, const T& value)
		{
			{
				std::unique_lock<std::shared_mutex> lock(mMutex);
				nlohmann::json* node = traverse(mUserConfig, key, true); // create=true
				if (node)
					*node = value;
			}
			save(); // 自动保存
		}

		/// <summary>
		/// 重置为默认值
		/// 从用户 JSON 中删除该 Key，下次 get 时会自动回退到 Schema 默认值
		/// </summary>
		void reset(const std::string& key);

		// 获取所有注册的 Schema (用于生成设置 UI)
		std::map<std::string, ConfigOption> getSchema() const;

		// 调试用：打印当前所有生效的配置
		std::string dump() const;

	private:
		ConfigManager() = default;
		~ConfigManager() = default;

		bool load();
		bool save();

		// 路径遍历辅助函数
		nlohmann::json* traverse(nlohmann::json& root, const std::string& key, bool createIfMissing);
		const nlohmann::json* traverse(const nlohmann::json& root, const std::string& key, bool createIfMissing) const;

	private:
		std::filesystem::path mUserConfigPath;
		nlohmann::json mUserConfig; // 只存用户修改过的
		std::map<std::string, ConfigOption> mSchema; // 对应插件注册的全量默认值

		mutable std::shared_mutex mMutex; // 读写锁
	};
}// namespace core