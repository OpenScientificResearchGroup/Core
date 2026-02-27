#include "Task/lgcTaskManager.hpp"

#include <thread>
#include <mutex>

#include "Log/lgcLogManager.hpp"

namespace core
{
	// 静态成员初始化
	// std::shared_ptr<TaskManager> TaskManager::mInstance = nullptr;
	// std::mutex TaskManager::mInitMutex;

	TaskManager& TaskManager::get()
	{
		static TaskManager instance;
		return instance;
		// 这里虽然没有加锁，但在 Init 之后通常是只读的
		// 如果必须极度安全，可以加双重检查锁，但对于高频调用的 get 影响性能
		// 工业级通常假定 init 发生在单线程阶段 (main 开始)
		//if (!mInstance)
		//	throw std::runtime_error("TaskManager not initialized! Call init() first.");
		//return *mInstance;
	}

	bool TaskManager::init(const size_t& numThreads)
	{
		//std::lock_guard<std::mutex> lock(mInitMutex);
		//if (mInstance) return; // 防止重复初始化
		int threads = numThreads;
		if (threads <= 0)
		{
			threads = std::thread::hardware_concurrency();
			// 至少保证有1个线程
			if (threads == 0)
			{
				APP_LOG_WARN("[Task Manager]: hardware_concurrency returned 0, defaulting to 1 thread.");
				threads = 1;
			}
		}
		mStop = false;
		mWorkers.reserve(numThreads);
		for (size_t i = 0; i < numThreads; ++i)
			mWorkers.emplace_back(&TaskManager::worker, this);
		{
			std::unique_lock<std::mutex> lock(mMutex);
			mTasks = std::priority_queue<Task>();
		}
		//mInstance.reset(new TaskManager(numThreads), [](TaskManager* p) {delete p; });
		APP_LOG_INFO("[Task Manager]: Task Manager initialized successfully.");
		return true;
	}

	void TaskManager::shutdown()
	{
		//std::lock_guard<std::mutex> lock(mInitMutex);
		//if (mInstance)
		//	mInstance.reset();

		mStop = true;
		mCondition.notify_all();
		for (std::thread& worker : mWorkers)
			if (worker.joinable())
				worker.join();
	}

	TaskManager::TaskManager()
		//: mStop(false)
	{
		//// 确保至少有一个线程
		//if (numThreads == 0) numThreads = 1;

		//mWorkers.reserve(numThreads);
		//for (size_t i = 0; i < numThreads; ++i)
		//	mWorkers.emplace_back(&TaskManager::worker, this);
	}

	TaskManager::~TaskManager()
	{
		shutdown();
		//{
		//	std::unique_lock<std::mutex> lock(mMutex);
		//	mStop = true;
		//}
		//mCondition.notify_all();
		//for (std::thread& worker : mWorkers)
		//	if (worker.joinable())
		//		worker.join();
	}

	void TaskManager::worker()
	{
		while (true)
		{
			Task task;
			{
				std::unique_lock<std::mutex> lock(mMutex);
				mCondition.wait(lock, [this] { return mStop || !mTasks.empty(); });
				if (mStop && mTasks.empty()) return;
				task = mTasks.top();
				mTasks.pop();
			} // 锁在这里释放，以便其他线程可以操作队列

			// 执行任务
			if (task.func)
				task.func();
		}
	}
} // namespace task
