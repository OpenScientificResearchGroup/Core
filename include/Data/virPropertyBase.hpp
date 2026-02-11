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

        virtual const std::any getValueAny() const = 0;
    };
} // namespace core