#pragma once
#include "defCoreApi.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

#include "virServiceBase.hpp"
#include "Log/lgcLogManager.hpp"

namespace core
{
	class CORE_API ServiceManager
	{
	public:
		static ServiceManager& get();

		ServiceManager(const ServiceManager&) = delete;
		ServiceManager& operator=(const ServiceManager&) = delete;

		bool init();
		void shutdown();

		template <typename Interface>
		void registerService(std::shared_ptr<Interface> instance)
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			std::type_index typeIdx = std::type_index(typeid(Interface));

			if (mServices.find(typeIdx) != mServices.end())
			{
				APP_LOG_WARN("[Service Manager]: Service already registered: {}", typeid(Interface).name());
				return;
			}

			ServiceEntry entry;
			entry.instance = std::static_pointer_cast<void>(instance);

			if constexpr (std::is_base_of_v<ServiceBase, Interface>)
				entry.shutdownFunc = [instance]() {if (instance) instance->shutdown(); };
			else
				entry.shutdownFunc = []() {};

			mServices[typeIdx] = std::move(entry);
		}

		template <typename Interface>
		std::shared_ptr<Interface> getService()
		{
			std::shared_lock<std::shared_mutex> lock(mMutex);
			auto it = mServices.find(std::type_index(typeid(Interface)));
			if (it == mServices.end()) return nullptr;

			return std::static_pointer_cast<Interface>(it->second.instance);
		}

		//template <typename Interface>
		//std::shared_ptr<Interface> requireService()
		//{
		//	auto ptr = getService<Interface>();
		//	if (!ptr) throw std::runtime_error("Required service not found: " + std::string(typeid(Interface).name()));
		//	return ptr;
		//}

		template <typename Interface>
		void unregisterService()
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			std::type_index typeIdx = std::type_index(typeid(Interface));
			mServices.erase(typeIdx);
		}

		template <typename Interface>
		void registerNamedService(const std::string& name, std::shared_ptr<Interface> instance)
		{
			if constexpr (std::is_base_of_v<ServiceBase, Interface>)
				static_assert(std::has_virtual_destructor<Interface>::value,
					"ServiceBase derived classes must have a virtual destructor!");

			std::unique_lock<std::shared_mutex> lock(mMutex);

			std::string key = std::string(typeid(Interface).name()) + "::" + name;

			ServiceEntry entry;

			entry.instance = std::static_pointer_cast<void>(instance);

			if constexpr (std::is_base_of_v<ServiceBase, Interface>)
				entry.shutdownFunc = [instance]() {if (instance)instance->shutdown(); };
			else
				entry.shutdownFunc = []() {};

			mNamedServices[key] = std::move(entry);
		}

		template <typename Interface>
		std::shared_ptr<Interface> getNamedService(const std::string& name)
		{
			std::shared_lock<std::shared_mutex> lock(mMutex);

			std::string key = std::string(typeid(Interface).name()) + "::" + name;
			auto it = mNamedServices.find(key);

			if (it == mNamedServices.end()) return nullptr;
			
			return std::static_pointer_cast<Interface>(it->second.instance);
		}

		template <typename Interface>
		void unregisterNamedService(const std::string& name)
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			std::string key = std::string(typeid(Interface).name()) + "::" + name;
            if (mNamedServices.find(key) != mNamedServices.end())
		    	mNamedServices.erase(key);
		}

	private:
		ServiceManager() = default;
		~ServiceManager();

	private:
		struct ServiceEntry
		{
			std::shared_ptr<void> instance;
			std::function<void()> shutdownFunc;
		};

		mutable std::shared_mutex mMutex;
		std::unordered_map<std::type_index, ServiceEntry> mServices;
		std::unordered_map<std::string, ServiceEntry> mNamedServices;
	};
}
