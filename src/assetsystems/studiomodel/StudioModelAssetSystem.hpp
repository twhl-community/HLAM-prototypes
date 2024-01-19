#pragma once

#include "assets/AssetSystem.hpp"

class StudioModelAssetSystem final : public AssetSystem
{
public:
	void Initialize(MultiAsset* multiAsset) override;
};
