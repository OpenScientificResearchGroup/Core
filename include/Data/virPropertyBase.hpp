#pragma once
#include "virObjectBase.hpp"

#include <any>

namespace core
{
	class PropertyBase : public ObjectBase
	{
	public:
		PropertyBase() = default;
		virtual ~PropertyBase() = default;

		const bool isReadOnly() const { return mIsReadOnly; };
		void setReadOnly(bool isReadOnly) { mIsReadOnly = isReadOnly; };
		const bool isVisible() const { return mIsVisible; };
		void setVisible(bool isVisible) { mIsVisible = isVisible; };

		virtual const std::any getValueAny() const = 0;

	protected:
		bool mIsReadOnly = false; // 是否允许编辑（UI层可以根据这个标志决定是否启用编辑控件）
		bool mIsVisible = true; // 是否可见（UI层可以根据这个标志决定是否显示属性）
	};
} // namespace core