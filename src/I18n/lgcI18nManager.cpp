/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "I18n/lgcI18nManager.hpp"

#include <fstream>

namespace core
{
	// 获取单例
	I18nManager& I18nManager::get()
	{
		static I18nManager instance;
		return instance;
	}

	bool I18nManager::init(const std::string& locale, const std::string& fallbackLocale)
	{
		std::unique_lock<std::shared_mutex> lock(mMutex);
		mCurrentLocale = locale;
		mFallbackLocale = fallbackLocale.empty() ? "en-US" : fallbackLocale;
		mTranslations.clear();
		return true;
	}

	void I18nManager::shutdown()
	{
		std::unique_lock<std::shared_mutex> lock(mMutex);
		mCurrentLocale = "";
		mFallbackLocale = "";
		mTranslations.clear();
	}

	bool I18nManager::loadFromFile(const std::string& path, const std::string& locale)
	{
		try
		{
			std::ifstream file(path);
			if (!file.is_open())
			{
				APP_LOG_ERROR("[I18n Manager]: Could not open file '{}'", path);
				return false;
			}

			nlohmann::json j = nlohmann::json::parse(file);
			if (!j.is_object())
			{
				APP_LOG_ERROR("[I18n Manager]: JSON format error in file '{}': expected an object at the root", path);
				return false;
			}

			{
				std::unique_lock<std::shared_mutex> lock(mMutex);

				auto& translations = mTranslations[locale]; // 获取或创建当前语言的翻译表
				for (auto it = j.begin(); it != j.end(); ++it)
				{
					const std::string& key = it.key();
					const auto& value = it.value();

					// 处理不同类型的值
					if (value.is_string())
						translations[key] = value.get<std::string>();
					else if (value.is_null())
					{
						// 空值：可以跳过或设置为空字符串
						translations[key] = "";
						APP_LOG_WARN("[I18n Manager]: Key '{}' has null value in file '{}', set to empty string", key, path);
					}
					else if (value.is_number())
					{
						// 数字类型：转换为字符串
						translations[key] = std::to_string(value.get<double>());
						APP_LOG_WARN("[I18n Manager]: Key '{}' has numeric value in file '{}', converted to string", key, path);
					}
					else if (value.is_boolean())
					{
						// 布尔类型：转换为字符串
						translations[key] = value.get<bool>() ? "true" : "false";
						APP_LOG_WARN("[I18n Manager]: Key '{}' has boolean value in file '{}', converted to string", key, path);
					}
					else
					{
						// 复杂类型：使用 dump()
						translations[key] = value.dump();
						APP_LOG_WARN("[I18n Manager]: Key '{}' has complex value in file '{}', converted to JSON string", key, path);
					}
				}
			}
		}
		catch (const nlohmann::json::parse_error& e)
		{
			APP_LOG_ERROR("[I18n Manager]: JSON parsing error in file '{}': {}", path, e.what());
			return false;
		}
		catch (const std::exception& e)
		{
			APP_LOG_ERROR("[I18n Manager]: Error reading file '{}': {}", path, e.what());
			return false;
		}
		return true;
	}

	bool I18nManager::loadFromString(const std::string& data, const std::string& locale)
	{
		try
		{
			nlohmann::json j = nlohmann::json::parse(data);
			if (!j.is_object())
			{
				APP_LOG_ERROR("[I18n Manager]: JSON format error in provided string for locale '{}': expected an object at the root", locale);
				return false;
			}

			{
				std::unique_lock<std::shared_mutex> lock(mMutex);

				auto& translations = mTranslations[locale]; // 获取或创建当前语言的翻译表
				for (auto it = j.begin(); it != j.end(); ++it)
				{
					const std::string& key = it.key();
					const auto& value = it.value();

					// 处理不同类型的值
					if (value.is_string())
						translations[key] = value.get<std::string>();
					else if (value.is_null())
					{
						// 空值：可以跳过或设置为空字符串
						translations[key] = "";
						APP_LOG_WARN("[I18n Manager]: Key '{}' has null value in provided string for locale '{}', set to empty string", key, locale);
					}
					else if (value.is_number())
					{
						// 数字类型：转换为字符串
						translations[key] = std::to_string(value.get<double>());
						APP_LOG_WARN("[I18n Manager]: Key '{}' has numeric value in provided string for locale '{}', converted to string", key, locale);
					}
					else if (value.is_boolean())
					{
						// 布尔类型：转换为字符串
						translations[key] = value.get<bool>() ? "true" : "false";
						APP_LOG_WARN("[I18n Manager]: Key '{}' has boolean value in provided string for locale '{}', converted to string", key, locale);
					}
					else
					{
						// 复杂类型：使用 dump()
						translations[key] = value.dump();
						APP_LOG_WARN("[I18n Manager]: Key '{}' has complex value in provided string for locale '{}', converted to JSON string", key, locale);
					}
				}
			}
		}
		catch (const nlohmann::json::parse_error& e)
		{
			APP_LOG_ERROR("[I18n Manager]: JSON parsing error in provided string for locale '{}': {}", locale, e.what());
			return false;
		}
		catch (const std::exception& e)
		{
			APP_LOG_ERROR("[I18n Manager]: Error processing provided string for locale '{}': {}", locale, e.what());
			return false;
		}
		return true;
	}

