/**
 * Image Annotation Tool for image annotations with pixelwise masks
 *
 * Author: Rudra Poudel
 */
#ifndef TestWindow_H
#define TestWindow_H

#include <QShortcut>

#include "ui_main_window.h"
#include "image_canvas.h"

QT_BEGIN_NAMESPACE

namespace Ui
{
    class MainWindow;
}

QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    Ui::MainWindow* ui;

    explicit MainWindow(QWidget* parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::Widget);
    ~MainWindow() override;

private:
    void readSettings();
    void loadConfigLabels();
    void openDirectory();
    void registerShortcuts();
    ImageCanvas* newImageCanvas();
    int getImageCanvas(QString name, ImageCanvas* ic);
    ImageCanvas* getImageCanvas(int index);
    ImageCanvas* getCurrentImageCanvas();
    ImageMask copied_mask;
    QVector<QShortcut*> shortcuts;
    bool isLoadingNewLabels;

public:
    ImageCanvas* image_canvas;
    //std::vector<ImageCanvas*> _image_canvas;
    //QScrollArea   *  scroll_area  ;

    Name2Labels labels;
    Id2Labels id_labels;
    QAction* save_action;
    QAction* copy_mask_action;
    QAction* paste_mask_action;
    QAction* clear_mask_action;
    QAction* close_tab_action;
    QAction* undo_action;
    QAction* swap_action;
    QAction* redo_action;
    QAction* next_file_action;
    QAction* previous_file_action;
    QString curr_open_dir;

    QString currentDir() const;
    QString currentFile() const;
    void initCanvasConnection(const ImageCanvas* ic);
    void allDisconnect(const ImageCanvas* ic);
    void setStarAtNameOfTab(bool star);
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;

protected:
    void closeEvent(QCloseEvent* event) override;

public slots:
    void changeLabel(QListWidgetItem*, QListWidgetItem*);
    void changeColor(QListWidgetItem*);
    void saveConfigFile();
    void loadConfigFile();
    void runWatershed();
    void swapView();
    void on_tree_widget_img_currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*);
    void on_actionOpenDir_triggered();
    //void on_actionOpen_jsq_triggered();
    void on_actionAbout_triggered();
    void closeTab(int index);
    void closeCurrentTab();
    void copyMask();
    void pasteMask();
    void clearMask();
    void nextFile();
    void previousFile();
    void updateConnect(int index);
    void treeWidgetClicked();
    void update();
};

#endif
