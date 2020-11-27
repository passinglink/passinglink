execute_process(
  COMMAND git rev-parse --short HEAD
  OUTPUT_VARIABLE GIT_COMMIT
  ERROR_QUIET
)

execute_process(
  COMMAND git describe --tags
  OUTPUT_VARIABLE GIT_VERSION
  ERROR_QUIET
)

string(STRIP "${GIT_COMMIT}" GIT_COMMIT)
string(STRIP "${GIT_VERSION}" GIT_VERSION)

set(VERSION "const char* git_commit = \"${GIT_COMMIT}\";\nconst char* git_version = \"${GIT_VERSION}\";")
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/version.cpp)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/version.cpp VERSION_)
else()
    set(VERSION_ "")
endif()

if (NOT "${VERSION}" STREQUAL "${VERSION_}")
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/version.cpp "${VERSION}")
endif()
