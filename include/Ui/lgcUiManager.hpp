#pragma once
#include "defCoreApi.hpp"

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>

class wxFrame;
class wxWindow;
class wxAuiManager;
class wxMenu;
class wxBitmap;
class wxObject;

//#include <nlohmann/json.hpp>

namespace core
{
	//enum class DockSide
	//{
	//	Left,
	//	Right,
	//	Top,
	//	Bottom,
	//	Center
	//};

	class CORE_API UiManager
	{
	public:
		static UiManager& get();

		UiManager(const UiManager&) = delete;
		UiManager& operator=(const UiManager&) = delete;

		void init(const std::string& title, const size_t& width = 800, const size_t& height = 600);
		void shutdown();
		void show();
		void close();
		wxFrame* getRootWindow() const;
		wxAuiManager* getAuiManager() const;

		//void addDockablePane(wxWindow* pane, DockSide side,
		//	const std::string& title, const std::string& internalName,
		//	int width = 300, int height = 200, const wxBitmap* icon = nullptr);

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

			// return wxDynamicCast(obj, T);
			return dynamic_cast<T*>(obj);
		}

		void refreshAnchorLayout(const std::string& anchorId);

		/// ===============================================
		/// Layout Management
		/// ===============================================

		void loadLayoutFromJson(const std::string& jsonFilePath);

		void saveLayoutToJson(const std::string& jsonFilePath);

		// void resetLayout();

		/// ===============================================
		/// ID Management
		/// ===============================================

		int getId(const std::string& name);

		bool removeId(const std::string& name);

		int createAnonymousId();

		std::string getName(int id) const;

	private:
		UiManager();
		~UiManager();

		// wxMenu* getOrAddViewMenu();

		// void togglePane(const std::string& internalName, bool show);

		// void syncMenuState();

	private:
		wxFrame* mFrame = nullptr;
		wxAuiManager* mAuiMgr = nullptr;

		// std::string mDefaultPerspective;

		// std::map<std::string, int> mPaneMenuMap;

		std::map<std::string, wxObject*> mAnchors;

		std::atomic<size_t> mNextId;
		std::map<std::string, int> mNameToId;
		std::map<int, std::string> mIdToName;

		mutable std::mutex mMutex;
	};
}
