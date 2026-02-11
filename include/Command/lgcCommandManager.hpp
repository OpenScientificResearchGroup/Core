#pragma once
#include "defCoreApi.hpp"
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "virCommandBase.hpp" // 你的命令基类
#include "lgcCompositeCommand.hpp"

namespace core
{
	class CORE_API CommandManager
	{
	public:
		CommandManager(size_t maxHistory = 50);
		~CommandManager();

		// 禁止拷贝 (管理 unique_ptr 的类通常不可拷贝)
		CommandManager(const CommandManager&) = delete;
		CommandManager& operator=(const CommandManager&) = delete;

		// --- 核心操作 ---

		// 提交新命令：执行、合并、入栈
		void submit(std::unique_ptr<CommandBase> cmd);

		// 撤销
		void undo();

		// 重做
		void redo();

		// --- 事务支持 (Transaction) ---
		// 开启事务：后续的 submit 不会直接进历史栈，而是进临时区
		void beginTransaction(const std::string& label);
		// 结束事务：将临时区的所有命令打包成一个 CompositeCommand 入栈
		void endTransaction();

		// 清空历史记录
		void clear();

		// --- 状态查询 ---
		bool allowUndo() const;
		bool allowRedo() const;

		// 获取当前撤销操作的名称（用于更新菜单，如 "撤销 画线"）
		std::string getUndoLabel() const;
		std::string getRedoLabel() const;

		// --- 文档状态 (Dirty/Clean) ---
		void markAsSaved(); // 比如按下 Ctrl+S 时调用
		bool isDirty() const; // 用于判断是否需要在标题栏显示星号 *

		// --- 信号/回调 (可选) ---
		// 当堆栈发生变化时调用，用于通知 UI 更新按钮状态
		// 你可以使用 std::function 或者 wxWidgets 的事件发送
		void setStateChangeCallback(std::function<void()> callback);

	private:
		// 清理 index 之后的所有历史（用于新命令提交时清空 Redo）
		void trimHistory();

		// 使用 deque 优化头部删除性能
		// 统一使用一个容器 + 索引的方式，更易于管理 SavePoint
		std::deque<std::unique_ptr<CommandBase>> mHistory;

		int mCurrentIndex = -1; // 指向“当前状态对应的命令”在数组中的位置。-1 表示初始状态
		int mSavePointIndex = -1; // 记录保存时的 index

		// 事务相关
		bool mIsInTransaction = false;
		std::unique_ptr<CompositeCommand> mPendingTransaction;

		size_t mMaxHistory;
		std::function<void()> mStateChangeCallback;

	};
} // namespace core