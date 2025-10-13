add_compile_definitions(SQLITE_ENABLE_FTS5)
add_library(sqlite3 STATIC ${PROJECT_SOURCE_DIR}/lib/sqlite3/sqlite3.c)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR}
	COMPONENTS
		Core
		Gui
		Sql
		Widgets
)
qt_standard_project_setup()

set(PROJECT_SOURCES
	gui/dialog/editfiledialog.cpp
	gui/dialog/editfiledialog.h
	gui/dialog/editfiledialog.ui
	gui/dialog/editfiledialogmulti.cpp
	gui/dialog/editfiledialogmulti.h
	gui/dialog/editfiledialogmulti.ui
	gui/dialog/edittagdialog.cpp
	gui/dialog/edittagdialog.h
	gui/dialog/edittagdialog.ui
	gui/dialog/edittagdialogmulti.cpp
	gui/dialog/edittagdialogmulti.h
	gui/dialog/edittagdialogmulti.ui
	gui/dialog/newfiledialog.cpp
	gui/dialog/newfiledialog.h
	gui/dialog/newfiledialog.ui
	gui/dialog/newtagdialog.cpp
	gui/dialog/newtagdialog.h
	gui/dialog/newtagdialog.ui
	gui/dialog/settingsdialog.cpp
	gui/dialog/settingsdialog.h
	gui/dialog/settingsdialog.ui
	gui/docked/filepreview.cpp
	gui/docked/filepreview.h
	gui/docked/filters.cpp
	gui/docked/filters.h
	gui/docked/properties.cpp
	gui/docked/properties.h
	gui/helper/paginator.cpp
	gui/helper/paginator.h
	gui/helper/paginator.ui
	gui/helper/plaintextlistedit.cpp
	gui/helper/plaintextlistedit.h
	gui/helper/taglineedit.cpp
	gui/helper/taglineedit.h
	gui/helper/tagselect.cpp
	gui/helper/tagselect.h
	gui/helper/tagselect.ui
	gui/model/filetablemodel.cpp
	gui/model/filetablemodel.h
	gui/model/tagtablemodel.cpp
	gui/model/tagtablemodel.h
	gui/filelist.cpp
	gui/filelist.h
	gui/filelist.ui
	gui/fileproperties.cpp
	gui/fileproperties.h
	gui/fileproperties.ui
	gui/mainwindow.cpp
	gui/mainwindow.h
	gui/mainwindow.ui
	gui/taglist.cpp
	gui/taglist.h
	gui/taglist.ui
	gui/tagproperties.cpp
	gui/tagproperties.h
	gui/tagproperties.ui
	database.cpp
	database.h
	#database.test.cpp
	#database.test.h
	error.cpp
	error.h
	file.cpp
	file.h
	filetag.cpp
	filetag.h
	main.cpp
	tag.cpp
	tag.h
	utils.cpp
	utils.h
)
qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

set_target_properties(${PROJECT_NAME}
	PROPERTIES
		WIN32_EXECUTABLE TRUE
)

target_link_libraries(${PROJECT_NAME} PUBLIC sqlite3)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/lib/sqlite3)

target_link_libraries(${PROJECT_NAME}
	PUBLIC
		Qt::Core
		Qt::Gui
		Qt::Sql
		Qt::Widgets
)
