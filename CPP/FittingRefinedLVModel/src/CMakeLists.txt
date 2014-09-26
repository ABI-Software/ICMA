# This is the root ITK CMakeLists file.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

PROJECT( RefinedFitting)

#BOOST definitions
SET(Boost_USE_STATIC_LIBS   ON)
SET(Boost_USE_MULTITHREADED ON)
SET(Boost_ADDITIONAL_VERSIONS "1.44" "1.44.0")
FIND_PACKAGE(Boost COMPONENTS thread date_time program_options filesystem system iostreams REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

FIND_PACKAGE(Threads REQUIRED)

IF(WIN32 AND __COMPILER_GNU)
        # mingw-gcc fails to link boost::thread
        ADD_DEFINITIONS(-DBOOST_THREAD_USE_LIB)
ENDIF(WIN32 AND __COMPILER_GNU)    

#SET(Cmgui_DIR "${sandbox}/cmiss/local/lib/cmake")

# Find GDCM - force it to look into cmgui version else will lead to name clashes
SET(GDCM_DIR ${Cmgui_DIR})
FIND_PACKAGE(GDCM)
INCLUDE_DIRECTORIES(${GDCM_INCLUDE_DIRS})

# We need to find Cmgui.  We can pass Cmgui_DIR via the command line
# otherwise we can set it via a CMake configuration gui application
# if Cmgui is not found on the standard system search path.


FIND_PACKAGE(Cmgui)

# We have to add the definitions from Cmgui, this is 
# absolutely necessary when using wxWidgets.
FOREACH(DEF ${CMGUI_DEFINITIONS})
	ADD_DEFINITIONS(-D${DEF})
ENDFOREACH(DEF ${CMGUI_DEFINITIONS})

# Add the Cmgui include dirs to this scope.  
INCLUDE_DIRECTORIES(${CMGUI_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMISS_ROOT}/itk/${LIB_ARCH_DIR}/include/InsightToolkit/Utilities/vxl/vcl)

SET(SRCS 
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
	tinyxml2.cpp
	LVChamberCircle.cpp
	XMLInputReader.cpp
	XMLSerialiser.cpp
	LongAxisFitting.cpp
	ShortAxisFitting.cpp
	TorsionFitting.cpp
	VolumeRegularizer.cpp
	LVHeartMesh.cpp
	StrainMeasures.cpp
	BundleToDICOMException.cpp
	SAXBasedCoordinateTransformer.cpp
	NormalLVMesh.cpp
	main.cpp
)


ADD_EXECUTABLE( RefinedFitting ${SRCS})

TARGET_LINK_LIBRARIES( RefinedFitting ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} ${CMGUI_LIBRARIES}  ${GDCM__LIBRARIES})
