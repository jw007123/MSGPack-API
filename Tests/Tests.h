#pragma once

#include <chrono>
#include <functional>

#include "Packer.h"
#include "Unpacker.h"

namespace MSGPack
{
	/*
	*	Basic unit-test class. Pass different specialisations of Packer and
	*	Unpacker as template arguments to test the full template set too.
	*/
	class Tests
	{
	public:
		template <typename T, typename S>
		bool Run(PackerBase<T>& packer_, UnpackerBase<S>& unpacker_);

	private:
		enum Test : u8
		{
			SimpleTypes   = 0,
			BinaryAndExts = 1,
			Arrays		  = 2,
			Maps		  = 3,
			Num
		};

		const char* TestStrings[Test::Num] =
		{
			"Simple Types",
			"Binary and Exts",
			"Arrays",
			"Maps"
		};

		template <typename T, typename S>
		bool TestSimpleTypes(PackerBase<T>& packer_, UnpackerBase<S>& unpacker_);

		template <typename T, typename S>
		bool TestBinaryAndExts(PackerBase<T>& packer_, UnpackerBase<S>& unpacker_);

		template <typename T, typename S>
		bool TestArrays(PackerBase<T>& packer_, UnpackerBase<S>& unpacker_);

		template <typename T, typename S>
		bool TestMaps(PackerBase<T>& packer_, UnpackerBase<S>& unpacker_);
	};

	template <typename T, typename S>
	bool Tests::Run(PackerBase<T>& packer_, UnpackerBase<S>& unpacker_)
	{
		using namespace std::chrono;
		using namespace std::placeholders;

		const time_point<system_clock> allStartPt = system_clock::now();
		u32 testsPassed							  = 0;

		for (u32 i = 0; i < Test::Num; ++i)
		{
			packer_.Clear();

			const time_point<system_clock> testStartPt = system_clock::now();

			bool testPassed;
			switch (i)
			{
				case Test::SimpleTypes:
				{
					testPassed = TestSimpleTypes(packer_, unpacker_);
					break;
				}
				case Test::BinaryAndExts:
				{
					testPassed = TestBinaryAndExts(packer_, unpacker_);
					break;
				}
				case Test::Arrays:
				{
					testPassed = TestArrays(packer_, unpacker_);
					break;
				}
				case Test::Maps:
				{
					testPassed = TestMaps(packer_, unpacker_);
					break;
				}
				default:
					assert(0);
					break;
			}

			const time_point<system_clock> testEndPt   = system_clock::now();
			const milliseconds testTime				   = duration_cast<milliseconds>(testEndPt - testStartPt);

			if (testPassed)
			{
				printf("%s: Passed in %zu[ms]\n\n", TestStrings[i], testTime.count());
			}
			else
			{
				printf("%s: Failed in %zu[ms]\n\n", TestStrings[i], testTime.count());
			}

			testsPassed += testPassed;
		}

		const time_point<system_clock> allEndPt = system_clock::now();
		const milliseconds allTime				= duration_cast<milliseconds>(allEndPt - allStartPt);

		if (testsPassed == Test::Num)
		{
			printf("All tests passed in %zu[ms]!\n\n", allTime.count());
		}
		else
		{
			printf("Some tests failed in %zu[ms]!\n\n", allTime.count());
		}

		return (testsPassed == Test::Num);
	}

