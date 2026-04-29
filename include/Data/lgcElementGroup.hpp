/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "virNodeSetBase.hpp"

#include <string>
#include <unordered_map>
#include <memory>

#include <nlohmann/json.hpp>

namespace core
{
	class ElementGroup : public NodeSetBase
	{
		ElementGroup() = default;
		virtual ~ElementGroup() = default;
	};
} // namespace core