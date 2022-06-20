cmake_minimum_required (VERSION 3.8)

file(GLOB SOURCES *.cpp *.h)
list(REMOVE_ITEM SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/AssemblyInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CSPSPServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Form1.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stdafx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h)

add_executable(CSPSPServer ${SOURCES})
set_target_properties(CSPSPServer PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
if(WIN32)
  target_link_libraries(CSPSPServer wsock32)
else()
  target_link_libraries(CSPSPServer pthread)
endif()