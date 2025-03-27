#include <QSettings>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QMessageBox>
#include <QShortcut>
#include <QColorDialog>
#include <QFileDialog>
#include <QJsonDocument>
#include "pixel_annotation_tool_version.h"

#include "main_window.h"
#include "label_widget.h"
#include "about_dialog.h"

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags): QMainWindow(parent, flags), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(QApplication::translate("MainWindow", "PixelAnnotationTool " PIXEL_ANNOTATION_TOOL_GIT_TAG,
                                           Q_NULLPTR));
    ui->list_label->setSpacing(1);
    image_canvas = Q_NULLPTR;
    isLoadingNewLabels = false;

    save_action = new QAction(tr("&Save current image"), this);
    copy_mask_action = new QAction(tr("&Copy Mask"), this);
    paste_mask_action = new QAction(tr("&Paste Mask"), this);
    clear_mask_action = new QAction(tr("&Clear Mask mask"), this);
    close_tab_action = new QAction(tr("&Close current tab"), this);
    undo_action = new QAction(tr("&Undo"), this);
    swap_action = new QAction(tr("&Swap check box Watershed"), this);
    redo_action = new QAction(tr("&Redo"), this);
    next_file_action = new QAction(tr("&Select next file"), this);
    previous_file_action = new QAction(tr("&Select previous file"), this);

    save_action->setShortcut(QKeySequence::Save);
    copy_mask_action->setShortcut(QKeySequence::Copy);
    paste_mask_action->setShortcut(QKeySequence::Paste);
    clear_mask_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    close_tab_action->setShortcut(QKeySequence::Close);
    undo_action->setShortcuts(QKeySequence::Undo);
    swap_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Space));
    redo_action->setShortcuts(QKeySequence::Redo);
    next_file_action->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Down));
    previous_file_action->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Up));

    undo_action->setEnabled(false);
    redo_action->setEnabled(false);

    ui->menuFile->addAction(save_action);
    ui->menuEdit->addAction(close_tab_action);
    ui->menuEdit->addAction(undo_action);
    ui->menuEdit->addAction(redo_action);
    ui->menuEdit->addAction(copy_mask_action);
    ui->menuEdit->addAction(paste_mask_action);
    ui->menuEdit->addAction(clear_mask_action);
    ui->menuEdit->addAction(swap_action);
    ui->menuEdit->addAction(next_file_action);
    ui->menuEdit->addAction(previous_file_action);

    ui->tabWidget->clear();

    connect(ui->button_watershed, &QPushButton::released, this, &MainWindow::runWatershed);
    connect(swap_action, &QAction::triggered, this, &MainWindow::swapView);
    connect(ui->actionOpen_config_file, &QAction::triggered, this, &MainWindow::loadConfigFile);
    connect(ui->actionSave_config_file, &QAction::triggered, this, &MainWindow::saveConfigFile);
    connect(close_tab_action, &QAction::triggered, this, &MainWindow::closeCurrentTab);
    connect(copy_mask_action, &QAction::triggered, this, &MainWindow::copyMask);
    connect(paste_mask_action, &QAction::triggered, this, &MainWindow::pasteMask);
    connect(clear_mask_action, &QAction::triggered, this, &MainWindow::clearMask);
    connect(next_file_action, &QAction::triggered, this, &MainWindow::nextFile);
    connect(previous_file_action, &QAction::triggered, this, &MainWindow::previousFile);
    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::updateConnect);
    connect(ui->tree_widget_img, &QTreeWidget::itemClicked, this, &MainWindow::treeWidgetClicked);

    registerShortcuts();

    labels = defaultLabels();
    loadConfigLabels();

    connect(ui->list_label, &QListWidget::currentItemChanged, this, &MainWindow::changeLabel);
    connect(ui->list_label, &QListWidget::itemDoubleClicked, this, &MainWindow::changeColor);
    ui->list_label->setEnabled(false);

    setAcceptDrops(true);
    readSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::readSettings()
{
    QSettings settings;

    resize(settings.value("window/size", QSize(1511, 967)).toSize());
    move(settings.value("window/pos", QPoint(0, 0)).toPoint());
    ui->spinbox_pen_size->setValue(settings.value("pen_size", QVariant(30)).toInt());
    ui->spinbox_alpha->setValue(settings.value("alpha", QVariant(0.4)).toDouble());
    ui->spinbox_scale->setValue(settings.value("scale", QVariant(1.0)).toDouble());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    QSettings settings;
    settings.setValue("window/size", size());
    settings.setValue("window/pos", pos());
    settings.setValue("pen_size", ui->spinbox_pen_size->value());
    settings.setValue("alpha", ui->spinbox_alpha->value());
    settings.setValue("scale", ui->spinbox_scale->value());

    event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* e)
{
    for (const QUrl& url : e->mimeData()->urls())
    {
        QDir d = QFileInfo(url.toLocalFile()).absoluteDir();
        QString absolute = d.absolutePath();
        curr_open_dir = absolute;
        this->openDirectory();
    }
}

