/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Data/lgcGroup.hpp"

#include <memory>

#include <nlohmann/json.hpp>

namespace core
{
	bool Group::read(const nlohmann::json& j)
	{
		nlohmann::json propJ = j["properties"];
		for (const auto& [key, value] : propJ.items())
		{
			auto it = mProperties.find(key);
			if (it != mProperties.end())
			{
				if (it->second)
					it->second->read(value);
			}
			else
			{
				std::unique_ptr<PropertyBase> newProp;

				if (value.is_boolean())
					newProp = std::make_unique<Property<bool>>(false);
				else if (value.is_number_integer())
					newProp = std::make_unique<Property<int>>(0);
				else if (value.is_number_float())
					newProp = std::make_unique<Property<double>>(.0f);
				else if (value.is_string())
					newProp = std::make_unique<Property<std::string>>("");

				if (newProp)
				{
					newProp->read(value);
					mProperties[key] = std::move(newProp);
				}
			}
		}
		nlohmann::json nodeJ = j["nodes"];
		for (const auto& [key, value] : nodeJ.items())
		{
			std::unique_ptr<NodeBase> newNode = std::make_unique<Group>();
			newNode->read(value);
			mNodes[key] = std::move(newNode);
		}
		return true;
	}

	nlohmann::json Group::write() const
	{
		nlohmann::json j;
		nlohmann::json propJ;
		for (const auto& [key, prop] : mProperties)
			if (prop)
				propJ[key] = prop->write();
		nlohmann::json nodeJ;
		for (const auto& [key, node] : mNodes)
			if (node)
				nodeJ[key] = node->write();
		j["properties"] = propJ;
		j["nodes"] = nodeJ;
		return j;
	}

	const std::unordered_map<std::string, std::unique_ptr<NodeBase>>& Group::getAllNodes() const
	{
		return mNodes;
	}

	bool Group::insertNode(std::unique_ptr<NodeBase> node)
	{
		auto it = mNodes.find(node->getUuid());
		if (it != mNodes.end()) return false; // 节点已存在，不添加
		node->setParent(this);
		mNodes[node->getUuid()] = std::move(node);
		return true;
	}

	bool Group::deleteNode(const std::string& uuid)
	{
		auto it = mNodes.find(uuid);
		if (it == mNodes.end()) return false;
		mNodes.erase(it);
		return true;
	}
}