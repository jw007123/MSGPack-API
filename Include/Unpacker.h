#pragma once

#include "Literals.h"
#include "Bytecodes.h"

#ifdef _WINDOWS
	#define NOMINMAX
	#include "winsock2.h"
	#pragma comment(lib, "Ws2_32.lib")
#else
	#include <arpa/inet.h>
#endif

#include <cassert>
#include <array>
#include <vector>
#include <variant>
#include <stack>
#include <optional>
#include <stdexcept>

namespace MSGPack 
{
	/// No Time API
	template <bool Secure = _DEBUG>
	class Unpacker
	{
	public:
		Unpacker(const std::pair<void*, u64>& memBlock_);
		~Unpacker();

		/// Resets the unpack process to the beginning
		void Reset();

		/// Returns the ByteCode of the currently pointed-to type
		ByteCodes PeekType() const;

		/// Nil has no type, so just checks it exists and moves on
		void UnpackNil();

		/// Returns the value of the bool
		bool UnpackBool();

		/// Returns the packed scalar type. May not match the type given to Pack[] due to size optimisations
		template <typename T>
		T UnpackNumber();

		/// Returns the string via &&
		std::string UnpackString();

		/// Returns a ptr to the start of the binary blob in the memory block and its size. This ptr is only valid for
		/// as long as Unpacker exists, so it's recommended to memcpy/move this to your own memory ASAP
		std::pair<void*, u32> UnpackBinary();

		/// As with UnpackBinary, the data is only valid for Unpacker's life-time. Returns a tuple of the signed
		/// integer, ptr and size
		std::tuple<i32, void*, u32> UnpackExt();

		/// Starts the unpack process for an array. Returns the number of elements in the array
		u32 UnpackArray();

		/// Starts the unpack process for a map. Returns the number of elements in the map
		u32 UnpackMap();

	private:
		const void* const blockPtr;
		const u64		  blockSize;
		u64				  blockPos;

		/// Safely increments the blockPos member var
		void IncrementPosition(const u64 increment_);

		/// Returns the ptr to the element at blockPtr[blockPos_]
		template <typename T>
		T* GetData() const;

		/// Fix[type] functions
		u8			UnpackFixUInt();
		i8			UnpackFixInt();
		std::string UnpackFixStr();
		u8			UnpackFixArr();
		u8			UnpackFixMap();

		/// Fixed sizes for u8, u16, u32, u64
		u8  UnpackU8();
		u16 UnpackU16();
		u32 UnpackU32();
		u64 UnpackU64();

		/// Fixed sizes for i8, i16, i32, i64
		i8  UnpackI8();
		i16 UnpackI16();
		i32 UnpackI32();
		i64 UnpackI64();

		/// Floats and doubles
		f32 UnpackF32();
		f64 UnpackF64();

		/// Various string sizes
		std::string UnpackStr8();
		std::string UnpackStr16();
		std::string UnpackStr32();

		/// Binary blobs
		std::pair<void*, u32> UnpackBin8();
		std::pair<void*, u32> UnpackBin16();
		std::pair<void*, u32> UnpackBin32();

		/// FixExt
		template <u32 N>
		std::tuple<i32, void*, u32> UnpackFixExt();

		/// Ext
		std::tuple<i32, void*, u32> UnpackExt8();
		std::tuple<i32, void*, u32> UnpackExt16();
		std::tuple<i32, void*, u32> UnpackExt32();
	};

	template <bool Secure>
	Unpacker<Secure>::Unpacker(const std::pair<void*, u64>& memBlock_) :
					  blockPtr(memBlock_.first),
					  blockSize(memBlock_.second)
	{
		blockPos = 0;
	}

	template <bool Secure>
	Unpacker<Secure>::~Unpacker()
	{

	}

	template <bool Secure>
	void Unpacker<Secure>::Reset()
	{
		blockPos = 0;
	}

	template <bool Secure>
	ByteCodes Unpacker<Secure>::PeekType() const
	{
		// Get base code
		const ByteCodes code = (ByteCodes)(*GetData<u8>());

		// Check for fixed types
		if (code >= 0x00 && code <= 0x7f)
		{
			return ByteCodes::FixUInt8;
		}
		else if (code >= 0x80 && code <= 0x8f)
		{
			return ByteCodes::FixMap;
		}
		else if (code >= 0x90 && code <= 0x9f)
		{
			return ByteCodes::FixArr;
		}
		else if (code >= 0xa0 && code <= 0xbf)
		{
			return ByteCodes::FixString;
		}
		else if (code >= 0xe0 && code <= 0xff)
		{
			return ByteCodes::FixInt8;
		}

		// It's not fixed
		return code;
	}

