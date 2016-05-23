##from http://www.cmake.org/pipermail/cmake/2008-September/023808.html
#i.e. this is bad: SET(CMAKE_BUILD_TYPE "Release")
# If the user specifies -DCMAKE_BUILD_TYPE on the command line, take their definition
# and dump it in the cache along with proper documentation, otherwise set CMAKE_BUILD_TYPE
# to Debug prior to calling PROJECT()

#${ITK_VERSION_MAJOR}.${ITK_VERSION_MINOR}.${ITK_VERSION_PATCH} 

## bash command to check if CMakeLists.txt contains an entry for all *.cxx: 
## for i in *cxx ; do if ! `grep -q ${i%.cxx} CMakeLists.txt`; then echo "$i not found"'!'; fi; done


IF(DEFINED CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
   SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()


IF(DEFINED CMAKE_INSTALL_PREFIX)
   SET(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE STRING "Choose the install directory.")
ELSE()
   SET(CMAKE_INSTALL_PREFIX /opt/VTK-CLIs/ CACHE STRING "Choose the install directory.")
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




PROJECT(VTK-CLIs)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


SET(Required_Packages
  VTK
)


FIND_PACKAGE (VTK REQUIRED)
IF ( VTK_FOUND )
      INCLUDE( ${USE_VTK_FILE} )
ENDIF( VTK_FOUND )

#Boost
FIND_PACKAGE(Boost COMPONENTS graph)

INCLUDE_DIRECTORIES(
  # $ENV{HOME}/vtk/
  )

SET(Libraries
  ${VTK_LIBRARIES}
)

SET(vtkSplineDrivenImageSlicer "filter/external/vtkSplineDrivenImageSlicer/Filters/")

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

OPTION(BUILD_vtkPolyDataToGraph "Build programs depending on vtkPolyDataToGraph from David Doria (not in VTK)" ON)
IF(BUILD_vtkPolyDataToGraph)
 SET(VTKGraphConversionsDir "filter/external/VTK-GraphConversions/")
 INCLUDE_DIRECTORIES(${VTKGraphConversionsDir})
# ADD_SUBDIRECTORY(filters/external/VTK-GraphConversions/)
ENDIF()


SET(CurrentExe "marching-cubes")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "marching-cubes_MPI")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "discrete_marching-cubes")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe} "filters_mod/VTK/Filters/General/vtkDiscreteMarchingCubes.cxx")
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "lego_surf")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "discrete_marching-cubes_MPI")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe} "filters_mod/VTK/Filters/General/vtkDiscreteMarchingCubes.cxx")
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "probe-surf2vrml")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries} "-lglut -lGL")
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "probe-surf2x3d")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries} "-lglut -lGL")
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "pvtp2vtp")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "vtp2pvtp")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

IF(BUILD_vtkPolyDataToGraph)
SET(CurrentExe "label_biconnected_components")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe} ${VTKGraphConversionsDir}/vtkPolyDataToGraph.cxx)
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)
ENDIF()

SET(CurrentExe "shortest-path_dij")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "hull")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "image-clipper")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "analyse_surface")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "analyse_S+V")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "DumpXMLFile")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "threshold")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "read_VTPs_SDP")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "smooth-ws_SDP")
ADD_EXECUTABLE(${CurrentExe}  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "read_VTIs_SDP")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "ribbon_FrenetSerret")
ADD_EXECUTABLE(${CurrentExe}  ${CurrentExe} "${vtkSplineDrivenImageSlicer}/vtkFrenetSerretFrame.cxx")
TARGET_INCLUDE_DIRECTORIES(${CurrentExe} PRIVATE "${vtkSplineDrivenImageSlicer}")
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "vtp2stl")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "vtp2x3d")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "vtp2multi-piece_vtp")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "dice")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "d3")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

IF(Boost_FOUND)
SET(CurrentExe "dot2vtp")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries} ${Boost_LIBRARIES}) # https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=646207  http://stackoverflow.com/questions/6646405/how-do-you-add-boost-libraries-in-cmakelists-txt
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)
ENDIF()

IF(BUILD_vtkPolyDataToGraph)
SET(CurrentExe "vtp2dot")
ADD_EXECUTABLE(${CurrentExe}
  ${CurrentExe} ${VTKGraphConversionsDir}/vtkPolyDataToGraph.cxx)
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)
ENDIF()

SET(CurrentExe "vtk2vtp")
ADD_EXECUTABLE(${CurrentExe}  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "decimate-QC")
ADD_EXECUTABLE(${CurrentExe}  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "largest_mesh-part")
ADD_EXECUTABLE(${CurrentExe}  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)
