#include <UserInterface/MainWindow.hpp>
#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QDir>
#include <RMG-Core/Core.hpp>
#include <iostream>
#include <cstdlib>

#ifndef _WIN32
#include <signal.h>
#endif

void message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QByteArray localMsg = msg.toLocal8Bit();
    bool showDebugQtMessages = false;
    const char* env = std::getenv("RMG_SHOW_DEBUG_QT_MESSAGES");
    showDebugQtMessages = env != nullptr && std::string(env) == "1";

    std::string typeString;
    switch (type) {
        case QtDebugMsg:
        case QtWarningMsg:
        case QtInfoMsg:
            if (!showDebugQtMessages) return;
            typeString = (type == QtDebugMsg) ? "[QT DEBUG] " :
                         (type == QtWarningMsg) ? "[QT WARNING] " : "[QT INFO] ";
            break;
        case QtCriticalMsg:
            typeString = "[QT CRITICAL] ";
            break;
        case QtFatalMsg:
            typeString = "[QT FATAL] ";
            break;
    }
    std::cerr << typeString << localMsg.constData() << std::endl;
}

void signal_handler(int sig) {
    QGuiApplication::quit();
}

int main(int argc, char **argv) {
    qInstallMessageHandler(message_handler);

#ifndef _WIN32
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    const char* wayland = std::getenv("RMG_WAYLAND");
    if (wayland != nullptr && std::string(wayland) == "1") {
        setenv("QT_QPA_PLATFORM", "wayland", 1);
    } else {
        setenv("QT_QPA_PLATFORM", "xcb", 1);
    }

    setenv("QT_VULKAN_LIB", "libvulkan.so.1", 0);

    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSwapInterval(0);
    format.setMajorVersion(3);
    format.setMinorVersion(3);
    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication::setDesktopFileName("com.github.Rosalie241.RMG");
#endif

    QApplication app(argc, argv);
    UserInterface::MainWindow window;

#ifdef PORTABLE_INSTALL
    if (CoreGetPortableDirectoryMode()) {
        QDir::setCurrent(app.applicationDirPath());
    }
#endif

    QCoreApplication::setApplicationName("IceHace's Modded Mupen GUI");
    QCoreApplication::setApplicationVersion(QString::fromStdString(CoreGetVersion()));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

#ifndef PORTABLE_INSTALL
    QCommandLineOption libPathOption("lib-path", "Changes the path where the libraries are stored", "path");
    QCommandLineOption corePathOption("core-path", "Changes the path where the core library is stored", "path");
    QCommandLineOption pluginPathOption("plugin-path", "Changes the path where the plugins are stored", "path");
    QCommandLineOption sharedDataPathOption("shared-data-path", "Changes the path where the shared data is stored", "path");
    libPathOption.setFlags(QCommandLineOption::HiddenFromHelp);
    corePathOption.setFlags(QCommandLineOption::HiddenFromHelp);
    pluginPathOption.setFlags(QCommandLineOption::HiddenFromHelp);
    sharedDataPathOption.setFlags(QCommandLineOption::HiddenFromHelp);
#endif

    QCommandLineOption debugMessagesOption({"d", "debug-messages"}, "Prints debug callback messages to stdout");
    QCommandLineOption fullscreenOption({"f", "fullscreen"}, "Launches ROM in fullscreen mode");
    QCommandLineOption noGuiOption({"n", "nogui"}, "Hides GUI elements (menubar, toolbar, statusbar)");
    QCommandLineOption quitAfterEmulationOption({"q", "quit-after-emulation"}, "Quits RMG when emulation has finished");
    QCommandLineOption loadStateSlot("load-state-slot", "Loads save state slot when launching the ROM", "Slot Number");
    QCommandLineOption diskOption("disk", "64DD Disk to open ROM in combination with", "64DD Disk");

#ifndef PORTABLE_INSTALL
    parser.addOption(libPathOption);
    parser.addOption(corePathOption);
    parser.addOption(pluginPathOption);
    parser.addOption(sharedDataPathOption);
#endif
    parser.addOption(debugMessagesOption);
    parser.addOption(fullscreenOption);
    parser.addOption(noGuiOption);
    parser.addOption(quitAfterEmulationOption);
    parser.addOption(loadStateSlot);
    parser.addOption(diskOption);
    parser.addPositionalArgument("ROM", "ROM to open");

    parser.process(app);

#ifndef PORTABLE_INSTALL
    QString libPathOverride = parser.value(libPathOption);
    QString corePathOveride = parser.value(corePathOption);
    QString pluginPathOverride = parser.value(pluginPathOption);
    QString sharedDataPathOverride = parser.value(sharedDataPathOption);
    if (!libPathOverride.isEmpty()) CoreSetLibraryPathOverride(libPathOverride.toStdString());
    if (!corePathOveride.isEmpty()) CoreSetCorePathOverride(corePathOveride.toStdString());
    if (!pluginPathOverride.isEmpty()) CoreSetPluginPathOverride(pluginPathOverride.toStdString());
    if (!sharedDataPathOverride.isEmpty()) CoreSetSharedDataPathOverride(sharedDataPathOverride.toStdString());
#endif

    CoreSetPrintDebugCallback(parser.isSet(debugMessagesOption));
    QStringList args = parser.positionalArguments();

    CoreAddCallbackMessage(CoreDebugMessageType::Info, "Initializing on " + QGuiApplication::platformName().toStdString());

    if (!window.Init(&app, !parser.isSet(noGuiOption), !args.empty())) {
        return 1;
    }

    if (!args.empty()) {
        bool parsedNumber = false;
        int saveStateSlot = parser.value(loadStateSlot).toInt(&parsedNumber);
        if (parser.value(loadStateSlot).isEmpty() || !parsedNumber || saveStateSlot < 0 || saveStateSlot > 9) {
            saveStateSlot = -1;
        }
        window.OpenROM(args.at(0), parser.value(diskOption), parser.isSet(fullscreenOption), parser.isSet(quitAfterEmulationOption), saveStateSlot);
    }

    window.show();
    return app.exec();
}
