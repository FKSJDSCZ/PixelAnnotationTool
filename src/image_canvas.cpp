#include <QDebug>
#include <QPainter>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QScrollBar>
#include <QMouseEvent>

#include "image_canvas.h"
#include "main_window.h"

ImageCanvas::ImageCanvas(MainWindow* mainWindow) : _mainWindow(mainWindow)
{
    _initPixmap();
    setScaledContents(true);
    setMouseTracking(true);

    _scale = _mainWindow->ui->spinbox_scale->value();
    _alpha = _mainWindow->ui->spinbox_alpha->value();
    _penSize = _mainWindow->ui->spinbox_pen_size->value();
    _buttonPressed = false;
    _undoList.clear();
    _undoIndex = 0;
    _undo = false;

    _scrollArea = new QScrollArea(mainWindow);
    _scrollArea->setBackgroundRole(QPalette::Dark);
    _scrollArea->setWidget(this);
    setParent(_scrollArea);
    resize(800, 600);
}

ImageCanvas::~ImageCanvas()
{
    _scrollArea->deleteLater();
}

void ImageCanvas::_initPixmap()
{
    auto newPixmap = QPixmap(width(), height());
    newPixmap.fill(Qt::white);
    if (!pixmap().isNull())
    {
        QPainter painter(&newPixmap);
        painter.drawPixmap(0, 0, pixmap());
        painter.end();
    }
    setPixmap(newPixmap);
}

void ImageCanvas::loadImage(const QString& filename)
{
    if (!_image.isNull())
        saveMask();

    _imageFile = filename;
    QFileInfo file(_imageFile);
    if (!file.exists()) return;

    _image = mat2QImage(cv::imread(_imageFile.toStdString()));

    _maskFile = file.dir().absolutePath() + "/" + file.completeBaseName() + "_mask.png";
    _watershedFile = file.dir().absolutePath() + "/" + file.completeBaseName() + "_watershed_mask.png";

    _watershed = ImageMask(_image.size());
    _undoList.clear();
    _undoIndex = 0;
    if (QFile(_maskFile).exists())
    {
        _mask = ImageMask(_maskFile, _mainWindow->id_labels);
        // mainWindow->runWatershed(this); // ui->button_watershed->released());
        // mainWindow->ui->checkbox_manuel_mask->setChecked(true);
        _undoList.push_back(_mask);
        _undoIndex++;
    }
    else
    {
        clearMask();
    }
    _mainWindow->undo_action->setEnabled(false);
    _mainWindow->redo_action->setEnabled(false);

    setPixmap(QPixmap::fromImage(_image));
    resize(_scale * _image.size());
}

void ImageCanvas::saveMask()
{
    if (isFullZero(_mask.id))
        return;

    _mask.id.save(_maskFile);
    if (!_watershed.id.isNull())
    {
        QImage watershed = _watershed.id;
        //         if (!_ui->checkbox_border_ws->isChecked()) {
        //             watershed = removeBorder(_watershed.id, _ui->id_labels);
        //         }
        watershed.save(_watershedFile);
        QFileInfo file(_imageFile);
        QString color_file = file.dir().absolutePath() + "/" + file.completeBaseName() + "_color_mask.png";
        idToColor(watershed, _mainWindow->id_labels).save(color_file);
    }
    _undoList.clear();
    _undoIndex = 0;
    _mainWindow->setStarAtNameOfTab(false);
}

void ImageCanvas::scaleChanged(double scale)
{
    _scale = scale;
    resize(_scale * _image.size());

    adjustScrollBars();
    repaint();
}

void ImageCanvas::adjustScrollBars()
{
    // from : https://github.com/BestVanRome
    //   x ------>
    //     _________
    // y  |.........|
    // |  |.........|
    // |  |.........|
    // |  |.........|
    // v  |.........|
    //     ---------
    QPointF mPos = _mousePosition;
    QSize imSize = _scaledImageSize;

    if (QScrollBar* verticalScroll = _scrollArea->verticalScrollBar())
    {
        auto posHeightRel = mPos.y() / imSize.height(); //Relation Mauspos to Height of Image
        double verticalScrollSpace = verticalScroll->maximum() - verticalScroll->minimum();
        // general calculating of moving-space
        verticalScroll->setValue(verticalScrollSpace * posHeightRel);
        //alternative: QWheelEvent::angleDelta().y() -> see example!!! : https://doc.qt.io/qt-5/qwheelevent.html#angleDelta
    }

    if (QScrollBar* horizontalScroll = _scrollArea->horizontalScrollBar())
    {
        auto posWidthRel = (mPos.x() / imSize.width()); ////Relation Mauspos to Width of Image

        double horizontalScrollSpace = horizontalScroll->maximum() - horizontalScroll->minimum();
        // general calculating of moving-space
        horizontalScroll->setValue(horizontalScrollSpace * posWidthRel);
    }
}

