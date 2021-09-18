#===============================================================================
#
# Copyright (C) 2020 Istituto Italiano di Tecnologia (IIT)
#
# This software may be modified and distributed under the terms of the
# GPL-2+ license. See the accompanying LICENSE file for details.
#
#===============================================================================
cmake_minimum_required(VERSION 3.5)
project(contour_following)
# Add standard uninstall target path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(GazeboYARPPlugins REQUIRED)

find_package(gazebo REQUIRED)

find_package(ICUB REQUIRED COMPONENTS ctrlLib iKin)

find_package(YARP COMPONENTS os sig math dev REQUIRED)

# add uninstall target path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
add_executable(${PROJECT_NAME} ${PROJECT_NAME}.cpp )

target_link_libraries(${PROJECT_NAME} PRIVATE YARP::YARP_os
                                              YARP::YARP_init
                                              YARP::YARP_dev
                                              YARP::YARP_math
                                              YARP::YARP_sig
                                              GazeboYARPPlugins::gazebo_yarp_singleton
                                              ICUB::skinDynLib
                                              ICUB::iKin
                                              ${GAZEBO_LIBRARIES})


                                                     
target_include_directories(${PROJECT_NAME}
PRIVATE
${GAZEBO_INCLUDE_DIRS}
)                                       

# Add standard uninstall target
include(AddUninstallTarget)