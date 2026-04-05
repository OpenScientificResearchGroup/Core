# Core

![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![License](https://img.shields.io/badge/license-MIT-blue)
![Standard](https://img.shields.io/badge/c%2B%2B-17-blue)
<!-- ![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey) -->

[English](README.md)

**Core** 是一个高性能、工业级的 C++ 软件微内核架构，专为构建模块化、可扩展的桌面应用程序而设计。

其通过严格的分层架构实现了业务逻辑与底层基础设施的解耦。内置完善的管理器体系，让开发者可以专注于插件开发和业务实现，而无需关心底层的复杂性。

## 核心特性

本架构以微内核为中心，协调以下核心管理器：

*   **🔌 插件管理器 (Plugin Manager)**: 支持 DLL/SO 模块的动态加载/卸载，具备热插拔扩展能力。
*   **⚙️ 配置管理器 (Config Manager)**: 集中式配置管理，支持 JSON 及配置热重载。
*   **🌍 国际化管理器 (I18n Manager)**: 工业级多语言支持（动态语言切换、参数插值格式化）。
*   **📝 日志管理器 (Log Manager)**: 基于 spdlog 的高性能异步日志系统。
*   **📡 事件管理器 (Event Manager)**: 模块间通信解耦的事件总线（发布/订阅模式）。
*   **💾 数据管理器 (Data Manager)**: 抽象化的数据持久层接口。
*   **🎨 UI 管理器 (Ui Manager)**: 基于 wxWidgets 的跨平台 UI 生命周期管理。
*   **🛠️ 服务/任务/命令/资源管理器**: 提供后台任务调度、命令模式封装及资源池化管理等完整基础设施。

## 教程

教程在[这里](docs/tutorial/1.Overview.md)！

## 第三方依赖

本项目使用了以下优秀的开源库，感谢开发者的贡献：

*   **[wxWidgets](https://www.wxwidgets.org/)**: 跨平台 GUI 库 (wxWindows Library Licence)。
*   **[spdlog](https://github.com/gabime/spdlog)**: 极速 C++ 日志库 (MIT)。
*   **[fmt](https://github.com/fmtlib/fmt)**: 现代字符串格式化库 (MIT)。
*   **[json](https://github.com/nlohmann/json)**: 现代 C++ JSON 解析库 (MIT)。
*   **[minizip-ng](https://github.com/zlib-ng/minizip-ng)**: Zip 压缩解压库 (Zlib)。

<!-- ## 构建指南

### 前置条件
*   支持 C++17 或更高版本的编译器 (MSVC, GCC, Clang)
*   CMake 3.15+
*   建议使用 vcpkg 安装依赖库 -->

<!-- ### 编译步骤
```bash
# 克隆仓库
git clone https://github.com/OpenScientificResearchGroup/Core.git
cd Core

# CMake 配置 (假设使用 vcpkg)
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path/to/vcpkg]/scripts/buildsystems/vcpkg.cmake

# 编译
cmake --build build --config Release
``` -->

## 使用示例

```cpp
// winMain.hpp
#pragma once
#include <wx/wx.h>

class App : public wxApp
{
public:
	virtual bool OnInit() override;
	virtual int OnExit() override;
};
```

```cpp
// winMain.cpp
#include "winMain.hpp"

#include <wx/wx.h>

#include "lgcCore.hpp"

wxIMPLEMENT_APP(win::App);

bool App::OnInit()
{
	wxInitAllImageHandlers();
	core::initializeCore(
		"Test",
		"./user_conf.json",
		"./core_conf.json",
		"./CorePlugins",
		"./UserPlugins"
	);
	core::UiManager::get().show();
	SetTopWindow(core::UiManager::get().getRootWindow());
	return true;
}

int App::OnExit()
{
	core::shutdownCore();
	return 0;
}
```

## 许可证

本项目采用 **Mozilla Public License 2.0** 开源。详情请参阅 [LICENSE](LICENSE) 文件。