// Standalone UI render test (no network)
// Used to verify display on RPi under Cage

#include "SpellMasterUI.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    SpellMasterUI ui;
    ui.setWindowFlags(Qt::FramelessWindowHint);
    QApplication::setOverrideCursor(Qt::BlankCursor);
    ui.show();

    return app.exec();
}
