#include <cstddef>
#include <cstring>
#include <iterator>

#include "application/MultiAsset.hpp"
#include "assets/IAssetLoader.hpp"
#include "assetsystems/wad/WadAssetSystem.hpp"
#include "assetsystems/wad/ui/WadMainWindow.hpp"

constexpr std::size_t WadIdSize = 4;
constexpr char Wad2Id[] = "WAD2";
constexpr char Wad3Id[] = "WAD3";

class WadAssetLoader final : public IAssetLoader
{
public:
	explicit WadAssetLoader(WadAssetSystem* assetSystem)
		: _assetSystem(assetSystem)
	{
	}

	QString GetName() const override { return QStringLiteral("Half-Life 1 Wad"); }

	QStringList GetFileTypes() const override { return { QStringLiteral("*.wad") }; }

	bool TryLoadFile(FILE* file) override
	{
		// TODO: should probably be handled in TryLoadWadFile
		char id[WadIdSize + 1];

		if (fread(id, WadIdSize, 1, file) != 1)
		{
			return false;
		}

		id[std::size(id) - 1] = '\0';

		// TODO: should this support WAD2? Engine assumes wads are WAD3, probably to support old wads that use the wrong id.
		if (strncmp(id, Wad2Id, WadIdSize) != 0 && strncmp(id, Wad3Id, WadIdSize) != 0)
		{
			return false;
		}

		_assetSystem->GetWindow()->OpenFile(file);

		return true;
	}

private:
	WadAssetSystem* _assetSystem;
};

void WadAssetSystem::Initialize(MultiAsset* multiAsset)
{
	_multiAsset = multiAsset;
	multiAsset->GetAssetLoaders()->Add(std::make_unique<WadAssetLoader>(this));
}

WadMainWindow* WadAssetSystem::GetWindow()
{
	if (!_window)
	{
		_window = new WadMainWindow(_multiAsset);
	}

	return _window;
}
