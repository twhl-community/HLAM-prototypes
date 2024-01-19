#pragma once

#include <memory>
#include <vector>

#include <QMainWindow>
#include <QPixmap>

#include "formats/sprite/SpriteFile.hpp"

class MultiAsset;
class QTimeLine;
class SpriteFrameItemDelegate;
class Ui_SpriteMainWindow;

class UiSpriteFile
{
public:
	SpriteFile Sprite;

	std::vector<QPixmap> Pixmaps;
};

class SpriteMainWindow final : public QMainWindow
{
public:
	explicit SpriteMainWindow(MultiAsset* multiAsset);
	~SpriteMainWindow();

	const UiSpriteFile* GetSpriteFile() const { return &_spriteFile; }

	void OpenFile(FILE* file);

private slots:
	void FrameChanged(int frame);

private:
	MultiAsset* _multiAsset;
	std::unique_ptr<Ui_SpriteMainWindow> _ui;
	SpriteFrameItemDelegate* _itemDelegate;

	QTimeLine* _timeline;

	UiSpriteFile _spriteFile;
};
