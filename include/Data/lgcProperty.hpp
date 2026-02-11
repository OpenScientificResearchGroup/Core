#pragma once
#include "defCoreApi.hpp"
#include "virPropertyBase.hpp"

namespace core
{
    template <typename T> 
    class CORE_API Property : public PropertyBase
    {
	public:
		Property() = default;

		explicit Property(const T& val)
			: mVal(val)
		{

		}

		virtual ~Property() = default;

		virtual bool read(const nlohmann::json& j) override
		{
			if (!j.is_null())
				setValue(j.get<T>());
			return true;
		}

		virtual nlohmann::json write() const override
		{
			return nlohmann::json(mVal);
		}

		virtual void setValue(const T& val)
		{
			// if (val != mVal)
				mVal = val;
		}

		virtual const T& getValue() const
		{
			return mVal;
		}

		virtual const std::any getValueAny() const override
		{
			return mVal;
		}

		virtual Property<T>& operator=(const T& val)
		{
			setValue(val);
			return *this;
		}

		virtual operator T() const
		{
			return getValue();
		}
	protected:
		T mVal;
	};
}
