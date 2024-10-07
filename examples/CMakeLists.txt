# reconfirm depends on libavahi-core
pkg_check_modules(AVAHI_CORE
	REQUIRED
	IMPORTED_TARGET
	avahi-core)

add_executable(RecordBrowser RecordBrowser.cpp)
target_link_libraries(RecordBrowser ${PROJECT_NAME})

add_executable(Reconfirm Reconfirm.cpp)
target_link_libraries(Reconfirm ${PROJECT_NAME} PkgConfig::AVAHI_CORE)