	template <typename T, typename S>
	bool Tests::TestSimpleTypes(PackerBase<T>& packer_, UnpackerBase<S>& unpacker_)
	{
		for (u32 i = 0; i < 10; ++i)
		{
			packer_.PackBool(i == 0);
		}

		packer_.PackNil();

		for (u32 i = 0; i < 10; ++i)
		{
			packer_.PackString(std::to_string(i).c_str());
		}

		for (u32 i = 0; i < 10; ++i)
		{
			packer_.PackNumber<u8>(i);
			packer_.PackNumber<u16>(i + std::numeric_limits<u8>::max());
			packer_.PackNumber<u32>(i + std::numeric_limits<u16>::max());
			packer_.PackNumber<u64>(i + std::numeric_limits<u32>::max());

			packer_.PackNumber<i8>(i);
			packer_.PackNumber<i16>(i + std::numeric_limits<i8>::max());
			packer_.PackNumber<i32>(i + std::numeric_limits<i16>::max());
			packer_.PackNumber<i64>(i + std::numeric_limits<i32>::max());

			packer_.PackNumber<f32>(i);
			packer_.PackNumber<f64>(i);
		}

		unpacker_.Set(packer_.Message());
		for (u32 i = 0; i < 10; ++i)
		{
			const bool v = unpacker_.UnpackBool();
			if (((i == 0) && !v) || (i != 0 && v))
			{
				return false;
			}
		}

		unpacker_.UnpackNil();

		for (u32 i = 0; i < 10; ++i)
		{
			const char* v = unpacker_.UnpackString().first;
			if (strcmp(v, std::to_string(i).c_str()))
			{
				return false;
			}
		}

		for (u32 i = 0; i < 10; ++i)
		{
			const u8 v0 = unpacker_.UnpackNumber<u8>();
			if (v0 != i)
			{
				return false;
			}

			const u16 v1 = unpacker_.UnpackNumber<u16>();
			if (v1 != (i + std::numeric_limits<u8>::max()))
			{
				return false;
			}

			const u32 v2 = unpacker_.UnpackNumber<u32>();
			if (v2 != (i + std::numeric_limits<u16>::max()))
			{
				return false;
			}

			const u64 v3 = unpacker_.UnpackNumber<u64>();
			if (v3 != (i + std::numeric_limits<u32>::max()))
			{
				return false;
			}

			const i8 i0 = unpacker_.UnpackNumber<i8>();
			if (i0 != i)
			{
				return false;
			}

			const i16 i1 = unpacker_.UnpackNumber<i16>();
			if (i1 != (i + std::numeric_limits<i8>::max()))
			{
				return false;
			}

			const i32 i2 = unpacker_.UnpackNumber<i32>();
			if (i2 != (i + std::numeric_limits<i16>::max()))
			{
				return false;
			}

			const i64 i3 = unpacker_.UnpackNumber<i64>();
			if (i3 != (i + std::numeric_limits<i32>::max()))
			{
				return false;
			}

			const f32 f0 = unpacker_.UnpackNumber<f32>();
			if (std::abs(f0 - (f32)i) > std::numeric_limits<f32>::epsilon())
			{
				return false;
			}

			const f64 f1 = unpacker_.UnpackNumber<f64>();
			if (std::abs(f1 - (f64)i) > std::numeric_limits<f64>::epsilon())
			{
				return false;
			}
		}

		return true;
	}

	template <typename T, typename S>
	bool Tests::TestBinaryAndExts(PackerBase<T>& packer_, UnpackerBase<S>& unpacker_)
	{
		std::array<f32, 10> arr;
		for (u32 i = 0; i < 10; ++i)
		{
			arr[i] = i;
		}

		for (u32 i = 1; i < 10; ++i)
		{
			packer_.PackBinary((u8*)arr.data(), i * sizeof(f32));
			packer_.PackExt(-123, (u8*)arr.data(), i * sizeof(f32));
		}

		unpacker_.Set(packer_.Message());
		for (u32 i = 1; i < 10; ++i)
		{
			std::pair<void*, u32> mem = unpacker_.UnpackBinary();
			if (memcmp(mem.first, (void*)arr.data(), i * sizeof(f32)))
			{
				return false;
			}

			std::tuple<i32, void*, u32> tup = unpacker_.UnpackExt();
			if (std::get<0>(tup) != -123)
			{
				return false;
			}

			if (memcmp(std::get<1>(tup), (void*)arr.data(), i * sizeof(f32)))
			{
				return false;
			}
		}

		return true;
	}

	template <typename T, typename S>
	bool Tests::TestArrays(PackerBase<T>& packer_, UnpackerBase<S>& unpacker_)
	{
		for (u32 i = 0; i < 10; ++i)
		{
			packer_.StartArray();
			for (u32 j = 0; j < (i * 100); ++j)
			{
				packer_.PackNumber(j);
			}
			packer_.EndArray();
		}

		unpacker_.Set(packer_.Message());
		for (u32 i = 0; i < 10; ++i)
		{
			const u32 sz = unpacker_.UnpackArray();
			if (sz != (i * 100))
			{
				return false;
			}

			for (u32 j = 0; j < (i * 100); ++j)
			{
				const u32 v = unpacker_.UnpackNumber<u32>();
				if (v != j)
				{
					return false;
				}
			}
		}

		return true;
	}

	template <typename T, typename S>
	bool Tests::TestMaps(PackerBase<T>& packer_, UnpackerBase<S>& unpacker_)
	{
		for (u32 i = 0; i < 10; ++i)
		{
			packer_.StartMap();
			for (u32 j = 0; j < (i * 100); ++j)
			{
				packer_.PackString(std::to_string(j).c_str());
				packer_.PackNumber(j);
			}
			packer_.EndMap();
		}

		unpacker_.Set(packer_.Message());
		for (u32 i = 0; i < 10; ++i)
		{
			const u32 sz = unpacker_.UnpackMap();
			if (sz != (i * 100))
			{
				return false;
			}

			for (u32 j = 0; j < (i * 100); ++j)
			{
				const char* s = unpacker_.UnpackString().first;
				if (strcmp(s, std::to_string(j).c_str()))
				{
					return false;
				}

				const u32 v = unpacker_.UnpackNumber<u32>();
				if (v != j)
				{
					return false;
				}
			}
		}

		return true;
	}
}
