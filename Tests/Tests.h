#pragma once

#include <chrono>
#include <functional>

#include "Packer.h"
#include "Unpacker.h"

namespace MSGPack
{
	class Tests
	{
	public:
		bool Run();

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

		bool TestSimpleTypes();
		bool TestBinaryAndExts();
		bool TestArrays();
		bool TestMaps();
	};
}
