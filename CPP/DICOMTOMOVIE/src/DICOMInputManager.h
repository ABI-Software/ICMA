/*******************************************************************************
 *  Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 *  The contents of this file are subject to the Mozilla Public License
 *  Version 1.1 (the "License"); you may not use this file except in
 *  compliance with the License. You may obtain a copy of the License at
 *  http://www.mozilla.org/MPL/
 *
 *  Software distributed under the License is distributed on an "AS IS"
 *  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 *  License for the specific language governing rights and limitations
 *  under the License.
 *
 *  The Original Code is ICMA
 *
 *  The Initial Developer of the Original Code is University of Auckland,
 *  Auckland, New Zealand.
 *  Copyright (C) 2007-2010 by the University of Auckland.
 *  All Rights Reserved.
 *
 *  Contributor(s): Jagir R. Hussan
 *
 *  Alternatively, the contents of this file may be used under the terms of
 *  either the GNU General Public License Version 2 or later (the "GPL"), or
 *  the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 *  in which case the provisions of the GPL or the LGPL are applicable instead
 *  of those above. If you wish to allow use of your version of this file only
 *  under the terms of either the GPL or the LGPL, and not to allow others to
 *  use your version of this file under the terms of the MPL, indicate your
 *  decision by deleting the provisions above and replace them with the notice
 *  and other provisions required by the GPL or the LGPL. If you do not delete
 *  the provisions above, a recipient may use your version of this file under
 *  the terms of any one of the MPL, the GPL or the LGPL.
 *
 * "2014"
 *******************************************************************************/


#ifndef DICOMINPUTMANAGER_H_
#define DICOMINPUTMANAGER_H_

#include "itkGDCMImageIO.h"
#include "itkImageSeriesReader.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkRGBToLuminanceImageFilter.h"
#include "gdcmUIDGenerator.h"
#include "itkImageSeriesWriter.h"
#include "itkImageSeriesReader.h"
#include "itkExtractImageFilter.h"
#include "itkImage.h"
#include "itkRGBPixel.h"
#include "itkOrientImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkResampleImageFilter.h"
#include "itkIdentityTransform.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkCastImageFilter.h"
#include "URLFileDownloader.h"
#include "boost/algorithm/string/predicate.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"
#include "boost/filesystem.hpp"
#include "DCMTKUtils.h"
#include "DICOMProperties.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <sys/stat.h>

/** \class DICOMInputManager
 * \brief A service class that connects to WADO service or reads a DICOM file from the file system
 *
 * DICOM files accessed via a WADO service are stored locally in the instances working directory
 * The file is then processed to determine the last cardiac cycle and the frames are uniformly sampled
 * 19 frames (Maximum frames that CAP client handle (as of Feb 2012) ) are then extracted and stored for
 * processing.
 *
 * The cardiac cycle is computed using the HeartRate and FrameTime values stored in the DICOM header of the
 * input file.
 *
 * The working directory defaults to the directory from which the binary creating the class instance is invoked,
 * this can be overridden by setting the directory option.
 *
 * For files that are accessible to the class instance, no file allocation is done.
 *
 * The services allows for creating DICOM files with relevant header entries from the input file
 * for chosen frames.
 *
 * The public datatype DICOMSliceImageType which is a itk::Image<float,2> is defined and utilised by this class
 * and is returned as a datatype in some of its functions
 */

//Public types
//The pixel type needs to be float to enable FFT etc, which do not work on unsigned char
typedef itk::RGBPixel<unsigned char> PixelType;
typedef itk::Image<PixelType, 3> InputImageType;
typedef itk::Image<PixelType, 3> OutputImageType;
typedef itk::Image<PixelType, 2> DICOMSliceImageType;
typedef itk::Image<short, 3> Image3D; //Normalized Image filter input
typedef itk::RGBToLuminanceImageFilter<InputImageType,Image3D> NormalizedInputCasterType;


typedef itk::ImageSeriesWriter<DICOMSliceImageType, OutputImageType> DICOMWriterType;
typedef itk::ImageFileWriter<DICOMSliceImageType> ImageFileWriterType;

typedef itk::GDCMImageIO ImageIOType;

typedef itk::ExtractImageFilter<InputImageType, DICOMSliceImageType> ExtractImageFilterType;
typedef itk::OrientImageFilter<InputImageType, InputImageType> OrientImageFilterType;
typedef itk::ResampleImageFilter<DICOMSliceImageType, DICOMSliceImageType> ResampleImageFilterType;
typedef itk::IdentityTransform<double, 2> TransformType;
typedef itk::NearestNeighborInterpolateImageFunction<DICOMSliceImageType> InterpolatorType;
typedef itk::ImageSeriesReader<InputImageType> SeriesReaderType;

