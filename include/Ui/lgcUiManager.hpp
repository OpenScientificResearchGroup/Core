#pragma once
#include "defCoreApi.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
// #include <mutex>
#include <atomic>
#include <shared_mutex>

class wxFrame;
class wxAuiManager;
class wxObject;

namespace core
{
	class CORE_API UiManager
	{
	public:
		static UiManager& get();

		UiManager(const UiManager&) = delete;
		UiManager& operator=(const UiManager&) = delete;

		bool init(const std::string& title, const size_t& width = 800, const size_t& height = 600);
		void shutdown();

		void show();
		void close();
		wxFrame* getRootWindow() const;
		wxAuiManager* getAuiManager() const;

		/// ===============================================
		/// Anchor Management
		/// ===============================================

		void registerAnchor(const std::string& anchorId, wxObject* obj);

		void unregisterAnchor(const std::string& anchorId);

		wxObject* getAnchor(const std::string& anchorId);

		template <typename T>
		T* getAnchorAs(const std::string& anchorId)
		{
			wxObject* obj = getAnchor(anchorId);
			if (!obj) return nullptr;
			return dynamic_cast<T*>(obj);
		}

		void refreshAnchorLayout(const std::string& anchorId);

		/// ===============================================
		/// Layout Management
		/// ===============================================

		void loadLayoutFromJson(const std::string& jsonFilePath);

		void saveLayoutToJson(const std::string& jsonFilePath);

		/// ===============================================
		/// ID Management
		/// ===============================================

		size_t getId(const std::string& name);

		bool removeId(const std::string& name);

		size_t createAnonymousId();

		std::string getName(size_t id) const;

	private:
		UiManager();
		~UiManager();

	private:
		wxFrame* mFrame;
		wxAuiManager* mAuiMgr;

		// 考虑读多写少
		mutable std::shared_mutex mAnchorMutex;
		std::map<std::string, wxObject*> mAnchors;

		// 考虑到读多写少，
		mutable std::shared_mutex mIdMutex;
		std::atomic<size_t> mNextId;
		std::map<std::string, size_t> mNameToId;
		std::map<size_t, std::string> mIdToName;
		
	};
}
