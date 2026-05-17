// colortest.cpp - Color and touch test for Cage + Qt6
// Build: g++ colortest.cpp -o colortest $(pkg-config --cflags --libs Qt6Widgets) -fPIC
// Run:   sudo XDG_RUNTIME_DIR=/run/user/1000 cage -- /home/spellmaster/colortest

#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QCursor>

class ColorTest : public QWidget {
    QLabel *label;
    int idx = 0;
    struct Color { const char *name; const char *hex; const char *fg; };
    Color colors[6] = {
        {"Roed", "#ff0000", "white"},
        {"Groen", "#00ff00", "black"},
        {"Blaa", "#0000ff", "white"},
        {"Hvid", "#ffffff", "black"},
        {"Sort", "#000000", "white"},
        {"Gul", "#ffff00", "black"}
    };
public:
    ColorTest() {
        setCursor(Qt::BlankCursor);
        label = new QLabel(this);
        label->setAlignment(Qt::AlignCenter);
        auto *btn = new QPushButton("Naeste farve", this);
        btn->setStyleSheet("font-size: 24px; padding: 20px;");
        btn->setCursor(Qt::BlankCursor);
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(label);
        layout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, &ColorTest::next);
        next();
        showFullScreen();
    }
    void next() {
        auto &c = colors[idx % 6];
        setStyleSheet(QString("background-color: %1;").arg(c.hex));
        label->setStyleSheet(QString("font-size: 48px; color: %1;").arg(c.fg));
        label->setText(QString("%1 (%2/6)").arg(c.name).arg(idx % 6 + 1));
        idx++;
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setOverrideCursor(Qt::BlankCursor);
    ColorTest w;
    return app.exec();
}
