#pragma once
#include "defCoreApi.hpp"

#include <string>
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <functional>
#include <vector>
#include <optional>
#include <atomic>

#include "virContainerBase.hpp"

namespace core
{
	class CORE_API DataManager
	{
	public:
		static DataManager& get();

		DataManager() = default;
		~DataManager() = default;

		DataManager(const DataManager&) = delete;
		DataManager& operator=(const DataManager&) = delete;

		template <typename T>
		std::shared_ptr<T> createDocument(const std::string& name = "Untitled")
		{
			static_assert(std::is_base_of<ContainerBase, T>::value, "T must inherit from Document");

			// auto doc = std::make_shared<T>(name + "-" + std::to_string(mUntitledCount++));
			auto doc = std::make_shared<T>();
			doc->setName(name + "-" + std::to_string(mUntitledCount++));
			doc->setUuid();
			doc->setTimeStamp();
			// registerDocument(doc);
			{
				std::unique_lock lock(mMutex);
				mDocs[doc->getUuid()] = doc;
			}

			setActiveDocument(doc->getUuid());

			if (mOnDocListChanged) mOnDocListChanged();

			return doc;
		}

		bool closeDocument(const std::string& id);

		void closeAllDocuments();

		void setActiveDocument(const std::string& id);
		std::shared_ptr<ContainerBase> getActiveDocument() const;

		std::shared_ptr<ContainerBase> getDocument(const std::string& id) const;

		std::vector<std::shared_ptr<ContainerBase>> getAllDocuments() const;

		void setOnActiveDocChangedCallback(std::function<void(std::shared_ptr<ContainerBase> oldDoc, std::shared_ptr<ContainerBase> newDoc)> callback);

		void setOnDocListChangedCallback(std::function<void()> callback);

	private:
		// void registerDocument(std::shared_ptr<ContainerBase> doc);
		// std::string generateUniqueName(const std::string& base);

	private:
		std::unordered_map<std::string, std::shared_ptr<ContainerBase>> mDocs;

		std::string mActiveDocUuid;

		mutable std::shared_mutex mMutex;

		std::function<void(std::shared_ptr<ContainerBase> oldDoc, std::shared_ptr<ContainerBase> newDoc)> mOnActiveDocChanged;
		std::function<void()> mOnDocListChanged;

		std::atomic<int> mUntitledCount{ 1 };
	};
}// namespace core