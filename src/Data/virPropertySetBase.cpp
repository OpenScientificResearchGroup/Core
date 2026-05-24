/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Core/Data/virPropertySetBase.hpp"

#include "Core/Data/virNodeBase.hpp"
#include "Core/Data/lgcAttribute.hpp"

namespace core
{
	PropertySetBase::PropertySetBase(const std::string& name)
		: PropertyContainerBase(), mName(name)
	{
		//insertProperty<std::string>("property_set_name", "");
	}

	ObjectType PropertySetBase::getObjectType() const
	{
		return ObjectType::PROPERTY_SET;
	}

	//void PropertySetBase::setName(const std::string& name)
	//{
	//	mName = name;
	//}

	const std::string& PropertySetBase::getName() const
	{
		return mName;
		//return selectProperty<std::string>("property_set_name")->get();
	}

	std::vector<std::string> PropertySetBase::getPath() const
	{
		std::vector<std::string> path;

		// 1. 首先放入属性自身的 Key
		path.push_back(this->getName());

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

	//bool PropertySetBase::read(const nlohmann::json& j)
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
	//			if (value.is_object())
	//			{
	//				std::unique_ptr<PropertySetBase> newPropSet;
	//				newPropSet = std::make_unique<PropertySetBase>(this);
	//				if (newPropSet)
	//				{
	//					newPropSet->read(value);
	//					mPropertySets[key] = std::move(newPropSet);
	//				}
	//			}
	//			else
	//			{
	//				std::unique_ptr<PropertyBase> newProp;
	//				if (value.is_boolean())
	//					newProp = std::make_unique<Attribute<bool>>(false);
	//				else if (value.is_number_integer())
	//					newProp = std::make_unique<Attribute<int>>(0);
	//				else if (value.is_number_float())
	//					newProp = std::make_unique<Attribute<double>>(.0f);
	//				else if (value.is_string())
	//					newProp = std::make_unique<Attribute<std::string>>("");

	//				if (newProp)
	//				{
	//					newProp->read(value);
	//					mProperties[key] = std::move(newProp);
	//				}
	//			}
	//		}
	//	}
	//	return true;
	//}

	//nlohmann::json PropertySetBase::write() const
	//{
	//	nlohmann::json j;
	//	for (const auto& [key, prop] : mProperties)
	//		if (prop)
	//			j[key] = prop->write();
	//	for (const auto& [name, set] : mPropertySets)
	//		if (set)
	//			j[name] = set->write();
	//	return j;
	//}

	//bool PropertySetBase::insertPropertySet(std::unique_ptr<PropertySetBase> propertySet)
	//{
	//	auto it = mPropertySets.find(propertySet->getName());
	//	if (it != mPropertySets.end())
	//		return false; // 属性组已存在，不添加
	//	mPropertySets[propertySet->getName()] = std::move(propertySet);
	//	return true;
	//}

	//PropertySetBase* PropertySetBase::selectPropertySet(const std::string& name) const
	//{
	//	auto it = mPropertySets.find(name);
	//	if (it == mPropertySets.end())
	//		return nullptr; // 属性不存在，返回默认值
	//	return it->second.get();
	//}

	//bool PropertySetBase::deletePropertySet(const std::string& name)
	//{
	//	auto it = mPropertySets.find(name);
	//	if (it == mPropertySets.end())
	//		return false;
	//	mPropertySets.erase(it);
	//	return true;
	//}

	//bool PropertySetBase::deleteProperty(const std::string& key)
	//{
	//	auto it = mProperties.find(key);
	//	if (it == mProperties.end())
	//		return false;
	//	mProperties.erase(it);
	//	return true;
	//}

	//ObjectBase* PropertySetBase::getParent() const
	//{
	//	return mParent;
	//}

	//void PropertySetBase::setParent(PropertyContainerBase* parent)
	//{
	//	mParent = parent;
	//}

	//PropertyContainerBase* PropertySetBase::getParent() const
	//{
	//	return mParent;
	//}

	//void PropertySetBase::onUpdate(ObjectBase* obj)
	//{
	//	static_cast<PropertyContainerBase*>(mParent)->onUpdate(obj);
	//}

	//void PropertySetBase::onAttach(ObjectBase* obj)
	//{
	//	static_cast<PropertyContainerBase*>(mParent)->onAttach(obj);
	//}

	//void PropertySetBase::onDetach(ObjectBase* obj)
	//{
	//	static_cast<PropertyContainerBase*>(mParent)->onDetach(obj);
	//}

	//void PropertySetBase::onLink(ObjectBase* obj)
	//{
	//	static_cast<PropertyContainerBase*>(mParent)->onLink(obj);
	//}

	//void PropertySetBase::onUnlink(ObjectBase* obj)
	//{
	//	static_cast<PropertyContainerBase*>(mParent)->onUnlink(obj);
	//}

	//void PropertySetBase::attach()
	//{
	//	static_cast<PropertyContainerBase*>(mParent)->onAttach(this);
	//}

	//void PropertySetBase::detach()
	//{
	//	static_cast<PropertyContainerBase*>(mParent)->onDetach(this);
	//}
}