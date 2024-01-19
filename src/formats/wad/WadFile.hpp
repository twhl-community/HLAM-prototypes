#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

constexpr std::size_t ColormapColorCount = 256;

struct RGB24
{
	std::uint8_t R;
	std::uint8_t G;
	std::uint8_t B;
};

class WadEntry
{
public:
	std::string Name;
	unsigned int Width{0};
	unsigned int Height{0};
	std::vector<std::uint8_t> Pixels;
	std::vector<RGB24> Colormap; // 256 colors = 768 bytes
};

class WadFile
{
public:
	std::vector<WadEntry> Entries;
};

std::optional<WadFile> TryLoadWadFile(const std::string& fileName);
std::optional<WadFile> TryLoadWadFile(FILE* file);
