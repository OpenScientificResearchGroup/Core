/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Data/virNodeSetBase.hpp"

#include <memory>

#include <nlohmann/json.hpp>

#include "Data/lgcAttribute.hpp"
#include "Data/lgcAttributeGroup.hpp"

namespace core
{
    bool NodeSetBase::read(const nlohmann::json &j)
    {
        for (const auto& [key, value] : j.items())
        {
            auto it = mProperties.find(key);
            if (it != mProperties.end())
            {
                if (it->second)
                    it->second->read(value);
            }
            else
            {
                if (value.is_object())
                {
                    std::unique_ptr<AttributeGroup> newAttributeGroup;
                    newAttributeGroup = std::make_unique<AttributeGroup>(this);
                    if (newAttributeGroup)
                    {
                        newAttributeGroup->setName(key);
                        newAttributeGroup->setParent(this);
                        newAttributeGroup->read(value);
                        mPropertySets[key] = std::move(newAttributeGroup);
                    }
                }
                else
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
        }
        nlohmann::json nodeJ = j["nodes"];
        for (const auto &node : nodeJ)
        {
            std::unique_ptr<NodeBase> newNode = std::make_unique<NodeSetBase>();
            newNode->read(node);
            mNodes[newNode->getUuid()] = std::move(newNode);
        }
        return true;
    }

    nlohmann::json NodeSetBase::write() const
    {
        nlohmann::json j;
        for (const auto &[key, prop] : mProperties)
            if (prop)
                j[key] = prop->write();
        for (const auto& [name, set] : mPropertySets)
            if (set)
                j[name] = set->write();
        nlohmann::json nodeJ;
        for (const auto &[key, node] : mNodes)
            if (node)
				nodeJ.push_back(node->write());
        j["nodes"] = nodeJ;
        return j;
    }

    ObjectType NodeSetBase::getObjectType() const
    {
		return ObjectType::NODE_SET;
    }

    const std::unordered_map<std::string, std::unique_ptr<NodeBase>> & NodeSetBase::getAllNodes() const
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

    bool NodeSetBase::deleteNode(const std::string &uuid)
    {
        auto it = mNodes.find(uuid);
        if (it == mNodes.end())
            return false;
        mNodes.erase(it);
        return true;
    }
}