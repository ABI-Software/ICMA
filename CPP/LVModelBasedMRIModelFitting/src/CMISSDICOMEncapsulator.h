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

#ifndef CMISSDICOMENCAPSULATOR_H_
#define CMISSDICOMENCAPSULATOR_H_
#include <vector>
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkJoinSeriesImageFilter.h"

#include "itkGDCMImageIO.h"
#include "itkJPEGImageIO.h"
#include "gdcmUIDGenerator.h"
#include "itkMetaDataObject.h"
#include "tinyxml2.h"
#include "Point3D.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "boost/date_time/time_resolution_traits.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>


/** \class CMISSDICOMEncapsulator
 * \brief A service class that bundles different View frames and cmiss data into a single dicom
 *
 * This class provides the facilities for bundling frames from different views, associated metadata
 * This file needs to be updated with exnode and exelem files when the fitting process is completed
 */

typedef vnl_matrix<double> MatrixType;

class CMISSDICOMEncapsulator {
	typedef itk::RGBPixel<unsigned char> RGBPixelType;
	typedef itk::Image<RGBPixelType, 2> SliceImageType;
	typedef itk::Image<RGBPixelType, 3> OutputImageType;
	//Private types
	typedef itk::JoinSeriesImageFilter<SliceImageType, OutputImageType> JoinSeriesImageFilterType;
	typedef itk::ImageFileWriter<OutputImageType> WriterType;
	typedef itk::ImageFileReader<SliceImageType> ReaderType;
	typedef itk::GDCMImageIO ImageIOType;
	OutputImageType::Pointer image; //Pointer to the 3D dicom image composed for the input views
	std::string tagApplicationName; // Application Name tag 0018,9524 - CMISS
	std::string tagTransducerData; //TransducerData 0018,5010  - Details of view rotation angle, frame count, apex, basel,baser
	std::string modelName;
	std::string fileNamePrefix; //Name of the output dicom file
	std::string workingDirectory; //The directory where the dicom file would be stored
	std::string patientName;
	std::string patientID;
	std::string studyID;
	std::string inputXML;
	std::string targetView;
	std::string sopInstanceUID;
	std::vector<std::string>* views; //The views to be bundled
	std::vector<std::string>* mesh;
	std::vector<int> viewStart;
	std::vector<int> viewEnd;
	Point3D apex;
	Point3D base;
	Point3D avgRvInsert;
	std::vector<Point3D> edbp;
	std::vector<Point3D> esbp;
	double edtime;
	double estime;
	double endcycle;
	std::map<std::string, std::vector<SliceImageType::Pointer> >* myImages;
	std::map<std::string, std::map<double, std::vector<Point3D> > >* meshMarkers;
	std::map<std::string, MatrixType >* planeTransforms;
	std::map<std::string, int>* types;
	tinyxml2::XMLDocument doc;
	/**
	 * Retrieves the frames from each view and creates a 3D image by joining them
	 * in the time/z direction. The resulting image is stored in the member image
	 * @return is an integer, the number of frames that were joined
	 */
	int composeImages();
	void addMARK(tinyxml2::XMLElement* parent, int id, Point3D point);
	void addCoordinate(tinyxml2::XMLElement* parent, Point3D point);
	std::string getSerializedMeshData();
public:
	/**
	 * Simple constructor that takes the target DICOM filename as input
	 * @param is std:string
	 */
	CMISSDICOMEncapsulator(std::string modelName, std::string outputDICOMPrefix);

	/**
	 * Set the Views that need to be bundled into a single DICOM
	 * @param is a std::vector of Pointers to UltrasoundViewManager
	 */
	void setViews(std::vector<std::string>* myViews, std::map<std::string, std::vector<SliceImageType::Pointer> >* images) {
		views = myViews;
		myImages = images;
	}

	void setViewImageBoundary(std::vector<int> start, std::vector<int> end) {
		viewStart = start;
		viewEnd = end;
	}

	void setMesh(std::vector<std::string>* mesh) {
		this->mesh = mesh;
	}

	inline std::string setWorkingDirectory(std::string wDir) {
		boost::filesystem::path dir(wDir);
		workingDirectory = dir.string();
		return workingDirectory;
	}
	/**
	 * Get the directory where the bundled DICOM file will be stored
	 * @return is a std::string
	 */
	inline std::string getWorkingDirectory() {
		return workingDirectory;
	}

	void setStudyInformation(std::string pid, std::string pname, std::string sid) {
		patientID = pid;
		patientName = pname;
		studyID = sid;
	}

	void setMeshMarkers(std::map<std::string, std::map<double, std::vector<Point3D> > >* meshMarkers) {
		this->meshMarkers = meshMarkers;
	}

	void setInputXML(std::string inputXML) {
		this->inputXML = inputXML;
	}

	void setMeshTargetView(std::string tview){
		targetView = tview;
	}

	void setMeshParameters(Point3D Apex, Point3D Base, Point3D rv, std::vector<Point3D> edbase, double edt, double endcycle) {
		apex = Apex;
		base = Base;
		avgRvInsert = rv;
		edbp = edbase;
		edtime = edt;
		this->endcycle = endcycle;
	}

	void setESFrameParameters(std::vector<Point3D> esb, double est) {
		esbp = esb;
		estime = est;
	}

	void setPlaneTransforms(std::map<std::string, MatrixType >* planeTransforms,std::map<std::string, int>* types){
		this->planeTransforms = planeTransforms;
		this->types = types;
	}

	/**
	 * Builds and saves a DICOM file with the header information present in the parameter
	 * The input vector is expected to be a vector of tagkey,tagvalue pairs. Unique sopiuid
	 * and other private tags are added to the composed file.
	 * @param is a std::vector of std::pair of std::string
	 * @return is a std::string
	 */
	std::string buildOutput();
	virtual ~CMISSDICOMEncapsulator();
};

#endif /* CMISSDICOMENCAPSULATOR_H_ */
