/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Core/Data/lgcDataManager.hpp"

#include "Core/Data/virDocumentBase.hpp"

namespace core
{
	DataManager& DataManager::get()
	{
		static DataManager instance;
		return instance;
	}

	DataManager::DataManager() = default;
	DataManager::~DataManager() = default;

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

	bool DataManager::add(std::unique_ptr<DocumentBase> doc)
	{
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			std::string uuid = doc->getUuid();
			mDocs[uuid] = std::move(doc);
			mActiveDocUuid = uuid;
		}
		return true;
	}

	bool DataManager::remove(const std::string& uuid)
	{
		//std::unique_ptr<DocumentBase> docToClose = nullptr;
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

    bool DataManager::removeActive()
    {
        return remove(mActiveDocUuid);
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

			if (mActiveDocUuid == uuid) return;
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
            // 如果目标 UUID 不存在且文档列表不为空，保持当前活动文档不变
            if (it == mDocs.end() && !mDocs.empty()) return;

            //if (!uuid.empty() && !newDoc) return;
		}
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			mActiveDocUuid = uuid;
		}
	}

	DocumentBase* DataManager::getActive() const
	{
		std::shared_lock<std::shared_mutex> lock(mMutex);
		if (mActiveDocUuid.empty()) return nullptr;

		auto it = mDocs.find(mActiveDocUuid);
		if (it != mDocs.end())
			return it->second.get();
		return nullptr;
	}

	DocumentBase* DataManager::get(const std::string& uuid) const
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

	std::vector<DocumentBase*> DataManager::getAll() const
	{
		std::shared_lock<std::shared_mutex> lock(mMutex);
		std::vector<DocumentBase*> list;
		list.reserve(mDocs.size());
		for (const auto& kv : mDocs)
			list.push_back(kv.second.get());
		return list;
	}
} // namespace core
