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
		// 例如：用户拖动滑块调整亮度，产生了 50 个命令，
		// 我们希望这 50 个命令合并成 1 个，否则用户要按 50 次 Ctrl+Z。
		// 【修改说明】：这里不能使用 std::unique_ptr<CommandBase> (按值传递)，
		// 也不建议使用 const std::unique_ptr<CommandBase>& (引用传递)。
		// 使用 const CommandBase* (裸指针) 是最佳实践。
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