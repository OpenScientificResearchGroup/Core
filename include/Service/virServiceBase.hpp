#pragma once
#include "defCoreApi.hpp"

#include <string>

namespace core
{
    class ServiceBase
    {
    public:
        virtual ~ServiceBase() = default;

        virtual bool init() { return true; }

        virtual void shutdown() {}

        virtual const std::string& getName() const = 0;
    };
}
