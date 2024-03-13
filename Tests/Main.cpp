#if defined(_WINDOWS)
	#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <thread>

#include "Tests.h"

int main()
{
	printf("Running solver unit tests...\n\n");

	MSGPack::Tests msgpackTests;
	if (!msgpackTests.Run())
	{
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return -1;
	}

	std::this_thread::sleep_for(std::chrono::seconds(5));
	return 0;
}
