/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include <string>

namespace util
{
	namespace Uuid
	{
		/// <summary>
		/// 生成一个符合 RFC 4122 标准的 Version 4 UUID 字符串
		/// 格式: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
		/// 其中 y 是 8, 9, A, 或 B
		/// </summary>
		/// <returns>UUID</returns>
		std::string generate();
	}
}
