/**
 * Image Annotation Tool for image annotations with pixelwise masks
 *
 * Author: Rudra Poudel
 */
#include "main_window.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName("pixelannotationtool_org");
    QApplication::setOrganizationDomain("pixelannotationtool_domain");
    QApplication::setApplicationName("PixelAnnotationTool");

    MainWindow win;
    win.show();

    return QApplication::exec();
}
