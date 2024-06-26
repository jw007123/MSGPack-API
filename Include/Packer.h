#pragma once

#include "Literals.h"
#include "Bytecodes.h"
#include "Defines.h"
#include "PackerBase.h"

#include <cassert>
#include <array>
#include <vector>
#include <variant>
#include <stack>
#include <stdexcept>

namespace MSGPack
{
	/*
	*	Size   := If != std::numeric_limits<u32>::max(), uses a fixed
	*			  store on the stack of Size bytes.
	* 
	*   Secure := Performs certain run-time checks to guard against
	*			  incorrectly-packed data
	*
	*	Local  := Disables hton[s/l/ll] endianness conversions on the assumption
	*			  that packing and unpacking is an operation local to the PC.
	*/
	template <u32  Size   = std::numeric_limits<u32>::max(),
			  bool Secure = SecureBase,
			  bool Local  = false>
	class Packer : public PackerBase<Packer<Size, Secure, Local>>
	{
	public:
		Packer();
		~Packer();

		/// Clears the packer
		void Clear();

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

		std::variant<std::array<u8, (Size == std::numeric_limits<u32>::max()) ? 1 : Size>,
					 std::vector<u8>> data;
		u32							  dataStaticSize;

		std::stack<StartAndNumItems> containerStartIdxs;

		/// Pushes a single byte onto the variant. Returns the position of the first byte
		u64 PushByte(const u8 byte_);

		/// Pushes a selection of bytes onto the variant. Returns the position of the first byte
		u64 PushBytes(const u8* const bytes_, const u64 size_);

		/// Changes the byte at position_ to val_
		void ChangeByte(const u64 position_, const u8 val_);

		/// Changes the selection of bytes starting at position_ to bytes_
		void ChangeBytes(const u64 position_, const u8* const bytes_, const u32 len_);

		/// Host -> Network byte order functions
		u16 HostToNetwork(const u16 val_) const;
		u32 HostToNetwork(const u32 val_) const;
		u64 HostToNetwork(const u64 val_) const;

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
		template <u32 N>
		void PackFixExtN(const i32 type_, const u8* const data_);

		/// Ext
		void PackExt8(const i32 type_, const u8* const data_, const u8 len_);
		void PackExt16(const i32 type_, const u8* const data_, const u16 len_);
		void PackExt32(const i32 type_, const u8* const data_, const u32 len_);
	};

	/*
	*	Public
	*/

	template <u32 Size, bool Secure, bool Local>
	Packer<Size, Secure, Local>::Packer()
	{
		if constexpr (Size != std::numeric_limits<u32>::max())
		{
			std::array<u8, Size>& arr = std::get<std::array<u8, Size>>(data);
			arr.fill('\0');

			dataStaticSize = 0;
		}
		else
		{
			data = std::vector<u8>();
		}
	}

	template <u32 Size, bool Secure, bool Local>
	Packer<Size, Secure, Local>::~Packer()
	{
		// No open arrays/maps
		if constexpr (Secure)
		{
			if (containerStartIdxs.size() != 0)
			{
				throw std::runtime_error("Open Maps/Arrays when Pack completed!");
			}
		}
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::Clear()
	{
		while (!containerStartIdxs.empty())
		{
			containerStartIdxs.pop();
		}

		if constexpr (Size == std::numeric_limits<u32>::max())
		{
			std::vector<u8>& arr = std::get<std::vector<u8>>(data);
			arr.clear();
		}
		else
		{
			dataStaticSize = 0;
		}
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackNil()
	{
		PushByte(ByteCodes::Nil);

		// Add to map/array size
		if (containerStartIdxs.size())
		{
			containerStartIdxs.top().numItems++;
		}
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackBool(const bool val_)
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

	template <u32 Size, bool Secure, bool Local>
	template <typename T>
	void Packer<Size, Secure, Local>::PackNumber(const T val_)
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
				if constexpr (Secure)
				{
					throw std::runtime_error("Unknown error during PackNumber!");
				}
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
				if constexpr (Secure)
				{
					throw std::runtime_error("Unknown error during PackNumber!");
				}
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
				if constexpr (Secure)
				{
					throw std::runtime_error("Unknown error during PackNumber!");
				}
			}
		}
		else
		{
			if constexpr (Secure)
			{
				throw std::runtime_error("Unknown error during PackNumber!");
			}
		}

		// Add to map/array size
		if (containerStartIdxs.size())
		{
			containerStartIdxs.top().numItems++;
		}
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackString(const char* val_)
	{
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
			if constexpr (Secure)
			{
				throw std::runtime_error("Strings >= 2^32 not supported during Pack!");
			}
		}

		// Add to map/array size
		if (containerStartIdxs.size())
		{
			containerStartIdxs.top().numItems++;
		}
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackBinary(const u8* const val_, const u32 len_)
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
			if constexpr (Secure)
			{
				throw std::runtime_error("Binary >= 2^32 not supported during Pack!");
			}
		}

