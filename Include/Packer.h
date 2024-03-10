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

namespace MSGPack
{
	/// Indicate Size = -1 for dynamic
	template <u32 Size>
	class Packer
	{
	public:
		Packer();
		~Packer();

		/// Signifies no data
		void PackNil();

		/// Binary true/false value
		void PackBool(const bool val_);

		/// Force <T> if want exact type, otherwise let program decide
		template <typename T>
		void PackNumber(const T val_);

		/// Must be null-terminated
		void PackString(const char* val_);

		/// Binary in form of [val_ = ptr, len_ = size]
		void PackBinary(const u8* const val_, const u32 len_);

		/// Packs the ext type with the integer and data_
		void PackExt(const i32 type_, const u8* const data_, const u32 len_);

		/// Starts an array with the size determined between this call and EndArray()
		void StartArray();

		/// Stops writing to the array and defines the correct MSGPack size
		void EndArray();

		/// Starts a map with the size determined between this call and EndMap()
		void StartMap();

		/// Stops writing to the map and defines the correct MSGPack size
		void EndMap();

		/// Returns the size of data stored in this
		u64 CurrentSize() const;

		/// Returns a std::pair<void* u64> of the full packed message
		std::pair<void*, u64> Message() const;

	private:
		struct StartAndNumItems
		{
			u64 startIdx;
			u64 numItems;
		};

		std::variant<std::array<u8, (Size == -1) ? 1 : Size>, std::vector<u8>> data;
		u32																	   dataStaticSize;

		std::stack<StartAndNumItems> containerStartIdxs;

		/// Pushes a single byte onto the variant. Returns the position of the first byte
		u64 PushByte(const u8 byte_);

		/// Pushes a selection of bytes onto the variant. Returns the position of the first byte
		u64 PushBytes(const u8* const bytes_, const u64 size_);

		/// Changes the byte at position_ to val_
		void ChangeByte(const u64 position_, const u8 val_);

		/// Fix[type] functions
		void PackFixUInt(const u8 val_);
		void PackFixInt(const i8 val_);
		void PackFixStr(const char* val_, const u8 len_);

		/// Fixed sizes for u8, u16, u32, u64
		void PackU8(const u8 val_);
		void PackU16(const u16 val_);
		void PackU32(const u32 val_);
		void PackU64(const u64 val_);

		/// Fixed sizes for i8, i16, i32, i64
		void PackI8(const i8 val_);
		void PackI16(const i16 val_);
		void PackI32(const i32 val_);
		void PackI64(const i64 val_);

		/// Floats and doubles
		void PackF32(const f32 val_);
		void PackF64(const f64 val_);

		/// Various string sizes
		void PackStr8(const char* val_, const u8 len_);
		void PackStr16(const char* val_, const u16 len_);
		void PackStr32(const char* val_, const u32 len_);

		/// Binary blobs
		void PackBin8(const u8* const val_, const u8 len_);
		void PackBin16(const u8* const val_, const u16 len_);
		void PackBin32(const u8* const val_, const u32 len_);

		/// FixExt
		void PackFixExt1(const i32 type_, const u8* const data_);
		void PackFixExt2(const i32 type_, const u8* const data_);
		void PackFixExt4(const i32 type_, const u8* const data_);
		void PackFixExt8(const i32 type_, const u8* const data_);
		void PackFixExt16(const i32 type_, const u8* const data_);

		/// Ext
		void PackExt8(const i32 type_, const u8* const data_, const u8 len_);
		void PackExt16(const i32 type_, const u8* const data_, const u16 len_);
		void PackExt32(const i32 type_, const u8* const data_, const u32 len_);
	};

	template <u32 Size>
	Packer<Size>::Packer()
	{
		dataStaticSize = 0;

		if (Size != -1)
		{
			std::array<u8, Size>& arr = std::get<std::array<u8, Size>>(data);
			arr.fill('\0');
		}
	}

	template <u32 Size>
	Packer<Size>::~Packer()
	{
		// No open arrays/maps
		assert(containerStartIdxs.size() == 0);
	}

