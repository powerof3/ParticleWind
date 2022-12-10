vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO martinus/unordered_dense
    REF 39322aeb0ec2ca8c2722000066483d6c72c69136
    SHA512 4d72ec59b3dded80f6ef8e1623803d1347ad854fafb04806a05618bafe3709609329239c80c5f96bb56406a89250c4347f250383b7142b0036b82e62e7bdb3e7
    HEAD_REF main
)

vcpkg_cmake_configure(
	SOURCE_PATH "${SOURCE_PATH}"
	OPTIONS
		-DRH_STANDALONE_PROJECT=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(
	PACKAGE_NAME unordered_dense
	CONFIG_PATH lib/cmake/unordered_dense
)

file(REMOVE_RECURSE
	"${CURRENT_PACKAGES_DIR}/debug"
	"${CURRENT_PACKAGES_DIR}/lib"
)

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
