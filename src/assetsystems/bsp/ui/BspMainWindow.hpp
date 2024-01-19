#pragma once

#include <memory>
#include <vector>

#include <QMainWindow>

#include "formats/bsp/BspFile.hpp"

class MultiAsset;
class SceneWidget;
class Ui_BspMainWindow;

class BspMainWindow final : public QMainWindow
{
public:
	explicit BspMainWindow(MultiAsset* multiAsset);
	~BspMainWindow();

	void OpenFile(FILE* file);

private:
	MultiAsset* const _multiAsset;
	std::unique_ptr<Ui_BspMainWindow> _ui;
	SceneWidget* _sceneWidget;

	BspFile _bspFile;
};