	template <u32 Size>
	void Packer<Size>::PackNil()
	{
		PushByte(ByteCodes::Nil);

		// Add to map/array size
		if (containerStartIdxs.size())
		{
			containerStartIdxs.top().numItems++;
		}
	}

	template <u32 Size>
	void Packer<Size>::PackBool(const bool val_)
	{
		if (val_)
		{
			PushByte(ByteCodes::BoolTrue);
		}
		else
		{
			PushByte(ByteCodes::BoolFalse);
		}

		// Add to map/array size
		if (containerStartIdxs.size())
		{
			containerStartIdxs.top().numItems++;
		}
	}

	template <u32 Size>
	template <typename T>
	void Packer<Size>::PackNumber(const T val_)
	{
		if constexpr (std::is_unsigned_v<T> && std::is_integral_v<T>)
		{
			if ((u8)val_ == val_)
			{
				if (val_ <= 127)
				{
					// Can pack using 7 bits
					PackFixUInt(val_);
				}
				else
				{
					PackU8(val_);
				}
			}
			else if ((u16)val_ == val_)
			{
				PackU16(val_);
			}
			else if ((u32)val_ == val_)
			{
				PackU32(val_);
			}
			else if ((u64)val_ == val_)
			{
				PackU64(val_);
			}
			else
			{
				assert(0);
			}
		}
		else if constexpr (std::is_signed_v<T> && std::is_integral_v<T>)
		{
			if ((i8)val_ == val_)
			{
				if (val_ < 0 && val_ >= -31)
				{
					// Can pack using 5 bits
					PackFixInt(val_);
				}
				else
				{
					PackI8(val_);
				}
			}
			else if ((i16)val_ == val_)
			{
				PackI16(val_);
			}
			else if ((i32)val_ == val_)
			{
				PackI32(val_);
			}
			else if ((i64)val_ == val_)
			{
				PackI64(val_);
			}
			else
			{
				assert(0);
			}
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			if constexpr (sizeof(T) == sizeof(f32))
			{
				PackF32(val_);
			}
			else if constexpr (sizeof(T) == sizeof(f64))
			{
				PackF64(val_);
			}
			else
			{
				assert(0);
			}
		}
		else
		{
			assert(0);
		}

		// Add to map/array size
		if (containerStartIdxs.size())
		{
			containerStartIdxs.top().numItems++;
		}
	}

	template <u32 Size>
	void Packer<Size>::PackString(const char* val_)
	{
		// Include null-terminator
		const u32 len = strlen(val_) + 1;

		if (len <= 31)
		{
			PackFixStr(val_, len);
		}
		else if (len <= std::numeric_limits<u8>::max())
		{
			// <= 2^8 - 1
			PackStr8(val_, len);
		}
		else if (len <= std::numeric_limits<u16>::max())
		{
			// <= 2^16 - 1
			PackStr16(val_, len);
		}
		else if (len <= std::numeric_limits<u32>::max())
		{
			// <= 2^32 - 1
			PackStr32(val_, len);
		}
		else
		{
			// Strings >= 2^32 not supported
			assert(0);
		}

		// Add to map/array size
		if (containerStartIdxs.size())
		{
			containerStartIdxs.top().numItems++;
		}
	}

	template <u32 Size>
	void Packer<Size>::PackBinary(const u8* const val_, const u32 len_)
	{
		if (len_ <= std::numeric_limits<u8>::max())
		{
			// <= 2^8 - 1
			PackBin8(val_, len_);
		}
		else if (len_ <= std::numeric_limits<u16>::max())
		{
			// <= 2^16 - 1
			PackBin16(val_, len_);
		}
		else if (len_ <= std::numeric_limits<u32>::max())
		{
			// <= 2^32 - 1
			PackBin32(val_, len_);
		}
		else
		{
			// Binary >= 2^32 not supported
			assert(0);
		}

		// Add to map/array size
		if (containerStartIdxs.size())
		{
			containerStartIdxs.top().numItems++;
		}
	}

