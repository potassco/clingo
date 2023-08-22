add_library(spdlog "src/spdlog/spdlog.cc")
target_include_directories(spdlog PUBLIC "include")

add_library(spdlog::spdlog ALIAS spdlog)
