#include <cstdint>
#include <cstdio>
#include <vector>

#include "formats/sprite/SpriteFile.hpp"
#include "utils/BinaryReader.hpp"
#include "utils/IOutils.hpp"

const char* SpriteTypeToString(SpriteType type)
{
	switch (type)
	{
	case SpriteType::VP_PARALLEL_UPRIGHT: return "vp_parallel_upright";
	case SpriteType::FACING_UPRIGHT: return "facing_upright";
	default:
	case SpriteType::VP_PARALLEL: return "vp_parallel";
	case SpriteType::ORIENTED: return "oriented";
	case SpriteType::VP_PARALLEL_ORIENTED: return "vp_parallel_oriented";
	}
}

const char* SpriteTextureFormatToString(SpriteTextureFormat format)
{
	switch (format)
	{
	default:
	case SpriteTextureFormat::NORMAL: return "normal";
	case SpriteTextureFormat::ADDITIVE: return "additive";
	case SpriteTextureFormat::INDEXALPHA: return "indexalpha";
	case SpriteTextureFormat::ALPHTEST: return "alphatest";
	}
}

constexpr int SpriteVersion = 2;

static std::optional<SingleSpriteFrame> TryLoadSingleSpriteFrame(BinaryReader& reader)
{
	SingleSpriteFrame frame;

	frame.Origin.x = reader.ReadInt32();
	frame.Origin.y = reader.ReadInt32();

	frame.Width = reader.ReadInt32();
	frame.Height = reader.ReadInt32();

	if (frame.Width < 0 || frame.Height < 0)
	{
		return {};
	}

	frame.Pixels.resize(static_cast<std::size_t>(frame.Width) * frame.Height);

	reader.ReadBytes(reinterpret_cast<std::byte*>(frame.Pixels.data()), frame.Pixels.size());

	return frame;
}

static std::optional<SpriteGroup> TryLoadSpriteGroup(BinaryReader& reader)
{
	const std::int32_t numFrames = reader.ReadInt32();

	if (numFrames < 0)
	{
		return {};
	}

	SpriteGroup group;

	group.Intervals.reserve(numFrames);

	for (int i = 0; i < numFrames; ++i)
	{
		group.Intervals.push_back(reader.ReadFloat());

		if (group.Intervals.back() <= 0)
		{
			return {};
		}
	}

	group.Frames.reserve(numFrames);

	for (int i = 0; i < numFrames; ++i)
	{
		auto frame = TryLoadSingleSpriteFrame(reader);

		if (!frame)
		{
			return {};
		}

		group.Frames.push_back(std::move(*frame));
	}

	return group;
}

std::optional<SpriteFile> TryLoadSpriteFile(const std::string& fileName)
{
	FILE* file = std::fopen(fileName.c_str(), "rb");

	if (!file)
	{
		return {};
	}

	auto result = TryLoadSpriteFile(file);

	std::fclose(file);

	return result;
}

std::optional<SpriteFile> TryLoadSpriteFile(FILE* file)
{
	std::optional<std::vector<std::byte>> buffer = TryReadFileIntoBuffer(file);

	if (!buffer)
	{
		return {};
	}
	
	// TODO: catch out_of_range exceptions and return appropriate result.
	BinaryReader reader{ *buffer };

	const auto identification = reader.ReadFixedUTF8String(4);

	if (identification != "IDSP")
	{
		return {};
	}

	const auto version = reader.ReadInt32();

	if (version != SpriteVersion)
	{
		return {};
	}

	SpriteFile sprite;

	sprite.Type = static_cast<SpriteType>(reader.ReadInt32());
	sprite.TextureFormat = static_cast<SpriteTextureFormat>(reader.ReadInt32());
	sprite.BoundingRadius = reader.ReadFloat();

	sprite.Width = reader.ReadInt32();
	sprite.Height = reader.ReadInt32();

	const int numFrames = reader.ReadInt32();

	sprite.BeamLength = reader.ReadFloat();
	sprite.SyncType = static_cast<SyncType>(reader.ReadInt32());

	if (sprite.Type < SpriteType::VP_PARALLEL_UPRIGHT || sprite.Type > SpriteType::VP_PARALLEL_ORIENTED)
	{
		return {};
	}

	if (sprite.TextureFormat < SpriteTextureFormat::NORMAL || sprite.TextureFormat > SpriteTextureFormat::ALPHTEST)
	{
		return {};
	}

	if (sprite.SyncType < SyncType::SYNC || sprite.SyncType > SyncType::RAND)
	{
		return {};
	}

	// TODO: worth checking?
	if (sprite.BoundingRadius < 0)
	{
		return {};
	}

	if (sprite.Width < 0 || sprite.Height < 0)
	{
		return {};
	}

	if (numFrames < 0)
	{
		return {};
	}

	const std::int16_t colorCount = reader.ReadInt16();

	if (colorCount != ColormapColorCount)
	{
		return {};
	}

	sprite.Colormap.resize(ColormapColorCount);

	for (std::size_t i = 0; i < ColormapColorCount; ++i)
	{
		sprite.Colormap[i].R = reader.ReadUInt8();
		sprite.Colormap[i].G = reader.ReadUInt8();
		sprite.Colormap[i].B = reader.ReadUInt8();
	}

	sprite.Frames.reserve(numFrames);

	for (int i = 0; i < numFrames; ++i)
	{
		const SpriteFrameType type = static_cast<SpriteFrameType>(reader.ReadInt32());

		if (type == SpriteFrameType::SINGLE)
		{
			auto frame = TryLoadSingleSpriteFrame(reader);

			if (!frame)
			{
				return {};
			}

			sprite.Frames.push_back(std::move(*frame));
		}
		else
		{
			auto group = TryLoadSpriteGroup(reader);

			if (!group)
			{
				return {};
			}

			// Groups are discarded because the engine does not support them at all.
			// The engine does still keep them around and skips rendering group frames
			// but no sprite should contain group frames.
			// TODO: report removal.
		}
	}

	return sprite;
}
