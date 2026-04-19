#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QSettings>

#include "mainwindow.h"
#include "ui/stylesheet.h"
#include "core/datastructs.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("archvital");
    app.setOrganizationName("archvital");
    app.setApplicationVersion("1.0.0");

    // queued signals cross threads here, so Qt needs to know these types by name
    qRegisterMetaType<CpuData>("CpuData");
    qRegisterMetaType<MemoryData>("MemoryData");
    qRegisterMetaType<GpuData>("GpuData");
    qRegisterMetaType<SystemInfo>("SystemInfo");
    qRegisterMetaType<QVector<DiskPartitionData>>("QVector<DiskPartitionData>");
    qRegisterMetaType<QVector<DiskDeviceData>>("QVector<DiskDeviceData>");
    qRegisterMetaType<QVector<NetworkInterfaceData>>("QVector<NetworkInterfaceData>");
    qRegisterMetaType<QVector<ProcessData>>("QVector<ProcessData>");

    // pull the saved theme before the first window shows up
    QSettings settings("archvital", "archvital");
    Style::loadTheme(settings);

    // global app skin goes on once and then we leave it alone
    app.setStyleSheet(Style::appStyleSheet());

    // try Inter first, fall back to something sane if the system does not have it
    {
        QFont font("Inter");
        if (!QFontDatabase::families().contains("Inter"))
            font.setFamily("Noto Sans");
        font.setPointSize(10);
        app.setFont(font);
    }

    MainWindow w;
    w.show();

    return app.exec();
}