typedef itk::ResampleImageFilter<Image3D, Image3D> Resample3DImageFilterType;
typedef itk::IdentityTransform<double, 3> Transform3DType;
typedef itk::NearestNeighborInterpolateImageFunction<Image3D> Interpolator3DType;


class DICOMInputManager {

	InputImageType::Pointer dicomImage; //! The input dicom Image
	InputImageType::RegionType inputRegion;
	InputImageType::SizeType size;
	InputImageType::IndexType start;
	SeriesReaderType::DictionaryRawPointer dictionary; //! Pointer to the dicom dictionary of the input dicom file
	std::vector<std::pair<std::string, std::string> > dEntries; //! A vector of the dicom dictionary of the input dicom file
	std::vector<DICOMSliceImageType::Pointer>* frames; //! A vector Image slices of the samples frames from a cardiac cycle
	unsigned int numOfFrames; //Tagkey = 0028|0008
	double heartRate; //Tagkey = 0018|1088
	double frameTime; //Tagkey = 0018|1063
	std::vector<double> rwavevector; //Tagkey = 0018|6060
	std::string seriesUID; //! Series UID (0020, 000e) of the input DICOM file, used for creating frame DICOMS
	std::string studyUID; //! study UID (0020, 000d) of the input DICOM file, used for creating frame DICOMS
	std::string sopiuid; //! sopInstance UID (0008, 0018) of the input DICOM file, used for creating frame DICOMS
	std::string TransducerData;// Transducer data, used with ICMA data

	int NUMCAPFRAMES; //Defaults to 19
	double m_TargetWidth; //The target width of the frame image - defaults to 636 pixels
	double m_TargetHeight; //The target height of the frame image - defaults to 434 pixels
	std::string workingDir; //The working director where WADO stream is stored
	std::string inputFile; //The input file for itk file reader filter
	unsigned int EDframe; //The end diastole frame associated with this view, set by user when dicom is HDI 5000
	unsigned int ECframe; //The end of cardiac cycle frame relative to the EDframe, set by the user when dicom is HDI 5000
	bool cycleset;		// True when the EC and ED frames are set by the user

	std::string userid; //User id to authenticate instance to WADO server
	std::string passwd; //Password to authenticate instance to WADO server
	std::string proxy; //url of the proxy that the instance should use to access the WADO service
	bool initialized;
	std::string myFile; //Name of file were url stream is stored or refers to input file name (Note this is the uncompressed file in RGB space)
	bool iOwnmyFile;
	DICOMProperties* properties;
	std::map<std::string,std::string> dicomHeader;
	bool icmaDicom;
	inline std::string trim(const std::string& s);

	DICOMSliceImageType::Pointer extract2DImageSlice(int plane, int slice);

public:

	DCMTKUtils& dcmtkutil;
	/**
	 * Get the frames of interest from the input DICOM Image
	 * The output vector is deleted when the class instance is deleted. The DICOMSliceImageType::Pointers
	 * in the vector are ITK smart pointers and are deleted if their reference count is 0.
	 * @return A vector of DICOMSliceImageType::Pointer
	 */
	std::vector<DICOMSliceImageType::Pointer>* extractFrames();

	/**
	 * Set the userid to be used for authenticating the instance to the WADO service
	 * @param is std::string
	 */
	inline void setUserid(std::string user) {
		userid = user;
	}

	/**
	 * Set the password to be used for authenticating the instance to the WADO service
	 * @param is std::string
	 */
	inline void setPassword(std::string pass) {
		passwd = pass;
	}

	/**
	 * Set the proxy to be used for accessing the WADO service
	 * @param is std::string
	 */
	inline void setProxy(std::string url) {
		proxy = url;
	}

	/**
	 * Set the target Width (in pixels) of the frame Images
	 * @param is positive double
	 */
	inline void setTargetWidth(double width) {
		m_TargetWidth = width;
	}

	/**
	 * Set the target Height (in pixels) of the frame Images
	 * @param is positive double
	 */
	inline void setTargetHeight(double height) {
		m_TargetHeight = height;
	}

	/**
	 * The length of a cardiac cycle in the number of frames
	 * @return is a positive integer
	 */
	inline int getPulseLength();

	/**
	 * The current working directory of the instance
	 * @return is std::string
	 */
	inline std::string getWorkingDir() {
		return workingDir;
	}

	/**
	 * A vector of the dictionary entries. The entries are stored in tagkey, tagvalue pairs
	 * @return a std::vector of std::string valued std::pair
	 */
	inline std::vector<std::pair<std::string, std::string> > getDictionary() {
		return dEntries;
	}

