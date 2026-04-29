/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Data/lgcDataManager.hpp"

#include "Data/lgcDocument.hpp"

namespace core
{
	DataManager& DataManager::get()
	{
		static DataManager instance;
		return instance;
	}

	bool DataManager::init()
	{
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			mDocs.clear();
			mActiveDocUuid.clear();
			mUntitledCount = 1;
		}
		return true;
	}

	void DataManager::shutdown()
	{
		removeAll();
	}

	bool DataManager::add(std::unique_ptr<Document> doc)
	{
		mDocs[doc->getUuid()] = std::move(doc);
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			mActiveDocUuid = doc->getUuid();
		}
	}

	bool DataManager::remove(const std::string& uuid)
	{
		//std::unique_ptr<Document> docToClose = nullptr;
		bool isActive = false;

		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			auto it = mDocs.find(uuid);
			if (it == mDocs.end()) return false;

			//docToClose = std::move(it->second);
			isActive = (mActiveDocUuid == uuid);

			mDocs.erase(it);
		}

		if (isActive)
		{
			std::string nextId;
			{
				std::shared_lock<std::shared_mutex> lock(mMutex);
				if (!mDocs.empty())
					nextId = mDocs.begin()->first;
			}
			setActive(nextId);
		}

		return true;
	}

	void DataManager::removeAll()
	{
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			mDocs.clear();
			mActiveDocUuid.clear();
			mUntitledCount = 0;
		}
	}

	void DataManager::setActive(const std::string& uuid)
	{
		//std::shared_ptr<NodeBase> oldDoc = nullptr;
		//std::shared_ptr<NodeBase> newDoc = nullptr;

		{
			std::shared_lock<std::shared_mutex> lock(mMutex);

			if (mActiveDocUuid == uuid || uuid.empty()) return;
			//if (!mActiveDocUuid.empty())
			//{
			//	auto it = mDocs.find(mActiveDocUuid);
			//	if (it != mDocs.end())
			//		oldDoc = it->second;
			//}
			//if (!uuid.empty())
			//{
			//	auto it = mDocs.find(uuid);
			//	if (it != mDocs.end())
			//		newDoc = it->second;
			//}
			auto it = mDocs.find(uuid);
			if (it == mDocs.end()) return;

			//if (!uuid.empty() && !newDoc) return;
		}
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			mActiveDocUuid = uuid;
		}
	}

	Document* DataManager::getActive() const
	{
		std::shared_lock<std::shared_mutex> lock(mMutex);
		if (mActiveDocUuid.empty()) return nullptr;

		auto it = mDocs.find(mActiveDocUuid);
		if (it != mDocs.end())
			return it->second.get();
		return nullptr;
	}

	Document* DataManager::get(const std::string& uuid) const
	{
		std::shared_lock<std::shared_mutex> lock(mMutex);
		auto it = mDocs.find(uuid);
		if (it != mDocs.end())
			return it->second.get();
		return nullptr;
	}

	size_t DataManager::getCount()
	{
		return mUntitledCount++;
	}

	std::vector<Document*> DataManager::getAll() const
	{
		std::shared_lock<std::shared_mutex> lock(mMutex);
		std::vector<Document*> list;
		list.reserve(mDocs.size());
		for (const auto& kv : mDocs)
			list.push_back(kv.second.get());
		return list;
	}
} // namespace core
