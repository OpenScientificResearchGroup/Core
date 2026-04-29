/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include <vector>

#include <nlohmann/json.hpp>

namespace core
{
    enum class ObjectType
    {
		//OBJECT,

		PROPERTY,
		PROPERTY_SET,

		NODE,
		NODE_SET,

        //ATTRIBUTE,
        //ATTRIBUTE_GROUP,

        //ELEMENT,
        //ELEMENT_GROUP,

        //DOCUMENT
    };

    class ObjectBase
	{
	public:
		ObjectBase() : mParent(nullptr) {};
		ObjectBase(ObjectBase* parent) : mParent(parent) {};
		virtual ~ObjectBase() = default;

		ObjectBase(const ObjectBase&) = delete;
		ObjectBase& operator=(const ObjectBase&) = delete;
		ObjectBase(ObjectBase&&) = delete;
		ObjectBase& operator=(ObjectBase&&) = delete;

		virtual bool read(const nlohmann::json& j) = 0;
		virtual nlohmann::json write() const = 0;

		virtual void attach() = 0;
		virtual void detach() = 0;

		virtual ObjectType getObjectType() const = 0;

		void setParent(ObjectBase* parent) { mParent = parent; }
		ObjectBase* getParent() const { return mParent; }

	protected:
		ObjectBase* mParent;

	};
} // namespace core