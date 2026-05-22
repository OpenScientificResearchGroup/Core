/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Core/Data/virNodeSetBase.hpp"

#include <memory>

#include <nlohmann/json.hpp>

#include "Core/Service/lgcServiceManager.hpp"
#include "Core/Data/lgcAttribute.hpp"
#include "Core/Data/lgcAttributeGroup.hpp"

namespace core
{
	NodeSetBase::NodeSetBase()
		: NodeBase()
	{

	}

	NodeSetBase::~NodeSetBase() = default;

	bool NodeSetBase::read(const nlohmann::json& j)
	{
		for (const auto& [key, value] : j.items())
		{
			auto propIt = mProperties.find(key);
			if (propIt != mProperties.end())
			{
				if (propIt->second)
					propIt->second->read(value);
				continue;
			}
			auto setIt = mPropertySets.find(key);
			if (setIt != mPropertySets.end())
			{
				if (setIt->second)
					setIt->second->read(value);
				continue;
			}

			if (value.is_object())
			{
				std::unique_ptr<AttributeGroup> newAttributeGroup;
				newAttributeGroup = std::make_unique<AttributeGroup>();
				if (newAttributeGroup)
				{
					newAttributeGroup->setParent(this);
					newAttributeGroup->setName(key);
					newAttributeGroup->read(value);
					mPropertySets[key] = std::move(newAttributeGroup);
				}
			}
			else if (value.is_primitive())
			{
				std::unique_ptr<PropertyBase> newAttribute;
				if (value.is_boolean())
					newAttribute = std::make_unique<Attribute<bool>>(this, key, false);
				else if (value.is_number_integer())
					newAttribute = std::make_unique<Attribute<int>>(this, key, 0);
				else if (value.is_number_float())
					newAttribute = std::make_unique<Attribute<double>>(this, key, .0f);
				else if (value.is_string())
					newAttribute = std::make_unique<Attribute<std::string>>(this, key, "");

				if (newAttribute)
				{
					newAttribute->read(value);
					mProperties[key] = std::move(newAttribute);
				}
			}
		}
		const nlohmann::json& nodeJ = j["node"];
		for (const auto& node : nodeJ)
		{
			auto& srvMgr = ServiceManager::get();
			std::unique_ptr<NodeBase> newNode;
			auto cmdPtr = srvMgr.getNamedService<std::function<std::unique_ptr<core::NodeBase>(core::ObjectBase*)>>(node["type"].get<std::string>());
			if (cmdPtr) newNode = (*cmdPtr)(this);
			newNode->read(node);
			mNodes[newNode->getUuid()] = std::move(newNode);
		}
		const nlohmann::json& nodeSetJ = j["node_set"];
		for (const auto& nodeSet : nodeSetJ)
		{
			auto& srvMgr = ServiceManager::get();
			std::unique_ptr<NodeSetBase> newNodeSet;
			auto cmdPtr = srvMgr.getNamedService<std::function<std::unique_ptr<core::NodeSetBase>(core::ObjectBase*)>>(nodeSet["type"].get<std::string>());
			if (cmdPtr) newNodeSet = (*cmdPtr)(this);
			newNodeSet->read(nodeSet);
			mNodeSets[newNodeSet->getUuid()] = std::move(newNodeSet);
		}
		return true;
	}

	nlohmann::json NodeSetBase::write() const
	{
		nlohmann::json j;
		for (const auto& [key, prop] : mProperties)
			if (prop)
				j[key] = prop->write();
		for (const auto& [name, set] : mPropertySets)
			if (set)
				j[name] = set->write();
		nlohmann::json nodeJ = nlohmann::json::array();
		for (const auto& [key, node] : mNodes)
			if (node)
				nodeJ.push_back(node->write());
		j["node"] = nodeJ;
		nlohmann::json nodeSetJ = nlohmann::json::array();;
		for (const auto& [key, nodeSet] : mNodeSets)
			if (nodeSet)
				nodeSetJ.push_back(nodeSet->write());
		j["node_set"] = nodeSetJ;
		return j;
	}

	ObjectType NodeSetBase::getObjectType() const
	{
		return ObjectType::NODE_SET;
	}

	const std::unordered_map<std::string, std::unique_ptr<NodeBase>>& NodeSetBase::getNodes() const
	{
		return mNodes;
	}

	bool NodeSetBase::insertNode(std::unique_ptr<NodeBase> node)
	{
		auto it = mNodes.find(node->getUuid());
		if (it != mNodes.end())
			return false; // 节点已存在，不添加
		node->setParent(this);
		mNodes[node->getUuid()] = std::move(node);
		return true;
	}

	bool NodeSetBase::deleteNode(const std::string& uuid)
	{
		auto it = mNodes.find(uuid);
		if (it == mNodes.end())
			return false;
		mNodes.erase(it);
		return true;
	}

	const std::unordered_map<std::string, std::unique_ptr<NodeSetBase>>& NodeSetBase::getNodeSets() const
	{
		return mNodeSets;
	}

	bool NodeSetBase::insertNodeSet(std::unique_ptr<NodeSetBase> nodeSet)
	{
		auto it = mNodeSets.find(nodeSet->getUuid());
		if (it != mNodeSets.end())
			return false; // 节点已存在，不添加
		nodeSet->setParent(this);
		mNodeSets[nodeSet->getUuid()] = std::move(nodeSet);
		return true;
	}

	bool NodeSetBase::deleteNodeSet(const std::string& uuid)
	{
		auto it = mNodeSets.find(uuid);
		if (it == mNodeSets.end())
			return false;
		mNodeSets.erase(it);
		return true;
	}
}