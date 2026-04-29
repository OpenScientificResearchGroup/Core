/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "virProperty.hpp"

//#include <utility>

namespace core
{
	template <typename T>
	class Attribute : public Property<T>
	{
	public:
		Attribute(PropertyContainerBase* node, const std::string& key, const T& val)
			: Property<T>(node, key, val)
		{

		}

		Attribute(PropertyContainerBase* node, const std::string& key, T&& val)
			: Property<T>(node, key, std::forward<T>(val))
		{

		}

		virtual ~Attribute() = default;
	};
}