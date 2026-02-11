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
