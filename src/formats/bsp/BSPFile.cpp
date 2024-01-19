#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <span>
#include <utility>
#include <vector>

#include "formats/bsp/BspFile.hpp"
#include "utils/BinaryReader.hpp"
#include "utils/IOutils.hpp"

constexpr int BspVersion = 30;

constexpr int BspLumpCount = 15;

constexpr int BspFaceSize = 20;
constexpr int BspModelSize = 64;

namespace BspLumpId
{
enum BspLumpId : std::size_t
{
	Entities = 0,
	Planes = 1,
	Textures = 2,
	Vertexes = 3,
	Visibility = 4,
	Nodes = 5,
	TexInfo = 6,
	Faces = 7,
	Lighting = 8,
	Clipnodes = 9,
	Leafs = 10,
	MarkSurfaces = 11,
	Edges = 12,
	SurfEdges = 13,
	Models = 14,
};
}

struct BspLump
{
	int Offset;
	int SizeInBytes;
};

struct Edge
{
	std::uint16_t VertexIndexes[2];
};

static std::optional<std::string> TryLoadEntities(BinaryReader& reader, const std::array<BspLump, BspLumpCount>& lumps)
{
	const auto& lump = lumps[BspLumpId::Entities];

	std::string entities;

	entities.resize(std::max(0, lump.SizeInBytes - 1));

	reader.SetPosition(lump.Offset);
	reader.ReadBytes(reinterpret_cast<std::byte*>(entities.data()), entities.size());

	return entities;
}

static std::optional<std::vector<BspTexture>> TryLoadTextures(BinaryReader& reader, const std::array<BspLump, BspLumpCount>& lumps)
{
	// TODO: this can probably reuse the wad loading code.
	const auto& lump = lumps[BspLumpId::Textures];

	reader.SetPosition(lump.Offset);

	const std::int32_t textureCount = reader.ReadInt32();

	if (textureCount < 0)
	{
		return {};
	}

	std::vector<BspTexture> textures;

	textures.resize(textureCount);

	std::array<unsigned int, BspMipLevelCount> mipLevelOffsets{};

	for (std::size_t i = 0; auto& texture : textures)
	{
		reader.SetPosition(static_cast<std::size_t>(lump.Offset) + (4 * (i + 1)));

		const int offset = reader.ReadInt32();

		if (offset < 0 || offset >= lump.SizeInBytes)
		{
			return {};
		}

		const std::size_t textureOffset = static_cast<std::size_t>(lump.Offset) + offset;

		reader.SetPosition(textureOffset);

		texture.Name = reader.ReadFixedUTF8String(16);

		texture.Width = reader.ReadUInt32();
		texture.Height = reader.ReadUInt32();

		for (std::size_t mipLevel = 0; mipLevel < BspMipLevelCount; ++mipLevel)
		{
			mipLevelOffsets[mipLevel] = reader.ReadUInt32();
		}

		if (mipLevelOffsets[0] > 0)
		{
			for (std::size_t mipLevel = 0; mipLevel < BspMipLevelCount; ++mipLevel)
			{
				if (mipLevelOffsets[mipLevel] == 0)
				{
					return {};
				}

				reader.SetPosition(textureOffset + mipLevelOffsets[mipLevel]);

				const std::size_t pixelCount = static_cast<std::size_t>(texture.Width / (1 << mipLevel))
					* static_cast<std::size_t>(texture.Height / (1 << mipLevel));

				auto& data = texture.TextureDatas[mipLevel];

				data.resize(pixelCount);

				reader.ReadBytes(reinterpret_cast<std::byte*>(data.data()), data.size());
			}

			reader.SetPosition(textureOffset + mipLevelOffsets[0] + ((static_cast<std::size_t>(texture.Width) * texture.Height) / static_cast<std::size_t>(64) * 85) + 2);

			texture.Colormap.resize(ColormapColorCount);

			for (std::size_t i = 0; i < ColormapColorCount; ++i)
			{
				texture.Colormap[i].R = reader.ReadUInt8();
				texture.Colormap[i].G = reader.ReadUInt8();
				texture.Colormap[i].B = reader.ReadUInt8();
			}
		}

		++i;
	}

	return textures;
}

