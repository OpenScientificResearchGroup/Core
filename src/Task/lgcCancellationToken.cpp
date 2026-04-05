/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Task/lgcCancellationToken.hpp"

namespace core
{
	CancellationToken::CancellationToken() : mIsCancelled(std::make_shared<std::atomic<bool>>(false)) {}

	void CancellationToken::cancel() const
	{
		// std::memory_order_relaxed：这是最低级别的内存排序模型
		// 在这里使用是高效且通常安全的，因为它只保证原子操作本身的可见性，而不保证与其他内存操作的顺序关系
		// 对于仅仅设置一个标志位来说，这种宽松的顺序通常足够了
		mIsCancelled->store(true, std::memory_order_relaxed);
	}

	// [[nodiscard]]：这是一个 C++ 属性，告诉编译器，如果调用者不使用这个调用的返回值（即返回的 bool 值），就应该发出警告
	// 这鼓励了调用者真正地检查取消状态
	[[nodiscard]] bool CancellationToken::isCancellationRequested() const
	{
		return mIsCancelled->load(std::memory_order_relaxed);
	}

	// 重载 bool 操作符，方便在 if 语句中使用
	CancellationToken::operator bool() const
	{
		return isCancellationRequested();
	}
}
