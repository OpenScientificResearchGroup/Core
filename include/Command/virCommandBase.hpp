/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "defCoreApi.hpp"

#include <memory>
#include <string>

namespace core
{
	class CommandBase
	{
	public:
		CommandBase() = default;
		virtual ~CommandBase() = default;

		CommandBase(const CommandBase&) = delete; // 基类禁用拷贝，防止切片问题
		CommandBase& operator=(const CommandBase&) = delete;
		CommandBase(CommandBase&&) = default; // 默认构造和移动通常需要保留
		CommandBase& operator=(CommandBase&&) = default;

		// --- 1. 核心逻辑 ---
		// 返回 bool：因为执行可能会失败（例如磁盘满了、内存不够、文件被占用）
		// 如果返回 false，命令历史栈不应记录此命令
		// NodeBase::execute()：负责“如何计算”（数据驱动，逻辑核心）
		// Command::execute()：负责“如何改变状态”（动作分发，撤销栈核心）
		virtual bool execute() = 0;

		virtual void undo() = 0;

		// 通常 Redo 和 Execute 逻辑是一样的，但也可能有细微区别
		// 默认实现可以是直接调用 Execute()
		virtual void redo() { execute(); }

		// --- 2. UI 交互支持 ---
		// 获取命令名称，用于在菜单中显示。
		// 例如：菜单显示 "撤销 移动对象" (Undo Move Object)
		virtual std::string getName() const = 0;

		// --- 3. 性能优化 (命令合并) ---
		// 用于将连续的微小操作合并为一个。
		virtual bool mergeWith(const CommandBase* other) { return false; }

		// --- 4. 识别与分类 ---
		// 获取命令的唯一 ID 或类型。
		// 用于判断两个命令是否属于同一种类，辅助 MergeWith 判断。
		virtual std::string getId() const = 0;

		// --- 5. 内存管理 (可选，针对大型数据) ---
		// 返回该命令占用的内存大小（估算）。
		// 如果历史栈占用内存过大（如 500MB），管理器可以根据这个值清理最早的命令。
		virtual size_t getSizeInBytes() const { return sizeof(*this); }

		// --- 6. 控制是否入栈 ---
		// 有些命令不适合入栈（如保存命令），可以重载此方法返回 false
		virtual bool allowStack() const { return true; }

	};
}// namespace core