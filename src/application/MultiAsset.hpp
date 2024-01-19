#pragma once

#include <memory>
#include <vector>

#include <QObject>

#include "assets/AssetLoaders.hpp"
#include "assets/AssetSystem.hpp"

class MultiAsset final : public QObject
{
	Q_OBJECT

public:
	explicit MultiAsset();
	~MultiAsset();

	AssetLoaders* GetAssetLoaders() { return &_assetLoaders; }

	int Run(int argc, char** argv);

signals:
	/**
	*	@brief Opens a file prompt to open a file.
	*	@param parent The widget that the dialog will be opened on top of.
	*	@param defaultfilter If not empty, selects the default filter to use.
	*		This must be a case sensitive substring of the filter returned by IAssetLoader::GetName.
	*/
	void PromptOpenFile(QWidget* parent, QString defaultFilter);

private:
	AssetLoaders _assetLoaders;
	std::vector<std::unique_ptr<AssetSystem>> _assetSystems;
};