		// Add to map/array size
		if (containerStartIdxs.size())
		{
			containerStartIdxs.top().numItems++;
		}
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackExt(const i32 type_, const u8* const data_, const u32 len_)
	{
		if (len_ == 1)
		{
			PackFixExtN<0>(type_, data_);
		}
		else if (len_ == 2)
		{
			PackFixExtN<1>(type_, data_);
		}
		else if (len_ == 4)
		{
			PackFixExtN<2>(type_, data_);
		}
		else if (len_ == 8)
		{
			PackFixExtN<3>(type_, data_);
		}
		else if (len_ == 16)
		{
			PackFixExtN<4>(type_, data_);
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
			if constexpr (Secure)
			{
				throw std::runtime_error("ExtSize >= 2^32 not supported during Pack!");
			}
		}

		// Add to map/array size
		if (containerStartIdxs.size())
		{
			containerStartIdxs.top().numItems++;
		}
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::StartArray()
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

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::EndArray()
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
			const u16 nVal = HostToNetwork((u16)arrData.numItems);

			u8 bytes[1 + sizeof(u16)];
			bytes[0] = ByteCodes::Arr16;
			bytes[1] = nVal		   & 0xFF;
			bytes[2] = (nVal >> 8) & 0xFF;

			ChangeBytes(arrData.startIdx, bytes, sizeof(bytes));
		}
		else if (arrData.numItems <= std::numeric_limits<u32>::max())
		{
			const u32 nVal = HostToNetwork((u32)arrData.numItems);

			u8 bytes[1 + sizeof(u32)];
			bytes[0] = ByteCodes::Arr32;
			bytes[1] = nVal			& 0xFF;
			bytes[2] = (nVal >> 8)  & 0xFF;
			bytes[3] = (nVal >> 16) & 0xFF;
			bytes[4] = (nVal >> 24) & 0xFF;

			ChangeBytes(arrData.startIdx, bytes, sizeof(bytes));
		}
		else
		{
			if constexpr (Secure)
			{
				throw std::runtime_error("Array element count >= 2^32 not supported during Pack!");
			}
		}

		// Array completed
		containerStartIdxs.pop();
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::StartMap()
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

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::EndMap()
	{
		// Get top, check it is key : value and then / 2 to make rest of func easier
		StartAndNumItems mapData = containerStartIdxs.top();
		if constexpr (Secure)
		{
			if (mapData.numItems % 2)
			{
				throw std::runtime_error("Map with !%2 elements detected during Pack!");
			}
		}
		mapData.numItems /= 2;

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
			const u16 nVal = HostToNetwork((u16)mapData.numItems);

			u8 bytes[1 + sizeof(u16)];
			bytes[0] = ByteCodes::Map16;
			bytes[1] = nVal		   & 0xFF;
			bytes[2] = (nVal >> 8) & 0xFF;

			ChangeBytes(mapData.startIdx, bytes, sizeof(bytes));
		}
		else if (mapData.numItems <= (std::numeric_limits<u32>::max() * 2))
		{
			const u32 nVal = HostToNetwork((u32)mapData.numItems);

			u8 bytes[1 + sizeof(u32)];
			bytes[0] = ByteCodes::Map32;
			bytes[1] = nVal			& 0xFF;
			bytes[2] = (nVal >> 8)  & 0xFF;
			bytes[3] = (nVal >> 16) & 0xFF;
			bytes[4] = (nVal >> 24) & 0xFF;

			ChangeBytes(mapData.startIdx, bytes, sizeof(bytes));
		}
		else
		{
			// Maps can only contain 2^32 - 1 elements
			if constexpr (Secure)
			{
				throw std::runtime_error("Map element count >= 2^32 not supported during Pack!");
			}
		}

