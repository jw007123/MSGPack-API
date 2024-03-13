#pragma once

#if defined(_WINDOWS)
	#define NOMINMAX
	#include "winsock2.h"
	#pragma comment(lib, "Ws2_32.lib")
#else
	#if defined(__linux__)
		#include <endian.h>
	#elif defined(__FreeBSD__) || defined(__NetBSD__)
		#include <sys/endian.h>
	#elif defined(__OpenBSD__)
		#include <sys/types.h>
		#define hto16be(x) htobe16(x)
		#define hto32be(x) htobe32(x)
		#define hto64be(x) htobe64(x)
		#define be16toh(x) betoh16(x)
		#define be32toh(x) betoh32(x)
		#define be64toh(x) betoh64(x)
	#endif
#endif

namespace MSGPack
{
	#ifdef _DEBUG
		static constexpr const bool SecureBase = true;
	#else
		static constexpr const bool SecureBase = false;
	#endif
}