	template <bool Secure>
	void Unpacker<Secure>::UnpackNil()
	{
		const ByteCodes code = PeekType();
		if (code != ByteCodes::Nil)
		{
			// Error
			if constexpr (Secure)
			{
				throw std::runtime_error("Incorrect ByteCode found during Unpack!");
			}

			return;
		}

		// Nil is a single byte
		blockPos++;
	}

	template <bool Secure>
	bool Unpacker<Secure>::UnpackBool()
	{
		const ByteCodes code = PeekType();
		if ((code != ByteCodes::BoolTrue) && (code != ByteCodes::BoolFalse))
		{
			// Error
			if constexpr (Secure)
			{
				throw std::runtime_error("Incorrect ByteCode found during Unpack!");
			}

			return false;
		}

		// Bool is a single byte
		blockPos++;
		return (code == ByteCodes::BoolTrue) ? true : false;
	}

	template <bool Secure>
	template <typename T>
	T Unpacker<Secure>::UnpackNumber()
	{
		const ByteCodes code = PeekType();

		switch (code)
		{
			case FixUInt8:
			{
				return UnpackFixUInt();
			}

			case UInt8:
			{
				return UnpackU8();
			}

			case UInt16:
			{
				return UnpackU16();
			}

			case UInt32:
			{
				return UnpackU32();
			}

			case UInt64:
			{
				return UnpackU64();
			}

			case FixInt8:
			{
				return UnpackFixInt();
			}

			case Int8:
			{
				return UnpackI8();
			}

			case Int16:
			{
				return UnpackI16();
			}

			case Int32:
			{
				return UnpackI32();
			}

			case Int64:
			{
				return UnpackI64();
			}

			case Float32:
			{
				return UnpackF32();
			}

			case Float64:
			{
				return UnpackF64();
			}

			default:
			{
				if constexpr (Secure)
				{
					throw std::runtime_error("Incorrect ByteCode found during Unpack!");
				}
			}
		}

		// Error. blockPos increments etc occur in switch functions
		return std::numeric_limits<T>::signaling_NaN();
	}

	template <bool Secure>
	std::string Unpacker<Secure>::UnpackString()
	{
		const ByteCodes code = PeekType();

		switch (code)
		{
			case FixString:
			{
				return UnpackFixStr();
			}

			case String8:
			{
				return UnpackStr8();
			}

			case String16:
			{
				return UnpackStr16();
			}

			case String32:
			{
				return UnpackStr32();
			}

			default:
			{
				if constexpr (Secure)
				{
					throw std::runtime_error("Incorrect ByteCode found during Unpack!");
				}
			}
		}

		// Error. blockPos increments etc occur in switch functions
		return std::string();
	}

	template <bool Secure>
	std::pair<void*, u32> Unpacker<Secure>::UnpackBinary()
	{
		const ByteCodes code = PeekType();

		switch (code)
		{
			case Bin8:
			{
				return UnpackBin8();
			}

			case Bin16:
			{
				return UnpackBin16();
			}

			case Bin32:
			{
				return UnpackBin32();
			}

			default:
			{
				if constexpr (Secure)
				{
					throw std::runtime_error("Incorrect ByteCode found during Unpack!");
				}
			}
		}

		// Error. blockPos increments etc occur in switch functions
		return std::make_pair<void*, u32>(nullptr, 0);
	}

	template <bool Secure>
	std::tuple<i32, void*, u32> Unpacker<Secure>::UnpackExt()
	{
		const ByteCodes code = PeekType();

		switch (code)
		{
			case FixExt1:
			{
				return UnpackFixExt<1>();
			}

			case FixExt2:
			{
				return UnpackFixExt<2>();
			}

			case FixExt4:
			{
				return UnpackFixExt<4>();
			}

			case FixExt8:
			{
				return UnpackFixExt<8>();
			}

			case FixExt16:
			{
				return UnpackFixExt<16>();
			}

			case Ext8:
			{
				return UnpackExt8();
			}

			case Ext16:
			{
				return UnpackExt16();
			}

			case Ext32:
			{
				return UnpackExt32();
			}

			default:
			{
				if constexpr (Secure)
				{
					throw std::runtime_error("Incorrect ByteCode found during Unpack!");
				}
			}
		}

		// Error. blockPos increments etc occur in switch functions
		return std::make_tuple<i32, void*, u32>(0, nullptr, 0);
	}

