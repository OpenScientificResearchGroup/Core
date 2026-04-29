/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Data/virPropertyBase.hpp"

#include "Data/virPropertySetBase.hpp"
#include "Data/virNodeBase.hpp"

namespace core
{
	PropertyBase::PropertyBase(PropertyContainerBase* propertySet, const std::string& key)
		//: mKey(key), mPropertySet(propertySet), mIsReadOnly(false), mIsVisible(true)
		: ObjectBase(propertySet), mKey(key)
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
			if (current->getObjectType() == ObjectType::NODE || current->getObjectType() == ObjectType::NODE_SET)
				return static_cast<NodeBase*>(current);

			// 继续向上追溯
			current = current->getParent();
		}

		return nullptr;
	}

	PropertyContainerBase* PropertyBase::getContainer() const
	{
		return static_cast<PropertyContainerBase*>(mParent);
	}

	bool PropertyBase::isLink() const
	{
		return mLink.isActive;
	}

	void PropertyBase::setLink(const std::string& uuid, const std::string& key = "")
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