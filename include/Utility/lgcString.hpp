#pragma once
#include <filesystem>
#include <string>

namespace util
{
    namespace string
    {
        std::string getFirstValidPathComponent(const std::string &pathStr);
        std::vector<std::string> split(const std::string &s, char delimiter);
    }
}