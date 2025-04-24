#pragma once

namespace SKWin {
	using LONG = long;
	using ULONG = unsigned long;
	using INT32 = int;
	using UINT32 = unsigned int;
	using INT16 = short;
	using UINT16 = unsigned short;
	using INT8 = signed char;
	using UINT8 = unsigned char;
	using BYTE = char;
	using CHAR = char;
	using BOOL = bool;
	using SIZE_T = size_t;

	using FLOAT = float;
	using DOUBLE = double;
}

//Courtesy of GPT.
namespace SKLinux {
	using LONG = long int;
	using ULONG = unsigned long int;
	using INT32 = int;
	using UINT32 = unsigned int;
	using INT16 = short int;
	using UINT16 = unsigned short int;
	using INT8 = signed char;
	using UINT8 = unsigned char;
	using BYTE = unsigned char;
	using CHAR = char;
	using BOOL = bool;
	using SIZE_T = size_t;

	using FLOAT = float;
	using DOUBLE = double;
};

#ifdef BUILD_WINDOWS
#define SK SKWin
#elif BUILD_LINUX
#define SK SKLinux
#endif