cmake_minimum_required (VERSION 3.2)

project(polycube)

set (CMAKE_CXX_STANDARD 11)

set (CMAKE_BUILD_TYPE, Debug)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

include(cmake/GetGitRevisionDescription.cmake)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/nlohmann_json")

# TODO: remove --tags in following commands once we
# have the first annotated tag version

# if this commit matches a tag, use that for the version, otherwise
# append a '+' to it and get the commit info.
git_get_exact_tag(EXTACT_GIT_TAG --abbrev=0 --tags)
if (EXTACT_GIT_TAG)
  set(VERSION ${EXTACT_GIT_TAG})
else ()
  git_describe(GIT_TAG --abbrev=0 --tags)

  get_git_head_revision(GIT_REFSPEC GIT_SHA1 --tags)
  string(SUBSTRING "${GIT_SHA1}" 0 8 GIT_SHA1_SHORT)

  # get branch
  if (GIT_REFSPEC)
    string(REPLACE "/" ";" GIT_REFSPEC_LIST ${GIT_REFSPEC})
    list(GET GIT_REFSPEC_LIST -1 GIT_BRANCH)
  else ()
    set(GIT_BRANCH "NOT_FOUND")
  endif()

  # check if tree is dirty
  git_describe(GIT_TAG_DIRTY --abbrev=0 --tags --dirty=magic0x6457)
  if (GIT_TAG_DIRTY MATCHES ^.*magic0x6457)
    set(IS_DIRTY "-dirty")
  endif ()

  set(VERSION "${GIT_TAG}+ [git: (branch/commit): ${GIT_BRANCH}/${GIT_SHA1_SHORT}${IS_DIRTY}]")
endif ()

message(STATUS "Version is ${VERSION}")

add_subdirectory(src)
