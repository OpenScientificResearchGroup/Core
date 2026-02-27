#include "Event/lgcEventManager.hpp"

namespace core
{
	EventManager& EventManager::get()
	{
		static EventManager instance;
		return instance;
	}

	EventManager::EventManager()
		: mNextId(0)
	{

	}

	EventManager::~EventManager()
	{

	};

	bool EventManager::init()
	{
		std::unique_lock<std::shared_mutex> lock(mMutex);
		mSubscribers.clear();
		mIdLookup.clear();
		mNextId = 0;
		return true;
	}

	void EventManager::shutdown()
	{
		std::unique_lock<std::shared_mutex> lock(mMutex);
		mSubscribers.clear();
		mIdLookup.clear();
		mNextId = 0;
	}
}
