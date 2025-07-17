#include <QApplication>
#include <QDebug>
#include "app/database.h"
#include "app/gui/mainwindow.h"
#include "app/utils.h"

int main(int argc, char *argv[])
{
	qInfo() << "qTaggle";
	try
	{
		QCoreApplication::setApplicationName("qTaggle");
		QCoreApplication::setOrganizationName("qTaggle");
		QApplication::setStyle("windowsvista");
		QApplication app(argc, argv);
		//app.setApplicationName("qTaggle");
		MainWindow window;
		window.show();
		return app.exec();
	}
	catch (const std::exception& e)
	{
		qFatal() << e.what();
		delete db;
		return EXIT_FAILURE;
	}
}
