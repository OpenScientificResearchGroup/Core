/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Data/lgcDataManager.hpp"

#include <sstream>

#include "Data/virContainerBase.hpp"

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
		closeAllDocuments();
	}

	bool DataManager::closeDocument(const std::string& uuid)
	{
		std::shared_ptr<ContainerBase> docToClose = nullptr;
		bool isActive = false;

		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			auto it = mDocs.find(uuid);
			if (it == mDocs.end()) return false;

			docToClose = it->second;
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
			setActiveDocument(nextId);
		}

		//if (mOnDocListChanged)
		//	mOnDocListChanged();

		return true;
	}

	void DataManager::closeAllDocuments()
	{
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			mDocs.clear();
			mActiveDocUuid.clear();
			mUntitledCount = 0;
		}

		//if (mOnActiveDocChanged)
		//	mOnActiveDocChanged(nullptr, nullptr);
		//if (mOnDocListChanged)
		//	mOnDocListChanged();
	}

	void DataManager::setActiveDocument(const std::string& uuid)
	{
		std::shared_ptr<ContainerBase> oldDoc = nullptr;
		std::shared_ptr<ContainerBase> newDoc = nullptr;

		{
			std::shared_lock<std::shared_mutex> lock(mMutex);

			if (mActiveDocUuid == uuid) return;
			if (!mActiveDocUuid.empty())
			{
				auto it = mDocs.find(mActiveDocUuid);
				if (it != mDocs.end()) oldDoc = it->second;
			}
			if (!uuid.empty())
			{
				auto it = mDocs.find(uuid);
				if (it != mDocs.end()) newDoc = it->second;
			}

			if (!uuid.empty() && !newDoc) return;
		}
		{
			std::unique_lock<std::shared_mutex> lock(mMutex);
			mActiveDocUuid = uuid;
		}

		//if (mOnActiveDocChanged)
		//	mOnActiveDocChanged(oldDoc, newDoc);
	}

	std::shared_ptr<ContainerBase> DataManager::getActiveDocument() const
	{
		std::shared_lock<std::shared_mutex> lock(mMutex);
		if (mActiveDocUuid.empty()) return nullptr;

		auto it = mDocs.find(mActiveDocUuid);
		if (it != mDocs.end())
			return it->second;
		return nullptr;
	}

	std::shared_ptr<ContainerBase> DataManager::getDocument(const std::string& uuid) const
	{
		std::shared_lock<std::shared_mutex> lock(mMutex);
		auto it = mDocs.find(uuid);
		if (it != mDocs.end())
			return it->second;
		return nullptr;
	}

	std::vector<std::shared_ptr<ContainerBase>> DataManager::getAllDocuments() const
	{
		std::shared_lock<std::shared_mutex> lock(mMutex);
		std::vector<std::shared_ptr<ContainerBase>> list;
		list.reserve(mDocs.size());
		for (const auto& kv : mDocs)
			list.push_back(kv.second);
		return list;
	}

	//void DataManager::setOnActiveDocChangedCallback(std::function<void(std::shared_ptr<ContainerBase> oldDoc, std::shared_ptr<ContainerBase> newDoc)> callback)
	//{
	//	mOnActiveDocChanged = callback;
	//}

	//void DataManager::setOnDocListChangedCallback(std::function<void()> callback)
	//{
	//	mOnDocListChanged = callback;
	//}

} // namespace core
