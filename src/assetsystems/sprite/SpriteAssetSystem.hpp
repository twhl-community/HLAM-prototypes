#pragma once

#include "assets/AssetSystem.hpp"

class MultiAsset;
class SpriteMainWindow;

class SpriteAssetSystem final : public AssetSystem
{
public:
	void Initialize(MultiAsset* multiAsset) override;

	SpriteMainWindow* GetWindow();

private:
	MultiAsset* _multiAsset{};
	SpriteMainWindow* _window{};
};
