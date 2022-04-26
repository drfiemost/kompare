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


kde4_add_library(komparediff2 SHARED ${komparediff2_SRCS})

target_link_libraries(komparediff2 ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS})

set_target_properties(komparediff2 PROPERTIES VERSION ${GENERIC_LIB_VERSION}
SOVERSION ${GENERIC_LIB_SOVERSION} )

install(TARGETS komparediff2 EXPORT LibKompareDiff2Targets ${INSTALL_TARGETS_DEFAULT_ARGS})

target_link_libraries(komparediff2 ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS})

add_subdirectory(tests)
