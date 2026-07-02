/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2026 Core contributors and Euler LeE.
 */
#pragma once
#include "Core/defCoreApi.hpp"

#include "virPropertySetBase.hpp"

namespace core
{
	class CORE_API AttributeGroup : public PropertySetBase
	{
	public:
		AttributeGroup(ObjectBase* parent, const std::string& name)
			: PropertySetBase(name)
		{
			setParent(parent);
		}

		virtual ~AttributeGroup() = default;

	};
}