# Find all dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(libgit2 REQUIRED IMPORTED_TARGET libgit2)

find_package(nlohmann_json 3.10.5 REQUIRED)
find_package(inja 3.4.0 REQUIRED)
find_package(xtypes_generator 0.0.1 REQUIRED)
pkg_check_modules(cpr REQUIRED IMPORTED_TARGET cpr)
