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
    class ServiceBase
    {
    public:
        virtual ~ServiceBase() = default;

        virtual bool init() { return true; }
        virtual void shutdown() {}
        virtual const std::string& getName() const = 0;
    };
}
