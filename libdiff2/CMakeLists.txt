set(CMAKE_AUTOMOC ON)

set(komparediff2_SRCS
	kompareprocess.cpp 
	komparemodellist.cpp 
	diffmodellist.cpp 
	diffmodel.cpp 
	difference.cpp 
	diffhunk.cpp 
	diffsettings.cpp
	settingsbase.cpp
	parser.cpp 
	parserbase.cpp 
	cvsdiffparser.cpp 
	diffparser.cpp 
	perforceparser.cpp 
	stringlistpair.cpp)


add_library(libdiff2 OBJECT ${komparediff2_SRCS})
set_property(TARGET libdiff2 PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(libdiff2 ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS})

add_subdirectory(tests)
