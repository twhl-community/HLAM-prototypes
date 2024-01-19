#include <cstdio>
#include <cstring>

#include "formats/wad/WadFile.hpp"
#include "utils/BinaryReader.hpp"
#include "utils/IOutils.hpp"

enum class WadLumpType : std::uint8_t
{
	Palette = 64,
	Colormap = 65,
	QPic = 66,
	Miptex = 67,
	Raw = 68,
	Colormap2 = 69,
	Font = 70
};

constexpr int WadHeaderSize = 12;
constexpr int WadEntrySize = 32;

static std::optional<WadEntry> TryReadWadEntry(const BinaryReader& reader, int tableOffset)
{
	auto tableEntry = reader.subspan(tableOffset);

	const int filePos = tableEntry.ReadInt32();
	const int diskSize = tableEntry.ReadInt32();
	const int size = tableEntry.ReadInt32();
	const WadLumpType type = static_cast<WadLumpType>(tableEntry.ReadUInt8());
	const std::uint8_t compression = tableEntry.ReadUInt8();
	const auto padding = tableEntry.ReadUInt16();

	auto name = tableEntry.ReadFixedUTF8String(16);

	// Check this after reading the name so we can debug it more easily.
	// TODO: handle all lump types (see qlumpy source code for more information).
	if (type != WadLumpType::Miptex)
	{
		return {};
	}

	WadEntry entry;

	entry.Name = std::move(name);

	auto miptexEntry = reader.subspan(filePos);

	// Skip name
	miptexEntry.SetPosition(16);

	entry.Width = miptexEntry.ReadUInt32();
	entry.Height = miptexEntry.ReadUInt32();

	// Note that the engine assumes that all mip levels are stored sequentially in memory with no gaps,
	// it does not use the remaining 3 offsets.
	// See Quake 1's source code for more information (Software and OpenGL renderers differ a bit).
	const unsigned int dataOffset = miptexEntry.ReadUInt32();

	auto dataEntry = miptexEntry.subspan(dataOffset);

	entry.Pixels.resize(static_cast<std::size_t>(entry.Width) * entry.Height);

	dataEntry.ReadBytes(reinterpret_cast<std::byte*>(entry.Pixels.data()), entry.Pixels.size());

	entry.Colormap.resize(ColormapColorCount);

	std::size_t totalPixelCount = 0;

	for (std::size_t i = 0; i < 4; ++i)
	{
		const std::size_t divisor = static_cast<std::size_t>(1U) << i;
		const std::size_t width = entry.Width / divisor;
		const std::size_t height = entry.Height / divisor;

		totalPixelCount += width * height;
	}

	// Colormap starts after the 4 mip levels.
	// There is a 2 byte int indicating palette size but this is assumed to always be the maximum.
	auto colorMapEntry = dataEntry.subspan(totalPixelCount + 2);

	for (std::size_t i = 0; i < ColormapColorCount; ++i)
	{
		entry.Colormap[i].R = colorMapEntry.ReadUInt8();
		entry.Colormap[i].G = colorMapEntry.ReadUInt8();
		entry.Colormap[i].B = colorMapEntry.ReadUInt8();
	}

	return entry;
}

std::optional<WadFile> TryLoadWadFile(const std::string& fileName)
{
	FILE* file = std::fopen(fileName.c_str(), "rb");

	if (!file)
	{
		return {};
	}

	auto result = TryLoadWadFile(file);

	std::fclose(file);

	return result;
}

std::optional<WadFile> TryLoadWadFile(FILE* file)
{
	std::optional<std::vector<std::byte>> buffer = TryReadFileIntoBuffer(file);

	if (!buffer)
	{
		return {};
	}

	// TODO: catch out_of_range exceptions and return appropriate result.
	BinaryReader reader{ *buffer };

	const auto identification = reader.ReadFixedUTF8String(4);

	if (identification != "WAD2" && identification != "WAD3")
	{
		return {};
	}

	const int lumpCount = reader.ReadInt32();
	const int lumpTableOffset = reader.ReadInt32();

	// Reject invalid headers.
	if (lumpCount < 0 || lumpTableOffset < WadHeaderSize)
	{
		return {};
	}

	WadFile wadFile;

	wadFile.Entries.reserve(lumpCount);

	int tableOffset = lumpTableOffset;

	for (int i = 0; i < lumpCount; ++i)
	{
		if (auto entry = TryReadWadEntry(reader, tableOffset); entry)
		{
			wadFile.Entries.push_back(std::move(*entry));
		}

		tableOffset += WadEntrySize;
	}

	return wadFile;
}
