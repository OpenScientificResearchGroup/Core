#pragma once
#include "defCoreApi.hpp"
#include "Data/lgcProperty.hpp"

#include <memory>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

// #include "Data/virContainerBase.hpp"

namespace core
{
	class ContainerBase;
	/// <summary>
	/// std::unordered_map<std::string, std::unique_ptr<ContainerBase>>特化特征类
	/// </summary>
    template <>
    class CORE_API Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>> : public PropertyBase
    {
	protected:
		std::unordered_map<std::string, std::unique_ptr<ContainerBase>> mVal;
	public:
		// Property() = delete;

		explicit Property();
		explicit Property(const std::unordered_map<std::string, std::unique_ptr<ContainerBase>>& value);
		
		Property(const Property&) = delete; // 禁用拷贝构造函数和拷贝赋值运算符
		Property& operator=(const Property&) = delete;
		Property(Property&&) = default; // 允许移动
		Property& operator=(Property&&) = default;

		/// <summary>
		/// 从json中读取数据
		/// </summary>
		/// <param name="doc">json</param>
		/// <param name="archive">压缩包上下文</param>
		/// <returns>true 如果读取成功，false 如果读取失败</returns>
		bool read(const nlohmann::json& j) override;

		/// <summary>
		/// 写入数据到json
		/// </summary>
		/// <param name="archive">压缩包上下文</param>
		/// <returns>json</returns>
		nlohmann::json write() const override;

		/// <summary>
		/// 设置特征引用
		/// </summary>
		/// <param name="val">Map映射</param>
		void setValue(const std::unordered_map<std::string, std::unique_ptr<ContainerBase>>& val);

		/// <summary>
		/// 获取特征引用
		/// </summary>
		/// <returns>Map映射</returns>
		const std::unordered_map<std::string, std::unique_ptr<ContainerBase>>& getValue() const;

		/// <summary>
		/// 获取特征引用
		/// </summary>
		/// <returns>std::any</returns>
		const std::any getValueAny() const override;

		/// <summary>
		/// 添加特征
		/// </summary>
		/// <param name="key">键</param>
		/// <param name="feature">值</param>
		void add(const std::string& key, std::unique_ptr<ContainerBase> feature);

		/// <summary>
		/// 删除特征
		/// </summary>
		/// <param name="key">键</param>
		void remove(const std::string& key);

		///// <summary>
		///// 获取类型
		///// </summary>
		///// <returns>类型</returns>
		//PropertyAttribute getAttr();

		/// <summary>
		/// 运算符重载（语法糖）
		/// </summary>
		/// <param name="val">值</param>
		/// <returns>指针</returns>
		Property<std::unordered_map<std::string, std::unique_ptr<ContainerBase>>>& operator=(const std::unordered_map<std::string, std::unique_ptr<ContainerBase>>& val);

		/// <summary>
		/// 允许直接读取
		/// </summary>
		operator std::unordered_map<std::string, std::unique_ptr<ContainerBase>>() const;

	};
}