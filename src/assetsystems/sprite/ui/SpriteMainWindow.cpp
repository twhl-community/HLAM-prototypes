#include <algorithm>
#include <cmath>

#include <QItemDelegate>
#include <QPainter>
#include <QTimeLine>

#include "ui_SpriteMainWindow.h"

#include "application/MultiAsset.hpp"

#include "formats/sprite/SpriteFile.hpp"

#include "assetsystems/sprite/ui/SpriteMainWindow.hpp"

static constexpr int SpriteFrameRate = 10;

class SpriteFrameItemDelegate : public QItemDelegate
{
public:
	static constexpr int Padding = 5;
	static constexpr int HorizontalSpacing = 2;

	explicit SpriteFrameItemDelegate(SpriteMainWindow* window)
		: QItemDelegate(window)
		, _window(window)
	{
	}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		const auto value = index.data(Qt::DecorationRole);

		if (!value.isValid())
		{
			return;
		}

		painter->save();

		painter->setRenderHint(QPainter::SmoothPixmapTransform, false);

		drawBackground(painter, option, index);

		if ((option.state & QStyle::State_HasFocus) != 0)
		{
			painter->fillRect(option.rect, QColor{ 0, 120, 215 });
			painter->setPen(Qt::white);
		}

		QRect rect = option.rect;

		rect.adjust(Padding, Padding, -Padding, -Padding);

		const std::size_t frameIndex = index.data(Qt::UserRole).value<std::size_t>();

		const auto& frame = _window->GetSpriteFile()->Sprite.Frames[frameIndex];

		painter->drawPixmap(rect.x(), rect.y(), frame.Width, frame.Height, _window->GetSpriteFile()->Pixmaps[frameIndex]);

		const QString text = index.data(Qt::DisplayRole).toString();

		painter->drawText(rect.adjusted(frame.Width + HorizontalSpacing, 0, 0, 0), text);

		if ((option.state & QStyle::State_HasFocus) != 0)
		{
			painter->drawRect(option.rect.adjusted(0, 0, -1, -1));
		}

		painter->restore();
	}

	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		const auto value = index.data(Qt::DecorationRole);

		if (!value.isValid())
		{
			return {};
		}

		const std::size_t frameIndex = index.data(Qt::UserRole).value<std::size_t>();

		const auto& frame = _window->GetSpriteFile()->Sprite.Frames[frameIndex];

		QSize size{ frame.Width, frame.Height };

		const QSize textSize = option.fontMetrics.size(0, index.data(Qt::DisplayRole).toString());

		size.setWidth(size.width() + textSize.width() + HorizontalSpacing);
		size.setHeight(std::max(size.height(), textSize.height()));

		size = size.grownBy(QMargins{ Padding, Padding, Padding, Padding });

		return size;
	}

private:
	SpriteMainWindow* _window;
};

SpriteMainWindow::SpriteMainWindow(MultiAsset* multiAsset)
	: _multiAsset(multiAsset)
{
	_ui = std::make_unique<Ui_SpriteMainWindow>();

	_ui->setupUi(this);

	_itemDelegate = new SpriteFrameItemDelegate(this);

	_ui->Frames->setItemDelegate(_itemDelegate);

	_timeline = new QTimeLine(1, this);
	_timeline->setLoopCount(0);
	_timeline->setEasingCurve(QEasingCurve{ QEasingCurve::Linear });
	_timeline->setUpdateInterval(static_cast<int>((1.f / SpriteFrameRate) * 1000)); // Matches default framerate.

	connect(_ui->ActionOpen, &QAction::triggered, this, [this]
		{
			emit _multiAsset->PromptOpenFile(this, "Half-Life 1 Sprite");
		});

	connect(_timeline, &QTimeLine::frameChanged, this, &SpriteMainWindow::FrameChanged);
}

SpriteMainWindow::~SpriteMainWindow() = default;

void SpriteMainWindow::OpenFile(FILE* file)
{
	auto spriteFile = TryLoadSpriteFile(file);

	if (!spriteFile)
	{
		return;
	}

	_timeline->stop();
	_ui->Frames->clear();

	_spriteFile.Sprite = std::move(*spriteFile);

	_ui->TypeLabel->setText(QString::fromUtf8(SpriteTypeToString(_spriteFile.Sprite.Type)));
	_ui->Formatlabel->setText(QString::fromUtf8(SpriteTextureFormatToString(_spriteFile.Sprite.TextureFormat)));
	_ui->DimensionsLabel->setText(QString{ "%1x%2" }.arg(_spriteFile.Sprite.Width).arg(_spriteFile.Sprite.Height));
	_ui->FrameCountLabel->setText(QString::number(_spriteFile.Sprite.Frames.size()));
	_ui->BoundingLabel->setText(QString::number(static_cast<int>(std::floor(_spriteFile.Sprite.BoundingRadius))));

	_spriteFile.Pixmaps.clear();

	_spriteFile.Pixmaps.reserve(_spriteFile.Sprite.Frames.size());

	QList<QRgb> colorTable;

	colorTable.resize(ColormapColorCount);

	for (std::size_t i = 0; i < _spriteFile.Sprite.Colormap.size(); ++i)
	{
		colorTable[i] = qRgb(_spriteFile.Sprite.Colormap[i].R, _spriteFile.Sprite.Colormap[i].G, _spriteFile.Sprite.Colormap[i].B);
	}

	for (const auto& frame : _spriteFile.Sprite.Frames)
	{
		QImage image{ frame.Pixels.data(), (int)frame.Width, (int)frame.Height, QImage::Format_Indexed8 };

		image.setColorTable(colorTable);

		_spriteFile.Pixmaps.push_back(QPixmap::fromImage(image));

		auto item = new QListWidgetItem(QIcon{ _spriteFile.Pixmaps.back()},
			QString{ "Frame index: %1\nDimensions: %2 x %3\nOrigin: %4, %5" }
			.arg(_spriteFile.Pixmaps.size() - 1)
			.arg(frame.Width)
			.arg(frame.Height)
			.arg(frame.Origin.x)
			.arg(frame.Origin.y));

		item->setData(Qt::UserRole, QVariant::fromValue(_spriteFile.Pixmaps.size() - 1));

		_ui->Frames->addItem(item);
	}

	if (!_spriteFile.Pixmaps.empty())
	{
		_timeline->setFrameRange(0, static_cast<int>(_spriteFile.Pixmaps.size() - 1));
		_timeline->setDuration(static_cast<int>(_spriteFile.Pixmaps.size() * 1000) / SpriteFrameRate);
		// This won't be called for the first frame so we have to do it manually.
		FrameChanged(0);
		_timeline->start();
	}

	show();
}

void SpriteMainWindow::FrameChanged(int frame)
{
	_ui->PreviewLabel->setPixmap(_spriteFile.Pixmaps[frame]);
}
