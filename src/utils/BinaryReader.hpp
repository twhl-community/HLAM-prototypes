#pragma once

#include <cstddef>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>

class BinaryReader final
{
public:
	explicit BinaryReader(std::span<const std::byte> data)
		: _data(data)
	{
	}

	BinaryReader(const BinaryReader&) = delete;
	BinaryReader& operator=(const BinaryReader&) = delete;

	std::size_t GetPosition() const
	{
		return _offset;
	}

	void SetPosition(std::size_t offset)
	{
		if (offset > _data.size())
		{
			throw std::out_of_range("Attempted to set read position beyond the end of the buffer");
		}

		_offset = offset;
	}

	BinaryReader subspan(std::size_t offset, std::size_t count = std::dynamic_extent) const
	{
		return BinaryReader{_data.subspan(offset, count)};
	}

	void ReadBytes(std::byte* dest, std::size_t sizeInBytes)
	{
		if (_offset >= _data.size() || (_offset + sizeInBytes) > _data.size())
		{
			throw std::out_of_range("Attempted to read beyond the end of the buffer");
		}

		std::memcpy(dest, _data.data() + _offset, sizeInBytes);
		_offset += sizeInBytes;
	}

	std::uint8_t ReadUInt8()
	{
		return ReadValue<std::uint8_t>();
	}

	std::uint16_t ReadUInt16()
	{
		return ReadValue<std::uint16_t>();
	}

	std::uint32_t ReadUInt32()
	{
		return ReadValue<std::uint32_t>();
	}

	std::int8_t ReadInt8()
	{
		return ReadValue<std::int8_t>();
	}

	std::int16_t ReadInt16()
	{
		return ReadValue<std::int16_t>();
	}

	std::int32_t ReadInt32()
	{
		return ReadValue<std::int32_t>();
	}

	float ReadFloat()
	{
		return ReadValue<float>();
	}

	std::string ReadFixedUTF8String(std::size_t sizeInCharacters)
	{
		std::string result;

		result.resize(sizeInCharacters);

		ReadBytes(reinterpret_cast<std::byte*>(result.data()), result.size());

		// If the string contains a null terminator we'll need to adjust the size to match.
		if (const std::size_t length = std::strlen(result.c_str()); length != result.size())
		{
			result.resize(length);
		}

		return result;
	}

private:
	template <typename T>
	T ReadValue()
	{
		T value;
		ReadBytes(reinterpret_cast<std::byte*>(&value), sizeof(T));
		return value;
	}

private:
	std::span<const std::byte> _data;
	std::size_t _offset{ 0 };
};
