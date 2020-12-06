execute_process(
  COMMAND git rev-parse --short HEAD
  OUTPUT_VARIABLE GIT_COMMIT
  ERROR_QUIET
)

execute_process(
  COMMAND git branch --show-current
  OUTPUT_VARIABLE GIT_BRANCH
  ERROR_QUIET
)

execute_process(
  COMMAND git describe --tags
  OUTPUT_VARIABLE GIT_VERSION
  ERROR_QUIET
)

string(STRIP "${GIT_COMMIT}" GIT_COMMIT)
string(STRIP "${GIT_BRANCH}" GIT_BRANCH)
string(STRIP "${GIT_VERSION}" GIT_VERSION)

set(
  VERSION "\
  const char* git_commit = \"${GIT_COMMIT}\";\n\
  const char* git_branch = \"${GIT_BRANCH}\";\n\
  const char* git_version = \"${GIT_VERSION}\";\n\
  const char* version_string() {\n\
    if constexpr (sizeof(\"${GIT_VERSION}\") <= 1) {\n\
      return \"${GIT_BRANCH}-${GIT_COMMIT}\";\n\
    } else {\n\
      return git_version;\n\
    }\n\
  }\n\
")

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/version.cpp)
  file(READ ${CMAKE_CURRENT_SOURCE_DIR}/version.cpp VERSION_)
else()
  set(VERSION_ "")
endif()

if (NOT "${VERSION}" STREQUAL "${VERSION_}")
  file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/version.cpp "${VERSION}")
endif()
