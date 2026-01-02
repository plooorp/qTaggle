#include <QApplication>
#include "app/database.h"
#include "app/gui/mainwindow.h"
#include "app/globals.h"

int main(int argc, char *argv[])
{
	qInfo() << "qTaggle";
	try
	{
		QCoreApplication::setApplicationName(u"qTaggle"_s);
		QCoreApplication::setOrganizationName(u"qTaggle"_s);
		QApplication::setStyle(u"windowsvista"_s);
		QApplication app(argc, argv);
		MainWindow window;
		window.show();
		return app.exec();
	}
	catch (const std::exception& e)
	{
		qCritical() << e.what();
		delete db;
		return EXIT_FAILURE;
	}
}
