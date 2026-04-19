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
		:ObjectBase(), mParent(nullptr)
	{
		// NodeBase 只通过父节点链传递事件，不保存 Document 指针。
		insertProperty<std::string>("uuid", std::string(""));
		//mProperties["uuid"]->setReadOnly(true);
		insertProperty<std::string>("name", std::string(""));
		insertProperty<std::string>("type", std::string(""));
		//mProperties["type"]->setReadOnly(true);
		insertProperty<long long>("timestamp", 0);
		//mProperties["timestamp"]->setReadOnly(true);
	}

	bool NodeBase::read(const nlohmann::json& j)
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
		return true;
	}

	nlohmann::json NodeBase::write() const
	{
		nlohmann::json j;
		for (const auto& [key, prop] : mProperties)
			if (prop)
				j[key] = prop->write();
		return j;
	}

	void NodeBase::setUuid(const std::string& uuid)
	{
		updateProperty<std::string>("uuid", uuid);
	}

	const std::string& NodeBase::getUuid() const
	{
		return selectProperty<Property<std::string>>("uuid")->get();
	}

	void NodeBase::setName(const std::string& name)
	{
		updateProperty<std::string>("name", name);
	}

	const std::string& NodeBase::getName() const
	{
		return selectProperty<Property<std::string>>("name")->get();
	}

	void NodeBase::setType(const std::string& type)
	{
		updateProperty<std::string>("type", type);
	}

	const std::string& NodeBase::getType() const
	{
		return selectProperty<Property<std::string>>("type")->get();
	}

	void NodeBase::setTimestamp(uint64_t timestamp)
	{
		updateProperty<long long>("timestamp", static_cast<long long>(timestamp));
	}

	const long long& NodeBase::getTimestamp() const
	{
		return selectProperty<Property<long long>>("timestamp")->get();
	}

	const std::unordered_map<std::string, std::unique_ptr<PropertyBase>>& NodeBase::getAllProperties() const
	{
		return mProperties;
	}

	PropertyBase* NodeBase::getProperty(const std::string& key) const
	{
		auto it = mProperties.find(key);
		if (it == mProperties.end()) return false;
		return it->second.get();
	}

	bool NodeBase::deleteProperty(const std::string& key)
	{
		auto it = mProperties.find(key);
		if (it == mProperties.end()) return false;
		mProperties.erase(it);
		return true;
	}

	void NodeBase::setParent(NodeBase* parent)
	{
		mParent = parent;
	}

	NodeBase* NodeBase::getParent() const
	{
		return mParent;
	}

	//void NodeBase::startTransaction()
	//{
	//	mTransactionCount++;
	//}

	//bool NodeBase::endTransaction()
	//{
	//	if (mTransactionCount <= 0) return false;
	//	mTransactionCount--;
	//	if (mTransactionCount == 0 && mIsDirty)
	//		update();
	//	return true;
	//}

	//void NodeBase::onUpdate(const std::string& key)
	//{
	//	mIsDirty = true;
	//	if (mTransactionCount > 0) return; // 事务中，不触发计算
	//	update();
	//}

	void NodeBase::onUpdate(ObjectBase* obj)
	{
		if(mParent)
			mParent->onUpdate(obj);
	}

	void NodeBase::onAttach(ObjectBase* obj)
	{
		if (mParent)
			mParent->onAttach(obj);
	}

	void NodeBase::onDetach(ObjectBase* obj)
	{
		if (mParent)
			mParent->onDetach(obj);
	}

	void NodeBase::onLink(ObjectBase* obj)
	{
		if (mParent)
			mParent->onLink(obj);
	}

	void NodeBase::onUnlink(ObjectBase* obj)
	{
		if (mParent)
			mParent->onUnlink(obj);
	}

	//void NodeBase::update()
	//{
	//	if (mParent)
	//		mParent->onUpdate(this);
	//}

	void NodeBase::attach()
	{
		if (mParent)
			mParent->onAttach(this);
	}

	void NodeBase::detach()
	{
		if (mParent)
			mParent->onDetach(this);
	}

	bool NodeBase::refresh()
	{
		//if (!mIsDirty || mTransactionCount > 0) return true; // 没脏就不动，极高提升效率
		//if (!mIsDirty) return true; // 没脏就不动，极高提升效率
		bool success = execute(); // 调用子类的具体实现
		//if (success) mIsDirty = false; // 只有成功了才清除脏标记
		return success;
	}
}