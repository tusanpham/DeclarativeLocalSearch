cmake_minimum_required(VERSION 2.8)

include_directories ("${PROJECT_SOURCE_DIR}/")

add_library(linenoise linenoise.c utf8.c)

# add the install targets
install (TARGETS linenoise DESTINATION lib)
