#pragma once

#include "assets/AssetSystem.hpp"

class BspMainWindow;

class BspAssetSystem final : public AssetSystem
{
public:
	void Initialize(MultiAsset* multiAsset) override;

	BspMainWindow* GetWindow();

private:
	MultiAsset* _multiAsset{};
	BspMainWindow* _window{};
};
