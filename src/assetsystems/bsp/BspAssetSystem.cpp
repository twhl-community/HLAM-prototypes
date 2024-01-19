#include <cstddef>
#include <cstring>
#include <iterator>

#include "application/MultiAsset.hpp"
#include "assets/IAssetLoader.hpp"
#include "assetsystems/bsp/BspAssetSystem.hpp"
#include "assetsystems/bsp/ui/BspMainWindow.hpp"

constexpr int BspVersion = 30;

class BspAssetLoader final : public IAssetLoader
{
public:
	explicit BspAssetLoader(BspAssetSystem* assetSystem)
		: _assetSystem(assetSystem)
	{
	}

	int GetPriority() const override { return -100; }

	QString GetName() const override { return QStringLiteral("Half-Life 1 Bsp"); }

	QStringList GetFileTypes() const override { return { QStringLiteral("*.bsp") }; }

	bool TryLoadFile(FILE* file) override
	{
		// TODO: should probably be handled in TryLoadBspFile
		int version;

		if (fread(&version, sizeof(int), 1, file) != 1)
		{
			return false;
		}

		if (version != BspVersion)
		{
			return false;
		}

		_assetSystem->GetWindow()->OpenFile(file);

		return true;
	}

private:
	BspAssetSystem* _assetSystem;
};

void BspAssetSystem::Initialize(MultiAsset* multiAsset)
{
	_multiAsset = multiAsset;
	multiAsset->GetAssetLoaders()->Add(std::make_unique<BspAssetLoader>(this));
}

BspMainWindow* BspAssetSystem::GetWindow()
{
	if (!_window)
	{
		_window = new BspMainWindow(_multiAsset);
	}

	return _window;
}
