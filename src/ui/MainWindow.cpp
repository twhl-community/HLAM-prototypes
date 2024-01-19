#include <cstdio>

#include <QFileDialog>
#include <QMessageBox>

#include "ui_MainWindow.h"

#include "application/MultiAsset.hpp"

#include "ui/MainWindow.hpp"

MainWindow::MainWindow(MultiAsset* multiAsset)
	: _multiAsset(multiAsset)
{
	_ui = std::make_unique<Ui_MainWindow>();

	_ui->setupUi(this);

	connect(_ui->ActionOpen, &QAction::triggered, this, [this]
		{
			emit _multiAsset->PromptOpenFile(this, {});
		});
	connect(_multiAsset, &MultiAsset::PromptOpenFile, this, &MainWindow::OnPromptOpenFile);

	QStringList extensions;
	QStringList filters;

	for (auto assetLoader : _multiAsset->GetAssetLoaders()->GetLoaders())
	{
		const QStringList fileTypes{ assetLoader->GetFileTypes() };
		extensions.append(fileTypes);
		filters.append(QString{ "%1 Files (%2)" }.arg(assetLoader->GetName(), fileTypes.join(" ")));
	}

	extensions.sort(Qt::CaseInsensitive);
	filters.sort(Qt::CaseInsensitive);

	_openFileFilters = QString{ "All Supported Files (%1);;" }.arg(extensions.join(" "));

	if (!filters.isEmpty())
	{
		_openFileFilters += filters.join(";;");
		_openFileFilters += ";;";
	}

	_openFileFilters += "All Files (*.*)";
}

MainWindow::~MainWindow() = default;

void MainWindow::OnPromptOpenFile(QWidget* parent, QString defaultFilter)
{
	const QString fileName = QFileDialog::getOpenFileName(parent, {}, {}, _openFileFilters, &defaultFilter);

	if (fileName.isEmpty())
	{
		return;
	}

	FILE* file = fopen(fileName.toStdString().c_str(), "rb");

	if (!file)
	{
		return;
	}

	bool handled = false;

	for (auto assetLoader : _multiAsset->GetAssetLoaders()->GetLoaders())
	{
		if (assetLoader->TryLoadFile(file))
		{
			handled = true;
			break;
		}

		rewind(file);
	}

	fclose(file);

	if (!handled)
	{
		QMessageBox::critical(this, "Asset not supported", "The selected file is not a supported asset type");
	}
}
