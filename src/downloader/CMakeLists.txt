add_sources(springlobbySrc
	prdownloader.cpp
	downloadlistctrl.cpp
	downloadsobserver.cpp
)
	
#currently unused
clear(filelistSrc)
add_sources( filelistSrc
	filelister/filelistctrl.cpp
	filelister/filelistdialog.cpp
	filelister/filelistfilter.cpp 
)

set(PRD_CONSOLETOOL OFF CACHE BOOL "disable console tool compile" FORCE)
set(PRD_SHAREDLIB OFF CACHE BOOL "disable shared lib compile" FORCE)
set(PRD_DEVELOP_FILES OFF CACHE BOOL "disable develop files" FORCE)
set(PRD_DO_INSTALL OFF CACHE BOOL "don't install any pr-downloader file" FORCE)
set(PRD_ARCHIVE_SUPPORT ON CACHE BOOL "disable archive support as it interferes with unitsync!" FORCE)
mark_as_advanced(FORCE CONSOLETOOL SHAREDLIB DEVELOP_FILES DO_INSTALL)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/CMakeLists.txt")
	MESSAGE(FATAL_ERROR "lib/CMakeLists.txt doesn't exist, please run \ngit submodule update --init")
else()
	Add_Subdirectory(lib)
endif()