	void I18nManager::setLocale(const std::string& locale)
	{
		std::unique_lock<std::shared_mutex> lock(mMutex);
		mCurrentLocale = locale;
		//// 1. 加载新语言
		//// 注意：实际项目中 load 应该在外部异步做，这里为了简单同步执行
		//bool loaded = loadLanguage(locale);

		//if (loaded)
		//{
		//	// 2. 通知监听者
		//	std::shared_lock lock(mMutex); // 读锁读取监听列表
		//	for (const auto& cb : mListeners)
		//		cb(locale);
		//}
	}

	std::string I18nManager::getLocale() const
	{
		std::shared_lock<std::shared_mutex> lock(mMutex);
		return mCurrentLocale;
	}

	std::string I18nManager::getFallbackLocale() const
	{
		std::shared_lock<std::shared_mutex> lock(mMutex);
		return mFallbackLocale;
	}

	void I18nManager::clear()
	{
		std::unique_lock<std::shared_mutex> lock(mMutex);
		mTranslations.clear();
	}

	void I18nManager::removeLocale(const std::string& locale)
	{
		std::unique_lock<std::shared_mutex> lock(mMutex);

		if (mTranslations.erase(locale) > 0)
		{
			APP_LOG_INFO("[I18n Manager]: Removed locale '{}'", locale);

			// 如果移除的是当前语言，切换到回退语言
			if (locale == mCurrentLocale && !mTranslations.empty())
			{
				mCurrentLocale = mFallbackLocale;
				APP_LOG_INFO("[I18n Manager]: Current locale '{}' removed, switched to fallback locale '{}'", locale, mFallbackLocale);
			}
		}
	}

	std::string_view I18nManager::getRaw(const std::string& key) const
	{
		std::shared_lock<std::shared_mutex> lock(mMutex); // 读锁，允许多个线程同时翻译

		// 1. 查找当前语言
		auto localeIt = mTranslations.find(mCurrentLocale);
		if (localeIt != mTranslations.end())
		{
			const auto& translations = localeIt->second;
			auto transIt = translations.find(key);
			if (transIt != translations.end())
				return transIt->second;
		}

		// 2. 查找回退语言
		if (mCurrentLocale != mFallbackLocale)
		{
			localeIt = mTranslations.find(mFallbackLocale);
			if (localeIt != mTranslations.end())
			{
				const auto& translations = localeIt->second;
				auto transIt = translations.find(key);
				if (transIt != translations.end())
					return transIt->second;
			}
		}
		//auto fit = mFallbackTranslations.find(key);
		//if (fit != mFallbackTranslations.end())
		//	return fit->second;

		// 3. 都没有，返回 key 本身作为兜底
		return key; // 注意：string_view 指向临时对象是危险的，但这里传入的 key 调用者通常持有
	}

	bool I18nManager::hasKey(const std::string& key, const std::string& locale) const
	{
		std::shared_lock<std::shared_mutex> lock(mMutex);

		std::string targetLocale = mCurrentLocale;
		if (locale != "")
			targetLocale = locale;
		
		// 检查指定语言
		auto localeIt = mTranslations.find(targetLocale);
		if (localeIt != mTranslations.end())
			return localeIt->second.find(key) != localeIt->second.end();

		return false;
	}

	//void I18nManager::subscribe(std::function<void(const std::string&)> cb)
	//{
	//	std::unique_lock lock(mMutex);
	//	mListeners.push_back(cb);
	//}
}