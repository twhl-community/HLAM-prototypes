#include <algorithm>
#include <cstring>
#include <vector>

#include <QFont>
#include <QFontMetrics>
#include <QItemDelegate>
#include <QPainter>
#include <QTimer>

#include "ui_WadMainWindow.h"

#include "application/MultiAsset.hpp"

#include "assetsystems/wad/ui/WadMainWindow.hpp"

#include "formats/wad/WadFile.hpp"

class TextureItemDelegate : public QItemDelegate
{
public:
	static constexpr int Padding = 2;
	static constexpr int VerticalSpacing = 2;

	explicit TextureItemDelegate(WadMainWindow* window)
		: QItemDelegate(window)
		, _window(window)
	{
	}

	void paint(QPainter* painter,
		const QStyleOptionViewItem& option,
		const QModelIndex& index) const override
	{
		painter->save();

		painter->setRenderHint(QPainter::SmoothPixmapTransform, false);

		drawBackground(painter, option, index);

		QRect rect = option.rect;

		rect.adjust(Padding, Padding, -Padding, -Padding);

		QSize size{ 0, 0 };

		if (const auto value = index.data(Qt::DecorationRole); value.isValid() && !value.isNull())
		{
			const auto& uiEntry = _window->GetWadFile()->Entries[index.data(Qt::UserRole).value<std::size_t>()];
			const auto& entry = uiEntry.Entry;

			int width = (int)entry.Width;
			int height = (int)entry.Height;

			if (Size != 1)
			{
				size = QSize{ Size, Size };

				// Compress size to fit desired size.
				width = std::min(Size, width);
				height = std::min(Size, height);
			}
			else
			{
				size = QSize{ width, height };
			}

			const QSize pixmapSize{ width, height };

			painter->drawPixmap(rect.x(), rect.y(), pixmapSize.width(), pixmapSize.height(), uiEntry.Pixmap);
		}

		const QString text = index.data(Qt::DisplayRole).toString();

		QSize textSize = option.fontMetrics.size(0, text);

		// Shrink to max size when not in 1:1 mode.
		if (textSize.width() < size.width() || (Size != 1 && textSize.width() > size.width()))
		{
			textSize.setWidth(size.width());
		}

		QRect displayRect{ rect.x(), rect.y() + size.height(), textSize.width(), textSize.height() + VerticalSpacing };

		painter->fillRect(displayRect, Qt::blue);

		displayRect.adjust(0, VerticalSpacing, 0, 0);

		painter->drawText(displayRect, text);

		if ((option.state & QStyle::State_HasFocus) != 0)
		{
			painter->setPen(Qt::white);
			painter->drawRect(rect.adjusted(0, 0, -1, -1));
		}

		painter->restore();
	}

	QSize sizeHint(const QStyleOptionViewItem& option,
		const QModelIndex& index) const override
	{
		QSize size{ 0, 0 };

		const auto value = index.data(Qt::DecorationRole);

		if (value.isValid() && !value.isNull())
		{
			const auto& entry = _window->GetWadFile()->Entries[index.data(Qt::UserRole).value<std::size_t>()].Entry;

			int width = (int)entry.Width;
			int height = (int)entry.Height;

			if (Size == 1)
			{
				size = QSize{ width, height };
			}
			else
			{
				size = QSize{ Size, Size };
			}
		}

		const QString text = index.data(Qt::DisplayRole).toString();

		QSize textSize = option.fontMetrics.size(0, text);

		// Only adjust width in 1:1 mode.
		if (Size == 1 && size.width() < textSize.width())
		{
			size.setWidth(textSize.width());
		}

		size.setHeight(size.height() + textSize.height() + VerticalSpacing);

		size = size.grownBy(QMargins{ Padding, Padding, Padding, Padding });

		return size;
	}

	int Size = 1;

private:
	WadMainWindow* _window;
};

constexpr int Sizes[] =
{
	1,
	32,
	64,
	128,
	256,
	512
};

