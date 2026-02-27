#include "Service/lgcServiceManager.hpp"
#include <iostream>

namespace core
{
	ServiceManager& ServiceManager::get()
	{
		static ServiceManager instance;
		return instance;
	}

	ServiceManager::~ServiceManager()
	{
		shutdown();
	}

	bool ServiceManager::init()
	{
		std::shared_lock<std::shared_mutex> lock(mMutex);
		mServices.clear();
		mNamedServices.clear();
		return true;
	}

	void ServiceManager::shutdown()
	{
		std::unique_lock<std::shared_mutex> lock(mMutex);

		for (auto& [key, entry] : mNamedServices)
		{
			if (entry.shutdownFunc)
			{
				try
				{
					entry.shutdownFunc();
				}
				catch (...)
				{
					APP_LOG_ERROR("[Service Manager]: Error shutting down named service: {}", key);
				}
			}
		}

		for (auto& [typeIdx, entry] : mServices)
		{
			if (entry.shutdownFunc)
			{
				try
				{
					entry.shutdownFunc();
				}
				catch (...)
				{
					APP_LOG_ERROR("[Service Manager]: Error shutting down service: {}", typeIdx.name());
				}
			}
		}

		mNamedServices.clear();
		mServices.clear();
	}
}
