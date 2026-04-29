/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "virPropertyContainerBase.hpp"

#include <string>

#include <nlohmann/json.hpp>

#include "Utility/lgcUuid.hpp"

namespace core
{
    class NodeBase : public PropertyContainerBase
	{
	public:
		NodeBase();
		virtual ~NodeBase() = default;

		virtual ObjectType getObjectType() const override;

		//virtual bool read(const nlohmann::json& j) override;
		//virtual nlohmann::json write() const override;

		void setUuid(const std::string& uuid = util::Uuid::generate());
		const std::string& getUuid() const;
		void setName(const std::string& name);
		const std::string& getName() const;
		void setType(const std::string& type);
		const std::string& getType() const;
		void setTimestamp(uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
		const long long& getTimestamp() const;

        //const std::unordered_map<std::string, std::unique_ptr<PropertySetBase>> &getAllProperties() const;
        //PropertyBase* getProperty(const std::string& key) const;

		//template <typename T>
		//bool insertProperty(const std::string& key, const T& value)
		//{
		//	auto it = mProperties.find(key);
		//	if (it != mProperties.end()) return false; // 属性已存在，不添加
		//	auto prop = std::make_unique<Property<T>>(key, this, value);
		//	mProperties[key] = std::move(prop);
		//	return true;
		//}

		//template <typename T>
		//T* selectProperty(const std::string& key) const
		//{
		//	auto it = mProperties.find(key);
		//	if (it == mProperties.end()) return nullptr; // 属性不存在，返回默认值
		//	return dynamic_cast<Property<T>*>(it->second.get());
		//}

		//template <typename T>
		//bool updateProperty(const std::string& key, const T& value)
		//{
		//	auto it = mProperties.find(key);
		//	if (it != mProperties.end())
		//	{
		//		Property<T>* typedRawPtr = dynamic_cast<Property<T>*>(it->second.get());
		//		if (typedRawPtr) typedRawPtr->set(value);
		//	}
		//	else
		//		return false; // 属性不存在，无法设置
		//	return true;
		//}

		//bool deleteProperty(const std::string& key);

		//void setParent(NodeBase* parent);
		//NodeBase* getParent() const;

		virtual void onUpdate(ObjectBase* obj) override;
		virtual void onAttach(ObjectBase* obj) override;
		virtual void onDetach(ObjectBase* obj) override;
		virtual void onLink(ObjectBase* obj) override;
		virtual void onUnlink(ObjectBase* obj) override;

		virtual void attach() override;
		virtual void detach() override;

		virtual bool execute() = 0;

	protected:
		//NodeBase* mParent; // 所属节点
		//std::unordered_map<std::string, std::unique_ptr<PropertySetBase>> mPropertySets;

	};
}
