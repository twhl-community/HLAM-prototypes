#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

constexpr std::size_t ColormapColorCount = 256;
constexpr std::size_t BspMipLevelCount = 4;
constexpr std::size_t BspTextureInfoDataCount = 2;
constexpr std::size_t BspHullCount = 4;

struct RGB24
{
	std::uint8_t R;
	std::uint8_t G;
	std::uint8_t B;
};

struct BspTexture
{
	std::string Name;
	unsigned int Width{ 0 };
	unsigned int Height{ 0 };
	std::array<std::vector<std::uint8_t>, BspMipLevelCount> TextureDatas;
	std::vector<RGB24> Colormap;
};

struct BspTextureInfo
{
	std::array<float, BspTextureInfoDataCount> STCoordinates{};
	std::array<glm::vec3, BspTextureInfoDataCount> Vertices{};

	const BspTexture* Texture{};

	int Flags{ 0 };
};

struct Face
{
	std::vector<glm::vec3> Vertexes;

	const BspTextureInfo* TextureInfo{};
};

struct BspModel
{
	glm::vec3 Mins{ 0 };
	glm::vec3 Maxs{ 0 };
	glm::vec3 Origin{ 0 };

	std::span<const Face> Faces;
};

class BspFile
{
public:
	std::string Entities;
	std::vector<BspTexture> Textures;
	std::vector<BspTextureInfo> TextureInfos;
	std::vector<Face> Faces;
	std::vector<BspModel> Models;
};

std::optional<BspFile> TryLoadBspFile(const std::string& fileName);
std::optional<BspFile> TryLoadBspFile(FILE* file);