static std::optional<std::vector<BspTextureInfo>> TryLoadTextureInfos(
	BinaryReader& reader, const std::array<BspLump, BspLumpCount>& lumps, const std::vector<BspTexture>& textures)
{
	const auto& lump = lumps[BspLumpId::TexInfo];

	reader.SetPosition(lump.Offset);

	std::vector<BspTextureInfo> textureInfos;

	textureInfos.resize(lump.SizeInBytes / 40);

	for (auto& textureInfo : textureInfos)
	{
		for (std::size_t dataIndex = 0; dataIndex < BspTextureInfoDataCount; ++dataIndex)
		{
			textureInfo.Vertices[dataIndex].x = reader.ReadFloat();
			textureInfo.Vertices[dataIndex].y = reader.ReadFloat();
			textureInfo.Vertices[dataIndex].z = reader.ReadFloat();
			textureInfo.STCoordinates[dataIndex] = reader.ReadFloat();
		}

		const int textureIndex = reader.ReadInt32();

		if (textureIndex < 0 || std::cmp_greater_equal(textureIndex, textures.size()))
		{
			return {};
		}

		textureInfo.Texture = &textures[textureIndex];

		textureInfo.Flags = reader.ReadInt32();
	}

	return textureInfos;
}

static std::vector<glm::vec3> LoadVertexes(BinaryReader& reader, const std::array<BspLump, BspLumpCount>& lumps)
{
	const auto& lump = lumps[BspLumpId::Vertexes];

	reader.SetPosition(lump.Offset);

	std::vector<glm::vec3> vertexes;

	vertexes.resize(lump.SizeInBytes / (sizeof(float) * 3));

	for (auto& vertex : vertexes)
	{
		vertex.x = reader.ReadFloat();
		vertex.y = reader.ReadFloat();
		vertex.z = reader.ReadFloat();
	}

	return vertexes;
}

static std::vector<Edge> LoadEdges(BinaryReader& reader, const std::array<BspLump, BspLumpCount>& lumps)
{
	const auto& lump = lumps[BspLumpId::Edges];

	reader.SetPosition(lump.Offset);

	std::vector<Edge> edges;

	edges.resize(lump.SizeInBytes / (sizeof(std::uint16_t) * 2));

	for (auto& edge : edges)
	{
		edge.VertexIndexes[0] = reader.ReadUInt16();
		edge.VertexIndexes[1] = reader.ReadUInt16();
	}

	return edges;
}

static std::vector<int> LoadSurfEdges(BinaryReader& reader, const std::array<BspLump, BspLumpCount>& lumps)
{
	const auto& lump = lumps[BspLumpId::SurfEdges];

	reader.SetPosition(lump.Offset);

	std::vector<int> surfEdges;

	surfEdges.resize(lump.SizeInBytes / sizeof(std::int32_t));

	reader.ReadBytes(reinterpret_cast<std::byte*>(surfEdges.data()), lump.SizeInBytes);

	return surfEdges;
}

static std::optional<std::vector<Face>> TryLoadFaces(
	BinaryReader& reader, const std::array<BspLump, BspLumpCount>& lumps, const std::vector<BspTextureInfo>& textureInfos)
{
	const auto vertexes = LoadVertexes(reader, lumps);
	const auto edges = LoadEdges(reader, lumps);
	const auto surfEdges = LoadSurfEdges(reader, lumps);

	const auto& lump = lumps[BspLumpId::Faces];

	reader.SetPosition(lump.Offset);

	std::vector<Face> faces;

	faces.resize(lump.SizeInBytes / BspFaceSize);

	for (auto& face : faces)
	{
		const std::int16_t planeNumber = reader.ReadInt16();
		const std::int16_t side = reader.ReadInt16();
		
		const int firstEdge = reader.ReadInt32();
		const std::int16_t numEdges = reader.ReadInt16();
		const std::int16_t texInfo = reader.ReadInt16();

		std::array<std::uint8_t, 4> styles{};

		for (auto& style : styles)
		{
			style = reader.ReadUInt8();
		}

		const int lightOffset = reader.ReadInt32();

		if (firstEdge < 0 || std::cmp_greater_equal(firstEdge, surfEdges.size()))
		{
			return {};
		}

		if (numEdges < 0)
		{
			return {};
		}

		if (texInfo < 0 || std::cmp_greater_equal(texInfo, textureInfos.size()))
		{
			return {};
		}

		face.TextureInfo = &textureInfos[texInfo];

		face.Vertexes.reserve(numEdges);

		for (std::size_t i = 0; i < numEdges; ++i)
		{
			const int edgeIndex = surfEdges[firstEdge + i];

			const std::size_t absoluteEdgeIndex = std::abs(edgeIndex);

			if (absoluteEdgeIndex >= edges.size())
			{
				return {};
			}

			const Edge& edge = edges[absoluteEdgeIndex];

			if (edgeIndex >= 0)
			{
				face.Vertexes.push_back(vertexes[edge.VertexIndexes[0]]);
			}
			else
			{
				face.Vertexes.push_back(vertexes[edge.VertexIndexes[1]]);
			}
		}
	}

	return faces;
}

