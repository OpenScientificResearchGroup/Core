/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Command/lgcCommandManager.hpp"

#include "Log/lgcLogManager.hpp"

namespace core
{
	CommandManager::CommandManager(const size_t& maxHistory)
		: mMaxHistory(maxHistory), mCurrentIndex(-1), mSavePointIndex(-1), mIsInTransaction(false)
	{

	}

	CommandManager::~CommandManager()
	{
		// unique_ptr 会自动清理，但需要在 DLL 内部清理，
		// 保证使用 DLL 的堆管理器进行 delete
		shutdown();
	}

	bool CommandManager::init(const size_t& maxHistory)
	{
		mMaxHistory = maxHistory;
		mCurrentIndex = -1;
		mSavePointIndex = -1;
		mIsInTransaction = false;

		return true;
	}

	void CommandManager::shutdown()
	{
		clear();
	}

	void CommandManager::submit(std::unique_ptr<CommandBase> cmd)
	{
		if (!cmd) return;

		// 0. 异常安全保护
		try
		{
			// 1. 尝试执行 (如果已经在事务中，通常假设调用者已经执行过，或者在此处执行)
			if (!cmd->execute())
			{
				if (mIsInTransaction)
					mPendingTransaction->undo(); // 如果执行失败，撤销之前已经执行的命令，保持事务的一致性
				return;
			}
		}
		catch (const std::exception& e)
		{
			APP_LOG_ERROR("[Command Manager]: Failed to execute command '{}': {}", cmd->getName(), e.what());
			return;
		}

		// --- 事务处理 ---
		if (mIsInTransaction)
		{
			if (mPendingTransaction)
				mPendingTransaction->append(std::move(cmd));
			return; // 事务中不触发回调，也不入主历史栈
		}

		if (cmd->allowStack())
		{
			// 2. 尝试合并 (Merge)
			// 检查 mCurrentIndex 是否有效，且指向栈顶
			if (mCurrentIndex >= 0 && mCurrentIndex < mHistory.size())
			{
				CommandBase* lastCmd = mHistory[mCurrentIndex].get();
				if (lastCmd->mergeWith(cmd.get()))
				{
					// 合并成功，cmd 自动销毁
					//if (mStateChangeCallback) mStateChangeCallback();
					return;
				}
			}

			// 3. 清空 Redo 部分 (trim history)
			// 如果当前不在末尾，提交新命令会丢失所有 Redo 的机会
			trimHistory();

			// 4. 入栈
			mHistory.push_back(std::move(cmd));
			mCurrentIndex++;

			// 5. 限制大小 (Deque pop_front 效率高)
			if (mHistory.size() > mMaxHistory)
			{
				mHistory.pop_front();
				mCurrentIndex--; // 因为删除了前面的，索引要减 1
				mSavePointIndex--; // 保存点索引也要减
			}
		}

		//if (mStateChangeCallback) mStateChangeCallback();
	}

	void CommandManager::beginTransaction(const std::string& name)
	{
		if (mIsInTransaction)
		{
			// 不支持嵌套事务（或者你可以实现栈式嵌套）
			APP_LOG_ERROR("[Command Manager]: beginTransaction() called while already in a transaction.");
			return;
		}
		mIsInTransaction = true;
		mPendingTransaction = std::make_unique<CompositeCommand>(name);
	}

	void CommandManager::endTransaction()
	{
		if (!mIsInTransaction) 
		{
			APP_LOG_ERROR("[Command Manager]: endTransaction() called without a matching beginTransaction.");
			return;
		}
		mIsInTransaction = false;

		if (mPendingTransaction && !mPendingTransaction->isEmpty())
		{
			// 递归调用 submit，但这次 mIsInTransaction 为 false，会进入主逻辑
			// CompositeCommand 的 execute 通常只是返回 true (因为内部命令都已经执行过了)
			// 需要调整 CompositeCommand::execute 的逻辑避免重复执行

			// 修正：这里直接入栈逻辑，绕过 submit 的 execute 调用
			// 或者让 CompositeCommand::execute 变为空操作，或者加个 flag

			// 最简单的做法：
			trimHistory();
			mHistory.push_back(std::move(mPendingTransaction));
			mCurrentIndex++;

			// 限制大小 (Deque pop_front 效率高)
			if (mHistory.size() > mMaxHistory)
			{
				mHistory.pop_front();
				mCurrentIndex--; // 因为删除了前面的，索引要减 1
				mSavePointIndex--; // 保存点索引也要减
			}
			//if (mStateChangeCallback) mStateChangeCallback();
		}
		mPendingTransaction.reset();
	}

	void CommandManager::undo()
	{
		if (!allowUndo()) return;

		try
		{
			mHistory[mCurrentIndex]->undo();
			mCurrentIndex--;
		}
		catch (...)
		{
			APP_LOG_ERROR("[Command Manager]: Failed to undo command '{}'.", mHistory[mCurrentIndex]->getName());
		}

		//if (mStateChangeCallback) mStateChangeCallback();
	}

	void CommandManager::redo()
	{
		if (!allowRedo()) return;

		try
		{
			mCurrentIndex++;
			mHistory[mCurrentIndex]->redo();
		}
		catch (...)
		{
			APP_LOG_ERROR("[Command Manager]: Failed to redo command '{}'.", mHistory[mCurrentIndex]->getName());
		}

		//if (mStateChangeCallback) mStateChangeCallback();
	}

	void CommandManager::trimHistory()
	{
		// 移除 mCurrentIndex 之后的所有元素
		while (mHistory.size() > mCurrentIndex + 1)
			mHistory.pop_back();

		// 如果由于 Trim 导致 SavePoint 被删除了（说明用户撤销到了保存点之前，然后又做了新操作）
		// 那么 SavePoint 就失效了（或者意味着永远回不到已保存状态）
		if (mSavePointIndex > mCurrentIndex) mSavePointIndex = -2; // 无效值
	}

	bool CommandManager::allowUndo() const
	{
		return mCurrentIndex >= 0;
	}

	bool CommandManager::allowRedo() const
	{
		return mCurrentIndex < static_cast<int>(mHistory.size()) - 1;
	}

	void CommandManager::markAsSaved()
	{
		mSavePointIndex = mCurrentIndex;
		//if (mStateChangeCallback) mStateChangeCallback();
	}

	bool CommandManager::isDirty() const
	{
		return mCurrentIndex != mSavePointIndex;
	}

	void CommandManager::clear()
	{
		mHistory.clear();
		mCurrentIndex = -1;
		mSavePointIndex = -1;
		mMaxHistory = 0;
		//if (mStateChangeCallback) mStateChangeCallback();
	}

	std::string CommandManager::getUndoName() const
	{
		if (allowUndo()) return mHistory[mCurrentIndex]->getName();
		return "";
	}

	std::string CommandManager::getRedoName() const
	{
		if (allowRedo()) return mHistory[mCurrentIndex + 1]->getName();
		return "";
	}

	//void CommandManager::setStateChangeCallback(std::function<void()> callback) {
	//	mStateChangeCallback = callback;
	//}
} // namespace core
