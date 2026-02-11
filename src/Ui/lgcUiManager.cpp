#include "Ui/lgcUiManager.hpp"

#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <wx/artprov.h>

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>

namespace core
{
	UiManager& UiManager::get()
	{
		static UiManager instance;
		return instance;
	}

	UiManager::UiManager()
	{
		mNextId = wxID_HIGHEST + 1;
	}

	UiManager::~UiManager()
	{
		//if (mAuiMgr)
		//{
		//	mAuiMgr->UnInit();
		//	delete mAuiMgr;
		//}
		// double check
		shutdown();
	}

	void UiManager::init(const std::string& title, const size_t& width, const size_t& height)
	{
		mFrame = new wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(width, height));
		registerAnchor("/Core/main_frame", mFrame);

		mAuiMgr = new wxAuiManager();
		mAuiMgr->SetManagedWindow(mFrame);
		registerAnchor("/Core/aui_manager", mAuiMgr);

		//wxMenuBar* mb = new wxMenuBar();

		//wxMenu* fileMenu = new wxMenu();
		//fileMenu->Append(wxID_EXIT, "退出(&X)");
		//mb->Append(fileMenu, "文件(&F)");
		//registerAnchor("FileMenu", fileMenu);

		//wxMenu* editMenu = new wxMenu();
		//mb->Append(editMenu, "编辑(&E)");
		//registerAnchor("EditMenu", editMenu);

		//wxMenu* viewMenu = new wxMenu();
		//int idReset = getId("Global.Layout.Reset");
		//viewMenu->Append(idReset, "重置窗口布局");
		//viewMenu->AppendSeparator();
		//mb->Append(viewMenu, "插入(&V)");
		//registerAnchor("ViewMenu", viewMenu);

		//wxMenu* insertMenu = new wxMenu();
		//mb->Append(insertMenu, "插入(&I)");
		//registerAnchor("InsertMenu", insertMenu);

		//wxMenu* pluginMenu = new wxMenu();
		//int idPluginManager = getId("Global.Plugin.Manager");
		//pluginMenu->Append(idPluginManager, "插件管理器");
		//pluginMenu->AppendSeparator();
		//mb->Append(pluginMenu, "插件(&P)");
		//registerAnchor("PluginMenu", pluginMenu);

		//wxMenu* windowMenu = new wxMenu();
		//mb->Append(windowMenu, "窗口(&W)");
		//registerAnchor("WindowMenu", windowMenu);

		//wxMenu* helpMenu = new wxMenu();
		//helpMenu->Append(wxID_ABOUT, "关于(&A)...");
		//mb->Append(helpMenu, "帮助(&H)");
		//registerAnchor("HelpMenu", helpMenu);

		//mFrame->SetMenuBar(mb);
		//registerAnchor("MenuBar", mb);

		//mFrame->CreateStatusBar();
		//mFrame->SetStatusText("就绪");

