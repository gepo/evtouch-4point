cmake_minimum_required (VERSION 2.8)
project (evtouch_calibrator) 

add_subdirectory(src)

# Debian package
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "evtouch-calibrator")
set(CPACK_PACKAGE_VERSION "1.0")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "xserver-xorg-input-evtouch")
set(DEBIAN_PACKAGE_SECTION "x11")
SET(CPACK_PACKAGE_CONTACT "Gennady Telegin <gepo@neokiosk.ru>")

SET(CPACK_PACKAGING_INSTALL_PREFIX "/usr/")
SET(CPACK_SYSTEM_NAME "lenny")

INCLUDE(CPack)