void MainWindow::closeCurrentTab()
{
    int index = ui->tabWidget->currentIndex();
    if (index >= 0)
        closeTab(index);
}

void MainWindow::closeTab(int index)
{
    ImageCanvas* ic = getImageCanvas(index);
    if (ic == Q_NULLPTR)
        throw std::logic_error("error index");

    if (ic->isNotSaved())
    {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Current image is not saved",
                                                                  "You will close the current image. Would you like to save the image before?",
                                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
            ic->saveMask();
        }
    }
    ui->tabWidget->removeTab(index);
    delete ic;
    if (ui->tabWidget->count() == 0)
    {
        image_canvas = Q_NULLPTR;
        ui->list_label->setEnabled(false);
    }
    else
    {
        image_canvas = getImageCanvas(std::min(index, ui->tabWidget->count() - 1));
    }
}

void MainWindow::registerShortcuts()
{
    QVector<QString> shortcutPrefix = {"", "Ctrl+", "Alt+", "Ctrl+Alt+"};

    for (int row = 0; row < 40; row++)
    {
        auto* shortcut = new QShortcut(QKeySequence(QString("%1%2").arg(shortcutPrefix[row / 10]).arg((row + 1) % 10)),
                                       this);
        shortcuts.append(shortcut);
        connect(shortcut, &QShortcut::activated, this, [=]()-> void
        {
            if (ui->list_label->isEnabled() && row < ui->list_label->count())
            {
                ui->list_label->setCurrentRow(row, QItemSelectionModel::ClearAndSelect);
                update();
            }
        });
    }
}

void MainWindow::loadConfigLabels()
{
    isLoadingNewLabels = true;
    ui->list_label->clear();
    QMapIterator it(labels);
    while (it.hasNext())
    {
        it.next();
        const LabelInfo& label = it.value();
        auto item = new QListWidgetItem(ui->list_label);
        auto label_widget = new LabelWidget(label, this);

        item->setSizeHint(label_widget->sizeHint());
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        ui->list_label->addItem(item);
        ui->list_label->setItemWidget(item, label_widget);

        auto& ref = labels[it.key()];
        ref.item = item;

        int id = ui->list_label->row(item);

        if (id < 40)
        {
            QShortcut* shortcut = shortcuts.at(id);
            QString text = label.name + " (" + shortcut->key().toString() + ")";
            label_widget->setText(text);
        }
    }
    id_labels = getId2Label(labels);
    isLoadingNewLabels = false;
}

void MainWindow::changeColor(QListWidgetItem* item)
{
    LabelWidget* widget = dynamic_cast<LabelWidget*>(ui->list_label->itemWidget(item));
    LabelInfo& label = labels[widget->getName()];
    QColor color = QColorDialog::getColor(label.color, this);
    if (color.isValid())
    {
        label.color = color;
        widget->setNewLabel(label);
    }
    image_canvas->setId(label.id);
    image_canvas->updateMaskColor(id_labels);
    image_canvas->refresh();
}

void MainWindow::changeLabel(QListWidgetItem* current, QListWidgetItem* previous)
{
    if (!isLoadingNewLabels && current)
    {
        if (previous)
        {
            dynamic_cast<LabelWidget*>(ui->list_label->itemWidget(previous))->setSelected(false);
        }
        else
        {
            for (int i = 0; i < ui->list_label->count(); i++)
            {
                dynamic_cast<LabelWidget*>(ui->list_label->itemWidget(ui->list_label->item(i)))->setSelected(false);
            }
        }
        auto label = dynamic_cast<LabelWidget*>(ui->list_label->itemWidget(current));
        label->setSelected(true);
        const QString key = label->getName();
        statusBar()->showMessage(
            QString("label=[%1] id=[%2] category=[%3] color=[%4]")
            .arg(key)
            .arg(labels[key].id)
            .arg(labels[key].category)
            .arg(labels[key].color.name())
        );
        image_canvas->setId(labels[key].id);
    }
}

void MainWindow::runWatershed()
{
    if (image_canvas)
    {
        QImage iwatershed = watershed(image_canvas->getImage(), image_canvas->getMask().id);
        if (!ui->checkbox_border_ws->isChecked())
        {
            iwatershed = removeBorder(iwatershed, id_labels);
        }
        image_canvas->setWatershedMask(iwatershed);
        ui->checkbox_watershed_mask->setCheckState(Qt::CheckState::Checked);
        image_canvas->update();
    }
}

