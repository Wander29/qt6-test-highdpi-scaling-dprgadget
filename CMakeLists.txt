cmake_minimum_required(VERSION 3.8)

project(
  dprgadget
  LANGUAGES CXX
  DESCRIPTION "Manual test cases for Qt's High-DPI support"
)

find_package( Qt6 COMPONENTS REQUIRED Core Gui Widgets )

add_executable(
  dprgadget
    main.cpp
    README.md
)

target_link_libraries(
  dprgadget
  PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)