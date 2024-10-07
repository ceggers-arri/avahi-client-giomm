# openSUSE 15.0 contains cmake-3.10
cmake_minimum_required(VERSION 3.10)

project(avahi-client-giomm VERSION 1.0.0 LANGUAGES CXX)

# pkg-config is required for giomm-2.4
# https://stackoverflow.com/questions/44487053/set-pkg-config-path-in-cmake
find_package(PkgConfig
	REQUIRED)

# libavahi-common has no own .pc file, so we emulate it here
find_library(AVAHI_COMMMON avahi-common)
add_library(PkgConfig::AVAHI_COMMON STATIC IMPORTED)
set_property(TARGET PkgConfig::AVAHI_COMMON PROPERTY
             IMPORTED_LOCATION "${AVAHI_COMMMON}")

# giomm-2.4
pkg_check_modules(GIOMM_2_4
	REQUIRED
	IMPORTED_TARGET
	giomm-2.4)


#
# avahi-lib
#

add_library(
	${PROJECT_NAME}
	impl/Client.cpp
	impl/EntryGroup.cpp
	impl/RecordBrowser.cpp
	impl/ServiceBrowser.cpp
	impl/ServiceResolver.cpp
)

target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
	GIOMM_DISABLE_DEPRECATED
	GLIBMM_DISABLE_DEPRECATED
)

target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
	PkgConfig::GIOMM_2_4
	PkgConfig::AVAHI_COMMON
)

add_subdirectory(examples)
