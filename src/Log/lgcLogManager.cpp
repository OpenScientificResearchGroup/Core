/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Log/lgcLogManager.hpp"
#include <filesystem> // C++17 标准库，用于创建目录
#include <mutex>
#include <vector>

// spdlog sinks
#include "spdlog/async.h"						// 引入异步头文件
#include "spdlog/sinks/msvc_sink.h"				// VS 输出窗口
#include "spdlog/sinks/rotating_file_sink.h"	// 轮转文件
#include "spdlog/sinks/stdout_color_sinks.h"	// 控制台颜色

// Windows API (用于分配控制台)
#ifdef _WIN32
#include <windows.h>
#endif

namespace core
{
	// 静态成员初始化
	std::shared_ptr<spdlog::logger> LogManager::mInstance;
	std::mutex LogManager::mInitMutex; // 定义互斥锁

	//LogManager::LogManager()
	//	: LogManager("logs", "app.log", 5 * 1024 * 1024, 3, true)
	//{
	//	// 默认构造函数不做任何事
	//}

	//LogManager::LogManager(std::string dir, std::string name, int maxFileSize, int maxFiles, bool consoleOut)
	//{
	//	init(dir, name, maxFileSize,  maxFiles, consoleOut);
	//}

	void LogManager::init(const std::string& path, const size_t& maxFileSize, const size_t& maxFiles, const bool& consoleOut)
	{
		// 加锁，防止多线程同时 init
		std::lock_guard<std::mutex> lock(mInitMutex);

		// 如果已经存在，先执行关闭操作，允许重新加载配置
		if (mInstance)
		{
			APP_LOG_INFO("[Log Manager]: Restart Log Manager.");
			spdlog::shutdown();
		}

		// 1. 确保日志目录存在 (健壮性)
		try
		{
			std::filesystem::path logPath(path);
			if (!std::filesystem::exists(logPath.parent_path()))
				std::filesystem::create_directories(logPath.parent_path());
		}
		catch (const std::exception& e)
		{
			// 如果目录创建失败，至少输出到 stderr
			fprintf(stderr, "Failed to create log directory: %s\n", e.what());
		}
		// 0. 初始化线程池 (队列大小 8192，后台线程数 1)
		spdlog::init_thread_pool(8192, 1);

		// 准备 sinks 列表
		std::vector<spdlog::sink_ptr> sinks;

		// --- 1. 文件日志 (Debug 和 Release 都要) ---
		// max_size: 5MB, max_files: 3
		auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(path, maxFileSize, maxFiles);
		// auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_st>("logs/app.log", 1024 * 1024 * 5, 3);
		fileSink->set_pattern("[%l] [%t] [%Y-%m-%d %H:%M:%S.%e] %v"); // 文件不需要颜色
		sinks.push_back(fileSink);

		// --- 2. Debug 模式特有配置 ---
		if (consoleOut)
		{
#if defined(_WIN32) && defined(_DEBUG)
			// A. 分配控制台窗口 (重点!)
			if (AllocConsole())
			{
				// 重定向 stdout / stderr 到新控制台
				FILE* fp = nullptr;
				freopen_s(&fp, "CONOUT$", "w", stdout);
				freopen_s(&fp, "CONOUT$", "w", stderr);

				// 设置控制台代码页为 UTF-8 (防止中文乱码)
				SetConsoleOutputCP(CP_UTF8);
			}
#endif
			// B. 添加控制台 Sink
			auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			// console_sink->set_pattern("%^[%H:%M:%S] [%t] %v%$"); // 只有时间，更简洁，带颜色
			console_sink->set_pattern("[%l] [%t] [%Y-%m-%d %H:%M:%S.%e] %v %$"); // 只有时间，更简洁，带颜色
			sinks.push_back(console_sink);

#if defined(_WIN32) && defined(_DEBUG)
			//// C. 添加 VS 输出窗口 Sink (OutputDebugString)
			//auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
			//msvc_sink->set_pattern("[App] %v"); // 在 VS 输出窗口加个前缀
			//sinks.push_back(msvc_sink);
#endif
		}
		// --- 3. 创建并注册 LogManager ---
		// mInstance = std::make_shared<spdlog::logger>("App", sinks.begin(), sinks.end());

		mInstance = std::make_shared<spdlog::async_logger>(
			"App",
			sinks.begin(), sinks.end(),
			spdlog::thread_pool(),
			spdlog::async_overflow_policy::block // 队列满了阻塞主线程(保证不丢日志)
		);

		// 设置全局级别
#ifdef _DEBUG
		mInstance->set_level(spdlog::level::trace); // Debug 看所有
		mInstance->flush_on(spdlog::level::trace);  // 立即刷新，防崩溃丢失日志
#else
		mInstance->set_level(spdlog::level::info);  // Release 只看 info
		mInstance->flush_on(spdlog::level::warn);   // 警告以上才立即刷新
#endif
		// 【关键特性】开启 Backtrace 支持
		// 允许存储最近 32 条日志，当发生 CRITICAL 错误时自动 Dump 出来
		mInstance->enable_backtrace(32);

		// 定期刷新：每 3 秒强制落盘一次
		// 这样即使程序意外崩溃（未调用 shutdown），也能保留崩溃前3秒内的日志
		spdlog::flush_every(std::chrono::seconds(3));
		// 注册为默认，方便第三方库如果也用 spdlog
		spdlog::register_logger(mInstance);
		spdlog::set_default_logger(mInstance);
		APP_LOG_INFO("[Log Manager]: Log Manager initialized successfully.");
	}

	void LogManager::shutdown()
	{
		std::lock_guard<std::mutex> lock(mInitMutex);

		if (mInstance)
		{
			mInstance->dump_backtrace();
			// 在多线程环境下，如果有其他地方持有 shared_ptr，reset 只是减少引用计数
			// spdlog::shutdown() 才是真正的停止所有日志活动
			spdlog::shutdown();
			mInstance.reset();
		}
#if defined(_WIN32) && defined(_DEBUG)		// 释放控制台资源
		FreeConsole();
#endif
	}

	std::shared_ptr<spdlog::logger> LogManager::get()
	{
		// 如果未初始化，返回一个空的或者默认的，防止崩溃
		if (!mInstance)
			// 【关键修改】
			// 不能直接 return spdlog::default_logger(); 
			// 必须先把它赋值给 mInstance，让它变成一个持久存在的对象（左值）
			return spdlog::default_logger();
		return mInstance;
	}

	LogManager::~LogManager()
	{
		// 确保资源释放
		shutdown();
	}
}
