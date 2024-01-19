#pragma once

#include <memory>
#include <vector>

#include <QMainWindow>
#include <QPixmap>

#include "formats/wad/WadFile.hpp"

class MultiAsset;
class QString;
class QTimer;
class TextureItemDelegate;
class Ui_WadMainWindow;

class UiWadEntry
{
public:
	WadEntry Entry;
	QPixmap Pixmap;
};

class UiWadFile
{
public:
	std::vector<UiWadEntry> Entries;
};

class WadMainWindow final : public QMainWindow
{
public:
	explicit WadMainWindow(MultiAsset* multiAsset);
	~WadMainWindow();

	const UiWadFile* GetWadFile() const { return &_wadFile; }

	void OpenFile(FILE* file);

private slots:
	void OnEntryChanged(int index);

	void OnSizeChanged(int index);

	void OnFilterChanged();

	void UpdateTextureList();

private:
	MultiAsset* _multiAsset;

	std::unique_ptr<Ui_WadMainWindow> _ui;

	UiWadFile _wadFile;

	TextureItemDelegate* _itemDelegate;

	QTimer* _filterTimer;
};
