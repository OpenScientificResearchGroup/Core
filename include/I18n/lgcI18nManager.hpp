#pragma once
#include "defCoreApi.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "Log/lgcLogManager.hpp"

namespace core
{
	class CORE_API I18nManager
	{
	public:
		static I18nManager& get();

		I18nManager(const I18nManager&) = delete;
		I18nManager& operator=(const I18nManager&) = delete;

		// 初始化配置
		bool init(const std::string& locale, const std::string& fallbackLocale = "");
		void shutdown();

		// 异步或同步加载语言
		bool loadFromFile(const std::string& path, const std::string& locale);
		bool loadFromString(const std::string& data, const std::string& locale);

		// 切换当前语言
		void setLocale(const std::string& locale);
		std::string getLocale() const;
		std::string getFallbackLocale() const;

		// 核心翻译函数：支持 fmt 格式化参数
		// 用法: i18n.t("user.welcome", fmt::arg("name", "Alice"));
		template <typename... Args>
		std::string t(const std::string& key, Args&&... args) const
		{
			std::string_view formatStr = getRaw(key);

			// 如果没有参数，直接返回原始字符串，避免格式化开销
			if constexpr (sizeof...(args) == 0)
				return std::string(formatStr);

			try
			{
				return fmt::vformat(formatStr, fmt::make_format_args(args...));
			}
			catch (const fmt::format_error& e)
			{
				APP_LOG_ERROR("[I18n Manager]: Format error for key '{}': {}", key, e.what());
				return std::string(formatStr);
			}
		}

		// 复数支持 (Pluralization)
		// 简化版实现：根据 count 自动查找 key_one, key_other 等
		template <typename... Args>
		std::string tc(const std::string& baseKey, int count, Args&&... args) const {
			// 简单的英语规则示例，实际工业级需要引入 CLDR 规则引擎
			std::string suffix = (count == 1) ? "_one" : "_other";
			std::string fullKey = baseKey + suffix;

			// 如果特定后缀不存在，回退到 baseKey (或者 "_other")
			if (!hasKey(fullKey))
			{
				fullKey = baseKey + "_other";
				if (!hasKey(fullKey)) fullKey = baseKey;
			}

			return t(fullKey, std::forward<Args>(args)...);
		}

		void clear();
		void removeLocale(const std::string& locale);

		// 订阅语言变更事件
		//void subscribe(std::function<void(const std::string&)> cb);

	private:
		I18nManager() = default;
		~I18nManager() = default;

		// 内部查找逻辑
		std::string_view getRaw(const std::string& key) const;
		bool hasKey(const std::string& key, const std::string& locale = "") const;

	private:
		mutable std::shared_mutex mMutex; // 读写锁：保证线程安全
		std::string mCurrentLocale;
		std::string mFallbackLocale;
		std::unordered_map<std::string, std::unordered_map<std::string, std::string>> mTranslations; // 缓存池：当前语言 + 回退语言 + 其他

		//std::vector<std::function<void(const std::string&)>> mListeners;
	};
}