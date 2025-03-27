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
    explicit ImageCanvas(MainWindow* mainWindow);
    ~ImageCanvas() override;

    void setId(int id);
    void setMask(const ImageMask& mask);
    void setActionMask(const ImageMask& mask);

    ImageMask getMask() const
    {
        return _mask;
    }

    QImage getImage() const
    {
        return _image;
    }


    void setWatershedMask(const QImage& watershed);
    void refresh();

    void updateMaskColor(const Id2Labels& labels)
    {
        _mask.updateColor(labels);
    }

    void loadImage(const QString& file);

    QScrollArea* getScrollParent() const
    {
        return _scrollArea;
    }

    bool isNotSaved() const
    {
        return _undoList.size() > 1;
    }

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

public slots :
    void scaleChanged(double);
    void alphaChanged(double);
    void setSizePen(int);
    void clearMask();
    void saveMask();
    void undo();
    void redo();

private:
    MainWindow* _mainWindow;

    void _initPixmap();
    void _drawFillCircle(QMouseEvent* e);

    QScrollArea* _scrollArea;
    double _scale;
    double _alpha;
    QImage _image;
    ImageMask _mask;
    ImageMask _watershed;
    QList<ImageMask> _undoList;
    bool _undo;
    int _undoIndex;
    QPoint _mousePosition;
    QString _imageFile;
    QString _maskFile;
    QString _watershedFile;
    ColorMask _labelColor;
    int _penSize;
    bool _buttonPressed;
    QSize _scaledImageSize;

private slots:
    void adjustScrollBars();
};


#endif //IMAGE_CANVAS_H