	template <bool Secure>
	u32 Unpacker<Secure>::UnpackArray()
	{
		const ByteCodes code = PeekType();

		switch (code)
		{
			case FixArr:
			{
				return UnpackFixArr();
			}

			case Arr16:
			{
				// Only interested in the numbers here
				return UnpackU16();
			}

			case Arr32:
			{
				// Only interested in the numbers here
				return UnpackU32();
			}

			default:
			{
				if constexpr (Secure)
				{
					throw std::runtime_error("Incorrect ByteCode found during Unpack!");
				}
			}
		}

		// Error. blockPos increments etc occur in switch functions
		return 0;
	}

	template <bool Secure>
	u32 Unpacker<Secure>::UnpackMap()
	{
		const ByteCodes code = PeekType();

		switch (code)
		{
			case FixMap:
			{
				return UnpackFixMap();
			}

			case Map16:
			{
				// Only interested in the numbers here
				return UnpackU16();
			}

			case Map32:
			{
				// Only interested in the numbers here
				return UnpackU32();
			}

			default:
			{
				if constexpr (Secure)
				{
					throw std::runtime_error("Incorrect ByteCode found during Unpack!");
				}
			}
		}

		// Error. blockPos increments etc occur in switch functions
		return 0;
	}

	template <bool Secure>
	void Unpacker<Secure>::IncrementPosition(const u64 increment_)
	{
		if constexpr (Secure)
		{
			if ((blockPos + increment_) > blockSize)
			{
				throw std::runtime_error("Error in Unpack() process. Attempted OOB access!");
			}
		}

		blockPos += increment_;
	}

	template <bool Secure>
	template <typename T>
	T* Unpacker<Secure>::GetData() const
	{
		if constexpr (Secure)
		{
			if (blockPos > blockSize)
			{
				throw std::runtime_error("Error in Unpack() process. Attempted OOB access!");
			}
		}

		return (T*)((u8*)blockPtr + blockPos);
	}

	template <bool Secure>
	u8 Unpacker<Secure>::UnpackFixUInt()
	{
		// We want the first 7 bits
		const u8 val = *GetData<u8>();

		// Increment blockPos by 1
		IncrementPosition(1);

		// val & 0111 1111
		return (val & 0x7f);
	}

	template <bool Secure>
	i8 Unpacker<Secure>::UnpackFixInt()
	{
		// We want first 5 bits
		u8 val = *GetData<u8>();

		// Increment blockPos by 1
		IncrementPosition(1);

		// val & 1110 1111
		val = (val & 0xef);

		// Cast to i8
		return *(i8*)&val;
	}

	template <bool Secure>
	std::string Unpacker<Secure>::UnpackFixStr()
	{
		// We want first 5 bits
		u8 strLen = *GetData<u8>();

		// Increment blockPos by 1
		IncrementPosition(1);

		// strLen & 1011 1111
		strLen = (strLen & 0xbf);

		// Pointer to next strLen bytes
		char* strData = GetData<char>();

		// Allow string to copy data
		std::string newStr = std::string(strData, strLen);

		// Safely increment blockPos
		IncrementPosition(strLen);

		// String is now safe to return
		return newStr;
	}

	template <bool Secure>
	u8 Unpacker<Secure>::UnpackFixArr()
	{
		// We want the first 4 bits
		const u8 val = *GetData<u8>();

		// Increment blockPos by 1
		IncrementPosition(1);

		// val & 1001 1111
		return (val & 0x9f);
	}

	template <bool Secure>
	u8 Unpacker<Secure>::UnpackFixMap()
	{
		// We want the first 4 bits
		const u8 val = *GetData<u8>();

		// Increment blockPos by 1
		IncrementPosition(1);

		// val & 1000 1111
		return (val & 0x8f);
	}

	template <bool Secure>
	u8 Unpacker<Secure>::UnpackU8()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value
		const u8 val = *GetData<u8>();

		// Increment over value
		IncrementPosition(sizeof(u8));

