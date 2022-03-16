cmake_minimum_required(VERSION 3.4 FATAL_ERROR)

project(kea_opencv_viewer CXX)

set(CMAKE_CXX_STANDARD 14)

# The install directory of the ToF library
# In this case I extracted it in my Downloads folder 
set(tof_DIR "/home/refael/Downloads/artifacts/tof-internal-linux-x86_64/lib/cmake/tof")
# This is where I built OpenCV, due to my interesting build setup
set(OpenCV_DIR "/home/refael/Documents/support/opencv/build")

find_package(tof REQUIRED)
find_package(OpenMP REQUIRED)
find_package(OpenCV REQUIRED core imgproc highgui)

set(kea_opencv_viewer_srcs src/main.cpp src/colormap.cpp)

include_directories(${OpenCV_INCLUDE_DIRS}) 

add_executable(kea_opencv_viewer ${kea_opencv_viewer_srcs})

target_link_libraries(kea_opencv_viewer PRIVATE tof::tof OpenMP::OpenMP_CXX
                                                ${OpenCV_LIBS})

install(TARGETS kea_opencv_viewer DESTINATION ${CMAKE_INSTALL_BINDIR})
