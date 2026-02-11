#pragma once

#include <nlohmann/json.hpp>

namespace core
{
    class ObjectBase
    {
    public:
        ObjectBase() = default;
        virtual ~ObjectBase() = default;

        virtual bool read(const nlohmann::json &j) = 0;
        virtual nlohmann::json write() const = 0;
    };
} // namespace core