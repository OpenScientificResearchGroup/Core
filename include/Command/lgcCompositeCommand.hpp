#pragma once
#include "defCoreApi.hpp"
#include "virCommandBase.hpp"

#include <vector>
#include <memory>
#include <string>

namespace core
{
	class CORE_API CompositeCommand : public CommandBase
	{
	public:
		CompositeCommand(const std::string& name = "Composite");

		~CompositeCommand();

		// =========================================================
		// 【关键修复】显式删除拷贝，启用移动
		// =========================================================

		// 1. 禁止拷贝构造 (因为 unique_ptr 不能拷贝)
		CompositeCommand(const CompositeCommand&) = delete;

		// 2. 禁止拷贝赋值 (防止编译器生成导致报错的代码)
		CompositeCommand& operator=(const CompositeCommand&) = delete;

		// 3. 允许移动构造 (将所有权转移，这是 unique_ptr 支持的)
		CompositeCommand(CompositeCommand&&) = default;

		// 4. 允许移动赋值
		CompositeCommand& operator=(CompositeCommand&&) = default;

		// =========================================================

		void append(std::unique_ptr<CommandBase> cmd);

		bool isEmpty() const;

		bool execute() override;

		void undo() override;

		void redo() override;

		std::string getLabel() const override;

		void setLabel(const std::string& name);

	private:
		std::vector<std::unique_ptr<CommandBase>> mCmds;
		std::string mName;

	};
} // namespace core