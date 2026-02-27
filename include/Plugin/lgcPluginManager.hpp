#pragma once
#include "defCoreApi.hpp"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <filesystem>
#include <memory>
#include <functional>
#include <mutex>

#include <nlohmann/json.hpp>

#include "virPluginBase.hpp"
#include "Config/lgcConfigManager.hpp" 

namespace core
{
	// 插件类型
	enum class PluginType
	{
		Builtin,
		ThirdParty
	};

	// 安装方式
	enum class InstallMethod
	{
		Copy,
		Link
	};

	// 运行状态
	enum class PluginState
	{
		Unloaded,   // 未加载（在清单中，但未加载DLL）
		Loaded,     // 已加载（DLL loaded & Init success）
		Disabled,   // 被禁用
		Broken,     // 文件损坏或缺失
		Error       // 加载过程出错
	};

	// 插件元数据
	struct PluginMetadata
	{
		std::string id;
		std::string name;
		std::string version;
		std::string binaryName;

		std::string defaultValuePath;

		std::vector<std::string> libraries;
		std::vector<std::string> resources;
		std::vector<std::string> dependencies;

		std::filesystem::path rootPath;
		PluginType type = PluginType::ThirdParty;
		InstallMethod installMethod = InstallMethod::Copy;

		bool isEnabled = true;
		std::string integrityStatus = "Unknown";
	};

	// 插件实例
	class PluginInstance
	{
	public:
		PluginMetadata metadata;
		PluginState state = PluginState::Unloaded;
		void* dllHandle = nullptr;

		std::shared_ptr<PluginBase> pluginObject;

	};

	class CORE_API PluginManager
	{
	public:
		static PluginManager& get();

		PluginManager(const PluginManager&) = delete;
		PluginManager& operator=(const PluginManager&) = delete;

		// --- 初始化与生命周期 ---

		/// <summary>
		/// 初始化系统：读取清单 -> 完整性检查 -> 预加载 Core 插件
		/// </summary>
		bool init(const std::string& coreDir, const std::string& userDir);

		/// <summary>
		/// 关闭系统：卸载所有插件
		/// </summary>
		void shutdown();

		/// <summary>
		/// 获取缺失的核心插件（如果非空，程序应报错并退出）
		/// </summary>
		std::vector<std::string> getMissingCorePlugins() const;

		/// <summary>
		/// 卸载所有插件（程序退出时调用）
		/// </summary>
		void unloadAll();

		// --- 加载逻辑 (懒加载) ---

		/// <summary>
		/// 加载单个插件 (会自动递归加载依赖)
		/// </summary>
		bool loadPlugin(const std::string& id);

		/// <summary>
		/// 批量加载所有已启用且未加载的插件
		/// </summary>
		void loadAll();

		// --- 状态控制 ---

		/// <summary>
		/// 设置插件启用/禁用
		/// 禁用时会自动递归禁用所有依赖此插件的上层插件
		/// </summary>
		/// <returns>受影响的插件ID列表</returns>
		std::vector<std::string> setPluginEnabled(const std::string& id, bool enabled);

		// --- 安装管理 ---
		bool installFromPath(const std::string& externalPath);
		bool installFromPackage(const std::string& zipPath);
		bool uninstallPlugin(const std::string& id);

		// --- 查询 ---
		std::shared_ptr<PluginInstance> getPlugin(const std::string& id);
		std::vector<std::shared_ptr<PluginInstance>> getAllPlugins() const;

	private:
		PluginManager() = default;
		~PluginManager();

		// 清单操作
		bool loadCoreManifest();
		bool loadUserManifest();
		void saveUserManifest();
		void parseManifestEntry(const std::string& id, const nlohmann::json& item, PluginType type);

		// 完整性与解析
		bool checkIntegrity(std::shared_ptr<PluginInstance> plugin);
		std::shared_ptr<PluginInstance> parseLocalPluginJson(const std::filesystem::path& dir);

		// 加载/卸载底层实现
		bool loadPluginRecursive(const std::string& id, std::set<std::string>& visited);
		bool loadDll(PluginInstance& plugin);
		void unloadDll(PluginInstance& plugin);

		// 递归禁用辅助
		void disablePluginRecursive(const std::string& targetId, std::vector<std::string>& affected, std::set<std::string>& visited);

		// 工具
		bool extractZip(const std::string& zipPath, const std::string& destDir);

	private:
		std::string mCoreManifestFile;
		std::string mUserManifestFile;
		std::filesystem::path mCoreDir;
		std::filesystem::path mUserDir;

		
		std::map<std::string, std::shared_ptr<PluginInstance>> mPlugins; // 插件表: ID -> Instance
		std::vector<std::string> mMissingCorePlugins; // 记录缺失的核心插件
		std::vector<std::string> mLoadOrder; // 记录加载顺序 (用于反向卸载)
	};
}
