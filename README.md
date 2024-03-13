# MSGPack C++ API
This repository is a simple, header-only implementation of the MSGPack standard for C++. It is cross-platform (Windows and Linux) and the two classes, Unpacker and Packer, each contain (optional) compile-time optimisations for certain use-cases. No external libraries are required and a small set of tests can be found in Tests/ in order to verify that the library is functional in your development environment.

An example of how to use the API is contained within the Examples/ folder as a single function, but we include a portion here for completeness. The following three code blocks contain an example set of JSON data and its packed/unpacked equivalent under the API.

```json
{
    "myMap" :
    {
        "hello" : "world",
        "mynum" : 2
    },
    "simple",
    "types",
    0
}
```
```cpp
Packer<1000> packer;

packer.PackString("myMap");
packer.StartMap();
{
    packer.PackString("hello");
    packer.PackString("world");
    
    packer.PackString("mynum");
    packer.PackNumber(2);
}
packer.EndMap();

packer.PackString("simple");
packer.PackString("types");
packer.PackNumber(0);
```
```cpp
Unpacker<> unpacker(packer.Message());
const std::string myMapStr = unpacker.UnpackString();
const u32 mapSz            = unpacker.UnpackMap();
for (u32 i = 0; i < mapSz; ++i)
{
	const std::string buff = unpacker.UnpackString();

	if (buff == "hello")
	{
		const std::string world = unpacker.UnpackString();
	}
	else if (buff == "mynum")
	{
		const u32 mynum = unpacker.UnpackNumber<u32>();
	}
	else
	{
		// Error
	}
}

const std::string simple = unpacker.UnpackString();
const std::string types	 = unpacker.UnpackString();
const u32 n0             = unpacker.UnpackNumber<u32>();
```
