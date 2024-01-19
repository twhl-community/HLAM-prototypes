#pragma once

#include <memory>

#include <QString>
#include <QMainWindow>

class MultiAsset;
class Ui_MainWindow;

class MainWindow final : public QMainWindow
{
public:
	explicit MainWindow(MultiAsset* multiAsset);
	~MainWindow();

private slots:
	void OnPromptOpenFile(QWidget* parent, QString defaultFilter);

private:
	MultiAsset* const _multiAsset;
	std::unique_ptr<Ui_MainWindow> _ui;

	QString _openFileFilters;
};
