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
		shutdownAll();
	}

	void ServiceManager::shutdownAll()
	{
		std::unique_lock<std::shared_mutex> lock(mMutex);

		for (auto& [key, entry] : mNamedServices) {
			if (entry.shutdownFunc) {
				try {
					entry.shutdownFunc();
				}
				catch (...) {
					// fprintf(stderr, "Error shutting down named service: %s\n", key.c_str());
				}
			}
		}

		for (auto& [typeIdx, entry] : mServices) {
			if (entry.shutdownFunc) {
				try {
					entry.shutdownFunc();
				}
				catch (...) {
					// fprintf(stderr, "Error shutting down service: %s\n", typeIdx.name());
				}
			}
		}

		mNamedServices.clear();
		mServices.clear();
	}
}
