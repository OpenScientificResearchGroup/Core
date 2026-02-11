#pragma once
#include "virObjectBase.hpp"

#include <string>
#include <any>

#include <nlohmann/json.hpp>

#include "Utility/lgcUuid.hpp"
#include "Data/virPropertyBase.hpp"
#include "Data/lgcProperty.hpp"
#include "Data/lgcPropertyContainerMap.hpp"

namespace core
{
	class ContainerBase : public ObjectBase
	{
	public:
		ContainerBase() :ObjectBase()
		{
			add<std::string>("uuid", std::string(""));
			add<std::string>("name", std::string(""));
			add<std::string>("type", std::string(""));
			add<long long>("time_stamp", 0);
			add<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>("containers", std::unordered_map<std::string, std::unique_ptr<ContainerBase>>());
			// std::unique_ptr<Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>> prop = std::make_unique<Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>>();
			// addProperty("containers", std::move(prop));
		}

		virtual ~ContainerBase() = default;

		// 1. 禁止拷贝构造 (因为 unique_ptr 不能拷贝)
		ContainerBase(const ContainerBase&) = delete;

		// 2. 禁止拷贝赋值 (防止编译器生成导致报错的代码)
		ContainerBase& operator=(const ContainerBase&) = delete;

		// 3. 允许移动构造 (将所有权转移，这是 unique_ptr 支持的)
		ContainerBase(ContainerBase&&) = default; // 或者 noexcept

		// 4. 允许移动赋值
		ContainerBase& operator=(ContainerBase&&) = default; // 或者 noexcept

		virtual bool read(const nlohmann::json& j) override
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
					std::unique_ptr<ObjectBase> newProp;

					if (value.is_boolean())
						newProp = std::make_unique<Property<bool>>(false);
					else if (value.is_number_integer())
						newProp = std::make_unique<Property<int>>(0);
					else if (value.is_number_float())
						newProp = std::make_unique<Property<double>>(.0f);
					else if (value.is_string())
						newProp = std::make_unique<Property<std::string>>("");

					if (newProp)
					{
						newProp->read(value);
						mProperties[key] = std::move(newProp);
					}
				}
			}
			return true;
		}

		virtual nlohmann::json write() const override
		{
			nlohmann::json j;
			for (const auto& [key, prop] : mProperties)
				if (prop)
					j[key] = prop->write();
			return j;
		}

		virtual void setUuid(const std::string& uuid = util::Uuid::generate())
		{
			set<std::string>("uuid", uuid);
		}

		virtual std::string getUuid() const
		{
			return get<std::string>("uuid");
		}

		virtual void setName(const std::string& name)
		{
			set<std::string>("name", name);
		}

		virtual std::string getName() const
		{
			return get<std::string>("name");
		}

		virtual void setType(const std::string& type)
		{
			set<std::string>("type", type);
		}

		virtual std::string getType() const
		{
			return get<std::string>("type");
		}

		virtual void setTimeStamp(uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
		{
			set<long long>("time_stamp", static_cast<long long>(timestamp));
		}

		virtual long long getTimeStamp() const
		{
			return get<long long>("time_stamp");
		}

		virtual const std::unordered_map<std::string, std::unique_ptr<ObjectBase>>& getAllProperties() const
		{
			return mProperties;
		}

		virtual void addProperty(const std::string& key, std::unique_ptr<ObjectBase> prop)
		{
			if (!prop) return;
			mProperties[key] = std::move(prop);
		}

		template <typename T>
		void add(const std::string& key, const T& value)
		{
			auto prop = std::make_unique<Property<T>>(value);
			mProperties[key] = std::move(prop);
		}

		template <typename T>
		T get(const std::string& key) const
		{
			auto it = mProperties.find(key);
			if (it == mProperties.end()) return T(); // 属性不存在，返回默认值
			// auto typedProp = std::dynamic_pointer_cast<Property<T>>(it->second); // 向下转型
			// if (!typedProp) return T(); // 类型不匹配，返回默认值
			// return typedProp->getValue();
			Property<T>* typedRawPtr = dynamic_cast<Property<T>*>(it->second.get());
			if (!typedRawPtr) return T();
			return typedRawPtr->getValue();
		}

		template <typename T>
		const T* getPtr(const std::string& key) const
		{
			auto it = mProperties.find(key);
			if (it == mProperties.end()) return nullptr; // 属性不存在，返回默认值
			// auto typedProp = std::dynamic_pointer_cast<Property<T>>(it->second); // 向下转型
			// if (!typedProp) return T(); // 类型不匹配，返回默认值
			// return typedProp->getValue();
			Property<T>* typedRawPtr = dynamic_cast<Property<T>*>(it->second.get());
			if (!typedRawPtr) return nullptr;
			return &typedRawPtr->getValue();
		}

		std::any getAny(const std::string& key) const
		{
			auto it = mProperties.find(key);
			if (it == mProperties.end()) return std::any(); // 属性不存在，返回默认值
			PropertyBase* rawPtr = dynamic_cast<PropertyBase*>(it->second.get());
			if (!rawPtr) return std::any();
			return rawPtr->getValueAny(); // 调用多态方法
		}

		template <typename T>
		T* getProperty(const std::string& key) const
		{
			auto it = mProperties.find(key);
			if (it == mProperties.end()) return nullptr;
			return dynamic_cast<T*>(it->second.get());
		}

		template <typename T>
		void set(const std::string& key, const T& value)
		{
			auto it = mProperties.find(key);
			if (it != mProperties.end())
			{
				//auto typedProp = std::dynamic_pointer_cast<Property<T>>(it->second);
				//if (typedProp) typedProp->setValue(value);
				Property<T>* typedRawPtr = dynamic_cast<Property<T>*>(it->second.get());
				if (typedRawPtr) typedRawPtr->setValue(value);
			}
		}

		virtual bool remove(const std::string& key)
		{
			auto it = mProperties.find(key);
			if (it == mProperties.end()) return false;
			mProperties.erase(it);
			return true;
		}

		virtual void clear()
		{
			if (!mProperties.empty())
				mProperties.clear(); // unique_ptr 计数归零，自动析构所有属性
		}

		virtual ContainerBase* findContainer(std::vector<std::string> path, size_t depth)
		{
			if (depth >= path.size()) return this;
			const std::string& currentKey = path[depth];
			auto containers = getPtr<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>("containers");
			auto it = containers->find(currentKey);
			if (it == containers->end()) return nullptr;
			return it->second->findContainer(path, depth + 1);
		}

	protected:
		std::unordered_map<std::string, std::unique_ptr<ObjectBase>> mProperties;
	};
}
