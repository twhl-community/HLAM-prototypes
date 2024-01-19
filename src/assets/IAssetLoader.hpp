#pragma once

#include <cstdio>

#include <QString>
#include <QStringList>

/**
*	@brief Represents a loader for an asset type.
*	@details The loader is responsible for adding the loaded asset to the correct asset system.
*	As such, asset loaders should be created and registered by the asset system that manages those assets.
*/
class IAssetLoader
{
public:
	virtual ~IAssetLoader() = default;

	/**
	*	@brief Gets the load priority. Higher priority loaders are checked earlier.
	*	@details This is needed to ensure that formats like the Half-Life 1 BSP format which lack header identifiers
	*	are checked after formats that have header identifiers.
	*/
	virtual int GetPriority() const { return 0; }

	/**
	*	@brief Gets the name of the format this loader supports.
	*/
	virtual QString GetName() const = 0;

	/**
	*	@brief Gets the list of file types that this loader supports.
	*	@details This is used to suggest file types to open. It is not used to determine which loader is used to load the file.
	*	@return List of strings in the format @c *.ext.
	*/
	virtual QStringList GetFileTypes() const = 0;

	/**
	*	@brief Try to load the given file.
	*	If the file is a format supported by this loader then it must return true, even if there is an error during loading.
	*/
	virtual bool TryLoadFile(FILE* file) = 0;
};
