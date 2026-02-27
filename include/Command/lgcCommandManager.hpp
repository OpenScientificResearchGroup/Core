#pragma once
#include "defCoreApi.hpp"

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "lgcCompositeCommand.hpp"
#include "virCommandBase.hpp"

namespace core
{
	class CORE_API CommandManager
	{
	public:
		CommandManager(const size_t& maxHistory = 50);
		~CommandManager();

		// 禁止拷贝 (管理 unique_ptr 的类通常不可拷贝)
		CommandManager(const CommandManager&) = delete;
		CommandManager& operator=(const CommandManager&) = delete;

		bool init(const size_t& maxHistory);
		void shutdown();

		// --- 核心操作 ---
		void submit(std::unique_ptr<CommandBase> cmd); // 提交新命令：执行、合并、入栈
		void undo(); // 撤销
		void redo(); // 重做
		void clear(); // 清空历史记录

		// --- 事务支持 (Transaction) ---
		void beginTransaction(const std::string& name); // 开启事务：后续的 submit 不会直接进历史栈，而是进临时区
		void endTransaction(); // 结束事务：将临时区的所有命令打包成一个 CompositeCommand 入栈

		// --- 状态查询 ---
		bool allowUndo() const;
		bool allowRedo() const;
		std::string getUndoName() const; // 获取当前撤销操作的名称（用于更新菜单，如 "撤销 画线"）
		std::string getRedoName() const; // 获取当前重做操作的名称（用于更新菜单，如 "重做 画线"）

		// --- 文档状态 (Dirty/Clean) ---
		void markAsSaved(); // 比如按下 Ctrl+S 时调用
		bool isDirty() const; // 用于判断是否需要在标题栏显示星号 *

		// --- 信号/回调 (可选) ---
		// 当堆栈发生变化时调用，用于通知 UI 更新按钮状态
		// 你可以使用 std::function 或者 wxWidgets 的事件发送
		//void setStateChangeCallback(std::function<void()> callback);

	private:
		// 清理 index 之后的所有历史（用于新命令提交时清空 Redo）
		void trimHistory();

	private:
		size_t mMaxHistory;
		std::deque<std::unique_ptr<CommandBase>> mHistory; // 使用 deque 优化头部删除性能,并且统一使用一个容器 + 索引的方式，更易于管理 SavePoint
		int mCurrentIndex; // 指向“当前状态对应的命令”在数组中的位置，-1 表示初始状态
		int mSavePointIndex; // 记录保存时的 index，-1 表示初始状态，-2 表示标志失效
		//std::function<void()> mStateChangeCallback;

		// 事务相关
		bool mIsInTransaction = false;
		std::unique_ptr<CompositeCommand> mPendingTransaction;

	};
} // namespace core