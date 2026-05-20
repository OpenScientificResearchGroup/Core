/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Core/Data/virPropertyBase.hpp"

#include "Core/Service/lgcServiceManager.hpp"

#include "Core/Command/virCommandBase.hpp"

#include "Core/Data/virPropertySetBase.hpp"
#include "Core/Data/virNodeBase.hpp"

namespace core
{
	PropertyBase::PropertyBase(PropertyContainerBase* propertySet, const std::string& key)
		//: mKey(key), mPropertySet(propertySet), mIsReadOnly(false), mIsVisible(true)
		: ObjectBase(propertySet), mKey(key), mIsOutput(false)
	{
		mLink.uuid.clear();
		mLink.path.clear();
		mLink.isActive = false;
	}

	ObjectType PropertyBase::getObjectType() const
	{
		return ObjectType::PROPERTY;
	}
	
	NodeBase* PropertyBase::getNode() const
	{
		ObjectBase* current = mParent;

		while (current != nullptr)
		{
			if (current->getObjectType() == ObjectType::NODE || current->getObjectType() == ObjectType::NODE_SET || current->getObjectType() == ObjectType::DOCUMENT)
				return static_cast<NodeBase*>(current);

			// 继续向上追溯
			current = current->getParent();
		}

		return nullptr;
	}

	std::vector<std::string> PropertyBase::getPath() const
	{
		std::vector<std::string> path;

		// 1. 首先放入属性自身的 Key
		path.push_back(this->mKey);

		// 2. 开始向上回溯父对象
		ObjectBase* current = mParent;

		while (current != nullptr)
		{
			// 检查是否已经到达了 Node 层级
			// 路径通常是相对于 Node 的，所以到达 Node 时停止
            if (current->getObjectType() == ObjectType::NODE || current->getObjectType() == ObjectType::NODE_SET || current->getObjectType() == ObjectType::DOCUMENT)
                break;

			// 如果是属性组（AttributeGroup/PropertySetBase），获取它的名称
			// 这里假设你在 ObjectBase 中有 getName() 或者在这些类里有对应的成员
			path.push_back(static_cast<PropertySetBase*>(current)->getName());

			// 继续向上移动
			current = current->getParent();
		}

		// 3. 因为是回溯，结果是 [属性, 子组, 父组]，需要反转
		std::reverse(path.begin(), path.end());

		return path;
	}

	PropertyContainerBase* PropertyBase::getContainer() const
	{
		return static_cast<PropertyContainerBase*>(mParent);
	}

	bool PropertyBase::isLink() const
	{
		return mLink.isActive;
	}

	void PropertyBase::setLink(const std::string& uuid, const std::string& key)
	{
		if (mLink.isActive)
			unlink();

		mLink.uuid = uuid;
		mLink.path = key;
		mLink.isActive = true;

		link();
	}

	void PropertyBase::resetLink()
	{
		if (mLink.isActive)
			unlink();

		mLink.uuid.clear();
		mLink.path.clear();
		mLink.isActive = false;
	}

	const PropertyLink& PropertyBase::getLink() const
	{
		return mLink;
	}

	bool PropertyBase::readLink(const nlohmann::json& j)
	{
		mLink.uuid = j["node"].get<std::string>();
		mLink.path = j.value("property", std::string(""));
		mLink.isActive = true;
		return true;
	}

	nlohmann::json PropertyBase::writeLink() const
	{
		nlohmann::json j;
		j["node"] = mLink.uuid;
		if (!mLink.path.empty())
			j["property"] = mLink.path;
		return j;
	}

	std::string PropertyBase::getKey() const
	{
		return mKey;
	}

	//bool PropertyBase::getDirty() const
	//{
	//	return mIsDirty;
	//}

	//void PropertyBase::setDirty(bool isDirty)
	//{
	//	mIsDirty = isDirty;
	//}

	// bool PropertyBase::getMode() const
	// {
	// 	return mIsOutput;
	// }

	// void PropertyBase::setMode(bool isOutput)
	// {
	// 	mIsOutput = isOutput;
	// }

	//const bool PropertyBase::isReadOnly() const
	//{
	//	return mIsReadOnly;
	//}

	//void PropertyBase::setReadOnly(bool isReadOnly)
	//{
	//	mIsReadOnly = isReadOnly;
	//}

	//const bool PropertyBase::isVisible() const
	//{
	//	return mIsVisible;
	//}

	//void PropertyBase::setVisible(bool isVisible)
	//{
	//	mIsVisible = isVisible;
	//}

	void PropertyBase::attach()
	{
		static_cast<PropertyContainerBase*>(mParent)->onAttach(this);
	}

	void PropertyBase::detach()
	{
		static_cast<PropertyContainerBase*>(mParent)->onDetach(this);
	}

	void PropertyBase::update()
	{
		static_cast<PropertyContainerBase*>(mParent)->onUpdate(this);
	}

	void PropertyBase::link()
	{
		static_cast<PropertyContainerBase*>(mParent)->onLink(this);
	}

	void PropertyBase::unlink()
	{
		static_cast<PropertyContainerBase*>(mParent)->onUnlink(this);
	}
}