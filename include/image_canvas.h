#ifndef IMAGE_CANVAS_H
#define IMAGE_CANVAS_H

#include <QLabel>
#include <QScrollArea>

#include "utils.h"
#include "image_mask.h"

class MainWindow;

class ImageCanvas : public QLabel
{
    Q_OBJECT

public:
    explicit ImageCanvas(QScrollArea* parent, MainWindow* mainWindow);

    void setLabelColor(int id);

    void setActionMask(const ImageMask& mask);

    void setWatershedMask(const QImage& watershed);

    void setPenSize(int penSize);

    ImageMask getMask() const;

    QImage getImage() const;

    void loadImage(const QString& filePath);

    void saveMask();

    void scaleChanged(double scale);

    void alphaChanged(double alpha);

    void refresh();

    void updateMaskColor(const Id2Labels& labels)
    {
        _mask.updateColor(labels);
    }

    bool isNotSaved() const
    {
        return _undoList.size() > 1;
    }

protected:
    void mouseMoveEvent(QMouseEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;

    void wheelEvent(QWheelEvent* event) override;

    void paintEvent(QPaintEvent* event) override;

public slots :
    void clearMask();

    void undo();

    void redo();

private:
    MainWindow* _mainWindow;

    void _initPixmap();

    void _drawFillCircle(const QMouseEvent* e);

    QScrollArea* _scrollArea;
    double _scale;
    double _alpha;
    QImage _image;
    ImageMask _mask;
    ImageMask _watershed;
    QList<ImageMask> _undoList;
    bool _undo;
    int _undoIndex;
    QPoint _globalMousePosition;
    QString _imageFilePath;
    QString _maskFilePath;
    QString _watershedFilePath;
    ColorMask _labelColor;
    int _penSize;
    bool _leftButtonPressed;
    bool _rightButtonPressed;
};


#endif //IMAGE_CANVAS_H
