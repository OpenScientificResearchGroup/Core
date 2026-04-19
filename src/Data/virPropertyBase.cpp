/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Data/lgcProperty.hpp"

#include "Data/virNodeBase.hpp"

namespace core
{
	PropertyBase::PropertyBase(const std::string& key, NodeBase* node)
		: mKey(key), mNode(node), mIsReadOnly(false), mIsVisible(true)
	{
		mLink.uuid.clear();
		mLink.key.clear();
		mLink.isActive = false;
	}

	bool PropertyBase::isLink() const
	{
		return mLink.isActive;
	}

	NodeBase* PropertyBase::getNode() const
	{
		return mNode;
	}

	void PropertyBase::setLink(const std::string& uuid, const std::string& key = "")
	{
		if (mLink.isActive)
			unlink();

		mLink.uuid = uuid;
		mLink.key = key;
		mLink.isActive = true;

		link();
	}

	void PropertyBase::resetLink()
	{
		if (mLink.isActive)
			unlink();

		mLink.uuid.clear();
		mLink.key.clear();
		mLink.isActive = false;
	}

	const PropertyLink& PropertyBase::getLink() const
	{
		return mLink;
	}

	bool PropertyBase::readLink(const nlohmann::json& j)
	{
		mLink.uuid = j["node"].get<std::string>();
		mLink.key = j.value("property", std::string(""));
		mLink.isActive = true;
		return true;
	}

	nlohmann::json PropertyBase::writeLink() const
	{
		nlohmann::json j;
		j["node"] = mLink.uuid;
		if (!mLink.key.empty())
			j["property"] = mLink.key;
		return j;
	}

	const bool PropertyBase::isReadOnly() const
	{
		return mIsReadOnly;
	}

	void PropertyBase::setReadOnly(bool isReadOnly)
	{
		mIsReadOnly = isReadOnly;
	}

	const bool PropertyBase::isVisible() const
	{
		return mIsVisible;
	}

	void PropertyBase::setVisible(bool isVisible)
	{
		mIsVisible = isVisible;
	}

	void PropertyBase::attach()
	{
		if (mNode)
			mNode->onAttach(this);
	}

	void PropertyBase::detach()
	{
		if (mNode)
			mNode->onDetach(this);
	}

	void PropertyBase::update()
	{
		if (mNode)
			mNode->onUpdate(this);
	}

	void PropertyBase::link()
	{
		if (mNode)
			mNode->onLink(this);
	}

	void PropertyBase::unlink()
	{
		if (mNode)
			mNode->onUnlink(this);
	}
}