void ImageCanvas::alphaChanged(double alpha)
{
    _alpha = alpha;
    repaint();
}

void ImageCanvas::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    QRect rect = painter.viewport();
    QSize size = _scale * _image.size();
    if (size != _image.size())
    {
        rect.size().scale(size, Qt::KeepAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(pixmap().rect());
    }
    painter.drawImage(QPoint(0, 0), _image);
    painter.setOpacity(_alpha);

    if (!_mask.id.isNull() && _mainWindow->ui->checkbox_manuel_mask->isChecked())
    {
        painter.drawImage(QPoint(0, 0), _mask.color);
    }

    if (!_watershed.id.isNull() && _mainWindow->ui->checkbox_watershed_mask->isChecked())
    {
        painter.drawImage(QPoint(0, 0), _watershed.color);
    }

    if (_mousePosition.x() > 10 && _mousePosition.y() > 10 &&
        _mousePosition.x() <= QLabel::size().width() - 10 &&
        _mousePosition.y() <= QLabel::size().height() - 10)
    {
        painter.setBrush(QBrush(_labelColor.color));
        painter.setPen(QPen(QBrush(_labelColor.color), 1.0));
        painter.drawEllipse(_mousePosition.x() / _scale - _penSize / 2, _mousePosition.y() / _scale - _penSize / 2, _penSize,
                            _penSize);
        painter.end();
    }
}

void ImageCanvas::mouseMoveEvent(QMouseEvent* e)
{
    _mousePosition.setX(e->position().x());
    _mousePosition.setY(e->position().y());

    if (_buttonPressed)
    {
        _drawFillCircle(e);
    }
    _scaledImageSize = _image.size() * _scale; //important for adjusting the scrollBars

    //using statusbar to show actual _mouse_pos
    _mainWindow->ui->statusbar->showMessage(
        QString("X: %1 Y: %2").arg(_mousePosition.x()).arg(_mousePosition.y())
    );
    update();
}

void ImageCanvas::setSizePen(int pen_size)
{
    _penSize = pen_size;
}


void ImageCanvas::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        _buttonPressed = false;

        if (_undo)
        {
            QMutableListIterator it(_undoList);
            int i = 0;
            while (it.hasNext())
            {
                it.next();
                if (i++ >= _undoIndex)
                    it.remove();
            }
            _undo = false;
            _mainWindow->redo_action->setEnabled(false);
        }
        _undoList.push_back(_mask);
        _undoIndex++;
        _mainWindow->setStarAtNameOfTab(true);
        _mainWindow->undo_action->setEnabled(true);
    }

    if (e->button() == Qt::RightButton)
    {
        // selection of label
        QColor maskColor = _mask.id.pixel(_mousePosition / _scale);
        QColor watershedColor = _watershed.id.pixel(_mousePosition / _scale);
        const LabelInfo* label = _mainWindow->id_labels[maskColor.red()]
                                     ? _mainWindow->id_labels[maskColor.red()] :
                                     _mainWindow->id_labels[watershedColor.red()];
        if (label)
        {
            if (!_watershed.id.isNull() && _mainWindow->ui->checkbox_watershed_mask->isChecked())
            {
                QColor color = QColor(_watershed.id.pixel(_mousePosition / _scale));
                QMap<int, const LabelInfo*>::const_iterator it = _mainWindow->id_labels.find(color.red());
                if (it != _mainWindow->id_labels.end())
                {
                    label = it.value();
                }
            }
            if (label->item)
            {
                emit _mainWindow->ui->list_label->currentItemChanged(label->item, Q_NULLPTR);
            }
            refresh();
        }
    }

    if (e->button() == Qt::MiddleButton)
    {
        int x, y;
        if (_penSize > 0)
        {
            x = e->position().x() / _scale;
            y = e->position().y() / _scale;
        }
        else
        {
            x = (e->position().x() + 0.5) / _scale;
            y = (e->position().y() + 0.5) / _scale;
        }

        _mask.exchangeLabel(x, y, _mainWindow->id_labels, _labelColor);
        update();
    }
}

