#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <ranges>
#include <vector>

#include "assets/IAssetLoader.hpp"

/**
*	@brief Stores the list of asset loaders.
*/
class AssetLoaders final
{
public:
	auto GetLoaders() const { return _loaders | std::views::transform([](const auto& loader) {return loader.get(); }); }

	void Add(std::unique_ptr<IAssetLoader> loader)
	{
		assert(loader);

		_loaders.insert(std::upper_bound(_loaders.begin(), _loaders.end(), loader, [](const auto& lhs, const auto& rhs)
			{
				return lhs->GetPriority() >= rhs->GetPriority();
			}), std::move(loader));
	}

private:
	std::vector<std::unique_ptr<IAssetLoader>> _loaders;
};
