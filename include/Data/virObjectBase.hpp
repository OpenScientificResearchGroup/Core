/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
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