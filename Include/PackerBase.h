#pragma once

#include "Literals.h"

#include <string>

namespace MSGPack
{
	/*
	*	CRTP class for Packer. See https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
	*	for further details, but allows functions to be written around Packer<T, S, U> rather than
	*	a specific object (i.e. Packer<-1, true, false>). E.g.
	*
	*	template <typename T>
	*	void PackItIn(MSGPack::PackerBase<T>& packer_)
	*	{
	*		packer_.PackNil();
	*	}
	*
	*	void Caller()
	*	{
	*		// Different template arguments
	*		MSGPack::Packer<-1, true, false> p1;
	*		MSGPack::Packer<-1, true, true>  p2;
	*
	*		// Works regardless!
	*		PackItIn(p1);
	*		PackItIn(p2);
	*	}
	*/
	template <typename T>
	class PackerBase
	{
	public:
		void Clear()
		{
			static_cast<T&>(*this).Clear();
		}

		void PackNil()
		{
			static_cast<T&>(*this).PackNil();
		}

		void PackBool(const bool val_)
		{
			static_cast<T&>(*this).PackBool(val_);
		}

		template <typename S>
		void PackNumber(const S val_)
		{
			static_cast<T&>(*this).PackNumber<S>(val_);
		}

		void PackString(const char* val_)
		{
			static_cast<T&>(*this).PackString(val_);
		}

		void PackBinary(const u8* const val_, const u32 len_)
		{
			static_cast<T&>(*this).PackBinary(val_, len_);
		}

		void PackExt(const i32 type_, const u8* const data_, const u32 len_)
		{
			static_cast<T&>(*this).PackExt(type_, data_, len_);
		}

		void StartArray()
		{
			static_cast<T&>(*this).StartArray();
		}

		void EndArray()
		{
			static_cast<T&>(*this).EndArray();
		}

		void StartMap()
		{
			static_cast<T&>(*this).StartMap();
		}

		void EndMap()
		{
			static_cast<T&>(*this).EndMap();
		}

		u64 CurrentSize() const
		{
			return static_cast<T&>(*this).CurrentSize();
		}

		std::pair<void*, u64> Message() const
		{
			return static_cast<const T&>(*this).Message();
		}
	};
}
