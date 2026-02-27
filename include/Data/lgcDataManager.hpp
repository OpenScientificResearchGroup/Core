#pragma once
#include "defCoreApi.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "virContainerBase.hpp"

namespace core
{
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

		template <typename T>
		std::shared_ptr<T> createDocument(const std::string& name = "Untitled")
		{
			static_assert(std::is_base_of<ContainerBase, T>::value, "T must inherit from Document");

			auto doc = std::make_shared<T>();
			doc->setName(name + "-" + std::to_string(mUntitledCount++));
			doc->setUuid();
			doc->setTimeStamp();
			// registerDocument(doc);
			doc->registerContainerIndex(doc->getUuid(), doc.get());
			{
				std::unique_lock lock(mMutex);
				mDocs[doc->getUuid()] = doc;
			}

			setActiveDocument(doc->getUuid());

			// if (mOnDocListChanged) mOnDocListChanged();

			return doc;
		}
		bool closeDocument(const std::string& uuid);
		void closeAllDocuments();
		void setActiveDocument(const std::string& uuid);
		std::shared_ptr<ContainerBase> getActiveDocument() const;
		std::shared_ptr<ContainerBase> getDocument(const std::string& uuid) const;
		std::vector<std::shared_ptr<ContainerBase>> getAllDocuments() const;

		// void setOnActiveDocChangedCallback(std::function<void(std::shared_ptr<ContainerBase> oldDoc, std::shared_ptr<ContainerBase> newDoc)> callback);

		// void setOnDocListChangedCallback(std::function<void()> callback);

	private:
		// void registerDocument(std::shared_ptr<ContainerBase> doc);
		// std::string generateUniqueName(const std::string& base);

	private:
		mutable std::shared_mutex mMutex;
		std::string mActiveDocUuid;
		std::unordered_map<std::string, std::shared_ptr<ContainerBase>> mDocs;
		std::atomic<size_t> mUntitledCount;
		// std::function<void(std::shared_ptr<ContainerBase> oldDoc, std::shared_ptr<ContainerBase> newDoc)> mOnActiveDocChanged;
		// std::function<void()> mOnDocListChanged;
		
	};
}// namespace core