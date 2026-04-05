/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "defCoreApi.hpp"

#include <memory>
#include <atomic>

namespace core
{
	class CORE_API CancellationToken
	{
	public:
		CancellationToken();

		// 请求取消任务
		void cancel() const;

		// 检查任务是否已被请求取消
		[[nodiscard]] bool isCancellationRequested() const;

		// 重载 bool 操作符，方便在 if 语句中使用
		explicit operator bool() const;

	private:
		/// <summary>
		/// 当您在 submit 函数中创建 token 时，会创建一个新的 shared_ptr，指向一个新的 atomic<bool>
		/// 当这个 token 对象（及其底层的 shared_ptr）被复制到其他地方（例如，作为参数传递给任务执行函数），shared_ptr 的引用计数会增加
		/// 这意味着： 只要还有一个 CancellationToken 对象的副本存在，并且它指向的原子变量的引用计数大于零，取消状态就会保持有效
		/// 只要没有所有的令牌副本都被销毁，状态就不会丢失
		/// 这使得令牌可以安全地在多个线程间传递和共享
		/// </summary>
		std::shared_ptr<std::atomic<bool>> mIsCancelled;
	};
}
