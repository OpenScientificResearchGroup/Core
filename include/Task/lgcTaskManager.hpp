#pragma once
#include "defCoreApi.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

#include "lgcCancellationToken.hpp"
#include "Log/lgcLogManager.hpp"

namespace core
{
	enum class Priority
	{
		LOW,
		NORMAL,
		HIGH,
		EMERGENCY,
	};

	class CORE_API TaskManager
	{
	public:
		static TaskManager& get();

		TaskManager(const TaskManager&) = delete;
		TaskManager& operator=(const TaskManager&) = delete;

		bool init(const size_t& numThreads = std::thread::hardware_concurrency());
		void shutdown();

		/// <summary>
		/// 提交一个可取消的任务
		/// 使用 SFINAE (std::enable_if_t) 来确保只有当函数能以 CancellationToken 作为首个参数调用时，此重载才有效
		/// </summary>
		template <typename Func, typename... Args, std::enable_if_t<std::is_invocable_v<Func, CancellationToken, Args...>, int> = 0>
		auto add(Priority priority, Func&& func, Args&&... args) -> std::pair<std::future<std::invoke_result_t<Func, CancellationToken, Args...>>, CancellationToken>
		{
			// 首先根据签名推导出任务的返回值类型
			using ReturnType = std::invoke_result_t<Func, CancellationToken, Args...>;

			// 为本次提交创建了一个新的、唯一的取消令牌
			CancellationToken token;

			//// packaged_task 的作用是将可调用对象（这里是一个 Lambda）的执行结果（返回值或异常）绑定到一个 std::future 上
			//// 它内部的调用签名是 ReturnType()（不接受参数），因为参数的传递被包装在了 Lambda 内部
			//// 使用 Lambda 包装任务，处理取消逻辑
			//// 【注意】这里使用了 C++20 的初始化捕获包展开 (...args = ...)。
			//// 如果是 C++17，需改用简单的 [func, args..., token] (会导致拷贝) 或使用 tuple/apply 技巧。
			//auto taskPtr = std::make_shared<std::packaged_task<ReturnType()>>(
			//	[func = std::forward<Func>(func), ...args = std::forward<Args>(args), token]() mutable {
			//		if (token.isCancellationRequested()) {
			//			throw std::runtime_error("Task cancelled before execution");
			//		}
			//		return func(token, std::forward<Args>(args)...);
			//	}
			//);

			// 【C++17 兼容关键】
			// C++17 不支持 Lambda 中的包展开捕获 (...args = forward(args))
			// 解决方案：将参数包移动到一个 std::tuple 中捕获
			auto argsTuple = std::make_tuple(std::forward<Args>(args)...);

			auto taskPtr = std::make_shared<std::packaged_task<ReturnType()>>(
				[func = std::forward<Func>(func), token, capturedArgs = std::move(argsTuple)]() mutable {
					if (token.isCancellationRequested())
					{
						APP_LOG_INFO("[Task Manager]: Task cancelled before execution.");
						throw std::runtime_error("Task cancelled before execution");
                    }
					// 使用 std::apply 解包 tuple 并调用函数
					return std::apply([&](auto&&... unpackedArgs) {
						return func(token, std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
						}, std::move(capturedArgs));
				}
			);

			std::future<ReturnType> future = taskPtr->get_future();

			{
				std::unique_lock<std::mutex> lock(mMutex);
				if (mStop) throw std::runtime_error("TaskManager is stopped");
				mTasks.emplace(Task{ priority, [taskPtr]() { (*taskPtr)(); } });
			}
			mCondition.notify_one();

			return { std::move(future), token };
		}

		/// 提交一个不可取消的任务
		/// 使用 SFINAE 来确保只有当函数不能以 CancellationToken 作为首个参数调用时，
		/// 此重载才有效，避免了歧义。
		template <typename Func, typename... Args, std::enable_if_t<!std::is_invocable_v<Func, CancellationToken, Args...>, int> = 0>
		auto add(Priority priority, Func&& func, Args&&... args) -> std::future<std::invoke_result_t<Func, Args...>>
		{
			using ReturnType = std::invoke_result_t<Func, Args...>;

			//// 【注意】这里使用了 C++20 的初始化捕获包展开 (...args = ...)。
			//// 如果是 C++17，需改用简单的 [func, args..., token] (会导致拷贝) 或使用 tuple/apply 技巧。
			//// 使用 std::bind 绑定函数和参数，更简洁
			//auto taskPtr = std::make_shared<std::packaged_task<ReturnType()>>(
			//	[func = std::forward<Func>(func), ...args = std::forward<Args>(args)]() mutable
			//	{
			//		return func(std::forward<Args>(args)...);
			//	}
			//);

			// C++17 Trick: 同样使用 tuple 替换 C++20 的 ...args 捕获
			auto argsTuple = std::make_tuple(std::forward<Args>(args)...);

			auto taskPtr = std::make_shared<std::packaged_task<ReturnType()>>(
				[func = std::forward<Func>(func), capturedArgs = std::move(argsTuple)]() mutable {
					return std::apply([&](auto&&... unpackedArgs) {
						return func(std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
						}, std::move(capturedArgs));
				}
			);

			std::future<ReturnType> future = taskPtr->get_future();

			{
				std::unique_lock<std::mutex> lock(mMutex);
				if (mStop) throw std::runtime_error("TaskManager is stopped");
				mTasks.emplace(Task{ priority, [taskPtr]() { (*taskPtr)(); } });
			}
			mCondition.notify_one();

			return future; // 只返回 future
		}

	private:
		TaskManager(); // 构造函数私有化
		~TaskManager(); // 析构函数

		void worker();

	private:
		struct Task
		{
			Priority priority;
			std::function<void()> func;

			// 为了让 std::priority_queue 成为一个 "max-heap"
			bool operator<(const Task& other) const { return priority < other.priority; }
		};

		//static std::shared_ptr<TaskManager> mInstance;
		//static std::mutex mInitMutex;

		std::mutex mMutex;
		std::atomic<bool> mStop;
		std::priority_queue<Task> mTasks;
		std::condition_variable mCondition;
		std::vector<std::thread> mWorkers;
	};
} // namespace core
