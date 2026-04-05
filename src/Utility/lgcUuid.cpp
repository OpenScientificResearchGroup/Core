/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace util
{
	namespace Uuid
	{
		std::string generate()
		{
			// 使用 16 字节 (128 位) 来存储 UUID 数据
			std::vector<unsigned char> data(16);

			// 使用 thread_local 优化性能，避免多线程竞争锁，且只初始化一次种子
			thread_local std::random_device rd;
			thread_local std::mt19937 gen(rd());
			thread_local std::uniform_int_distribution<> dis(0, 255);

			// 1. 生成 16 个随机字节
			for (auto& byte : data)
				byte = static_cast<unsigned char>(dis(gen));

			// 2. 设置版本号 (Version) 和 变体 (Variant)

			// 设置第 7 个字节 (索引 6) 的高 4 位为 0100 (即 4)
			// 这代表 Version 4 (随机 UUID)
			data[6] = (data[6] & 0x0f) | 0x40;

			// 设置第 9 个字节 (索引 8) 的高 2 位为 10
			// 这代表 Variant 1 (RFC 4122 标准)
			data[8] = (data[8] & 0x3f) | 0x80;

			// 3. 格式化为字符串
			std::stringstream ss;
			ss << std::hex << std::setfill('0');

			for (int i = 0; i < 16; ++i)
			{
				// 在第 4, 6, 8, 10 字节前插入连字符
				if (i == 4 || i == 6 || i == 8 || i == 10)
					ss << "-";
				ss << std::setw(2) << static_cast<int>(data[i]);
			}

			return ss.str();
		}
	}
} // namespace util