/*
typedef struct
{
	float mins[3], maxs[3];
	float origin[3];
	int headnode[MAX_MAP_HULLS];
	int visleafs; // not including the solid leaf 0
	int firstface, numfaces;
} dmodel_t;
*/

static std::optional<std::vector<BspModel>> TryLoadModels(
	BinaryReader& reader, const std::array<BspLump, BspLumpCount>& lumps, const std::vector<Face>& faces)
{
	const auto& lump = lumps[BspLumpId::Models];

	reader.SetPosition(lump.Offset);

	std::vector<BspModel> models;

	models.resize(lump.SizeInBytes / BspModelSize);

	for (auto& model : models)
	{
		model.Mins.x = reader.ReadFloat();
		model.Mins.y = reader.ReadFloat();
		model.Mins.z = reader.ReadFloat();

		model.Maxs.x = reader.ReadFloat();
		model.Maxs.y = reader.ReadFloat();
		model.Maxs.z = reader.ReadFloat();

		model.Origin.x = reader.ReadFloat();
		model.Origin.y = reader.ReadFloat();
		model.Origin.z = reader.ReadFloat();

		std::array<int, BspHullCount> headNodes{};

		for (auto& node : headNodes)
		{
			node = reader.ReadInt32();
		}

		const int visleaf = reader.ReadInt32();

		const int firstFace = reader.ReadInt32();
		const int faceCount = reader.ReadInt32();

		if (firstFace < 0 || std::cmp_greater_equal(firstFace, faces.size()))
		{
			return {};
		}

		if (faceCount < 0 || std::cmp_greater(firstFace + faceCount, faces.size()))
		{
			return {};
		}

		model.Faces = std::span{faces.data() + firstFace, static_cast<std::size_t>(faceCount)};
	}

	return models;
}

std::optional<BspFile> TryLoadBspFile(const std::string& fileName)
{
	FILE* file = std::fopen(fileName.c_str(), "rb");

	if (!file)
	{
		return {};
	}

	auto result = TryLoadBspFile(file);

	std::fclose(file);

	return result;
}

std::optional<BspFile> TryLoadBspFile(FILE* file)
{
	std::optional<std::vector<std::byte>> buffer = TryReadFileIntoBuffer(file);

	if (!buffer)
	{
		return {};
	}

	// TODO: catch out_of_range exceptions and return appropriate result.
	BinaryReader reader{ *buffer };

	const auto version = reader.ReadInt32();

	if (version != BspVersion)
	{
		return {};
	}

	std::array<BspLump, BspLumpCount> lumps{};

	for (auto& lump : lumps)
	{
		lump.Offset = reader.ReadInt32();
		lump.SizeInBytes = reader.ReadInt32();

		if (lump.Offset < 0 || lump.SizeInBytes < 0)
		{
			return {};
		}
	}

	BspFile bsp;

	auto entities = TryLoadEntities(reader, lumps);

	if (!entities)
	{
		return {};
	}

	auto textures = TryLoadTextures(reader, lumps);

	if (!textures)
	{
		return {};
	}

	auto textureInfos = TryLoadTextureInfos(reader, lumps, *textures);

	if (!textureInfos)
	{
		return {};
	}

	auto faces = TryLoadFaces(reader, lumps, *textureInfos);

	if (!faces)
	{
		return {};
	}

	auto models = TryLoadModels(reader, lumps, *faces);

	if (!models)
	{
		return {};
	}

	bsp.Entities = std::move(*entities);
	bsp.Textures = std::move(*textures);
	bsp.TextureInfos = std::move(*textureInfos);
	bsp.Faces = std::move(*faces);
	bsp.Models = std::move(*models);

	return bsp;
}