WadMainWindow::WadMainWindow(MultiAsset* multiAsset)
	: _multiAsset(multiAsset)
{
	_ui = std::make_unique<Ui_WadMainWindow>();

	_ui->setupUi(this);

	{
		auto margins = this->contentsMargins();

		margins.setLeft(0);
		margins.setRight(0);

		this->setContentsMargins(margins);
	}

	{
		auto margins = _ui->WadEntryList->contentsMargins();

		margins.setLeft(0);
		margins.setRight(0);

		_ui->WadEntryList->setContentsMargins(margins);
	}

	_itemDelegate = new TextureItemDelegate(this);

	_ui->WadEntryList->setItemDelegate(_itemDelegate);

	_filterTimer = new QTimer(this);
	_filterTimer->setSingleShot(true);

	for (auto size : Sizes)
	{
		if (size == 1)
		{
			_ui->Size->addItem("1:1");
		}
		else
		{
			_ui->Size->addItem(QString{ "%1x%1" }.arg(size));
		}
	}

	connect(_ui->ActionOpen, &QAction::triggered, this, [this]
		{
			emit _multiAsset->PromptOpenFile(this, "Half-Life 1 Wad");
		});

	connect(_ui->WadEntryList, &QListWidget::currentRowChanged, this, &WadMainWindow::OnEntryChanged);
	connect(_ui->Size, &QComboBox::currentIndexChanged, this, &WadMainWindow::OnSizeChanged);
	connect(_ui->Filter, &QComboBox::editTextChanged, this, &WadMainWindow::OnFilterChanged);
	connect(_filterTimer, &QTimer::timeout, this, &WadMainWindow::UpdateTextureList);
}

WadMainWindow::~WadMainWindow() = default;

static bool HasTextureSpecifiers(const char* name)
{
	return name[0] == '+' || name[0] == '-';
}

static const char* SkipTextureSpecifiers(const char* name)
{
	if (HasTextureSpecifiers(name))
	{
		name += 2;
	}

	return name;
}

void WadMainWindow::OpenFile(FILE* file)
{
	auto wadFile = TryLoadWadFile(file);

	if (!wadFile)
	{
		return;
	}

	std::sort(wadFile->Entries.begin(), wadFile->Entries.end(), [](const auto& lhs, const auto& rhs)
		{
			const char* leftName = lhs.Name.c_str();
			const char* rightName = rhs.Name.c_str();

			if (HasTextureSpecifiers(leftName) || HasTextureSpecifiers(rightName))
			{
				const char* skippedLeftName = SkipTextureSpecifiers(leftName);
				const char* skippedRightName = SkipTextureSpecifiers(rightName);

				// If the base names are the same then we want to sort according to specifiers.
				if (_stricmp(skippedLeftName, skippedRightName) != 0)
				{
					leftName = skippedLeftName;
					rightName = skippedRightName;
				}
			}

			return _stricmp(leftName, rightName) < 0;
		});

	_wadFile.Entries.clear();

	_wadFile.Entries.reserve(wadFile->Entries.size());

	QList<QRgb> colorTable;

	colorTable.resize(ColormapColorCount);

	for (auto& entry : wadFile->Entries)
	{
		QImage image{ entry.Pixels.data(), (int)entry.Width, (int)entry.Height, QImage::Format_Indexed8 };

		for (std::size_t i = 0; i < entry.Colormap.size(); ++i)
		{
			colorTable[i] = qRgb(entry.Colormap[i].R, entry.Colormap[i].G, entry.Colormap[i].B);
		}

		image.setColorTable(colorTable);

		_wadFile.Entries.emplace_back(std::move(entry), QPixmap::fromImage(image));
	}

	UpdateTextureList();

	OnSizeChanged(_ui->Size->currentIndex());

	show();
}

void WadMainWindow::OnEntryChanged(int index)
{
	if (index == -1)
	{
		_ui->TextureName->setText({});
		_ui->TextureDimensions->setText({});
		return;
	}

	const auto& entry = _wadFile.Entries[index];

	_ui->TextureName->setText(QString::fromStdString(entry.Entry.Name));
	_ui->TextureDimensions->setText(QString{ "%1x%2" }.arg(entry.Entry.Width).arg(entry.Entry.Height));
}

void WadMainWindow::OnSizeChanged(int index)
{
	const int size = Sizes[index];

	_itemDelegate->Size = size;

	// This causes the list to perform layout again, including calling sizeHint for items.
	// Cheaper than emitting sizeHintChanged for every single item.
	_ui->WadEntryList->setRootIndex(_ui->WadEntryList->rootIndex());
}

void WadMainWindow::OnFilterChanged()
{
	// Wait a short time to avoid constant updates while typing.
	_filterTimer->start(500);
}

void WadMainWindow::UpdateTextureList()
{
	const QString filter = _ui->Filter->currentText();

	_ui->WadEntryList->clear();

	for (std::size_t i = 0; const auto & entry : _wadFile.Entries)
	{
		QString name = QString::fromStdString(entry.Entry.Name);

		if (name.contains(filter, Qt::CaseInsensitive))
		{
			auto item = new QListWidgetItem(QIcon{ entry.Pixmap }, name);

			item->setData(Qt::UserRole, QVariant::fromValue(i));

			_ui->WadEntryList->addItem(item);
		}

		++i;
	}
}
