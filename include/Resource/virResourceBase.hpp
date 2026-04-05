/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
 * If a copy of the MPL was not distributed with this file, You can obtain one at 
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "defCoreApi.hpp"

#include <string>

namespace core
{
	class CORE_API ResourceBase
	{
	public:
		virtual ~ResourceBase() = default;

		// 获取资源名称/路径
		const std::string& getName() const { return mName; }
		// 获取资源占用的内存大小（用于统计）
		virtual size_t getSizeInBytes() const = 0;

	protected:
		friend class ResourceManager;
		void setName(const std::string& name) { mName = name; }
		std::string mName;

	};
}
