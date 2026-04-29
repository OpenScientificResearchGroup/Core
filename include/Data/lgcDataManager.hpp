/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "defCoreApi.hpp"

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace core
{
	class Document;

	class CORE_API DataManager
	{
	public:
		static DataManager& get();

		DataManager() = default;
		~DataManager() = default;

		// 禁止拷贝构造和赋值，确保单例模式
		DataManager(const DataManager&) = delete;
		DataManager& operator=(const DataManager&) = delete;

		bool init();
		void shutdown();

		//template <typename T>
		//std::shared_ptr<T> createDocument(const std::string& name = "Untitled")
		//{
		//	static_assert(std::is_base_of<Document, T>::value, "T must inherit from Document");

		//	auto doc = std::make_unique<T>();
		//	doc->setName(name + "-" + std::to_string(mUntitledCount++));
		//	doc->setUuid();
		//	doc->setTimeStamp();
		//	// registerDocument(doc);
		//	doc->registerContainerIndex(doc->getUuid(), doc.get());
		//	{
		//		std::unique_lock lock(mMutex);
		//		mDocs[doc->getUuid()] = doc;
		//	}

		//	setActiveDocument(doc->getUuid());

		//	return doc;
		//}

		bool add(std::unique_ptr<Document> doc);
		bool remove(const std::string& uuid);
		void removeAll();
		void setActive(const std::string& uuid);
		Document* getActive() const;
		Document* get(const std::string& uuid) const;
		size_t getCount();
		std::vector<Document*> getAll() const;

	private:
		mutable std::shared_mutex mMutex;
		std::string mActiveDocUuid;
		std::unordered_map<std::string, std::unique_ptr<Document>> mDocs;
		std::atomic<size_t> mUntitledCount;
		
	};
}// namespace core