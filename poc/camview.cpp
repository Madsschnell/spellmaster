// camview.cpp - Kamera-preview på DSI-skærm via Cage + Qt6
// Starter rpicam-vid som subprocess og viser MJPEG-frames i et QWidget.
// Kompiler: g++ camview.cpp -o camview $(pkg-config --cflags --libs Qt6Widgets) -fPIC
// Kør:      sudo XDG_RUNTIME_DIR=/run/user/1000 cage -- /home/spellmaster/camview

#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QProcess>
#include <QImage>
#include <QPixmap>
#include <QVBoxLayout>

class CamView : public QWidget {
    QLabel *imgLabel;
    QProcess *proc;
    QByteArray buffer;

public:
    CamView() {
        setCursor(Qt::BlankCursor);
        imgLabel = new QLabel(this);
        imgLabel->setAlignment(Qt::AlignCenter);
        imgLabel->setStyleSheet("background-color: black;");
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(imgLabel);

        proc = new QProcess(this);
        proc->start("rpicam-vid", {
            "-t", "0",
            "--width", "800",
            "--height", "480",
            "--framerate", "30",
            "--codec", "mjpeg",
            "--nopreview",
            "-o", "-"
        });

        connect(proc, &QProcess::readyReadStandardOutput, this, &CamView::onData);
        showFullScreen();
    }

    void onData() {
        buffer.append(proc->readAllStandardOutput());

        while (true) {
            int soi = buffer.indexOf(QByteArray("\xff\xd8", 2));
            if (soi < 0) break;
            int eoi = buffer.indexOf(QByteArray("\xff\xd9", 2), soi + 2);
            if (eoi < 0) break;

            QByteArray jpeg = buffer.mid(soi, eoi - soi + 2);
            buffer = buffer.mid(eoi + 2);

            QImage img;
            img.loadFromData(jpeg, "JPEG");
            if (!img.isNull())
                imgLabel->setPixmap(QPixmap::fromImage(img));
        }
    }

    ~CamView() {
        proc->kill();
        proc->waitForFinished();
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setOverrideCursor(Qt::BlankCursor);
    CamView w;
    return app.exec();
}
