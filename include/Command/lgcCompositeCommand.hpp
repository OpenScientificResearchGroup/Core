/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
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
		CompositeCommand(const std::string& name = "Composite", const std::string& id = "/Core/composite_command");
		virtual ~CompositeCommand() = default;

		CompositeCommand(const CompositeCommand&) = delete; // 禁止拷贝构造 (因为 unique_ptr 不能拷贝)
		CompositeCommand& operator=(const CompositeCommand&) = delete; // 禁止拷贝赋值 (防止编译器生成导致报错的代码)
		CompositeCommand(CompositeCommand&&) = default; // 允许移动构造 (将所有权转移，这是 unique_ptr 支持的)
		CompositeCommand& operator=(CompositeCommand&&) = default;// 允许移动赋值

		bool execute() override;
		void undo() override;
		void redo() override;
		std::string getName() const override;
		std::string getId() const override;

		void append(std::unique_ptr<CommandBase> cmd);
		bool isEmpty() const;
		void setName(const std::string& name);
		void setId(const std::string& id);

	private:
		std::vector<std::unique_ptr<CommandBase>> mCmds;
		std::string mName, mId;

	};
} // namespace core