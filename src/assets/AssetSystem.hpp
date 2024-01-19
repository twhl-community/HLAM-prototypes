#pragma once

class MultiAsset;

/**
*	@brief A system that handles specific types of assets. Has its own window and manages its own list of assets.
*/
class AssetSystem
{
public:
	virtual ~AssetSystem() = default;

	virtual void Initialize(MultiAsset* multiAsset) = 0;
};