void MainWindow::setStarAtNameOfTab(bool star)
{
    if (ui->tabWidget->count() > 0)
    {
        int index = ui->tabWidget->currentIndex();
        QString name = ui->tabWidget->tabText(index);
        if (star && !name.endsWith("*"))
        {
            //add star
            name += "*";
            ui->tabWidget->setTabText(index, name);
        }
        else if (!star && name.endsWith("*"))
        {
            //remove star
            int pos = name.lastIndexOf('*');
            name = name.left(pos);
            ui->tabWidget->setTabText(index, name);
        }
    }
}

void MainWindow::initCanvasConnection(const ImageCanvas* ic)
{
    if (ic)
    {
        connect(ui->spinbox_scale, SIGNAL(valueChanged(double)), ic, SLOT(scaleChanged(double)));
        connect(ui->spinbox_alpha, SIGNAL(valueChanged(double)), ic, SLOT(alphaChanged(double)));
        connect(ui->spinbox_pen_size, SIGNAL(valueChanged(int)), ic, SLOT(setSizePen(int)));
        connect(ui->checkbox_watershed_mask, SIGNAL(clicked()), ic, SLOT(update()));
        connect(ui->checkbox_manuel_mask, SIGNAL(clicked()), ic, SLOT(update()));
        connect(ui->actionClear, SIGNAL(triggered()), ic, SLOT(clearMask()));
        connect(undo_action, SIGNAL(triggered()), ic, SLOT(undo()));
        connect(redo_action, SIGNAL(triggered()), ic, SLOT(redo()));
        connect(save_action, SIGNAL(triggered()), ic, SLOT(saveMask()));
        connect(ui->checkbox_border_ws, SIGNAL(clicked()), this, SLOT(runWatershed()));
    }
}

void MainWindow::allDisconnect(const ImageCanvas* ic)
{
    if (ic)
    {
        disconnect(ui->spinbox_scale, SIGNAL(valueChanged(double)), ic, SLOT(scaleChanged(double)));
        disconnect(ui->spinbox_alpha, SIGNAL(valueChanged(double)), ic, SLOT(alphaChanged(double)));
        disconnect(ui->spinbox_pen_size, SIGNAL(valueChanged(int)), ic, SLOT(setSizePen(int)));
        disconnect(ui->checkbox_watershed_mask, SIGNAL(clicked()), ic, SLOT(update()));
        disconnect(ui->checkbox_manuel_mask, SIGNAL(clicked()), ic, SLOT(update()));
        disconnect(ui->actionClear, SIGNAL(triggered()), ic, SLOT(clearMask()));
        disconnect(undo_action, SIGNAL(triggered()), ic, SLOT(undo()));
        disconnect(redo_action, SIGNAL(triggered()), ic, SLOT(redo()));
        disconnect(save_action, SIGNAL(triggered()), ic, SLOT(saveMask()));
        disconnect(ui->checkbox_border_ws, SIGNAL(clicked()), this, SLOT(runWatershed()));
    }
}

ImageCanvas* MainWindow::newImageCanvas()
{
    ImageCanvas* ic = new ImageCanvas(this);
    ic->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ic->setScaledContents(true);
    initCanvasConnection(ic);
    return ic;
}

void MainWindow::updateConnect(int index)
{
    if (index < 0 || index >= ui->tabWidget->count())
        return;
    allDisconnect(image_canvas);
    image_canvas = getImageCanvas(index);
    ui->list_label->setEnabled(image_canvas);
    initCanvasConnection(image_canvas);
}

ImageCanvas* MainWindow::getImageCanvas(int index)
{
    QScrollArea* scroll_area = dynamic_cast<QScrollArea*>(ui->tabWidget->widget(index));
    ImageCanvas* ic = dynamic_cast<ImageCanvas*>(scroll_area->widget());
    return ic;
}

int MainWindow::getImageCanvas(QString name, ImageCanvas* ic)
{
    for (int i = 0; i < ui->tabWidget->count(); i++)
    {
        if (ui->tabWidget->tabText(i).startsWith(name))
        {
            return i;
        }
    }
    ic = newImageCanvas();
    QString iDir = currentDir();
    QString filepath(iDir + "/" + name);
    ic->loadImage(filepath);
    int index = ui->tabWidget->addTab(ic->getScrollParent(), name);
    return index;
}

QString MainWindow::currentDir() const
{
    QTreeWidgetItem* current = ui->tree_widget_img->currentItem();
    if (!current || !current->parent())
        return "";

    return current->parent()->text(0);
}

QString MainWindow::currentFile() const
{
    QTreeWidgetItem* current = ui->tree_widget_img->currentItem();
    if (!current || !current->parent())
        return "";

    return current->text(0);
}