	template <u32 Size>
	void Packer<Size>::PackExt(const i32 type_, const u8* const data_, const u32 len_)
	{
		if (len_ == 1)
		{
			PackFixExt1(type_, data_);
		}
		else if (len_ == 2)
		{
			PackFixExt2(type_, data_);
		}
		else if (len_ == 4)
		{
			PackFixExt4(type_, data_);
		}
		else if (len_ == 8)
		{
			PackFixExt8(type_, data_);
		}
		else if (len_ == 16)
		{
			PackFixExt16(type_, data_);
		}
		else if (len_ <= std::numeric_limits<u8>::max())
		{
			PackExt8(type_, data_, len_);
		}
		else if (len_ <= std::numeric_limits<u16>::max())
		{
			PackExt16(type_, data_, len_);
		}
		else if (len_ <= std::numeric_limits<u32>::max())
		{
			PackExt32(type_, data_, len_);
		}
		else
		{
			// ExtSize must be less than 2^32 - 1
			assert(0);
		}

		// Add to map/array size
		if (containerStartIdxs.size())
		{
			containerStartIdxs.top().numItems++;
		}
	}

	template <u32 Size>
	void Packer<Size>::StartArray()
	{
		// Add to map/array size. This goes before we push a new array as we're now counting
		// for that one instead
		if (containerStartIdxs.size())
		{
			containerStartIdxs.top().numItems++;
		}

		// Temp
		containerStartIdxs.push(StartAndNumItems{ PushByte(ByteCodes::NeverUse), 0 });
	}

	template <u32 Size>
	void Packer<Size>::EndArray()
	{
		const StartAndNumItems arrData = containerStartIdxs.top();
		if (arrData.numItems <= 15)
		{
			// Set byte as 1001[diff]
			u8 val = arrData.numItems;
			val    = val |  (1 << 7);
			val    = val & ~(1 << 6);
			val    = val & ~(1 << 5);
			val    = val |  (1 << 4);

			ChangeByte(arrData.startIdx, val);
		}
		else if (arrData.numItems <= std::numeric_limits<u16>::max())
		{
			ChangeByte(arrData.startIdx, ByteCodes::Arr16);
		}
		else if (arrData.numItems <= std::numeric_limits<u32>::max())
		{
			ChangeByte(arrData.startIdx, ByteCodes::Arr32);
		}
		else
		{
			// Arrays can only contain 2^32 - 1 elements
			assert(0);
		}

		// Array completed
		containerStartIdxs.pop();
	}

	template <u32 Size>
	void Packer<Size>::StartMap()
	{
		// Add to map/array size. This goes before we push a new map as we're now counting
		// for that one instead
		if (containerStartIdxs.size())
		{
			containerStartIdxs.top().numItems++;
		}

		// Temp
		containerStartIdxs.push(StartAndNumItems{ PushByte(ByteCodes::NeverUse), 0 });
	}

	template <u32 Size>
	void Packer<Size>::EndMap()
	{
		const StartAndNumItems mapData = containerStartIdxs.top();
		if (mapData.numItems <= (15 * 2))
		{
			// Set byte as 1000[diff]
			u8 val = mapData.numItems;
			val    = val |  (1 << 7);
			val    = val & ~(1 << 6);
			val    = val & ~(1 << 5);
			val    = val & ~(1 << 4);

			ChangeByte(mapData.startIdx, val);
		}
		else if (mapData.numItems <= (std::numeric_limits<u16>::max() * 2))
		{
			ChangeByte(mapData.startIdx, ByteCodes::Map16);
		}
		else if (mapData.numItems <= (std::numeric_limits<u32>::max() * 2))
		{
			ChangeByte(mapData.startIdx, ByteCodes::Map32);
		}
		else
		{
			// Maps can only contain 2^32 - 1 elements
			assert(0);
		}

		// Map completed
		containerStartIdxs.pop();
	}

