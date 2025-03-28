#include <QDebug>
#include <QPainter>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QScrollBar>
#include <QMouseEvent>

#include "image_canvas.h"
#include "main_window.h"

ImageCanvas::ImageCanvas(QScrollArea* parent, MainWindow* mainWindow) : _mainWindow(mainWindow), _scrollArea(parent)
{
    _initPixmap();
    setScaledContents(true);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    _scale = _mainWindow->ui->spinbox_scale->value();
    _alpha = _mainWindow->ui->spinbox_alpha->value();
    _penSize = _mainWindow->ui->spinbox_pen_size->value();
    _leftButtonPressed = false;
    _undoList.clear();
    _undoIndex = 0;
    _undo = false;

    _scrollArea->setBackgroundRole(QPalette::Dark);
    _scrollArea->setWidget(this);
    setParent(_scrollArea);
    resize(800, 600);
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

void ImageCanvas::setLabelColor(const int id)
{
    _labelColor.id = QColor(id, id, id);
    _labelColor.color = _mainWindow->id_labels[id]->color;
}

void ImageCanvas::setActionMask(const ImageMask& mask)
{
    _mask = mask;
    _undoList.push_back(_mask);
    _undoIndex++;
    _mainWindow->setStarAtNameOfTab(true);
    _mainWindow->undo_action->setEnabled(true);
}

void ImageCanvas::setWatershedMask(const QImage& watershed)
{
    _watershed.id = watershed;
    idToColor(_watershed.id, _mainWindow->id_labels, &_watershed.color);
}

void ImageCanvas::setPenSize(const int penSize)
{
    _penSize = penSize;
    update();
}

ImageMask ImageCanvas::getMask() const
{
    return _mask;
}

QImage ImageCanvas::getImage() const
{
    return _image;
}


void ImageCanvas::loadImage(const QString& filePath)
{
    if (!_image.isNull())
    {
        saveMask();
    }

    _imageFilePath = filePath;
    QFileInfo file(_imageFilePath);
    if (!file.exists())
    {
        return;
    }

    _image = mat2QImage(cv::imread(_imageFilePath.toStdString()));

    _maskFilePath = file.dir().absolutePath() + "/" + file.completeBaseName() + "_mask.png";
    _watershedFilePath = file.dir().absolutePath() + "/" + file.completeBaseName() + "_watershed_mask.png";

    _watershed = ImageMask(_image.size());
    _undoList.clear();
    _undoIndex = 0;
    if (QFile(_maskFilePath).exists())
    {
        _mask = ImageMask(_maskFilePath, _mainWindow->id_labels);
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
    {
        return;
    }

    _mask.id.save(_maskFilePath);
    if (!_watershed.id.isNull())
    {
        QImage watershed = _watershed.id;
        // if (!_mainWindow->ui->checkbox_border_ws->isChecked())
        // {
        //     watershed = removeBorder(_watershed.id, _mainWindow->id_labels);
        // }
        watershed.save(_watershedFilePath);
        QFileInfo file(_imageFilePath);
        QString color_file = file.dir().absolutePath() + "/" + file.completeBaseName() + "_color_mask.png";
        idToColor(watershed, _mainWindow->id_labels).save(color_file);
    }
    _undoList.clear();
    _undoIndex = 0;
    _mainWindow->setStarAtNameOfTab(false);
}

void ImageCanvas::scaleChanged(const double scale)
{
    resize(scale * _image.size());

    // Adjust scrollbars
    if (QScrollBar* vScrollBar = _scrollArea->verticalScrollBar())
    {
        qDebug() << vScrollBar->value();
        // Let y = _globalMousePosition.y() before resize, v = vScrollBar->value() before resize, and α = _scale before resize
        // Let y' = _globalMousePosition.y() after resize, v' = vScrollBar->value() after resize, and α' = _scale after resize
        // Then y - v = y' - v', y' = (α' / α) * y
        // Then v' = (α' / α - 1) * y + v
        vScrollBar->setValue(
            (scale / _scale - 1) * _globalMousePosition.y() + vScrollBar->value()
        );
    }

    if (QScrollBar* hScrollBar = _scrollArea->horizontalScrollBar())
    {
        hScrollBar->setValue(
            (scale / _scale - 1) * _globalMousePosition.x() + hScrollBar->value()
        );
    }

    _scale = scale;
    update();
}

void ImageCanvas::alphaChanged(const double alpha)
{
    _alpha = alpha;
    update();
}


void ImageCanvas::mouseMoveEvent(QMouseEvent* event)
{
    qDebug() << "ImageCanvas::mouseMoveEvent";
    _globalMousePosition = event->position().toPoint();

    if (_leftButtonPressed)
    {
        _drawFillCircle(event);
    }

    _mainWindow->ui->statusbar->showMessage(
        QString("[Global] X: %1 Y: %2").arg(_globalMousePosition.x()).arg(_globalMousePosition.y())
    );
    update();
}

void ImageCanvas::mousePressEvent(QMouseEvent* e)
{
    qDebug() << "ImageCanvas::mousePressEvent";
    setFocus();
    if (e->button() == Qt::LeftButton)
    {
        _leftButtonPressed = true;
        _drawFillCircle(e);
        update();
    }
}

void ImageCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    qDebug() << "ImageCanvas::mouseReleaseEvent";
    if (event->button() == Qt::LeftButton)
    {
        _leftButtonPressed = false;

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

    if (event->button() == Qt::RightButton)
    {
        // selection of label
        QColor maskColor = _mask.id.pixel(_globalMousePosition / _scale);
        QColor watershedColor = _watershed.id.pixel(_globalMousePosition / _scale);
        const LabelInfo* label = _mainWindow->id_labels[maskColor.red()]
                                     ? _mainWindow->id_labels[maskColor.red()] :
                                     _mainWindow->id_labels[watershedColor.red()];
        if (label)
        {
            if (!_watershed.id.isNull() && _mainWindow->ui->checkbox_watershed_mask->isChecked())
            {
                QColor color = QColor(_watershed.id.pixel(_globalMousePosition / _scale));
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

    if (event->button() == Qt::MiddleButton)
    {
        int x, y;
        if (_penSize > 0)
        {
            x = event->position().x() / _scale;
            y = event->position().y() / _scale;
        }
        else
        {
            x = (event->position().x() + 0.5) / _scale;
            y = (event->position().y() + 0.5) / _scale;
        }

        _mask.exchangeLabel(x, y, _mainWindow->id_labels, _labelColor);
        update();
    }
}

void ImageCanvas::keyPressEvent(QKeyEvent* event)
{
    qDebug() << "ImageCanvas::keyPressEvent";
    if (event->key() == Qt::Key_Space)
    {
        emit _mainWindow->ui->button_watershed->released();
    }
}

void ImageCanvas::wheelEvent(QWheelEvent* event)
{
    qDebug() << "ImageCanvas::wheelEvent";
    int delta = event->angleDelta().y() > 0 ? 1 : -1;
    if (Qt::ShiftModifier == event->modifiers())
    {
        _scrollArea->verticalScrollBar()->setEnabled(false);
        int value = _mainWindow->ui->spinbox_pen_size->value()
            + delta * _mainWindow->ui->spinbox_pen_size->singleStep();
        _mainWindow->ui->spinbox_pen_size->setValue(value);
    }
    else if (Qt::ControlModifier == event->modifiers())
    {
        double newScale = _mainWindow->ui->spinbox_scale->value()
            + delta * _mainWindow->ui->spinbox_scale->singleStep();
        newScale = std::min<double>(_mainWindow->ui->spinbox_scale->maximum(), newScale);
        newScale = std::max<double>(_mainWindow->ui->spinbox_scale->minimum(), newScale);
        // Notice that it is the mouse position before resize
        _globalMousePosition = event->position().toPoint();
        _mainWindow->ui->spinbox_scale->setValue(newScale);
    }
}

void ImageCanvas::paintEvent(QPaintEvent* event)
{
    qDebug() << "ImageCanvas::paintEvent";
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

    if (_globalMousePosition.x() > 10 && _globalMousePosition.y() > 10 &&
        _globalMousePosition.x() <= QLabel::size().width() - 10 &&
        _globalMousePosition.y() <= QLabel::size().height() - 10)
    {
        painter.setBrush(QBrush(_labelColor.color));
        painter.setPen(QPen(QBrush(_labelColor.color), 1.0));
        painter.drawEllipse(_globalMousePosition.x() / _scale - _penSize / 2,
                            _globalMousePosition.y() / _scale - _penSize / 2,
                            _penSize, _penSize);
    }
    painter.end();
}

void ImageCanvas::_drawFillCircle(const QMouseEvent* e)
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
}

void ImageCanvas::clearMask()
{
    _mask = ImageMask(_image.size());
    _watershed = ImageMask(_image.size());
    _undoList.clear();
    _undoIndex = 0;
    update();
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
