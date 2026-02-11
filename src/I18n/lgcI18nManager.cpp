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

	void I18nManager::init(const std::string& locale, const std::string& fallbackLocale)
	{
		std::unique_lock lock(mMutex);

		mCurrentLocale = locale;
		mFallbackLocale = fallbackLocale.empty() ? "en-US" : fallbackLocale;
	}

	//void I18nManager::setLoader(std::shared_ptr<ITranslationLoader> loader)
	//{
	//	std::unique_lock lock(mMutex);
	//	mLoader = loader;
	//}

	//void I18nManager::setFallbackLocale(const std::string& locale)
	//{
	//	std::unique_lock lock(mMutex);
	//	mFallbackLocale = locale;
	//	// 可以在这里预加载 fallback 数据
	//	if (mLoader)
	//		mFallbackTranslations = mLoader->load(mFallbackLocale);
	//}

	bool I18nManager::loadFromFile(const std::string& path, const std::string& locale)
	{
		try
		{
			std::ifstream file(path);
			if (!file.is_open())
			{
				std::cerr << "无法打开文件: " << path << std::endl;
				return false;
			}

			nlohmann::json j = nlohmann::json::parse(file);
			if (!j.is_object())
			{
				std::cerr << "JSON格式错误：应为对象类型" << std::endl;
				return false;
			}

			{
				std::unique_lock lock(mMutex);

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
						std::cout << "警告: 键 '" << key << "' 的值为 null，已设置为空字符串" << std::endl;
					}
					else if (value.is_number())
					{
						// 数字类型：转换为字符串
						translations[key] = std::to_string(value.get<double>());
						std::cout << "警告: 键 '" << key << "' 的值为数字，已转换为字符串" << std::endl;
					}
					else if (value.is_boolean())
					{
						// 布尔类型：转换为字符串
						translations[key] = value.get<bool>() ? "true" : "false";
						std::cout << "警告: 键 '" << key << "' 的值为布尔值，已转换为字符串" << std::endl;
					}
					else
					{
						// 复杂类型：使用 dump()
						translations[key] = value.dump();
						std::cout << "警告: 键 '" << key << "' 的值为复杂类型，已转换为JSON字符串" << std::endl;
					}
				}
			}
		}
		catch (const nlohmann::json::parse_error& e)
		{
			std::cerr << "JSON解析错误: " << e.what() << std::endl;
			return false;
		}
		catch (const std::exception& e)
		{
			std::cerr << "文件读取错误: " << e.what() << std::endl;
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
				std::cerr << "JSON格式错误：应为对象类型" << std::endl;
				return false;
			}

			{
				std::unique_lock lock(mMutex);

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
						std::cout << "警告: 键 '" << key << "' 的值为 null，已设置为空字符串" << std::endl;
					}
					else if (value.is_number())
					{
						// 数字类型：转换为字符串
						translations[key] = std::to_string(value.get<double>());
						std::cout << "警告: 键 '" << key << "' 的值为数字，已转换为字符串" << std::endl;
					}
					else if (value.is_boolean())
					{
						// 布尔类型：转换为字符串
						translations[key] = value.get<bool>() ? "true" : "false";
						std::cout << "警告: 键 '" << key << "' 的值为布尔值，已转换为字符串" << std::endl;
					}
					else
					{
						// 复杂类型：使用 dump()
						translations[key] = value.dump();
						std::cout << "警告: 键 '" << key << "' 的值为复杂类型，已转换为JSON字符串" << std::endl;
					}
				}
			}
		}
		catch (const nlohmann::json::parse_error& e)
		{
			std::cerr << "JSON解析错误: " << e.what() << std::endl;
			return false;
		}
		catch (const std::exception& e)
		{
			std::cerr << "文件读取错误: " << e.what() << std::endl;
			return false;
		}
		return true;
	}

	void I18nManager::setLocale(const std::string& locale)
	{
		std::unique_lock lock(mMutex);
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
		std::shared_lock lock(mMutex);
		return mCurrentLocale;
	}

	std::string I18nManager::getFallbackLocale() const
	{
		std::shared_lock lock(mMutex);
		return mFallbackLocale;
	}

	void I18nManager::clear()
	{
		std::unique_lock lock(mMutex);
		mTranslations.clear();
		std::cout << "[I18n] 已清除所有翻译资源" << std::endl;
	}

	void I18nManager::removeLocale(const std::string& locale)
	{
		std::unique_lock lock(mMutex);

		if (mTranslations.erase(locale) > 0)
		{
			std::cout << "[I18n] 已移除语言: " << locale << std::endl;

			// 如果移除的是当前语言，切换到回退语言
			if (locale == mCurrentLocale && !mTranslations.empty())
			{
				mCurrentLocale = mFallbackLocale;
				std::cout << "[I18n] 当前语言 '" << locale
					<< "' 被移除，已切换到回退语言 '" << mFallbackLocale << "'" << std::endl;
			}
		}
	}

	std::string_view I18nManager::getRaw(const std::string& key) const
	{
		std::shared_lock lock(mMutex); // 读锁，允许多个线程同时翻译

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
		std::shared_lock lock(mMutex);

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