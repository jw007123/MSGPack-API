#pragma once

#include "Literals.h"

#include <cassert>
#include <array>
#include <vector>
#include <variant>
#include <stack>
#include <stdexcept>

namespace MSGPack
{
	/*
	*	CRTP of Unpacker. See PackerBase.h for an explanation of why this class
	*	may be useful when using this library!
	*/
	template <typename T>
	class UnpackerBase
	{
	public:
		void Reset()
		{
			static_cast<T&>(*this).Reset();
		}

		void Set(const std::pair<void*, u64>& memBlock_)
		{
			static_cast<T&>(*this).Set(memBlock_);
		}

		ByteCodes PeekType() const
		{
			return static_cast<const T&>(*this).PeekType();
		}

		void UnpackNil()
		{
			static_cast<T&>(*this).UnpackNil();
		}

		bool UnpackBool()
		{
			return static_cast<T&>(*this).UnpackBool();
		}

		template <typename S>
		S UnpackNumber()
		{
			return static_cast<T&>(*this).UnpackNumber<S>();
		}

		std::string UnpackString()
		{
			return static_cast<T&>(*this).UnpackString();
		}

		std::pair<void*, u32> UnpackBinary()
		{
			return static_cast<T&>(*this).UnpackBinary();
		}

		std::tuple<i32, void*, u32> UnpackExt()
		{
			return static_cast<T&>(*this).UnpackExt();
		}

		u32 UnpackArray()
		{
			return static_cast<T&>(*this).UnpackArray();
		}

		u32 UnpackMap()
		{
			return static_cast<T&>(*this).UnpackMap();
		}
	};
}
