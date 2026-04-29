/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Data/virPropertyContainerBase.hpp"

#include "Data/lgcAttributeGroup.hpp"
#include "Data/lgcAttribute.hpp"

namespace core
{
	PropertyContainerBase::PropertyContainerBase()
	{

	}

	bool PropertyContainerBase::read(const nlohmann::json& j)
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
				if (value.is_object())
				{
					std::unique_ptr<AttributeGroup> newAttributeGroup;
					newAttributeGroup = std::make_unique<AttributeGroup>(this);
					if (newAttributeGroup)
					{
						newAttributeGroup->setName(key);
						newAttributeGroup->setParent(this);
						newAttributeGroup->read(value);
						mPropertySets[key] = std::move(newAttributeGroup);
					}
				}
				else
				{
					std::unique_ptr<PropertyBase> newAttribute;
					if (value.is_boolean())
						newAttribute = std::make_unique<Attribute<bool>>(this, key, false);
					else if (value.is_number_integer())
						newAttribute = std::make_unique<Attribute<int>>(this, key, 0);
					else if (value.is_number_float())
						newAttribute = std::make_unique<Attribute<double>>(this, key, .0f);
					else if (value.is_string())
						newAttribute = std::make_unique<Attribute<std::string>>(this, key, "");

					if (newAttribute)
					{
						newAttribute->read(value);
						mProperties[key] = std::move(newAttribute);
					}
				}
			}
		}
		return true;
	}

	nlohmann::json PropertyContainerBase::write() const
	{
		nlohmann::json j;
		for (const auto& [key, prop] : mProperties)
			if (prop)
				j[key] = prop->write();
		for (const auto& [name, set] : mPropertySets)
			if (set)
				j[name] = set->write();
		return j;
	}

	std::vector<PropertyBase*> PropertyContainerBase::getAllProperties() const
	{
		std::vector<PropertyBase*> result;

		// 1. 收集当前层级的所有直接属性
		for (const auto& [name, prop] : mProperties)
			if (prop)
				result.push_back(prop.get());

		// 2. 递归收集所有子组中的属性
		for (const auto& [name, set] : mPropertySets)
		{
			if (set)
			{
				// 调用子组的 getAllAttributes
				std::vector<PropertyBase*> subAttributes = set->getAllProperties();
				// 将子组的结果合并到当前结果集中
				result.insert(result.end(), subAttributes.begin(), subAttributes.end());
			}
		}

		return result;
	}

	PropertyBase* PropertyContainerBase::getProperty(const std::vector<std::string>& path) const
	{
		if (path.empty()) return nullptr;

		// 从当前容器开始
		const PropertyContainerBase* currentContainer = this;

		// 遍历路径，直到倒数第二个元素（这些都应该是 PropertySet）
		for (size_t i = 0; i < path.size() - 1; ++i)
		{
			const std::string& segment = path[i];

			// 在当前容器的子组中查找
			auto it = currentContainer->mPropertySets.find(segment);

			// 如果没找到子组，或者子组指针为空，说明路径无效
			if (it == currentContainer->mPropertySets.end() || !it->second)
				return nullptr;

			// 钻取到下一层容器
			// 注意：这里需要确保 PropertySetBase 继承自 PropertyContainerBase
			currentContainer = it->second.get();
		}

		// 最后一个 Key 应该是真正的属性（Property）
		const std::string& finalKey = path.back();
		auto itProp = currentContainer->mProperties.find(finalKey);

		if (itProp != currentContainer->mProperties.end())
			return itProp->second.get();

		// 如果最后一步在 mProperties 里没找到，
		// 逻辑上可能用户想找的是一个 Group，但此函数返回类型是 PropertyBase*，所以返回 nullptr
		return nullptr;
	}

	//void PropertyContainerBase::setName(const std::string& name)
	//{
	//	updateProperty<std::string>("name", name);
	//}

	//const std::string& PropertyContainerBase::getName() const
	//{
	//	return selectProperty<std::string>("name");
	//}

	bool PropertyContainerBase::insertPropertySet(std::unique_ptr<PropertySetBase> propertySet)
	{
		auto it = mPropertySets.find(propertySet->getName());
		if (it != mPropertySets.end())
			return false; // 属性组已存在，不添加
		mPropertySets[propertySet->getName()] = std::move(propertySet);
		return true;
	}

	PropertySetBase* PropertyContainerBase::selectPropertySet(const std::string& name) const
	{
		auto it = mPropertySets.find(name);
		if (it == mPropertySets.end())
			return nullptr; // 属性不存在，返回默认值
		return it->second.get();
	}

	bool PropertyContainerBase::deletePropertySet(const std::string& name)
	{
		auto it = mPropertySets.find(name);
		if (it == mPropertySets.end())
			return false;
		mPropertySets.erase(it);
		return true;
	}

	bool PropertyContainerBase::deleteProperty(const std::string& key)
	{
		auto it = mProperties.find(key);
		if (it == mProperties.end())
			return false;
		mProperties.erase(it);
		return true;
	}

	void PropertyContainerBase::onUpdate(ObjectBase* obj)
	{
		static_cast<PropertyContainerBase*>(mParent)->onUpdate(obj);
	}

	void PropertyContainerBase::onAttach(ObjectBase* obj)
	{
		static_cast<PropertyContainerBase*>(mParent)->onAttach(obj);
	}

	void PropertyContainerBase::onDetach(ObjectBase* obj)
	{
		static_cast<PropertyContainerBase*>(mParent)->onDetach(obj);
	}

	void PropertyContainerBase::onLink(ObjectBase* obj)
	{
		static_cast<PropertyContainerBase*>(mParent)->onLink(obj);
	}

	void PropertyContainerBase::onUnlink(ObjectBase* obj)
	{
		static_cast<PropertyContainerBase*>(mParent)->onUnlink(obj);
	}

	void PropertyContainerBase::attach()
	{
		static_cast<PropertyContainerBase*>(mParent)->onAttach(this);
	}

	void PropertyContainerBase::detach()
	{
		static_cast<PropertyContainerBase*>(mParent)->onDetach(this);
	}
}