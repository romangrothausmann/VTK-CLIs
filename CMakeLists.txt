##from http://www.cmake.org/pipermail/cmake/2008-September/023808.html
#i.e. this is bad: SET(CMAKE_BUILD_TYPE "Release")
# If the user specifies -DCMAKE_BUILD_TYPE on the command line, take their definition
# and dump it in the cache along with proper documentation, otherwise set CMAKE_BUILD_TYPE
# to Debug prior to calling PROJECT()

#${ITK_VERSION_MAJOR}.${ITK_VERSION_MINOR}.${ITK_VERSION_PATCH} 


IF(DEFINED CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
   SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()


IF(DEFINED CMAKE_INSTALL_PREFIX)
   SET(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE STRING "Choose the install directory.")
ELSE()
   SET(CMAKE_INSTALL_PREFIX /opt/vtk-simple/ CACHE STRING "Choose the install directory.")
ENDIF()

# IF(DEFINED ITK_DIR)
#    SET(ITK_DIR ${ITK_DIR})
# ELSE()
#    SET(ITK_DIR /opt/itk-4.6.1/lib/cmake/ITK-4.6/)
# ENDIF()

##-Werror for clean programming ;-)
# SET(CMAKE_CXX_FLAGS "") #dynamic linking, comment out for static linking
# IF(DEFINED CMAKE_CXX_FLAGS)
#    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror" CACHE STRING "NOT compiled with static libc, libgcc and libstdc++")
# ELSE()
#    SET(CMAKE_CXX_FLAGS "-static -static-libgcc -static-libstdc++ -Werror" CACHE STRING "compile with static libc, libgcc and libstdc++")
# ENDIF()




PROJECT(simple_VTK_programs)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


SET(Required_Packages
  VTK
)


FIND_PACKAGE (VTK REQUIRED)
IF ( VTK_FOUND )
      INCLUDE( ${USE_VTK_FILE} )
ENDIF( VTK_FOUND )

if (TARGET vtkParallelMPI)
  set(VTK_USE_MPI TRUE)
endif()

IF (NOT VTK_USE_MPI)
  MESSAGE(STATUS
    "VTK was not compiled with MPI enabled.\n"
    "Please recompile VTK with VTK_Group_MPI cmake variables turned on.")
ELSE (NOT VTK_USE_MPI)
  MESSAGE(STATUS "MPI Enabled in VTK.")

  FIND_PACKAGE (MPI)

  # if not found specify:
  # MPI_EXTRA_LIBRARY                /usr/lib/libmpi_cxx.so
  # MPI_LIBRARY                      /usr/lib/libmpi.so
ENDIF (NOT VTK_USE_MPI)


INCLUDE_DIRECTORIES(
  # $ENV{HOME}/vtk/
  )

SET(Libraries
  ${VTK_LIBRARIES}
)


SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


SET(CurrentExe "discrete_marching-cubes")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "discrete_marching-cubes_MPI")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries} ${MPI_LIBRARY} ${MPI_EXTRA_LIBRARY} )
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)
