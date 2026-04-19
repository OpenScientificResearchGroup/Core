/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "virCommandBase.hpp"
#include "Data/virNodeBase.hpp"

namespace core
{
	template <typename T>
	class SetProperty : public CommandBase
	{
	public:
		SetProperty(NodeBase* node, const std::string& key, const T& newValue)
			: mNode(node), mKey(key), mNewValue(newValue)
		{
			// 在构造时记录旧值，用于 Undo
			if (auto prop = mNode->selectProperty<T>(mKey))
				mOldValue = prop->get();
		}

		// --- 核心逻辑 ---
		bool execute() override
		{
			if (!mNode) return false;

			TransactionGuard guard(mNode);
			mNode->updateProperty<T>(mKey, mNewValue);
			return true;
		}

		void undo() override
		{
			TransactionGuard guard(mNode);
			mNode->updateProperty<T>(mKey, mOldValue);
		}

		// --- 性能优化：合并连续的修改 ---
		// 场景：滑块从 1.0 拖到 1.1, 1.2, ..., 2.0
		bool mergeWith(const CommandBase* other) override
		{
			// 检查 ID 是否一致（同一类命令）
			if (other->getId() != this->getId()) return false;

			auto otherCmd = static_cast<const SetPropertyCommand<T>*>(other);

			// 检查是否操作的是同一个 Node 的同一个 Key
			if (otherCmd->mNode == this->mNode && otherCmd->mKey == this->mKey)
			{
				// 更新新值，但保留最初的旧值
				this->mNewValue = otherCmd->mNewValue;
				return true;
			}
			return false;
		}

		std::string getName() const override
		{
			return "Modify " + mKey;
		}

		std::string getId() const override
		{
			return "SetProperty_" + mKey;
		}

	private:
		NodeBase* mNode;
		std::string mKey;
		T mOldValue;
		T mNewValue;

	};
}