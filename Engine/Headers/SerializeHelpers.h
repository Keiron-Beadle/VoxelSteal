#pragma once
#include "gil.h"
#include <Windows.h>
#undef SendMessage

//<-------------SERIALIZERS------------->

inline void SerializeString(std::vector<SK::BYTE>& data, std::string& string) {
	SK::UINT32 length = htonl(string.size());
	data.insert(data.end(), reinterpret_cast<SK::BYTE*>(&length), reinterpret_cast<SK::BYTE*>(&length) + sizeof(SK::UINT32));
	data.insert(data.end(), string.begin(), string.end());
}

inline void SerializeU32(std::vector<SK::BYTE>& data, SK::UINT32 value) {
	SK::UINT32 netValue = htonl(value);
	data.insert(data.end(), reinterpret_cast<SK::BYTE*>(&netValue), reinterpret_cast<SK::BYTE*>(&netValue) + sizeof(SK::UINT32));
}

inline void SerializeU16(std::vector<SK::BYTE>& data, SK::UINT16 value) {
	SK::UINT16 netValue = htons(value);
	data.insert(data.end(), reinterpret_cast<SK::BYTE*>(&netValue), reinterpret_cast<SK::BYTE*>(&netValue) + sizeof(SK::UINT16));
}

//<-------------DESERIALIZERS------------->

inline void DeserializeString(std::vector<SK::BYTE>& data, SK::INT32& idx, std::string& string) {
	SK::UINT32 length;
	memcpy(&length, &data[idx], sizeof(SK::UINT32));
	length = ntohl(length);
	idx += sizeof(SK::UINT32);
	string.resize(length);
	memcpy(string.data(), &data[idx], length);
	idx += length;
}

inline SK::UINT32 DeserializeU32(std::vector<SK::BYTE>& data, SK::INT32& idx) {
	SK::UINT32 value;
	std::memcpy(&value, data.data() + idx, sizeof(SK::UINT32));
	idx += sizeof(value);
	return ntohl(value);
}

inline SK::UINT16 DeserializeU16(std::vector<SK::BYTE>& data, SK::INT32& idx) {
	SK::UINT16 value;
	std::memcpy(&value, data.data() + idx, sizeof(SK::UINT16));
	idx += sizeof(value);
	return ntohs(value);
}

inline SK::UINT8 DeserializeU8(std::vector<SK::BYTE>& data, SK::INT32& idx) {
	SK::UINT8 value = data[idx++];
	return value;
}