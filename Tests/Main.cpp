#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <thread>

#include "Packer.h"
#include "Unpacker.h"

int main()
{
	MSGPack::Packer<> packer;
	packer.StartMap();
	for (usize i = 0; i < 10000; ++i)
	{
		packer.PackString(std::to_string(i));
		packer.PackNumber<f32>(100.0f);
	}
	packer.EndMap();

	MSGPack::Unpacker<> unpacker(packer.Message());
	const usize sz = unpacker.UnpackMap();
	for (usize i = 0; i < sz; ++i)
	{
		unpacker.UnpackString();
		unpacker.UnpackNumber<f32>();
	}

	std::this_thread::sleep_for(std::chrono::seconds(5));
	return 0;
}
