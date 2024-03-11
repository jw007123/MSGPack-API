#pragma once

namespace MSGPack
{
	enum ByteCodes
	{
		Nil		  = 0xc0,	// Specials
		NeverUse  = 0xc1,
		BoolFalse = 0xc2,	// Bools
		BoolTrue  = 0xc3,	
		UInt8	  = 0xcc,	// UInts
		UInt16	  = 0xcd,
		UInt32	  = 0xce,
		UInt64	  = 0xcf,
		Int8	  = 0xd0,	// Ints
		Int16	  = 0xd1,
		Int32	  = 0xd2,
		Int64	  = 0xd3,	
		Float32	  = 0xca,	// Floats
		Float64	  = 0xcb,	
		String8	  = 0xd9,	// Strings
		String16  = 0xda,
		String32  = 0xdb,	
		Bin8	  = 0xc4,	// Bin
		Bin16	  = 0xc5,
		Bin32	  = 0xc6,
		Arr16	  = 0xdc,   // Array
		Arr32	  = 0xdd,
		Map16	  = 0xde,	// Map
		Map32	  = 0xdf,
		FixExt1	  = 0xd4,	// FixExt
		FixExt2	  = 0xd5,
		FixExt4   = 0xd6,
		FixExt8	  = 0xd7,
		FixExt16  = 0xd8,
		Ext8	  = 0xc7,	// Ext
		Ext16	  = 0xc8,
		Ext32	  = 0xc9,
		FixUInt8  = 0x00,	// -> 0x7f
		FixMap	  = 0x80,	// -> 0x8f
		FixArr	  = 0x90,	// -> 0x9f
		FixString = 0xa0,	// -> 0xbf
		FixInt8   = 0xe0	// -> 0xff
	};
}
