/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "Core/defCoreApi.hpp"

#include "virPropertyContainerBase.hpp"

#include <string>

#include <nlohmann/json.hpp>

namespace core
{
	class NodeBase;
	class PropertyBase;

	class CORE_API PropertySetBase : public PropertyContainerBase
	{
	public:
		PropertySetBase(const std::string& name);
		virtual ~PropertySetBase() = default;

		ObjectType getObjectType() const override;

		//virtual bool read(const nlohmann::json& j) override;
		//virtual nlohmann::json write() const override;

		//void setName(const std::string& name);
		const std::string& getName() const;

		std::vector<std::string> getPath() const; // 获取属性在节点中的完整路径

		//bool insertPropertySet(std::unique_ptr<PropertySetBase> propertySet);
		//PropertySetBase* selectPropertySet(const std::string& name) const;
		//bool deletePropertySet(const std::string& name);

		//template <typename T>
		//bool insertProperty(const std::string& key, const T& value)
		//{
		//	auto it = mProperties.find(key);
		//	if (it != mProperties.end())
		//		return false; // 属性已存在，不添加
		//	auto prop = std::make_unique<Property<T>>(key, this, value);
		//	mProperties[key] = std::move(prop);
		//	return true;
		//}

		//template <typename T>
		//T* selectProperty(const std::string& key) const
		//{
		//	auto it = mProperties.find(key);
		//	if (it == mProperties.end())
		//		return nullptr; // 属性不存在，返回默认值
		//	return dynamic_cast<Property<T> *>(it->second.get());
		//}

		//template <typename T>
		//bool updateProperty(const std::string& key, const T& value)
		//{
		//	auto it = mProperties.find(key);
		//	if (it != mProperties.end())
		//	{
		//		Property<T>* typedRawPtr = dynamic_cast<Property<T> *>(it->second.get());
		//		if (typedRawPtr)
		//			typedRawPtr->set(value);
		//	}
		//	else
		//		return false; // 属性不存在，无法设置
		//	return true;
		//}

		//bool deleteProperty(const std::string& key);

		//void setParent(PropertyContainerBase* parent);
		//PropertyContainerBase* getParent() const;

		//virtual void onUpdate(ObjectBase* obj) override;
		//virtual void onAttach(ObjectBase* obj) override;
		//virtual void onDetach(ObjectBase* obj) override;
		//virtual void onLink(ObjectBase* obj) override;
		//virtual void onUnlink(ObjectBase* obj) override;

		//virtual void attach() override;
		//virtual void detach() override;

	protected:
		std::string mName;									// 属性组名称
		// PropertyContainerBase* mParent;					// 所属上一级

		//std::unordered_map<std::string, std::unique_ptr<PropertyBase>> mProperties;			// 属性列表，支持任意类型的属性
		//std::unordered_map<std::string, std::unique_ptr<PropertySetBase>> mPropertySets;	// 支持嵌套属性组
	};
} // namespace core