/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "virObjectBase.hpp"

#include <any>
#include <string>

#include <nlohmann/json.hpp>

namespace core
{
	class NodeBase;
    class PropertyContainerBase;

	struct PropertyLink
	{
		std::string uuid;				// 源节点的 ID
		std::string path;				// 源节点中的哪个属性
		bool isActive = false;			// 链接是否有效
	};

	class PropertyBase : public ObjectBase
	{
	public:
		PropertyBase(PropertyContainerBase* propertySet, const std::string& key);
		virtual ~PropertyBase() = default;

		ObjectType getObjectType() const override;

		virtual std::any getValueAny() const = 0;
		virtual void setValueAny(const std::any& value) = 0;

		NodeBase* getNode() const; // 通过所属属性容器访问父节点
		PropertyContainerBase* getContainer() const;

		// 引用元数据：支持引用整个节点（仅 node_uuid）或节点中的属性（node_uuid + property_key）。
		bool isLink() const;
		void setLink(const std::string& uuid, const std::string& key = "");
		void resetLink();
		const PropertyLink& getLink() const;
		bool readLink(const nlohmann::json& j);
		nlohmann::json writeLink() const;

		// 强制同步：由 Document 传入源属性的当前值，Property 自身不负责查找来源。
		virtual bool sync(const std::any& sourceValue) = 0;

		// UI元数据：由UI层维护，控制编辑权限和显示状态，不参与持久化
		//const bool isReadOnly() const;
		//void setReadOnly(bool isReadOnly);
		//const bool isVisible() const;
		//void setVisible(bool isVisible);

		virtual void attach() override;
		virtual void detach() override;
		virtual void update();
		virtual void link();
		virtual void unlink();

	protected:
		std::string mKey;							// 键
		//PropertyContainerBase* mPropertyContainer;	// 所属属性容器

		PropertyLink mLink;							// Link 信息，包含源节点 UUID、源属性 Key 和链接状态

		// 由UI层维护的元数据（不参与持久化），用于控制编辑权限和显示状态
		//bool mIsReadOnly;                   // 是否允许编辑（UI层可以根据这个标志决定是否启用编辑控件）
		//bool mIsVisible;                    // 是否可见（UI层可以根据这个标志决定是否显示属性）

	};
} // namespace core