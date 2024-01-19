#pragma once

#include "assets/AssetSystem.hpp"

class MultiAsset;
class WadMainWindow;

class WadAssetSystem final : public AssetSystem
{
public:
	void Initialize(MultiAsset* multiAsset) override;

	WadMainWindow* GetWindow();

private:
	MultiAsset* _multiAsset{};
	WadMainWindow* _window{};
};
