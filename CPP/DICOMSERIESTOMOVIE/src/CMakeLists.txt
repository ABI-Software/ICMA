# This is the root ITK CMakeLists file.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)


# This project is designed to be built outside the Insight source tree.
PROJECT( DICOMtoMOVIE )

# Find ITK.
FIND_PACKAGE(ITK REQUIRED)
INCLUDE(${ITK_USE_FILE})


FIND_LIBRARY(ARCHIVE_LIBRARY archive)
#message(STATUS "${ARCHIVE_LIBRARY}" )
#BOOST definitions
SET(Boost_USE_STATIC_LIBS   ON)
SET(Boost_USE_MULTITHREADED ON)
FIND_PACKAGE(Boost COMPONENTS thread regex filesystem system iostreams REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})



SET(SRCS 
	main.cxx
	DICOMImage.cpp
	MovieMaker.cpp
	DICOMInputManager.cpp
	DICOMProperties.cpp
	DCMTKUtils.cpp
	URLFileDownloader.cpp
	SegmentationAndFittingException.cpp
	XMLInputReader.cpp
	tinyxml2.cpp
	compress/LibArchiveWrapper.cpp
)

ADD_EXECUTABLE( movie ${SRCS})
TARGET_LINK_LIBRARIES( movie ${ITK_LIBRARIES} ${Boost_LIBRARIES} ${ARCHIVE_LIBRARY} curl z rt)

