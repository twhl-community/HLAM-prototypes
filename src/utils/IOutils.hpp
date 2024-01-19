#pragma once

#include <cstddef>
#include <cstdio>
#include <optional>
#include <vector>

inline std::optional<std::vector<std::byte>> TryReadFileIntoBuffer(FILE* file)
{
	std::vector<std::byte> buffer;

	std::fseek(file, 0, SEEK_END);
	buffer.resize(std::ftell(file));
	std::fseek(file, 0, SEEK_SET);

	const bool success = std::fread(buffer.data(), buffer.size(), 1, file) == 1;

	if (success)
	{
		return buffer;
	}

	return {};
}

