/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Core/Utility/lgcTime.hpp"

#include <chrono>

namespace util
{
	namespace Time
	{
		unsigned long long getCurrentTimeInMilliseconds()
		{
			// 获取当前时间点
			auto now = std::chrono::system_clock::now();
			// 转换为自纪元以来的毫秒数
			auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
			return static_cast<unsigned long long>(milliseconds);
		}
	}
}