	/**
	 * The studyiuid associated with the input DICOM
	 * @return a std::string
	 */
	inline std::string getStudyUID() {
		return studyUID;
	}

	/**
	 * The series uid associated with the input DICOM
	 * @return a std::string
	 */
	inline std::string getSeriesUID() {
		return seriesUID;
	}
	/**
	 * Simple constructor that takes a file or a url to a service from which the input DICOM can be
	 * accessed. If the input parameter begins with http, the input is considered as a url and a http
	 * or https request is initiated when Initialize is called.
	 * @param a std::string
	 * @param unsigned int, the number of frames to be extracted (-1) all frames
	 */
	DICOMInputManager(DCMTKUtils& DCMtkutil, std::string filename, int numframes = -1);
	/**
	 * Simple constructor that takes a file or a url to a service from which the input DICOM can be
	 * accessed. If the input parameter begins with http, the input is considered as a url and a http
	 * or https request is initiated when Initialize is called.
	 * The workingdirectory for the instance is specified as the second parameter
	 * @param a std::string, file or uri of interest.
	 * @param a std::string, working directory for the instance for managing streams.
	 * @param unsigned int, the number of frames to be extracted (-1) all frames
	 */
	DICOMInputManager(DCMTKUtils& DCMtkutil,std::string filename, std::string dir,
			int numframes = -1);

	/**
	 * Save a chosen frame (fno) as a DICOM file (filename). The DICOM file has all relavant header information
	 * from the input DICOM file. The filename should be fully qualified if the file needs to be stored in
	 * a desired directory. Default directory corresponds to the current working directory of the binary (which may
	 * be different from the working directory that is specified).
	 * @param a std::string, a fully qualified filename where the frame would be stored
	 * @param a positive integer, a positive integer referring to the frame of interest in the extracted frames
	 * 							  if the frame number is out of bounds an std::exception is thrown
	 * @return a std::string, the SOPUID of the generated DICOM
	 */
	std::string saveFrameToFile(std::string filename, unsigned int fno);

	/**
	 * Save a chosen frame (fno) as a JPEG file (filename). The filename should be fully qualified if the file needs to be stored in
	 * a desired directory. Default directory corresponds to the current working directory of the binary (which may
	 * be different from the working directory that is specified).
	 * @param a std::string, a fully qualified filename where the frame would be stored
	 * @param a positive integer, a positive integer referring to the frame of interest in the extracted frames
	 * 							  if the frame number is out of bounds an std::exception is thrown
	 */
	void saveFrameAsJpeg(std::string filename, unsigned int fno);
	/**
	 * Initialization of the instance downloads the input stream or accesses the input file and
	 * loads the dictionary and extracts the frames
	 * MUST be called to access the class services
	 */
	void Initialize();

	/**
	 * The disk filename of the input DICOM
	 * @return a std::string - the path to the DICOM file instance on disk
	 */
	std::string getDICOMFile();
	/**
	 * CAP frames for CAP segmentation
	 * @return float array of size NUMCAPFRAMES
	 */
	float* getFrames();

	/**
	 * HDI 5000 or related DICOMS which do not have heart rate, cinerate etc encoded
	 * disabling prediction of cycle, the user is required to set the ED and end of cycle frames
	 * @param ed an unsigned integer pointing to the end diastole frame
	 * @param ec an unsigned interger pointing to the end of cardiac cycle frame relative to the ed frame
	 */
	void setEDECframes(unsigned int ed, unsigned int ec);

	/*
	 * Returns the disk name (with path) of deconvoluted file
	 * this file is deleted when the object is destroyed
	 */
	std::string getDecompressedDicomFile();

	//Normalized Image filter input
	Image3D::Pointer getTargetDicomAsImage3D();

	/*
	 * Number of frames extracted by the manager
	 *
	 */
	unsigned int getNumberOfFrames() {
		return NUMCAPFRAMES;
	};

	double getFrameRate(){
		if(frameTime!=0.0)
			return 1000/frameTime;
		else
			return 1000.0/numOfFrames;
	};


	int* getRWaveMarkers();

	/**
	 *  get the Frame where the rwave starts
	 */
	int rWaveFrame();

	/*
	 * Save dicom file attributes as xml file
	 */
	void saveAttributesToFile(std::string filename);

	/*
	 * Returns true if the DICOM file is a ICMA file
	 */
	bool isICMAInstance();

	std::string getTransducerData(){
		return TransducerData;
	};

	std::string getSopInstanceUID(){
		return sopiuid;
	};


	virtual ~DICOMInputManager();
};

#endif /* DICOMINPUTMANAGER_H_ */
