/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "virNodeBase.hpp"

#include <string>
#include <unordered_map>
#include <memory>

#include <nlohmann/json.hpp>

namespace core
{
	class NodeSetBase : public NodeBase
	{
	public:
		NodeSetBase() = default;
		virtual ~NodeSetBase() = default;

		virtual bool read(const nlohmann::json& j) override;
		virtual nlohmann::json write() const override;

		ObjectType getObjectType() const override;

		const std::unordered_map<std::string, std::unique_ptr<NodeBase>>& getAllNodes() const;
		bool insertNode(std::unique_ptr<NodeBase> node);

		template <typename T>
		T* selectNode(const std::string& uuid)
		{
			auto it = mNodes.find(uuid);
			if (it == mNodes.end())
				return nullptr;
			return dynamic_cast<T*>(it->second.get());
		}

		bool deleteNode(const std::string& uuid);

		template <typename T>
		T* findNode(const std::vector<std::string>& path)
		{
			if (path.empty())
				return dynamic_cast<T*>(this);

			NodeSetBase* currentSet = this;
			NodeBase* lastFound = nullptr;

			for (size_t i = 0; i < path.size(); ++i)
			{
				auto it = currentSet->mNodeSets.find(path[i]);
				if (it == currentSet->mNodeSets.end())
					return nullptr;

				lastFound = it->second.get();

				// 如果还没到路径最后一段，需要切换当前 Set
				if (i < path.size() - 1)
				{
					currentSet = dynamic_cast<NodeSetBase*>(lastFound);
					if (!currentSet)
						return nullptr; // 路径中间出现了非容器节点
				}
			}

			return dynamic_cast<T*>(lastFound);
		}

	private:
		std::unordered_map<std::string, std::unique_ptr<NodeBase>> mNodes;
		std::unordered_map<std::string, std::unique_ptr<NodeSetBase>> mNodeSets;

	};
} // namespace core