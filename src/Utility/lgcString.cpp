/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#include "Utility/lgcString.hpp"

#include <sstream>
#include <vector>

namespace util
{
    namespace string
    {
        std::string getFirstValidPathComponent(const std::string &pathStr)
        {
            std::filesystem::path p(pathStr);
            for (const auto &component : p)
            {
                std::string componentStr = component.string();

                // 检查组件是否为空（尽管在标准路径中不常见，但安全起见）
                // 或者检查它是否是根目录表示 (在 POSIX 上是 "/")
                if (componentStr.empty() || componentStr == "/")
                    // 忽略根目录或空组件，继续检查下一个
                    continue;

                // 找到了第一个非根、非空的组件，返回它
                return componentStr;
            }
            // 如果路径只包含 "/" 或为空，则返回空字符串
            return "";
        }

        std::vector<std::string> split(const std::string &s, char delimiter)
        {
            std::vector<std::string> tokens;
            std::string token;
            std::istringstream tokenStream(s);
            while (std::getline(tokenStream, token, delimiter))
                if (!token.empty())
                    tokens.push_back(token);
            return tokens;
        }
    }
}