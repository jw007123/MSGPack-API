#include "Examples.h"

namespace MSGPack
{
	void Example()
	{
		/*
		*	"myMap" :
		*	{
		*		"hello" : "world",
		*		"mynum" : 2
		*	},
		*	"myArray" :
		*	[
		*		123,
		*		456,
		*		789
		*	],
		*	"simple",
		*	"types",
		*	0,
		*	1,
		*	2,
		*	false,
		*	BINARY_DATA
		*/

		std::array<u8, 5> binaryBlob = { 0, 1, 2, 3, 4 };

		Packer<1000> packer;

		packer.PackString("myMap");
		packer.StartMap();
		{
			packer.PackString("hello");
			packer.PackString("world");

			packer.PackString("mynum");
			packer.PackNumber(2);
		}
		packer.EndMap();

		packer.PackString("myArray");
		packer.StartArray();
		{
			packer.PackNumber(123);
			packer.PackNumber(456);
			packer.PackNumber(789);
		}
		packer.EndArray();

		packer.PackString("simple");
		packer.PackString("types");
		packer.PackNumber(0);
		packer.PackNumber(1);
		packer.PackNumber(2);
		packer.PackBool(false);
		packer.PackBinary(binaryBlob.data(), binaryBlob.size());

		Unpacker<> unpacker(packer.Message());
		const std::pair<char*, u32> myMapStr = unpacker.UnpackString();
		const u32 mapSz						 = unpacker.UnpackMap();
		for (u32 i = 0; i < mapSz; ++i)
		{
			const std::pair<char*, u32> buff = unpacker.UnpackString();

			if (!strcmp(buff.first, "hello"))
			{
				const std::pair<char*, u32> world = unpacker.UnpackString();
			}
			else if (!strcmp(buff.first, "mynum"))
			{
				const u32 mynum = unpacker.UnpackNumber<u32>();
			}
			else
			{
				// Error
			}
		}

		const std::pair<char*, u32> myArrayStr = unpacker.UnpackString();
		const u32 arrSz						   = unpacker.UnpackArray();
		for (u32 i = 0; i < arrSz; ++i)
		{
			const u32 num = unpacker.UnpackNumber<u32>();
		}

		const std::pair<char*, u32> simple	= unpacker.UnpackString();
		const std::pair<char*, u32> types	= unpacker.UnpackString();
		const u32 n0						= unpacker.UnpackNumber<u32>();
		const u32 n1						= unpacker.UnpackNumber<u32>();
		const u32 n2						= unpacker.UnpackNumber<u32>();
		const bool b0					    = unpacker.UnpackBool();
		const std::pair<void*, u32> binBlob = unpacker.UnpackBinary();
	}
}
