#pragma once
#include "defCoreApi.hpp"

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <shared_mutex>
#include <functional>
#include <iostream>

#include <fmt/format.h> // 需要安装 fmtlib，或者使用 C++20 <format>
#include <nlohmann/json.hpp>

namespace core
{
	//// 定义翻译数据的加载接口（策略模式）
	//class ITranslationLoader
	//{
	//public:
	//	virtual ~ITranslationLoader() = default;
	//	// 返回扁平化的 Key-Value map, 例如: {"auth.login.btn": "登录"}
	//	virtual std::unordered_map<std::string, std::string> load(const std::string& locale) = 0;
	//};

	class I18nManager
	{
	public:
		static I18nManager& get();
		I18nManager(const I18nManager&) = delete;
		I18nManager& operator=(const I18nManager&) = delete;

		// 初始化配置
		void init(const std::string& locale, const std::string& fallbackLocale = "");
		//void setLoader(std::shared_ptr<ITranslationLoader> loader);
		//void setFallbackLocale(const std::string& locale);

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
				// 容错：格式化失败返回原始模板，并在 stderr 报错
				std::cerr << "[I18n Error] Format failed for key '" << key << "': " << e.what() << std::endl;
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
		// bool parseJsonObject(const nlohmann::json& data, std::unordered_map<std::string, std::string>& translations);
		std::string_view getRaw(const std::string& key) const;
		bool hasKey(const std::string& key, const std::string& locale = "") const;

		// std::shared_ptr<ITranslationLoader> mLoader;
		std::string mCurrentLocale;
		std::string mFallbackLocale = "en-US";

		// 缓存池：当前语言 + 回退语言 + 其他
		std::unordered_map<std::string, std::unordered_map<std::string, std::string>> mTranslations;
		//std::unordered_map<std::string, std::string> mTranslations;
		//std::unordered_map<std::string, std::string> mFallbackTranslations;

		// 读写锁：保证线程安全
		mutable std::shared_mutex mMutex;

		//std::vector<std::function<void(const std::string&)>> mListeners;
	};
}