void ImageCanvas::mousePressEvent(QMouseEvent* e)
{
    setFocus();
    if (e->button() == Qt::LeftButton)
    {
        _buttonPressed = true;
        _drawFillCircle(e);
    }
}

void ImageCanvas::_drawFillCircle(QMouseEvent* e)
{
    if (_penSize > 0)
    {
        int x = e->position().x() / _scale - _penSize / 2;
        int y = e->position().y() / _scale - _penSize / 2;
        _mask.drawFillCircle(x, y, _penSize, _labelColor);
    }
    else
    {
        int x = (e->position().x() + 0.5) / _scale;
        int y = (e->position().y() + 0.5) / _scale;
        _mask.drawPixel(x, y, _labelColor);
    }
    update();
}

void ImageCanvas::clearMask()
{
    _mask = ImageMask(_image.size());
    _watershed = ImageMask(_image.size());
    _undoList.clear();
    _undoIndex = 0;
    repaint();
}

void ImageCanvas::wheelEvent(QWheelEvent* event)
{
    int delta = event->angleDelta().y() > 0 ? 1 : -1;
    if (Qt::ShiftModifier == event->modifiers())
    {
        _scrollArea->verticalScrollBar()->setEnabled(false);
        int value = _mainWindow->ui->spinbox_pen_size->value() + delta * _mainWindow->ui->spinbox_pen_size->
            singleStep();
        _mainWindow->ui->spinbox_pen_size->setValue(value);
        emit _mainWindow->ui->spinbox_pen_size->valueChanged(value);
        setSizePen(value);
        repaint();
    }
    else if (Qt::ControlModifier == event->modifiers())
    {
        _scrollArea->verticalScrollBar()->setEnabled(false);
        double value = _mainWindow->ui->spinbox_scale->value() + delta * _mainWindow->ui->spinbox_scale->singleStep();
        value = std::min<double>(_mainWindow->ui->spinbox_scale->maximum(), value);
        value = std::max<double>(_mainWindow->ui->spinbox_scale->minimum(), value);

        _mainWindow->ui->spinbox_scale->setValue(value);
        scaleChanged(value);
        repaint();
    }
    else
    {
        _scrollArea->verticalScrollBar()->setEnabled(true);
    }
}

void ImageCanvas::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space)
    {
        emit _mainWindow->ui->button_watershed->released();
    }
}

void ImageCanvas::setWatershedMask(const QImage& watershed)
{
    _watershed.id = watershed;
    idToColor(_watershed.id, _mainWindow->id_labels, &_watershed.color);
}

void ImageCanvas::setMask(const ImageMask& mask)
{
    _mask = mask;
}

void ImageCanvas::setActionMask(const ImageMask& mask)
{
    setMask(mask);
    _undoList.push_back(_mask);
    _undoIndex++;
    _mainWindow->setStarAtNameOfTab(true);
    _mainWindow->undo_action->setEnabled(true);
}

void ImageCanvas::setId(const int id)
{
    _labelColor.id = QColor(id, id, id);
    _labelColor.color = _mainWindow->id_labels[id]->color;
}

void ImageCanvas::refresh()
{
    if (!_watershed.id.isNull() && _mainWindow->ui->checkbox_watershed_mask->isChecked())
    {
        emit _mainWindow->ui->button_watershed->released();
    }
    update();
}


void ImageCanvas::undo()
{
    _undo = true;
    _undoIndex--;
    if (_undoIndex == 1)
    {
        _mask = _undoList.at(_undoIndex - 1);
        _mainWindow->undo_action->setEnabled(false);
        refresh();
    }
    else if (_undoIndex > 1)
    {
        _mask = _undoList.at(_undoIndex - 1);
        refresh();
    }
    else
    {
        _undoIndex = 0;
        _mainWindow->undo_action->setEnabled(false);
    }
    _mainWindow->redo_action->setEnabled(true);
}

void ImageCanvas::redo()
{
    _undoIndex++;
    if (_undoIndex < _undoList.size())
    {
        _mask = _undoList.at(_undoIndex - 1);
        refresh();
    }
    else if (_undoIndex == _undoList.size())
    {
        _mask = _undoList.at(_undoIndex - 1);
        _mainWindow->redo_action->setEnabled(false);
        refresh();
    }
    else
    {
        _undoIndex = _undoList.size();
        _mainWindow->redo_action->setEnabled(false);
    }
    _mainWindow->undo_action->setEnabled(true);
}
