#include "application/MultiAsset.hpp"
#include "assets/IAssetLoader.hpp"
#include "assetsystems/sprite/SpriteAssetSystem.hpp"

#include "assetsystems/sprite/ui/SpriteMainWindow.hpp"

constexpr std::size_t SpriteIdSize = 4;
constexpr char SpriteId[] = "IDSP";

class SpriteAssetLoader final : public IAssetLoader
{
public:
	explicit SpriteAssetLoader(SpriteAssetSystem* assetSystem)
		: _assetSystem(assetSystem)
	{
	}

	QString GetName() const override { return QStringLiteral("Half-Life 1 Sprite"); }

	QStringList GetFileTypes() const override { return { QStringLiteral("*.spr") }; }

	bool TryLoadFile(FILE* file) override
	{
		// TODO: should probably be handled in TryLoadSpriteFile
		char id[SpriteIdSize + 1];

		if (fread(id, SpriteIdSize, 1, file) != 1)
		{
			return false;
		}

		id[std::size(id) - 1] = '\0';

		if (strncmp(id, SpriteId, SpriteIdSize) != 0)
		{
			return false;
		}

		_assetSystem->GetWindow()->OpenFile(file);

		return true;
	}

private:
	SpriteAssetSystem* _assetSystem;
};

void SpriteAssetSystem::Initialize(MultiAsset* multiAsset)
{
	_multiAsset = multiAsset;
	multiAsset->GetAssetLoaders()->Add(std::make_unique<SpriteAssetLoader>(this));
}

SpriteMainWindow* SpriteAssetSystem::GetWindow()
{
	if (!_window)
	{
		_window = new SpriteMainWindow(_multiAsset);
	}

	return _window;
}

