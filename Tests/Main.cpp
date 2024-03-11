#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <thread>

#include "Packer.h"
#include "Unpacker.h"

int main()
{
	MSGPack::Packer<100> packer;
	packer.PackBool(true);
	packer.PackNumber<i16>(1000);

	MSGPack::Unpacker<> unpacker(packer.Message());
	unpacker.UnpackBool();
	unpacker.UnpackNumber<i16>();

	std::this_thread::sleep_for(std::chrono::seconds(5));
	return 0;
}
