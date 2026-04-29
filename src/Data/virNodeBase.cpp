/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Data/virNodeBase.hpp"

#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace core
{
	NodeBase::NodeBase()
		:PropertyContainerBase()
	{
		// NodeBase 只通过父节点链传递事件，不保存 Document 指针。
		insertProperty<std::string>("uuid", "");
		insertProperty<std::string>("name", "");
		insertProperty<std::string>("type", "");
		insertProperty<long long>("timestamp", 0);
	}

	ObjectType NodeBase::getObjectType() const
	{
		return ObjectType::NODE;
	}

	//bool NodeBase::read(const nlohmann::json& j)
	//{
	//	for (const auto& [key, value] : j.items())
	//	{
	//		auto it = mProperties.find(key);
	//		if (it != mProperties.end())
	//		{
	//			if (it->second)
	//				it->second->read(value);
	//		}
	//		else
	//		{
	//			std::unique_ptr<PropertyBase> newProp;

	//			if (value.is_boolean())
	//				newProp = std::make_unique<Property<bool>>(false);
	//			else if (value.is_number_integer())
	//				newProp = std::make_unique<Property<int>>(0);
	//			else if (value.is_number_float())
	//				newProp = std::make_unique<Property<double>>(.0f);
	//			else if (value.is_string())
	//				newProp = std::make_unique<Property<std::string>>("");

	//			if (newProp)
	//			{
	//				newProp->read(value);
	//				mProperties[key] = std::move(newProp);
	//			}
	//		}
	//	}
	//	return true;
	//}

	//nlohmann::json NodeBase::write() const
	//{
	//	nlohmann::json j;
	//	for (const auto& [key, prop] : mProperties)
	//		if (prop)
	//			j[key] = prop->write();
	//	return j;
	//}

	void NodeBase::setUuid(const std::string& uuid)
	{
		updateProperty<std::string>("uuid", uuid);
	}

	const std::string& NodeBase::getUuid() const
	{
		return selectProperty<std::string>("uuid")->get();
	}

	void NodeBase::setName(const std::string& name)
	{
		updateProperty<std::string>("name", name);
	}

	const std::string& NodeBase::getName() const
	{
		return selectProperty<std::string>("name")->get();
	}

	void NodeBase::setType(const std::string& type)
	{
		updateProperty<std::string>("type", type);
	}

	const std::string& NodeBase::getType() const
	{
		return selectProperty<std::string>("type")->get();
	}

	void NodeBase::setTimestamp(uint64_t timestamp)
	{
		updateProperty<long long>("timestamp", static_cast<long long>(timestamp));
	}

	const long long& NodeBase::getTimestamp() const
	{
		return selectProperty<long long>("timestamp")->get();
	}

	//const std::unordered_map<std::string, std::unique_ptr<PropertyBase>>& NodeBase::getAllAttributes() const
	//{
	//	return mAttributes;
	//}

	//PropertyBase* NodeBase::getProperty(const std::string& key) const
	//{
	//	auto it = mProperties.find(key);
	//	if (it == mProperties.end()) return false;
	//	return it->second.get();
	//}

	//bool NodeBase::deleteProperty(const std::string& key)
	//{
	//	auto it = mProperties.find(key);
	//	if (it == mProperties.end()) return false;
	//	mProperties.erase(it);
	//	return true;
	//}

	//void NodeBase::setParent(NodeBase* parent)
	//{
	//	mParent = parent;
	//}

	//NodeBase* NodeBase::getParent() const
	//{
	//	return mParent;
	//}

	void NodeBase::onUpdate(ObjectBase* obj)
	{
		static_cast<PropertyContainerBase*>(mParent)->onUpdate(obj);
	}

	void NodeBase::onAttach(ObjectBase* obj)
	{
		static_cast<PropertyContainerBase*>(mParent)->onAttach(obj);
	}

	void NodeBase::onDetach(ObjectBase* obj)
	{
		static_cast<PropertyContainerBase*>(mParent)->onDetach(obj);
	}

	void NodeBase::onLink(ObjectBase* obj)
	{
		static_cast<PropertyContainerBase*>(mParent)->onLink(obj);
	}

	void NodeBase::onUnlink(ObjectBase* obj)
	{
		static_cast<PropertyContainerBase*>(mParent)->onUnlink(obj);
	}

	void NodeBase::attach()
	{
		static_cast<PropertyContainerBase*>(mParent)->onAttach(this);
	}

	void NodeBase::detach()
	{
		static_cast<PropertyContainerBase*>(mParent)->onDetach(this);
	}
}