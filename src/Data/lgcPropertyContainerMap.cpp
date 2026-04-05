/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Data/lgcPropertyContainerMap.hpp"

#include <memory>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "Service/lgcServiceManager.hpp"

#include "Data/virContainerBase.hpp"

namespace core
{
	Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>::
		Property() : PropertyBase()
	{
		// mAttr = PropertyAttribute::METADATA;
	}

	Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>::
		Property(const std::unordered_map<std::string, std::unique_ptr<ContainerBase>>& value) : PropertyBase()
	{
		setValue(value);
		// mAttr = PropertyAttribute::METADATA;
	}

	void Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>::
		setValue(const std::unordered_map<std::string, std::unique_ptr<ContainerBase>>& val)
	{
		// mVal = val;
	}

	const std::unordered_map<std::string, std::unique_ptr<ContainerBase>>& Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>::
		getValue() const
	{
		return mVal;
		// return {};
	}

	const std::any Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>::
		getValueAny() const
	{
		// return mVal;
		return {};
	}

	void Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>::
		add(const std::string& key, std::unique_ptr<ContainerBase> feature)
	{
		mVal[key] = std::move(feature);
	}

	void Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>::
		remove(const std::string& key)
	{
		mVal.erase(key);
	}

	//PropertyAttribute Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>::
	//	getAttr()
	//{
	//	return mAttr;
	//}

	Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>& Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>::
		operator=(const std::unordered_map<std::string, std::unique_ptr<ContainerBase>>& val)
	{
		// setValue(val);
		return *this;
	}

	Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>::
		operator std::unordered_map<std::string, std::unique_ptr<ContainerBase>>() const
	{
		// return mVal;
		return {};
	}

	//std::unique_ptr<ContainerBase> createFeatureInstance(const FeatureType& type)
	//{
	//	// 假设你有这些子类
	//	if (type == FeatureType::IMPORT_STEP) return std::make_unique<ImportStepFeature>();
	//	// ... 其他形状 ...

	//	return nullptr; // 未知类型
	//}

	// --- 序列化实现 ---
	bool Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>::
		read(const nlohmann::json& j)
	{
		mVal.clear();

		// 1. 类型检查：必须是数组
		// 注意：这里假设传入的 j 已经是那个数组节点了
		if (j.is_null() || !j.is_array()) return false;

		// 2. 遍历数组中的每个元素
		for (const auto& shapeJson : j)
		{
			// -------------------------------------------------
			// 关键变化 A：从对象内部提取 Key (UUID)
			// -------------------------------------------------
			if (!shapeJson.contains("uuid"))
			{
				// std::cerr << "[Error] Shape missing 'uuid' field, skipping." << std::endl;
				continue;
			}
			std::string uuid = shapeJson["uuid"].get<std::string>();

			// -------------------------------------------------
			// 关键变化 B：提取类型 (Type) 用于工厂创建
			// -------------------------------------------------
			if (!shapeJson.contains("type"))
			{
				// std::cerr << "[Error] Shape " << uuid << " missing 'type' field." << std::endl;
				continue;
			}
			std::string type = shapeJson["type"].get<std::string>();

			// 3. 工厂模式创建对象
			// std::unique_ptr<ContainerBase> newContainer = createFeatureInstance(static_cast<FeatureType>(type));
			ServiceManager& serMgr = ServiceManager::get();
			auto cmdPtr = serMgr.getNamedService<std::function<std::unique_ptr<ContainerBase>()>>(type);
			std::unique_ptr<ContainerBase> newContainer = (*cmdPtr)();

			if (newContainer)
			{
				// 4. 让对象读取自己的属性 (包括 name, uuid, 以及其他属性)
				newContainer->read(shapeJson);

				// 5. 存入 unordered_map
				// 此时 map 的 key 是我们手动从 JSON 里拿出来的 uuid
				mVal[uuid] = std::move(newContainer);
			}
			//else
			//	std::cerr << "[Error] Unknown shape type: " << typeStr << std::endl;

		}
	}

	nlohmann::json Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>::
		write() const
	{
		nlohmann::json prop = nlohmann::json::array();
		for (const auto& [key, value] : mVal)
		{
			nlohmann::json feature;
			if (value)
				feature = value->write();
			prop.push_back(feature);
		}
		return prop;
	}
}