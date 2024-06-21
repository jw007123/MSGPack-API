#if defined(_WINDOWS)
	#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <thread>

#include "Tests.h"

int main()
{
	printf("Running MSGPack unit tests...\n\n");

	MSGPack::Packer<>   packer;
	MSGPack::Unpacker<> unpacker;

	MSGPack::Tests msgpackTests;
	if (!msgpackTests.Run(packer, unpacker))
	{
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return -1;
	}

	std::this_thread::sleep_for(std::chrono::seconds(5));
	return 0;
}
