#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <thread>

#include "Packer.h"

int main()
{
	MSGPack::Packer<100> packer;
	packer.PackNumber<i16>(1000);

	std::this_thread::sleep_for(std::chrono::seconds(5));
	return 0;
}
