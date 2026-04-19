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

		//NODE,
		PROPERTY,

		ELEMENT,
		GROUP,

		DOCUMENT
	};

	class ObjectBase
	{
	public:
		ObjectBase() = default;
		virtual ~ObjectBase() = default;

		ObjectBase(const ObjectBase&) = delete;
		ObjectBase& operator=(const ObjectBase&) = delete;
		ObjectBase(ObjectBase&&) = delete;
		ObjectBase& operator=(ObjectBase&&) = delete;

		virtual bool read(const nlohmann::json& j) = 0;
		virtual nlohmann::json write() const = 0;

		// // 轻量类型查询接口：用于在基类指针上判断对象是否兼容某个派生类型。
		// // 这比在业务代码中反复写 dynamic_cast 更集中，也更便于后续替换实现。
		// template <typename T>
		// bool is() const
		// {
		// 	return dynamic_cast<const T*>(this) != nullptr;
		// }

		// template <typename T>
		// T* as()
		// {
		// 	return dynamic_cast<T*>(this);
		// }

		// template <typename T>
		// const T* as() const
		// {
		// 	return dynamic_cast<const T*>(this);
		// }

		virtual void attach() = 0;
		virtual void detach() = 0;
		//virtual void update() = 0;

		virtual ObjectType getObjectType() const = 0;
	};
} // namespace core