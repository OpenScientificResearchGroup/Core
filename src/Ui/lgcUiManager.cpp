/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Ui/lgcUiManager.hpp"

#include <fstream>
#include <iostream>

#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <wx/artprov.h>

#include <nlohmann/json.hpp>

#include "Log/lgcLogManager.hpp"

namespace core
{
	UiManager& UiManager::get()
	{
		static UiManager instance;
		return instance;
	}

	UiManager::UiManager()
	{

	}

	UiManager::~UiManager()
	{
		// double check
		shutdown();
	}

	bool UiManager::init(const std::string& title, const size_t& width, const size_t& height)
	{
		mFrame = nullptr;
		mAuiMgr = nullptr;
		mNextId = wxID_HIGHEST + 1;

		mFrame = new wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(width, height));
		registerAnchor("/Core/main_frame", mFrame);

		mAuiMgr = new wxAuiManager();
		mAuiMgr->SetManagedWindow(mFrame);
		registerAnchor("/Core/aui_manager", mAuiMgr);

		return true;
	}

	void UiManager::shutdown()
	{
		// 先断开 AUI 管理
		if (mAuiMgr)
		{
			mAuiMgr->UnInit();
			delete mAuiMgr;
			mAuiMgr = nullptr;
		}

		// 如果 mFrame 是在这个类里 new 出来的，且没有交给 wxApp 的 TopWindow 管理，
		// 这里可能需要 Destroy。但通常 Frame 由 wxApp 管理，这里置空即可。
		mFrame = nullptr;

		// 清理锚点引用，防止野指针
		{
			std::unique_lock<std::shared_mutex> lock(mAnchorMutex);
			mAnchors.clear();
		}
		{
			std::unique_lock<std::shared_mutex> lock(mIdMutex);
			mNameToId.clear();
			mIdToName.clear();
		}
		mNextId = 0;
	}

	void UiManager::show()
	{
		if (mFrame) mFrame->Show();
	}

	void UiManager::close()
	{
		if (mFrame) mFrame->Close();
	}

	wxFrame* UiManager::getRootWindow() const
	{
		return mFrame;
	}

	wxAuiManager* UiManager::getAuiManager() const
	{
		return mAuiMgr;
	}

	void UiManager::registerAnchor(const std::string& anchorId, wxObject* obj)
	{
		if (obj)
			mAnchors[anchorId] = obj;

		// 如果是窗口，监听它的销毁事件，自动移除 Anchor
		if (wxWindow* win = dynamic_cast<wxWindow*>(obj))
		{
			// Bind 这里的 lambda 需要小心 this 指针的生命周期，
			// 但由于 UiManager 是单例，通常没问题。
			win->Bind(wxEVT_DESTROY, [this, anchorId](wxWindowDestroyEvent& event) {
				// 注意：不要在 Destroy 事件里去 lock mutex 并 erase，
				// 可能会有死锁风险，或者需要在 unregisterAnchor 里处理重入。
				this->unregisterAnchor(anchorId);
				event.Skip();
				});
		}
	}

	void UiManager::unregisterAnchor(const std::string& anchorId)
	{
		mAnchors.erase(anchorId);
	}

	wxObject* UiManager::getAnchor(const std::string& anchorId)
	{
		auto it = mAnchors.find(anchorId);
		if (it != mAnchors.end())
			return it->second;
		return nullptr;
	}

	void UiManager::refreshAnchorLayout(const std::string& anchorId)
	{
		if (wxWindow* win = getAnchorAs<wxWindow>(anchorId))
		{
			win->Layout();
			win->Refresh();
			win->Update();
			return;
		}

		if (wxSizer* sizer = getAnchorAs<wxSizer>(anchorId))
		{
			sizer->Layout();
			return;
		}
	}

	void UiManager::saveLayoutToJson(const std::string& jsonFilePath)
	{
		if (!mAuiMgr || !mFrame) return;

		std::string perspective = mAuiMgr->SavePerspective().ToStdString();

		int w, h;
		mFrame->GetSize(&w, &h);
		bool isMax = mFrame->IsMaximized();

		nlohmann::json j;
		j["app_name"] = "App";
		j["version"] = "1.0";
		j["window"] = {
			{"width", w},
			{"height", h},
			{"maximized", isMax}
		};
		j["layout_perspective"] = perspective;

		try
		{
			std::ofstream o(jsonFilePath);
			o << std::setw(4) << j << std::endl;
			APP_LOG_INFO("[Ui Manager]: Layout is saved to {}", jsonFilePath);
		}
		catch (const std::exception& e)
		{
			APP_LOG_ERROR("[Ui Manager]: Layout save failed: {}", e.what());
		}
	}

	void UiManager::loadLayoutFromJson(const std::string& jsonFilePath)
	{
		if (!mAuiMgr || !mFrame) return;

		//if (mDefaultPerspective.empty())
		//	mDefaultPerspective = mAuiMgr->SavePerspective().ToStdString();

		std::ifstream i(jsonFilePath);
		if (!i.good())
		{
			APP_LOG_ERROR("[Ui Manager]: Layout file not found, using default.");
			return;
		}

		try
		{
			nlohmann::json j;
			i >> j;

			if (j.contains("window"))
			{
				auto& win = j["window"];
				if (win.value("maximized", false))
					mFrame->Maximize();
				else
					mFrame->SetSize(win.value("width", 800), win.value("height", 600));
			}

			if (j.contains("layout_perspective"))
			{
				std::string perspective = j["layout_perspective"];
				mAuiMgr->LoadPerspective(perspective);
			}

			APP_LOG_INFO("[Ui Manager]: Layout loaded successfully.");

		}
		catch (const std::exception& e)
		{
			APP_LOG_ERROR("[Ui Manager]: Failed to load layout from JSON: {}, using default.", e.what());
		}
	}

	size_t UiManager::getId(const std::string& name)
	{
		// 第一次检查：读锁
		{
			std::shared_lock<std::shared_mutex> lock(mIdMutex);
			auto it = mNameToId.find(name);
			if (it != mNameToId.end())
				return it->second;
		}

		// 可能多个线程同时到达这里，需要写锁保护
		std::unique_lock<std::shared_mutex> lock(mIdMutex);
		// 第二次检查：防止其他线程已经插入
		auto it = mNameToId.find(name);
		if (it != mNameToId.end())
			return it->second;

		size_t newId = mNextId++;  // 写锁保护下安全
		mNameToId[name] = newId;
		mIdToName[newId] = name;
		return newId;
	}

	bool UiManager::removeId(const std::string& name)
	{
		std::unique_lock<std::shared_mutex> lock(mIdMutex);
		// std::lock_guard<std::mutex> lock(mMutex);
		auto it = mNameToId.find(name);
		if (it != mNameToId.end())
		{
			size_t id = it->second;
			mNameToId.erase(it);
			mIdToName.erase(id);
			return true;
		}
		return false;
	}

	size_t UiManager::createAnonymousId()
	{
		return mNextId++;
	}

	std::string UiManager::getName(size_t id) const
	{
		std::shared_lock<std::shared_mutex> lock(mIdMutex);
		// std::lock_guard<std::mutex> lock(mMutex);
		auto it = mIdToName.find(id);
		if (it != mIdToName.end())
			return it->second;
		return "Anonymous_or_Unknown";
	}
}
