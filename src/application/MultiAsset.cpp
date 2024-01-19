#include <QApplication>
#include <QMessageBox>
#include <QSurfaceFormat>

#include "application/MultiAsset.hpp"

#include "assetsystems/bsp/BspAssetSystem.hpp"
#include "assetsystems/sprite/SpriteAssetSystem.hpp"
#include "assetsystems/studiomodel/StudioModelAssetSystem.hpp"
#include "assetsystems/wad/WadAssetSystem.hpp"

#include "ui/MainWindow.hpp"

MultiAsset::MultiAsset() = default;
MultiAsset::~MultiAsset() = default;

int MultiAsset::Run(int argc, char** argv)
{
	//Neither OpenGL ES nor Software OpenGL will work here
	QApplication::setAttribute(Qt::ApplicationAttribute::AA_UseDesktopOpenGL, true);
	QApplication::setAttribute(Qt::ApplicationAttribute::AA_ShareOpenGLContexts, true);

	// Set up the OpenGL surface settings to match or exceed the Half-Life engine's requirements
	QSurfaceFormat::FormatOptions formatOptions{ QSurfaceFormat::FormatOption::DeprecatedFunctions };

#ifdef _DEBUG
	formatOptions.setFlag(QSurfaceFormat::FormatOption::DebugContext, true);
#endif

	QSurfaceFormat defaultFormat{ formatOptions };

	defaultFormat.setMajorVersion(4);
	defaultFormat.setMinorVersion(5);
	defaultFormat.setProfile(QSurfaceFormat::OpenGLContextProfile::CompatibilityProfile);

	defaultFormat.setDepthBufferSize(24);
	defaultFormat.setStencilBufferSize(8);
	defaultFormat.setSwapBehavior(QSurfaceFormat::SwapBehavior::DoubleBuffer);
	defaultFormat.setRedBufferSize(4);
	defaultFormat.setGreenBufferSize(4);
	defaultFormat.setBlueBufferSize(4);
	defaultFormat.setAlphaBufferSize(4);

	QSurfaceFormat::setDefaultFormat(defaultFormat);

	QApplication app{argc, argv};

	QApplication::setWindowIcon(QIcon{ ":/multiasset.ico" });

	_assetSystems.push_back(std::make_unique<BspAssetSystem>());
	_assetSystems.push_back(std::make_unique<StudioModelAssetSystem>());
	_assetSystems.push_back(std::make_unique<SpriteAssetSystem>());
	_assetSystems.push_back(std::make_unique<WadAssetSystem>());

	for (auto& assetSystem : _assetSystems)
	{
		assetSystem->Initialize(this);
	}

	auto mainWindow = new MainWindow(this);

	mainWindow->showMaximized();
	
	return app.exec();
}
