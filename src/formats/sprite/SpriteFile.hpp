#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <variant>

#include <glm/vec2.hpp>

constexpr std::size_t ColormapColorCount = 256;

enum class SpriteType : int
{
	VP_PARALLEL_UPRIGHT = 0,
	FACING_UPRIGHT = 1,
	VP_PARALLEL = 2,
	ORIENTED = 3,
	VP_PARALLEL_ORIENTED = 4,
};

enum class SpriteTextureFormat : int
{
	NORMAL = 0,
	ADDITIVE = 1,
	INDEXALPHA = 2,
	ALPHTEST = 3,
};

enum class SyncType : int
{
	SYNC = 0,
	RAND
};

enum class SpriteFrameType
{
	SINGLE = 0,
	GROUP
};

struct RGB24
{
	std::uint8_t R;
	std::uint8_t G;
	std::uint8_t B;
};

class SingleSpriteFrame
{
public:
	glm::ivec2 Origin{ 0 };
	int Width{ 0 };
	int Height{ 0 };
	std::vector<std::uint8_t> Pixels;
};

class SpriteGroup
{
public:
	std::vector<float> Intervals;
	std::vector<SingleSpriteFrame> Frames;
};

class SpriteFile
{
public:
	SpriteType Type{ SpriteType::ORIENTED };
	SpriteTextureFormat TextureFormat{ SpriteTextureFormat::NORMAL };
	float BoundingRadius{ 0 };
	int Width{ 0 };
	int Height{ 0 };
	float BeamLength{ 0 };
	SyncType SyncType{ SyncType::SYNC };

	std::vector<RGB24> Colormap; // 256 colors = 768 bytes

	std::vector<SingleSpriteFrame> Frames;
};

const char* SpriteTypeToString(SpriteType type);
const char* SpriteTextureFormatToString(SpriteTextureFormat format);

std::optional<SpriteFile> TryLoadSpriteFile(const std::string& fileName);
std::optional<SpriteFile> TryLoadSpriteFile(FILE* file);
