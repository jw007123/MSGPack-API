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
	template <u32 Size>
	class Unpacker
	{
	public:


	private:

	};
}
