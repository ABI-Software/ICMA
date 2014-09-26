# This is the root ITK CMakeLists file.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)


# This project is designed to be built outside the Insight source tree.
PROJECT( MRIModelStrainMeasures )

IF(WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ELSE(WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ENDIF(WIN32)


#SET(Cmgui_DIR "${sandbox}/cmiss/local/lib/cmake")

# Find GDCM - force it to look into cmgui version else will lead to name clashes
SET(GDCM_DIR ${Cmgui_DIR})
FIND_PACKAGE(GDCM)
INCLUDE_DIRECTORIES(${GDCM_INCLUDE_DIRS})


#BOOST definitions
SET(Boost_USE_STATIC_LIBS   ON)
SET(Boost_USE_MULTITHREADED ON)
SET(Boost_ADDITIONAL_VERSIONS "1.44" "1.44.0")
FIND_PACKAGE(Boost COMPONENTS  date_time filesystem system iostreams REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

# We need to find Cmgui.  We can pass Cmgui_DIR via the command line
# otherwise we can set it via a CMake configuration gui application
# if Cmgui is not found on the standard system search path.


FIND_PACKAGE(Cmgui)

# Add the Cmgui include dirs to this scope.  
INCLUDE_DIRECTORIES(${CMGUI_INCLUDE_DIRS})

SET(SRCS 
	main.cxx
	tinyxml2.cpp
	LVChamberCircle.cpp
	LVChamberRadius.cpp
	StrainMeasures.cpp
	LVMyocardialVolume.cpp
	alglib/alglibinternal.cpp
	alglib/alglibmisc.cpp
	alglib/ap.cpp
	alglib/integration.cpp
	alglib/interpolation.cpp
	alglib/linalg.cpp
	alglib/optimization.cpp
	alglib/solvers.cpp
	alglib/specialfunctions.cpp
	alglib/statistics.cpp
)

ADD_EXECUTABLE( MRIModelStrainMeasures ${SRCS})
TARGET_LINK_LIBRARIES( MRIModelStrainMeasures ${Boost_LIBRARIES} ${CMGUI_LIBRARIES} ${GDCM__LIBRARIES})