void MainWindow::treeWidgetClicked()
{
    QString iFile = currentFile();
    QString iDir = currentDir();
    if (iFile.isEmpty() || iDir.isEmpty())
        return;

    allDisconnect(image_canvas);
    int index = getImageCanvas(iFile, image_canvas);
    initCanvasConnection(image_canvas);
    ui->tabWidget->setCurrentIndex(index);
}

void MainWindow::on_tree_widget_img_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    treeWidgetClicked();
}

void MainWindow::on_actionOpenDir_triggered()
{
    statusBar()->clearMessage();
    QString openedDir = QFileDialog::getExistingDirectory(this, "Choose a directory to be read in", curr_open_dir);
    if (openedDir.isEmpty())
        return;

    curr_open_dir = openedDir;
    this->openDirectory();
}

void MainWindow::openDirectory()
{
    QTreeWidgetItem* currentTreeDir = new QTreeWidgetItem(ui->tree_widget_img);
    currentTreeDir->setExpanded(true);
    currentTreeDir->setText(0, curr_open_dir);

    QDir current_dir(curr_open_dir);
    QStringList files = current_dir.entryList();
    static QStringList ext_img = {"png", "jpg", "bmp", "pgm", "jpeg", "jpe", "jp2", "pbm", "ppm", "tiff", "tif"};
    for (int i = 0; i < files.size(); i++)
    {
        if (files[i].size() < 4)
            continue;

        QString ext = files[i].section(".", -1, -1);
        bool is_image = false;
        for (int e = 0; e < ext_img.size(); e++)
        {
            if (ext.toLower() == ext_img[e])
            {
                is_image = true;
                break;
            }
        }
        if (!is_image)
            continue;

        if (files[i].toLower().indexOf("_mask.png") > -1)
            continue;

        QTreeWidgetItem* currentFile = new QTreeWidgetItem(currentTreeDir);
        currentFile->setText(0, files[i]);
    }
    // setWindowTitle("PixelAnnotation - " + openedDir);
}

void MainWindow::nextFile()
{
    QTreeWidgetItem* currentItem = ui->tree_widget_img->currentItem();
    if (!currentItem) return;
    QTreeWidgetItem* nextItem = ui->tree_widget_img->itemBelow(currentItem);
    if (!nextItem) return;
    ui->tree_widget_img->setCurrentItem(nextItem);
    treeWidgetClicked();
}

void MainWindow::previousFile()
{
    QTreeWidgetItem* currentItem = ui->tree_widget_img->currentItem();
    if (!currentItem) return;
    QTreeWidgetItem* previousItem = ui->tree_widget_img->itemAbove(currentItem);
    if (!previousItem) return;
    ui->tree_widget_img->setCurrentItem(previousItem);
    treeWidgetClicked();
}

void MainWindow::saveConfigFile()
{
    QString file = QFileDialog::getSaveFileName(this, tr("Save Config File"), QString(), tr("JSon file (*.json)"));
    QFile save_file(file);
    if (!save_file.open(QIODevice::WriteOnly))
    {
        qWarning("Couldn't open save file.");
        return;
    }
    QJsonObject object;
    labels.write(object);
    QJsonDocument saveDoc(object);
    save_file.write(saveDoc.toJson());
    save_file.close();
}

void MainWindow::loadConfigFile()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Open Config File"), QString(), tr("JSon file (*.json)"));
    QFile open_file(file);
    if (!open_file.open(QIODevice::ReadOnly))
    {
        qWarning("Couldn't open save file.");
        return;
    }
    QJsonObject object;
    QByteArray saveData = open_file.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));

    labels.clear();
    labels.read(loadDoc.object());
    open_file.close();

    loadConfigLabels();
    update();
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog* d = new AboutDialog(this);
    d->setModal(true);
    d->show();
}

void MainWindow::copyMask()
{
    if (ImageCanvas* ic = getCurrentImageCanvas())
    {
        copied_mask = ic->getMask();
    }
}

void MainWindow::pasteMask()
{
    if (ImageCanvas* ic = getCurrentImageCanvas())
    {
        ic->setActionMask(copied_mask);
    }
}

void MainWindow::clearMask()
{
    if (ImageCanvas* ic = getCurrentImageCanvas())
    {
        ic->setActionMask(ImageMask(QSize(ic->width(), ic->height())));
    }
}

ImageCanvas* MainWindow::getCurrentImageCanvas()
{
    int index = ui->tabWidget->currentIndex();
    return index == -1 ? Q_NULLPTR : getImageCanvas(index);
}

void MainWindow::swapView()
{
    ui->checkbox_watershed_mask->setCheckState(
        ui->checkbox_watershed_mask->checkState() == Qt::CheckState::Checked ? Qt::CheckState::Unchecked
            : Qt::CheckState::Checked
    );
    update();
}

void MainWindow::update()
{
    QWidget::update();
    if (ImageCanvas* ic = getCurrentImageCanvas())
    {
        ic->update();
    }
}
