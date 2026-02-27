#pragma once
#include "defCoreApi.hpp"
#include <string>
#include <atomic>

//// 强制检查：如果插件和主程序 CRT 设置不一致，编译报错
//#if defined(_MT) && !defined(_DLL)
//#error "Must use Dynamic CRT (/MD or /MDd)"
//#endif

// 纯虚接口
namespace core
{
	class PluginBase
	{
	public:
		PluginBase() = default;
		virtual ~PluginBase() = default;

		// 获取插件信息
		virtual const std::string getId() const = 0;
		virtual const std::string getName() const = 0;
		virtual const std::string getVersion() const = 0;
		virtual const std::string getCert() const = 0;

		// 【关键】安装插件：传入主窗口指针，允许插件添加菜单、工具栏等
		virtual bool init() = 0;

		// 【关键】卸载插件：清理添加的菜单项、释放资源
		virtual void shutdown() = 0;

	};
}
