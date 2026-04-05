/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
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
		shutdown();
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
