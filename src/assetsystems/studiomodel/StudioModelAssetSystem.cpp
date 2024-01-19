#include <cstddef>
#include <cstring>
#include <iterator>

#include "application/MultiAsset.hpp"
#include "assets/IAssetLoader.hpp"
#include "assetsystems/studiomodel/StudioModelAssetSystem.hpp"

constexpr std::size_t StudioModelIdSize = 4;
constexpr char StudioModelId[] = "IDST";

class StudioModelAssetLoader final : public IAssetLoader
{
public:
	explicit StudioModelAssetLoader(StudioModelAssetSystem* assetSystem)
		: _assetSystem(assetSystem)
	{
	}

	QString GetName() const override { return QStringLiteral("Half-Life 1 StudioModel"); }

	QStringList GetFileTypes() const override { return { QStringLiteral("*.mdl") }; }

	bool TryLoadFile(FILE* file) override
	{
		char id[StudioModelIdSize + 1];

		if (fread(id, StudioModelIdSize, 1, file) != 1)
		{
			return false;
		}

		id[std::size(id) - 1] = '\0';

		if (strncmp(id, StudioModelId, StudioModelIdSize) != 0)
		{
			return false;
		}

		return true;
	}

private:
	StudioModelAssetSystem* _assetSystem;
};

void StudioModelAssetSystem::Initialize(MultiAsset* multiAsset)
{
	multiAsset->GetAssetLoaders()->Add(std::make_unique<StudioModelAssetLoader>(this));
}