		// Map completed
		containerStartIdxs.pop();
	}

	template <u32 Size, bool Secure, bool Local>
	u64 Packer<Size, Secure, Local>::CurrentSize() const
	{
		if constexpr (Size == std::numeric_limits<u32>::max())
		{
			const std::vector<u8>& arr = std::get<std::vector<u8>>(data);
			return arr.size();
		}
		else
		{
			return dataStaticSize;
		}
	}

	template <u32 Size, bool Secure, bool Local>
	std::pair<void*, u64> Packer<Size, Secure, Local>::Message() const
	{
		if constexpr (Size == std::numeric_limits<u32>::max())
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

	/*
	*	Private
	*/

	template <u32 Size, bool Secure, bool Local>
	u16 Packer<Size, Secure, Local>::HostToNetwork(const u16 val_) const
	{
		if constexpr (Local)
		{
			return val_;
		}
		else
		{
			#if defined(_WINDOWS)
				return htons(val_);
			#else
				return htobe16(val_);
			#endif
		}
	}

	template <u32 Size, bool Secure, bool Local>
	u32 Packer<Size, Secure, Local>::HostToNetwork(const u32 val_) const
	{
		if constexpr (Local)
		{
			return val_;
		}
		else
		{
			#if defined(_WINDOWS)
				return htonl(val_);
			#else
				return htobe32(val_);
			#endif
		}
	}

	template <u32 Size, bool Secure, bool Local>
	u64 Packer<Size, Secure, Local>::HostToNetwork(const u64 val_) const
	{
		if constexpr (Local)
		{
			return val_;
		}
		else
		{
			#if defined(_WINDOWS)
				return htonll(val_);
			#else
				return htobe64(val_);
			#endif
		}
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackFixUInt(const u8 val_)
	{
		// Replace last bit in val with 0
		const u8 val = val_ & ~(1 << 7);

		PushByte(val);
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackFixInt(const i8 val_)
	{
		// Replace last 3 bits in val with 1
		u8 val = val_;
		val    = val | (1 << 7);
		val    = val | (1 << 6);
		val    = val | (1 << 5);

		PushByte(val);
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackFixStr(const char* val_, const u8 len_)
	{
		// Replace last 3 bits in len_ with 101
		u8 val = len_;
		val    = val |  (1 << 7);
		val    = val & ~(1 << 6);
		val    = val |  (1 << 5);

		PushByte(val);
		PushBytes((u8*)val_, len_);
	}

	template <u32 Size, bool Secure, bool Local>
	u64 Packer<Size, Secure, Local>::PushByte(const u8 byte_)
	{
		if constexpr (Size == std::numeric_limits<u32>::max())
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

	template <u32 Size, bool Secure, bool Local>
	u64 Packer<Size, Secure, Local>::PushBytes(const u8* const bytes_, const u64 size_)
	{
		if constexpr (Size == std::numeric_limits<u32>::max())
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

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::ChangeByte(const u64 position_, const u8 val_)
	{
		if constexpr (Size == std::numeric_limits<u32>::max())
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

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::ChangeBytes(const u64 position_, const u8* const bytes_, const u32 len_)
	{
		if constexpr (Size == std::numeric_limits<u32>::max())
		{
			std::vector<u8>& arr = std::get<std::vector<u8>>(data);
			arr.insert(arr.begin() + position_, len_ - 1, ByteCodes::NeverUse);
		}
		else
		{
			std::array<u8, Size>& arr = std::get<std::array<u8, Size>>(data);

			// i32 to avoid position_ = 0 and u32--
			for (i32 i = (dataStaticSize - 1); i > position_; --i)
			{
				// Backwards iteration to avoid overwriting data
				arr[i + (len_ - 1)] = arr[i];
			}

			// Add extra bytes
			dataStaticSize += (len_ - 1);
		}

		for (u32 i = 0; i < len_; ++i)
		{
			ChangeByte(position_ + i, bytes_[i]);
		}
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackU8(const u8 val_)
	{
		u8 bytes[1 + sizeof(u8)];
		bytes[0] = ByteCodes::UInt8;
		bytes[1] = val_;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackU16(const u16 val_)
	{
		const u16 nVal = HostToNetwork(val_);

		u8 bytes[1 + sizeof(u16)];
		bytes[0] = ByteCodes::UInt16;
		bytes[1] = nVal		   & 0xFF;
		bytes[2] = (nVal >> 8) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackU32(const u32 val_)
	{
		const u32 nVal = HostToNetwork(val_);

		u8 bytes[1 + sizeof(u32)];
		bytes[0] = ByteCodes::UInt32;
		bytes[1] = nVal			& 0xFF;
		bytes[2] = (nVal >> 8)  & 0xFF;
		bytes[3] = (nVal >> 16) & 0xFF;
		bytes[4] = (nVal >> 24) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackU64(const u64 val_)
	{
		const u64 nVal = HostToNetwork(val_);

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

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackI8(const i8 val_)
	{
		u8 bytes[1 + sizeof(i8)];
		bytes[0] = ByteCodes::Int8;
		bytes[1] = val_;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackI16(const i16 val_)
	{
		// The u16/u32/u64 in these functions aren't typos; it makes
		// no difference either way
		const u16 nVal = HostToNetwork(*(u16*)&val_);

		u8 bytes[1 + sizeof(u16)];
		bytes[0] = ByteCodes::Int16;
		bytes[1] = nVal		   & 0xFF;
		bytes[2] = (nVal >> 8) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackI32(const i32 val_)
	{
		const u32 nVal = HostToNetwork(*(u32*)&val_);

		u8 bytes[1 + sizeof(u32)];
		bytes[0] = ByteCodes::Int32;
		bytes[1] = nVal			& 0xFF;
		bytes[2] = (nVal >> 8)  & 0xFF;
		bytes[3] = (nVal >> 16) & 0xFF;
		bytes[4] = (nVal >> 24) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackI64(const i64 val_)
	{
		const u64 nVal = HostToNetwork(*(u64*)&val_);

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

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackF32(const f32 val_)
	{
		// We can recover f32/f64 values back later
		const u32 nVal = HostToNetwork(*(u32*)&val_);

		u8 bytes[1 + sizeof(u32)];
		bytes[0] = ByteCodes::Float32;
		bytes[1] = nVal			& 0xFF;
		bytes[2] = (nVal >> 8)  & 0xFF;
		bytes[3] = (nVal >> 16) & 0xFF;
		bytes[4] = (nVal >> 24) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackF64(const f64 val_)
	{
		const u64 nVal = HostToNetwork(*(u64*)&val_);

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

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackStr8(const char* val_, const u8 len_)
	{
		u8 bytes[1 + sizeof(u8)];
		bytes[0] = ByteCodes::String8;
		bytes[1] = len_;

		PushBytes(bytes, sizeof(bytes));
		PushBytes((u8*)val_, len_);
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackStr16(const char* val_, const u16 len_)
	{
		const u16 nLen = HostToNetwork(len_);

		u8 bytes[1 + sizeof(u16)];
		bytes[0] = ByteCodes::String16;
		bytes[1] = nLen		   & 0xFF;
		bytes[2] = (nLen >> 8) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
		PushBytes((u8*)val_, len_);
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackStr32(const char* val_, const u32 len_)
	{
		const u32 nLen = HostToNetwork(len_);

		u8 bytes[1 + sizeof(u32)];
		bytes[0] = ByteCodes::String32;
		bytes[1] = nLen			& 0xFF;
		bytes[2] = (nLen >> 8)  & 0xFF;
		bytes[3] = (nLen >> 16) & 0xFF;
		bytes[4] = (nLen >> 24) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
		PushBytes((u8*)val_, len_);
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackBin8(const u8* const val_, const u8 len_)
	{
		u8 bytes[1 + sizeof(u8)];
		bytes[0] = ByteCodes::Bin8;
		bytes[1] = len_;

		PushBytes(bytes, sizeof(bytes));
		PushBytes(val_, len_);
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackBin16(const u8* const val_, const u16 len_)
	{
		const u16 nLen = HostToNetwork(len_);

		u8 bytes[1 + sizeof(u16)];
		bytes[0] = ByteCodes::Bin16;
		bytes[1] = nLen		   & 0xFF;
		bytes[2] = (nLen >> 8) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
		PushBytes(val_, len_);
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackBin32(const u8* const val_, const u32 len_)
	{
		const u32 nLen = HostToNetwork(len_);

		u8 bytes[1 + sizeof(u32)];
		bytes[0] = ByteCodes::Bin32;
		bytes[1] = nLen			& 0xFF;
		bytes[2] = (nLen >> 8)  & 0xFF;
		bytes[3] = (nLen >> 16) & 0xFF;
		bytes[4] = (nLen >> 24) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
		PushBytes(val_, len_);
	}

	template <u32 Size, bool Secure, bool Local>
	template <u32 N>
	void Packer<Size, Secure, Local>::PackFixExtN(const i32 type_, const u8* const data_)
	{
		const u32 nType = HostToNetwork(*(u32*)&type_);

		u8 bytes[1 + sizeof(u32) + (1 << N)];
		bytes[0] = ByteCodes::FixExt1 + N;
		bytes[1] = nType		 & 0xFF;
		bytes[2] = (nType >> 8)  & 0xFF;
		bytes[3] = (nType >> 16) & 0xFF;
		bytes[4] = (nType >> 24) & 0xFF;

		for (u32 i = 0; i < (1 << N); ++i)
		{
			bytes[5 + i] = data_[i];
		}

		PushBytes(bytes, sizeof(bytes));
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackExt8(const i32 type_, const u8* const data_, const u8 len_)
	{
		const u32 nType = HostToNetwork(*(u32*)&type_);

		u8 bytes[1 + sizeof(u8) + sizeof(u32)];
		bytes[0] = ByteCodes::Ext8;
		bytes[1] = len_;
		bytes[2] = nType		 & 0xFF;
		bytes[3] = (nType >> 8)  & 0xFF;
		bytes[4] = (nType >> 16) & 0xFF;
		bytes[5] = (nType >> 24) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
		PushBytes(data_, len_);
	}

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackExt16(const i32 type_, const u8* const data_, const u16 len_)
	{
		const u32 nType = HostToNetwork(*(u32*)&type_);
		const u16 nLen  = HostToNetwork(len_);

		u8 bytes[1 + sizeof(u16) + sizeof(u32)];
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

	template <u32 Size, bool Secure, bool Local>
	void Packer<Size, Secure, Local>::PackExt32(const i32 type_, const u8* const data_, const u32 len_)
	{
		const u32 nType = HostToNetwork(*(u32*)&type_);
		const u32 nLen  = HostToNetwork(len_);

		u8 bytes[1 + sizeof(u32) + sizeof(u32)];
		bytes[0] = ByteCodes::Ext32;
		bytes[1] = nLen			 & 0xFF;
		bytes[2] = (nLen >> 8)   & 0xFF;
		bytes[3] = (nLen >> 16)  & 0xFF;
		bytes[4] = (nLen >> 24)  & 0xFF;
		bytes[5] = nType		 & 0xFF;
		bytes[6] = (nType >> 8)  & 0xFF;
		bytes[7] = (nType >> 16) & 0xFF;
		bytes[8] = (nType >> 24) & 0xFF;

		PushBytes(bytes, sizeof(bytes));
		PushBytes(data_, len_);
	}
}
