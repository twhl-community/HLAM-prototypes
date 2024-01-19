#include "ui_BspMainWindow.h"

#include "application/MultiAsset.hpp"

#include "formats/bsp/BspFile.hpp"

#include "assetsystems/bsp/ui/BspMainWindow.hpp"
#include "assetsystems/bsp/ui/SceneWidget.hpp"

BspMainWindow::BspMainWindow(MultiAsset* multiAsset)
	: _multiAsset(multiAsset)
{
	_ui = std::make_unique<Ui_BspMainWindow>();

	_ui->setupUi(this);

	_sceneWidget = new SceneWidget(this);

	_ui->HorizontalLayout->addWidget(_sceneWidget, 1);

	_sceneWidget->setFocus();

	connect(_ui->ActionOpen, &QAction::triggered, this, [this]
		{
			emit _multiAsset->PromptOpenFile(this, "Half-Life 1 Bsp");
		});
}

BspMainWindow::~BspMainWindow() = default;

void BspMainWindow::OpenFile(FILE* file)
{
	auto bspFile = TryLoadBspFile(file);

	if (!bspFile)
	{
		return;
	}

	_bspFile = std::move(*bspFile);

	_ui->Entities->setPlainText(QString::fromStdString(_bspFile.Entities));

	_ui->Textures->clear();

	for (const auto& texture : _bspFile.Textures)
	{
		_ui->Textures->addItem(QString::fromStdString(texture.Name));
	}

	_sceneWidget->SetBspFile(&_bspFile);

	show();
}