	template <u32 Size>
	u64 Packer<Size>::CurrentSize() const
	{
		if constexpr (Size == -1)
		{
			const std::vector<u8>& arr = std::get<std::vector<u8>>(data);
			return arr.size();
		}
		else
		{
			return dataStaticSize;
		}
	}

	template <u32 Size>
	std::pair<void*, u64> Packer<Size>::Message() const
	{
		if constexpr (Size == -1)
		{
			const std::vector<u8>& arr = std::get<std::vector<u8>>(data);
			return std::make_pair<void*, u64>((void*)arr.data(), arr.size());
		}
		else
		{
			const std::array<u8, Size>& arr = std::get<std::array<u8, Size>>(data);
			return std::make_pair<void*, u64>((void*)arr.data(), dataStaticSize);
		}
	}

	template <u32 Size>
	void Packer<Size>::PackFixUInt(const u8 val_)
	{
		// Replace first bit in val with 0
		const u8 val = val_ & ~(1 << 7);

		PushByte(val);
	}

	template <u32 Size>
	void Packer<Size>::PackFixInt(const i8 val_)
	{
		// Replace first 3 bits in val with 1
		u8 val = val_;
		val    = val | (1 << 7);
		val    = val | (1 << 6);
		val    = val | (1 << 5);

		PushByte(val);
	}

	template <u32 Size>
	void Packer<Size>::PackFixStr(const char* val_, const u8 len_)
	{
		// Replace first 3 bits in len_ with 101
		u8 val = len_;
		val    = val |  (1 << 7);
		val    = val & ~(1 << 6);
		val    = val |  (1 << 5);

		PushByte(val);
		PushBytes((u8*)val_, len_);
	}

	template <u32 Size>
	u64 Packer<Size>::PushByte(const u8 byte_)
	{
		if constexpr (Size == -1)
		{
			std::vector<u8>& arr = std::get<std::vector<u8>>(data);
			arr.push_back(byte_);

			return (arr.size() - 1);
		}
		else
		{
			std::array<u8, Size>& arr = std::get<std::array<u8, Size>>(data);
			arr[dataStaticSize++]	  = byte_;

			return (dataStaticSize - 1);
		}
	}

	template <u32 Size>
	u64 Packer<Size>::PushBytes(const u8* const bytes_, const u64 size_)
	{
		if constexpr (Size == -1)
		{
			std::vector<u8>& arr = std::get<std::vector<u8>>(data);
			arr.insert(arr.end(), bytes_, bytes_ + size_);

			return (arr.size() - size_);
		}
		else
		{
			std::array<u8, Size>& arr = std::get<std::array<u8, Size>>(data);
			memcpy(arr.data() + dataStaticSize, bytes_, size_);
			dataStaticSize += size_;

			return (dataStaticSize - size_);
		}
	}

	template <u32 Size>
	void Packer<Size>::ChangeByte(const u64 position_, const u8 val_)
	{
		if constexpr (Size == -1)
		{
			std::vector<u8>& arr = std::get<std::vector<u8>>(data);
			arr[position_]		 = val_;
		}
		else
		{
			std::array<u8, Size>& arr = std::get<std::array<u8, Size>>(data);
			arr[position_]			  = val_;
		}
	}