		// Return u8
		return val;
	}

	template <bool Secure>
	u16 Unpacker<Secure>::UnpackU16()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value in network order
		const u16 val = *GetData<u16>();

		// Move over sizeof(u16)
		IncrementPosition(sizeof(u16));

		// Network to host
		return ntohs(val);
	}

	template <bool Secure>
	u32 Unpacker<Secure>::UnpackU32()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value in network order
		const u32 val = *GetData<u32>();

		// Move over sizeof(u32)
		IncrementPosition(sizeof(u32));

		// Network to host
		return ntohl(val);
	}

	template <bool Secure>
	u64 Unpacker<Secure>::UnpackU64()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value in network order
		const u64 val = *GetData<u64>();

		// Move over sizeof(u64)
		IncrementPosition(sizeof(u64));

		// Network to host
		return ntohll(val);
	}

	template <bool Secure>
	i8 Unpacker<Secure>::UnpackI8()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value
		const u8 val = *GetData<u8>();

		// Increment over value
		IncrementPosition(sizeof(u8));

		// Return i8
		return *(i8*)&val;
	}

	template <bool Secure>
	i16 Unpacker<Secure>::UnpackI16()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value in network order
		const u16 val = *GetData<u16>();

		// Move over sizeof(u16)
		IncrementPosition(sizeof(u16));

		// Network to host
		const u16 hVal = ntohs(val);

		// Return i16
		return *(i16*)&hVal;
	}

	template <bool Secure>
	i32 Unpacker<Secure>::UnpackI32()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value in network order
		const u32 val = *GetData<u32>();

		// Move over sizeof(u32)
		IncrementPosition(sizeof(u32));

		// Network to host
		const u32 hVal = ntohl(val);

		// Return i32
		return *(i32*)&hVal;
	}

	template <bool Secure>
	i64 Unpacker<Secure>::UnpackI64()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value in network order
		const u64 val = *GetData<u64>();

		// Move over sizeof(u64)
		IncrementPosition(sizeof(u64));

		// Network to host
		const u64 hVal = ntohll(val);

		// Return i64
		return *(i64*)&hVal;
	}

	template <bool Secure>
	f32 Unpacker<Secure>::UnpackF32()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value in network order
		const u32 val = *GetData<u32>();

		// Move over sizeof(u32)
		IncrementPosition(sizeof(u32));

		// Network to host
		const u32 hVal = ntohs(val);

		// Return f32
		return *(f32*)&hVal;
	}

	template <bool Secure>
	f64 Unpacker<Secure>::UnpackF64()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value in network order
		const u64 val = *GetData<u64>();

		// Move over sizeof(u64)
		IncrementPosition(sizeof(u64));

		// Network to host
		const u64 hVal = ntohs(val);

		// Return f64
		return *(f64*)&hVal;
	}

	template <bool Secure>
	std::string Unpacker<Secure>::UnpackStr8()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value
		const u8 strLen = *GetData<u8>();

		// Increment over value
		IncrementPosition(sizeof(u8));

		// Pointer to next strLen bytes
		char* strData = GetData<char>();

		// Allow string to copy data
		std::string newStr = std::string(strData, strLen);

		// Safely increment blockPos
		IncrementPosition(strLen);

		// String is now safe to return
		return newStr;
	}

	template <bool Secure>
	std::string Unpacker<Secure>::UnpackStr16()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value in network order
		const u16 val = *GetData<u16>();

		// Move over sizeof(u16)
		IncrementPosition(sizeof(u16));

		// Network to host
		const u16 strLen = ntohs(val);

		// Pointer to next strLen bytes
		char* strData = GetData<char>();

		// Allow string to copy data
		std::string newStr = std::string(strData, strLen);

		// Safely increment blockPos
		IncrementPosition(strLen);

		// String is now safe to return
		return newStr;
	}

	template <bool Secure>
	std::string Unpacker<Secure>::UnpackStr32()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value in network order
		const u32 val = *GetData<u32>();

		// Move over sizeof(u32)
		IncrementPosition(sizeof(u32));

		// Network to host
		const u32 strLen = ntohl(val);

		// Pointer to next strLen bytes
		char* strData = GetData<char>();

		// Allow string to copy data
		std::string newStr = std::string(strData, strLen);

		// Safely increment blockPos
		IncrementPosition(strLen);

		// String is now safe to return
		return newStr;
	}

	template <bool Secure>
	std::pair<void*, u32> Unpacker<Secure>::UnpackBin8()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value
		const u8 binLen = *GetData<u8>();

		// Increment over value
		IncrementPosition(sizeof(u8));

		// Pointer to next binLen bytes
		u8* binData = GetData<u8>();

		// Create bin pair
		std::pair<void*, u32> binBlob((void*)binData, binLen);

		// Safely increment blockPos
		IncrementPosition(binLen);

		// Blob is now safe to return
		return binBlob;
	}

	template <bool Secure>
	std::pair<void*, u32> Unpacker<Secure>::UnpackBin16()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value in network order
		const u16 val = *GetData<u16>();

		// Move over sizeof(u16)
		IncrementPosition(sizeof(u16));

		// Network to host
		const u16 binLen = ntohs(val);

		// Pointer to next binLen bytes
		u8* binData = GetData<u8>();

		// Create bin pair
		std::pair<void*, u32> binBlob((void*)binData, binLen);

		// Safely increment blockPos
		IncrementPosition(binLen);

		// Blob is now safe to return
		return binBlob;
	}

	template <bool Secure>
	std::pair<void*, u32> Unpacker<Secure>::UnpackBin32()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value in network order
		const u32 val = *GetData<u32>();

		// Move over sizeof(u32)
		IncrementPosition(sizeof(u32));

		// Network to host
		const u32 binLen = ntohl(val);

		// Pointer to next binLen bytes
		u8* binData = GetData<u8>();

		// Create bin pair
		std::pair<void*, u32> binBlob((void*)binData, binLen);

		// Safely increment blockPos
		IncrementPosition(binLen);

		// Blob is now safe to return
		return binBlob;
	}

	template <bool Secure>
	template <u32 N>
	std::tuple<i32, void*, u32> Unpacker<Secure>::UnpackFixExt()
	{
		// Get integer
		const i32 intVal = UnpackI32();

		// Get data
		u8* data = GetData<u8>();

		// Create tuple
		std::tuple<i32, void*, u32> tuple(intVal, (void*)data, N);

		// Safely increment blockPos
		IncrementPosition(N);

		// Return tuple
		return tuple;
	}

	template <bool Secure>
	std::tuple<i32, void*, u32> Unpacker<Secure>::UnpackExt8()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value
		const u8 extLen = *GetData<u8>();

		// Move over extLen
		IncrementPosition(sizeof(u8));

		// Obtain integer value
		const u32 valN   = *GetData<u32>();
		const u32 intVal = ntohl(valN);

		// Move over integer
		IncrementPosition(sizeof(u32));

		// Get data
		u8* data = GetData<u8>();

		// Create tuple
		std::tuple<i32, void*, u32> tuple(*(i32*)&intVal, (void*)data, extLen);

		// Increment over data
		IncrementPosition(extLen);

		// Return tuple
		return tuple;
	}

	template <bool Secure>
	std::tuple<i32, void*, u32> Unpacker<Secure>::UnpackExt16()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value
		const u16 extLenN = *GetData<u16>();
		const u16 extLen  = ntohs(extLenN);

		// Move over extLen
		IncrementPosition(sizeof(u16));

		// Obtain integer value
		const u32 valN   = *GetData<u32>();
		const u32 intVal = ntohl(valN);

		// Move over integer
		IncrementPosition(sizeof(u32));

		// Get data
		u8* data = GetData<u8>();

		// Create tuple
		std::tuple<i32, void*, u32> tuple(*(i32*)&intVal, (void*)data, extLen);

		// Increment over data
		IncrementPosition(extLen);

		// Return tuple
		return tuple;
	}

	template <bool Secure>
	std::tuple<i32, void*, u32> Unpacker<Secure>::UnpackExt32()
	{
		// Move over ByteCode
		IncrementPosition(1);

		// Obtain value
		const u32 extLenN = *GetData<u32>();
		const u32 extLen  = ntohl(extLenN);

		// Move over extLen
		IncrementPosition(sizeof(u32));

		// Obtain integer value
		const u32 valN   = *GetData<u32>();
		const u32 intVal = ntohl(valN);

		// Move over integer
		IncrementPosition(sizeof(u32));

		// Get data
		u8* data = GetData<u8>();

		// Create tuple
		std::tuple<i32, void*, u32> tuple(*(i32*)&intVal, (void*)data, extLen);

		// Increment over data
		IncrementPosition(extLen);

		// Return tuple
		return tuple;
	}
}