		//mFrame->Bind(wxEVT_MENU, [this](wxCommandEvent&) { close(); }, wxID_EXIT);
		//mFrame->Bind(wxEVT_MENU, [this](wxCommandEvent&) { resetLayout(); }, idReset);
		//mFrame->Bind(wxEVT_MENU, [](wxCommandEvent&) { wxMessageBox("Core v1.0"); }, wxID_ABOUT);
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
			std::lock_guard<std::mutex> lock(mMutex);
			mAnchors.clear();
		}
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

	//void UiManager::addDockablePane(wxWindow* pane, DockSide side,
	//	const std::string& title, const std::string& internalName,
	//	int width, int height, const wxBitmap* icon)
	//{
	//	if (!mAuiMgr || !pane) return;

	//	wxAuiPaneInfo info;
	//	info.Name(internalName).Caption(title)
	//		.CloseButton(true).MaximizeButton(true).Floatable(true)
	//		.Dock().Resizable(true);

	//	if (icon) info.Icon(*icon);
	//	else info.Icon(wxArtProvider::GetBitmap(wxART_REPORT_VIEW, wxART_OTHER, wxSize(16, 16)));

	//	switch (side)
	//	{
	//	case DockSide::Left:   info.Left().Layer(1).Position(1).BestSize(width, -1); break;
	//	case DockSide::Right:  info.Right().Layer(1).Position(1).BestSize(width, -1); break;
	//	case DockSide::Top:    info.Top().Layer(1).Position(1).BestSize(-1, height); break;
	//	case DockSide::Bottom: info.Bottom().Layer(1).Position(1).BestSize(-1, height); break;
	//	case DockSide::Center: info.CenterPane().PaneBorder(false); break;
	//	}

	//	mAuiMgr->AddPane(pane, info);

	//	if (mDefaultPerspective.empty())
	//	{
	//	}

	//	if (side != DockSide::Center)
	//	{
	//		wxMenu* viewMenu = getOrAddViewMenu();
	//		if (viewMenu) {
	//			int menuId = wxNewId();
	//			viewMenu->AppendCheckItem(menuId, title);
	//			viewMenu->Check(menuId, true);

	//			mFrame->Bind(wxEVT_MENU, [this, internalName](wxCommandEvent& e) {
	//				togglePane(internalName, e.IsChecked());
	//				}, menuId);

	//			mPaneMenuMap[internalName] = menuId;
	//		}
	//	}

	//	mAuiMgr->Update();
	//}

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
		j["app_name"] = "IndustrialCad";
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
			std::cout << "[Layout] Saved to " << jsonFilePath << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cerr << "[Layout] Save failed: " << e.what() << std::endl;
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
			std::cout << "[Layout] Config not found, using default." << std::endl;
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

			// syncMenuState();

			std::cout << "[Layout] Loaded successfully." << std::endl;

		}
		catch (const std::exception& e)
		{
			std::cerr << "[Layout] Parse failed: " << e.what() << ", using default." << std::endl;
		}
	}

	//void UiManager::resetLayout()
	//{
	//	if (!mDefaultPerspective.empty())
	//	{
	//		mAuiMgr->LoadPerspective(mDefaultPerspective);
	//		syncMenuState();
	//	}
	//}

	int UiManager::getId(const std::string& name)
	{
		std::lock_guard<std::mutex> lock(mMutex);

		auto it = mNameToId.find(name);
		if (it != mNameToId.end())
			return it->second;

		int newId = mNextId++;

		mNameToId[name] = newId;
		mIdToName[newId] = name;

		return newId;
	}

	bool UiManager::removeId(const std::string& name)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		auto it = mNameToId.find(name);
		if (it != mNameToId.end())
		{
			int id = it->second;
			mNameToId.erase(it);
			mIdToName.erase(id);
			return true;
		}
		return false;
	}

	int UiManager::createAnonymousId()
	{
		return mNextId++;
	}

	std::string UiManager::getName(int id) const
	{
		std::lock_guard<std::mutex> lock(mMutex);
		auto it = mIdToName.find(id);
		if (it != mIdToName.end())
			return it->second;
		return "Anonymous_or_Unknown";
	}

	//void UiManager::syncMenuState()
	//{
	//	wxMenu* viewMenu = getOrAddViewMenu();
	//	if (!viewMenu) return;

	//	for (auto const& [name, menuId] : mPaneMenuMap)
	//	{
	//		auto& pane = mAuiMgr->GetPane(name);
	//		if (pane.IsOk())
	//			viewMenu->Check(menuId, pane.IsShown());
	//	}
	//}

	//void UiManager::togglePane(const std::string& internalName, bool show)
	//{
	//	auto& pane = mAuiMgr->GetPane(internalName);
	//	if (pane.IsOk())
	//	{
	//		pane.Show(show);
	//		mAuiMgr->Update();
	//	}
	//}

	//wxMenu* UiManager::getOrAddViewMenu()
	//{
	//	if (!mFrame || !mFrame->GetMenuBar()) return nullptr;
	//	int idx = mFrame->GetMenuBar()->FindMenu("视图(&V)");
	//	if (idx != wxNOT_FOUND) return mFrame->GetMenuBar()->GetMenu(idx);
	//	return nullptr;
	//}
}