	template <u32 Size>
	void Packer<Size>::PackU8(const u8 val_)
	{
		u8 bytes[1 + sizeof(u8)];
		bytes[0] = ByteCodes::UInt8;
		bytes[1] = val_;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size>
	void Packer<Size>::PackU16(const u16 val_)
	{
		const u16 nVal = htons(val_);

		u8 bytes[1 + sizeof(u16)];
		bytes[0] = ByteCodes::UInt16;
		bytes[1] = nVal		   & 0xFF;
		bytes[2] = (nVal >> 8) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size>
	void Packer<Size>::PackU32(const u32 val_)
	{
		const u32 nVal = htonl(val_);

		u8 bytes[1 + sizeof(u32)];
		bytes[0] = ByteCodes::UInt32;
		bytes[1] = nVal			& 0xFF;
		bytes[2] = (nVal >> 8)  & 0xFF;
		bytes[3] = (nVal >> 16) & 0xFF;
		bytes[4] = (nVal >> 24) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size>
	void Packer<Size>::PackU64(const u64 val_)
	{
		const u64 nVal = htonll(val_);

		u8 bytes[1 + sizeof(u64)];
		bytes[0] = ByteCodes::UInt64;
		bytes[1] = nVal			& 0xFF;
		bytes[2] = (nVal >> 8)  & 0xFF;
		bytes[3] = (nVal >> 16) & 0xFF;
		bytes[4] = (nVal >> 24) & 0xFF;
		bytes[5] = (nVal >> 32) & 0xFF;
		bytes[6] = (nVal >> 40) & 0xFF;
		bytes[7] = (nVal >> 48) & 0xFF;
		bytes[8] = (nVal >> 56) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size>
	void Packer<Size>::PackI8(const i8 val_)
	{
		u8 bytes[1 + sizeof(i8)];
		bytes[0] = ByteCodes::Int8;
		bytes[1] = val_;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size>
	void Packer<Size>::PackI16(const i16 val_)
	{
		// The u16/u32/u64 in these functions aren't typos; it makes
		// no difference either way
		const u16 nVal = htons(val_);

		u8 bytes[1 + sizeof(u16)];
		bytes[0] = ByteCodes::Int16;
		bytes[1] = nVal		   & 0xFF;
		bytes[2] = (nVal >> 8) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size>
	void Packer<Size>::PackI32(const i32 val_)
	{
		const u32 nVal = htonl(val_);

		u8 bytes[1 + sizeof(u32)];
		bytes[0] = ByteCodes::Int32;
		bytes[1] = nVal			& 0xFF;
		bytes[2] = (nVal >> 8)  & 0xFF;
		bytes[3] = (nVal >> 16) & 0xFF;
		bytes[4] = (nVal >> 24) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size>
	void Packer<Size>::PackI64(const i64 val_)
	{
		const u64 nVal = htonll(val_);

		u8 bytes[1 + sizeof(u64)];
		bytes[0] = ByteCodes::Int64;
		bytes[1] = nVal			& 0xFF;
		bytes[2] = (nVal >> 8)  & 0xFF;
		bytes[3] = (nVal >> 16) & 0xFF;
		bytes[4] = (nVal >> 24) & 0xFF;
		bytes[5] = (nVal >> 32) & 0xFF;
		bytes[6] = (nVal >> 40) & 0xFF;
		bytes[7] = (nVal >> 48) & 0xFF;
		bytes[8] = (nVal >> 56) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size>
	void Packer<Size>::PackF32(const f32 val_)
	{
		// We can recover f32/f64 values back later
		const u32 nVal = htonf(val_);

		u8 bytes[1 + sizeof(u32)];
		bytes[0] = ByteCodes::Float32;
		bytes[1] = nVal			& 0xFF;
		bytes[2] = (nVal >> 8)  & 0xFF;
		bytes[3] = (nVal >> 16) & 0xFF;
		bytes[4] = (nVal >> 24) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size>
	void Packer<Size>::PackF64(const f64 val_)
	{
		const u64 nVal = htond(val_);

		u8 bytes[1 + sizeof(u64)];
		bytes[0] = ByteCodes::Float64;
		bytes[1] = nVal			& 0xFF;
		bytes[2] = (nVal >> 8)  & 0xFF;
		bytes[3] = (nVal >> 16) & 0xFF;
		bytes[4] = (nVal >> 24) & 0xFF;
		bytes[5] = (nVal >> 32) & 0xFF;
		bytes[6] = (nVal >> 40) & 0xFF;
		bytes[7] = (nVal >> 48) & 0xFF;
		bytes[8] = (nVal >> 56) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size>
	void Packer<Size>::PackStr8(const char* val_, const u8 len_)
	{
		u8 bytes[1 + sizeof(u8)];
		bytes[0] = ByteCodes::String8;
		bytes[1] = len_;

		PushBytes(bytes, sizeof(bytes));
		PushBytes((u8*)val_, len_);
	}

	template <u32 Size>
	void Packer<Size>::PackStr16(const char* val_, const u16 len_)
	{
		const u16 nLen = htons(len_);

		u8 bytes[1 + sizeof(u16)];
		bytes[0] = ByteCodes::String16;
		bytes[1] = nLen		   & 0xFF;
		bytes[2] = (nLen >> 8) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
		PushBytes((u8*)val_, len_);
	}

	template <u32 Size>
	void Packer<Size>::PackStr32(const char* val_, const u32 len_)
	{
		const u32 nLen = htonl(len_);

		u8 bytes[1 + sizeof(u32)];
		bytes[0] = ByteCodes::String32;
		bytes[1] = nLen			& 0xFF;
		bytes[2] = (nLen >> 8)  & 0xFF;
		bytes[3] = (nLen >> 16) & 0xFF;
		bytes[4] = (nLen >> 24) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
		PushBytes((u8*)val_, len_);
	}

	template <u32 Size>
	void Packer<Size>::PackBin8(const u8* const val_, const u8 len_)
	{
		u8 bytes[1 + sizeof(u8)];
		bytes[0] = ByteCodes::Bin8;
		bytes[1] = len_;

		PushBytes(bytes, sizeof(bytes));
		PushBytes(val_, len_);
	}

	template <u32 Size>
	void Packer<Size>::PackBin16(const u8* const val_, const u16 len_)
	{
		const u16 nLen = htons(len_);

		u8 bytes[1 + sizeof(u16)];
		bytes[0] = ByteCodes::Bin16;
		bytes[1] = nLen		   & 0xFF;
		bytes[2] = (nLen >> 8) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
		PushBytes(val_, len_);
	}

	template <u32 Size>
	void Packer<Size>::PackBin32(const u8* const val_, const u32 len_)
	{
		const u32 nLen = htonl(len_);

		u8 bytes[1 + sizeof(u32)];
		bytes[0] = ByteCodes::Bin32;
		bytes[1] = nLen			& 0xFF;
		bytes[2] = (nLen >> 8)  & 0xFF;
		bytes[3] = (nLen >> 16) & 0xFF;
		bytes[4] = (nLen >> 24) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
		PushBytes(val_, len_);
	}

	template <u32 Size>
	void Packer<Size>::PackFixExt1(const i32 type_, const u8* const data_)
	{
		const i32 nType = htonl(type_);

		u8 bytes[1 + sizeof(i32) + 1];
		bytes[0] = ByteCodes::FixExt1;
		bytes[1] = nType		 & 0xFF;
		bytes[2] = (nType >> 8)  & 0xFF;
		bytes[3] = (nType >> 16) & 0xFF;
		bytes[4] = (nType >> 24) & 0xFF;
		bytes[5] = data_[0];

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size>
	void Packer<Size>::PackFixExt2(const i32 type_, const u8* const data_)
	{
		const i32 nType = htonl(type_);

		u8 bytes[1 + sizeof(i32) + 2];
		bytes[0] = ByteCodes::FixExt2;
		bytes[1] = nType		 & 0xFF;
		bytes[2] = (nType >> 8)  & 0xFF;
		bytes[3] = (nType >> 16) & 0xFF;
		bytes[4] = (nType >> 24) & 0xFF;
		bytes[5] = data_[0];
		bytes[6] = data_[1];

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size>
	void Packer<Size>::PackFixExt4(const i32 type_, const u8* const data_)
	{
		const i32 nType = htonl(type_);

		u8 bytes[1 + sizeof(i32) + 4];
		bytes[0] = ByteCodes::FixExt4;
		bytes[1] = nType		 & 0xFF;
		bytes[2] = (nType >> 8)  & 0xFF;
		bytes[3] = (nType >> 16) & 0xFF;
		bytes[4] = (nType >> 24) & 0xFF;
		bytes[5] = data_[0];
		bytes[6] = data_[1];
		bytes[7] = data_[2];
		bytes[8] = data_[3];

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size>
	void Packer<Size>::PackFixExt8(const i32 type_, const u8* const data_)
	{
		const i32 nType = htonl(type_);

		u8 bytes[1 + sizeof(i32) + 8];
		bytes[0]  = ByteCodes::FixExt8;
		bytes[1]  = nType		  & 0xFF;
		bytes[2]  = (nType >> 8)  & 0xFF;
		bytes[3]  = (nType >> 16) & 0xFF;
		bytes[4]  = (nType >> 24) & 0xFF;
		bytes[5]  = data_[0];
		bytes[6]  = data_[1];
		bytes[7]  = data_[2];
		bytes[8]  = data_[3];
		bytes[9]  = data_[4];
		bytes[10] = data_[5];
		bytes[11] = data_[6];
		bytes[12] = data_[7];

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size>
	void Packer<Size>::PackFixExt16(const i32 type_, const u8* const data_)
	{
		const i32 nType = htonl(type_);

		u8 bytes[1 + sizeof(i32) + 16];
		bytes[0]  = ByteCodes::FixExt16;
		bytes[1]  = nType		  & 0xFF;
		bytes[2]  = (nType >> 8)  & 0xFF;
		bytes[3]  = (nType >> 16) & 0xFF;
		bytes[4]  = (nType >> 24) & 0xFF;
		bytes[5]  = data_[0];
		bytes[6]  = data_[1];
		bytes[7]  = data_[2];
		bytes[8]  = data_[3];
		bytes[9]  = data_[4];
		bytes[10] = data_[5];
		bytes[11] = data_[6];
		bytes[12] = data_[7];
		bytes[13] = data_[8];
		bytes[14] = data_[9];
		bytes[15] = data_[10];
		bytes[16] = data_[11];
		bytes[17] = data_[12];
		bytes[18] = data_[13];
		bytes[19] = data_[14];
		bytes[20] = data_[15];

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size>
	void Packer<Size>::PackExt8(const i32 type_, const u8* const data_, const u8 len_)
	{
		const i32 nType = htonl(type_);

		u8 bytes[1 + sizeof(u8) + sizeof(i32)];
		bytes[0] = ByteCodes::Ext8;
		bytes[1] = len_;
		bytes[2] = nType		 & 0xFF;
		bytes[3] = (nType >> 8)  & 0xFF;
		bytes[4] = (nType >> 16) & 0xFF;
		bytes[5] = (nType >> 24) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
		PushBytes(data_, len_);
	}

	template <u32 Size>
	void Packer<Size>::PackExt16(const i32 type_, const u8* const data_, const u16 len_)
	{
		const i32 nType = htonl(type_);
		const u16 nLen  = htons(len_);

		u8 bytes[1 + sizeof(u16) + sizeof(i32)];
		bytes[0] = ByteCodes::Ext16;
		bytes[1] = nLen			 & 0xFF;
		bytes[2] = (nLen >> 8)   & 0xFF;
		bytes[3] = nType		 & 0xFF;
		bytes[4] = (nType >> 8)  & 0xFF;
		bytes[5] = (nType >> 16) & 0xFF;
		bytes[6] = (nType >> 24) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
		PushBytes(data_, len_);
	}

	template <u32 Size>
	void Packer<Size>::PackExt32(const i32 type_, const u8* const data_, const u32 len_)
	{
		const i32 nType = htonl(type_);
		const u32 nLen  = htonl(len_);

		u8 bytes[1 + sizeof(u32) + sizeof(i32)];
		bytes[0] = ByteCodes::Ext32;
		bytes[1] = nLen			 & 0xFF;
		bytes[2] = (nLen >> 8)   & 0xFF;
		bytes[3] = (nLen >> 16)  & 0xFF;
		bytes[4] = nType		 & 0xFF;
		bytes[5] = (nType >> 8)  & 0xFF;
		bytes[6] = (nType >> 16) & 0xFF;
		bytes[7] = (nType >> 24) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
		PushBytes(data_, len_);
	}
}
