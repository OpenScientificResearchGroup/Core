/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "virPropertyBase.hpp"

#include <string>

namespace core
{
    template <typename T>
    class Property : public PropertyBase
    {
	public:
		// 构造函数：必须显式初始化基类
        Property(NodeBase *node, const std::string &key, const T &val)
            : PropertyBase(node, key), mVal(val)
        {

		}

        Property(NodeBase*node, const std::string &key, T &&val)
            : PropertyBase(node, key), mVal(std::move(val))
        {

		}

        virtual ~Property() = default;

        // 方便快捷使用的运算符
        Property<T> &operator=(const T &val)
        {
			set(val);
			return *this;
		}

        Property<T> &operator=(T &&val)
        {
			set(std::move(val));
			return *this;
		}

		virtual bool read(const nlohmann::json& j) override
		{
			if (j.is_object())
			{
				if (j.contains("link"))
					readLink(j["link"]);

				if (j.contains("value"))
					mVal = j["value"].get<T>();
			}
			else if (!j.is_null())
			{
				resetLink();
				mVal = j.get<T>();
			}
			return true;
		}

		virtual nlohmann::json write() const override
		{
			if (!isLink())
				return nlohmann::json(mVal);

			nlohmann::json j;
			j["link"] = writeLink();
			j["value"] = nlohmann::json(mVal);
			return j;
		}

		void set(const T& val)
		{
			// if (val != mVal)
			// 工业软件常见逻辑：手动修改已链接的属性，会自动断开链接
			mVal = val;
			if (mLink.isActive)
				resetLink();
			else
				update();
		}

		void set(T&& val)
		{
			// if (val != mVal)
			// 工业软件常见逻辑：手动修改已链接的属性，会自动断开链接
			mVal = std::move(val);
			if (mLink.isActive)
				resetLink();
			else
				update();
		}

		const T& get() const
		{
			return mVal;
		}

		std::any getValueAny() const override
		{
			return mVal;
		}

		void setValueAny(const std::any& value) override
		{
			try
			{
				set(std::any_cast<T>(value));
			}
			catch (...)
			{
				/* 错误处理或日志 */
			}
		}

		bool sync(const std::any& sourceValue) override
		{
			if (!mLink.isActive) return false;
			try
			{
				// 同步阶段只拷贝值，不触发 update，避免形成“同步 -> 脏标记 -> 再同步”的回路。
				this->mVal = std::any_cast<T>(sourceValue);
			}
			catch (...)
			{
				// 类型不匹配时保持当前值，交由上层文档逻辑处理。
			}
			return true;
		}

		ObjectType getObjectType() const
		{
			return ObjectType::ATTRIBUTE;
		}

	private:
		T mVal;

	};
}
