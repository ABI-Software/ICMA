# This is the root ITK CMakeLists file.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)


# This project is designed to be built outside the Insight source tree.
PROJECT( LVModelBasedSpeckleTracking )

IF(WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ELSE(WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
ENDIF(WIN32)

# Find ITK.
FIND_PACKAGE(ITK REQUIRED)
INCLUDE(${ITK_USE_FILE})

#BOOST definitions
SET(Boost_USE_STATIC_LIBS   ON)
SET(Boost_USE_MULTITHREADED ON)
SET(Boost_ADDITIONAL_VERSIONS "1.44" "1.44.0")
FIND_PACKAGE(Boost COMPONENTS thread date_time program_options filesystem system REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

# We need to find Cmgui.  We can pass Cmgui_DIR via the command line
# otherwise we can set it via a CMake configuration gui application
# if Cmgui is not found on the standard system search path.

#SET(Cmgui_DIR "${sandbox}/cmissnoitk/local/lib/cmake")
FIND_PACKAGE(Cmgui)



# Add the Cmgui include dirs to this scope.  
INCLUDE_DIRECTORIES(${CMGUI_INCLUDE_DIRS})


SET(SRCS 
	main.cxx
	LVHeartMeshModel.cpp
	filesystem.cpp
	timesmoother.cpp
	SpeckleLibrary.cpp
	MotionEstimator.cpp
	SpeckleDetector.cpp
	SegmentationAndFittingException.cpp
	DICOMInputManager.cpp
	DCMTKUtils.cpp
	URLFileDownloader.cpp
	MarkerPlotter.cpp
	CMISSDICOMEncapsulator.cpp
	TimeVaryingSmoother.cpp
	XMLInputReader.cpp
	XMLSerialiser.cpp
	tinyxml2.cpp
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

ADD_EXECUTABLE( Speckle ${SRCS})
#message(STATUS "=== ${ITK_LIBRARIES}")
TARGET_LINK_LIBRARIES( Speckle ${Boost_LIBRARIES} ${ITK_LIBRARIES} ${CMGUI_LIBRARIES} curl)
