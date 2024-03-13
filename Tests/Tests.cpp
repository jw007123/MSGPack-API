#include "Tests.h"

namespace MSGPack
{
	bool Tests::Run()
	{
		using namespace std::chrono;

		std::function<bool()> TestFuncs[Test::Num] =
		{
			std::bind(&Tests::TestSimpleTypes,   this),
			std::bind(&Tests::TestBinaryAndExts, this),
			std::bind(&Tests::TestArrays,		 this),
			std::bind(&Tests::TestMaps,			 this)
		};

		const time_point<system_clock> allStartPt = system_clock::now();
		u32 testsPassed							  = 0;

		for (u32 i = 0; i < Test::Num; ++i)
		{
			const time_point<system_clock> testStartPt = system_clock::now();
			const bool testPassed					   = TestFuncs[i]();
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

	bool Tests::TestSimpleTypes()
	{
		Packer<> packer;
		for (u32 i = 0; i < 10; ++i)
		{
			packer.PackBool(i == 0);
		}

		packer.PackNil();

		for (u32 i = 0; i < 10; ++i)
		{
			packer.PackString(std::to_string(i));
		}

		for (u32 i = 0; i < 10; ++i)
		{
			packer.PackNumber<u8>(i);
			packer.PackNumber<u16>(i + std::numeric_limits<u8>::max());
			packer.PackNumber<u32>(i + std::numeric_limits<u16>::max());
			packer.PackNumber<u64>(i + std::numeric_limits<u32>::max());

			packer.PackNumber<i8>(i);
			packer.PackNumber<i16>(i + std::numeric_limits<i8>::max());
			packer.PackNumber<i32>(i + std::numeric_limits<i16>::max());
			packer.PackNumber<i64>(i + std::numeric_limits<i32>::max());

			packer.PackNumber<f32>(i);
			packer.PackNumber<f64>(i);
		}

		Unpacker<> unpacker(packer.Message());
		for (u32 i = 0; i < 10; ++i)
		{
			const bool v = unpacker.UnpackBool();
			if (((i == 0) && !v) || (i != 0 && v))
			{
				return false;
			}
		}

		unpacker.UnpackNil();

		for (u32 i = 0; i < 10; ++i)
		{
			const std::string v = unpacker.UnpackString();
			if (v != std::to_string(i))
			{
				return false;
			}
		}

		for (u32 i = 0; i < 10; ++i)
		{
			const u8 v0 = unpacker.UnpackNumber<u8>();
			if (v0 != i)
			{
				return false;
			}

			const u16 v1 = unpacker.UnpackNumber<u16>();
			if (v1 != (i + std::numeric_limits<u8>::max()))
			{
				return false;
			}

			const u32 v2 = unpacker.UnpackNumber<u32>();
			if (v2 != (i + std::numeric_limits<u16>::max()))
			{
				return false;
			}

			const u64 v3 = unpacker.UnpackNumber<u64>();
			if (v3 != (i + std::numeric_limits<u32>::max()))
			{
				return false;
			}

			const i8 i0 = unpacker.UnpackNumber<i8>();
			if (i0 != i)
			{
				return false;
			}

			const i16 i1 = unpacker.UnpackNumber<i16>();
			if (i1 != (i + std::numeric_limits<i8>::max()))
			{
				return false;
			}

			const i32 i2 = unpacker.UnpackNumber<i32>();
			if (i2 != (i + std::numeric_limits<i16>::max()))
			{
				return false;
			}

			const i64 i3 = unpacker.UnpackNumber<i64>();
			if (i3 != (i + std::numeric_limits<i32>::max()))
			{
				return false;
			}

			const f32 f0 = unpacker.UnpackNumber<f32>();
			if (std::abs(f0 - (f32)i) > std::numeric_limits<f32>::epsilon())
			{
				return false;
			}

			const f64 f1 = unpacker.UnpackNumber<f64>();
			if (std::abs(f1 - (f64)i) > std::numeric_limits<f64>::epsilon())
			{
				return false;
			}
		}

		return true;
	}

	bool Tests::TestBinaryAndExts()
	{
		std::array<f32, 10> arr;
		for (u32 i = 0; i < 10; ++i)
		{
			arr[i] = i;
		}

		Packer<> packer;
		for (u32 i = 1; i < 10; ++i)
		{
			packer.PackBinary((u8*)arr.data(), i * sizeof(f32));
			packer.PackExt(-123, (u8*)arr.data(), i * sizeof(f32));
		}

		Unpacker<> unpacker(packer.Message());
		for (u32 i = 1; i < 10; ++i)
		{
			std::pair<void*, u32> mem = unpacker.UnpackBinary();
			if (memcmp(mem.first, (void*)arr.data(), i * sizeof(f32)))
			{
				return false;
			}

			std::tuple<i32, void*, u32> tup = unpacker.UnpackExt();
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

	bool Tests::TestArrays()
	{
		Packer<> packer;
		for (u32 i = 0; i < 10; ++i)
		{
			packer.StartArray();
			for (u32 j = 0; j < (i * 100); ++j)
			{
				packer.PackNumber(j);
			}
			packer.EndArray();
		}

		Unpacker<> unpacker(packer.Message());
		for (u32 i = 0; i < 10; ++i)
		{
			const u32 sz = unpacker.UnpackArray();
			if (sz != (i * 100))
			{
				return false;
			}

			for (u32 j = 0; j < (i * 100); ++j)
			{
				const u32 v = unpacker.UnpackNumber<u32>();
				if (v != j)
				{
					return false;
				}
			}
		}

		return true;
	}

	bool Tests::TestMaps()
	{
		Packer<> packer;
		for (u32 i = 0; i < 10; ++i)
		{
			packer.StartMap();
			for (u32 j = 0; j < (i * 100); ++j)
			{
				packer.PackString(std::to_string(j));
				packer.PackNumber(j);
			}
			packer.EndMap();
		}

		Unpacker<> unpacker(packer.Message());
		for (u32 i = 0; i < 10; ++i)
		{
			const u32 sz = unpacker.UnpackMap();
			if (sz != (i * 100))
			{
				return false;
			}

			for (u32 j = 0; j < (i * 100); ++j)
			{
				const std::string s = unpacker.UnpackString();
				if (s != std::to_string(j))
				{
					return false;
				}

				const u32 v = unpacker.UnpackNumber<u32>();
				if (v != j)
				{
					return false;
				}
			}
		}

		return true;
	}
}
