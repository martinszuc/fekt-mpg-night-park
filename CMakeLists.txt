cmake_minimum_required(VERSION 3.10)
project(mpg_projekt)
set(CMAKE_CXX_STANDARD 17)

add_executable(mpg_projekt xszucm00.cpp)

if(APPLE)
    add_definitions(-DGL_SILENCE_DEPRECATION)
    target_link_libraries(mpg_projekt "-framework OpenGL" "-framework GLUT")
elseif(WIN32)
    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)
    target_link_libraries(mpg_projekt OpenGL::GL GLUT::GLUT)
    # copy freeglut.dll next to the binary so it runs without PATH setup
    add_custom_command(TARGET mpg_projekt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:mpg_projekt>
            $<TARGET_FILE_DIR:mpg_projekt>
        COMMAND_EXPAND_LISTS)
else()
    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)
    target_link_libraries(mpg_projekt OpenGL::GL GLUT::glut)
endif()
