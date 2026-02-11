#pragma once
#include "defCoreApi.hpp"

#include <memory>
#include <string>

// 引入 spdlog 头文件
// 【关键】必须在引入 spdlog 头文件之前定义此宏
// 否则 SPDLOG_LOGGER_INFO 等宏无法生成文件名和行号
#ifdef _DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE 
#elif defined _RELEASE
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif
#include "spdlog/spdlog.h"

namespace core
{
	class CORE_API LogManager
	{
	public:
		LogManager() = delete;
		// explicit LogManager(std::string dir, std::string name, int maxFileSize, int maxFiles, bool consoleOut);
		LogManager(const LogManager&) = delete;
		LogManager& operator=(const LogManager&) = delete;

	
		/// <summary>
		/// 初始化日志系统
		/// </summary>
		/// <param name="path">日志路径（默认/Logs/setup.log）</param>
		/// <param name="maxFileSize">最大日志大小（默认5MB）</param>
		/// <param name="maxFiles">最大日志分段数量（默认3）</param>
		/// <param name="consoleOut">是否启用控制台实时输出日志</param>
		static void init(const std::string& path = "./Logs/setup.log", const size_t& maxFileSize = 1024 * 1024 * 5, const size_t& maxFiles = 3, const bool& consoleOut = false);

		/// <summary>
		/// 关闭日志系统（释放控制台等资源）
		/// </summary>
		static void shutdown();

		/// <summary>
		/// 获取核心 LogManager 实例（通常不需要直接调用，使用宏即可）
		/// </summary>
		/// <returns>改为按值返回，确保多线程下的绝对安全</returns>
		static std::shared_ptr<spdlog::logger> get();

	private:
		~LogManager();

		static std::shared_ptr<spdlog::logger> mInstance;
		static std::mutex mInitMutex;
	};
}

// --- 定义宏，让调用代码更短 ---
// 使用 spdlog 自带的宏 (SPDLOG_LOGGER_INFO)，这样可以自动获取文件名和行号
// 比如：[info] [main.cpp:25] Hello World
#define APP_LOG_TRACE(...)		SPDLOG_LOGGER_TRACE(core::LogManager::get(), __VA_ARGS__)
#define APP_LOG_DEBUG(...)		SPDLOG_LOGGER_DEBUG(core::LogManager::get(), __VA_ARGS__)
#define APP_LOG_INFO(...)		SPDLOG_LOGGER_INFO(core::LogManager::get(), __VA_ARGS__)
#define APP_LOG_WARN(...)		SPDLOG_LOGGER_WARN(core::LogManager::get(), __VA_ARGS__)
#define APP_LOG_ERROR(...)		SPDLOG_LOGGER_ERROR(core::LogManager::get(), __VA_ARGS__)
#define APP_LOG_CRITICAL(...)	SPDLOG_LOGGER_CRITICAL(core::LogManager::get(), __VA_ARGS__)
