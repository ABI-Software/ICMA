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

#include "XMLInputReader.h"
#include "URLFileDownloader.h"
#include "ImageFileReader.h"
#include "ThresholdBasedLongAxisBoundaryDetector.h"
#include "ThresholdBasedShortAxisBoundaryDetector.h"
#include "MarkerPlotter.h"
#include "CAP/LVHeartMeshModel.h"
#include "CAP/SinglePlaneLVHeartMeshModel.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "RegionOfInterest.h"
#include "RCMeshFitting.h"
#include "CMISSDICOMEncapsulator.h"
#include "itkRGBPixel.h"
#include "itkImageFileReader.h"

const int dim = 2;
const double cutoffSaxradius = 10.0;
const unsigned int framestep = 3;
typedef unsigned char PixelType;
typedef itk::Image<PixelType, dim> ImageType;
typedef itk::Image<itk::RGBPixel<unsigned char>, dim> RGBImageType;
typedef itk::ImageFileReader<RGBImageType> RGBImageReaderType;

int main(int argc, char* argv[]) {

	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " input.xml [workingdir]" << std::endl;
		return -1;
	}
	//Read input file
	XMLInputReader input(argv[1]);

	//Create working directory
	boost::filesystem::path workingDir;
	boost::filesystem::path wd;
	if (argc > 2) {
		wd = boost::filesystem::path(argv[2]);
	} else {
		wd = boost::filesystem::path(boost::filesystem::current_path());
	}
	if (boost::filesystem::exists(wd)) {
		std::ostringstream fn;
		int ctr = 1;
		fn << "out" << ctr;
		workingDir = wd / fn.str();
		while (boost::filesystem::exists(workingDir)) {
			fn.str("");
			ctr++;
			fn << "out" << ctr;
			workingDir = wd / fn.str();
		}
	} else {
		workingDir = wd / "out0";
	}
	boost::filesystem::create_directories(workingDir);

	std::string selectedLongAxisPlane = input.getSelectedLongAxisPlane();
	double imageHeight = input.getImageHeight(selectedLongAxisPlane);
	double imageWidth = input.getImageWidth(selectedLongAxisPlane);
	std::map<std::string, std::vector<std::string> > imageURI;
	std::map<std::string, std::vector<ImageType::Pointer> > images;
	std::map<std::string, std::vector<std::string> > imagefilenames;
	std::map<std::string, MatrixType> planeTransforms;
	std::vector<std::string> planes = input.getPlaneIds();
	const unsigned int numPlanes = planes.size();

	std::vector<int> viewStart(numPlanes, 0);
	std::vector<int> viewEnd(numPlanes, -1);

	for (int pl = 0; pl < numPlanes; pl++) {
		std::vector<std::string> imageURIs = input.getFrameUrls(planes[pl]);
		//Download the image files
		unsigned int numFiles = imageURIs.size();
		std::vector<ImageType::Pointer> AxisImages(numFiles);
		std::vector<std::string> AxisImageFilenames(numFiles);
		std::string imageDirectory = input.getImageDirectory();
		double edtime = input.getEDTime(planes[pl]);
		double endcycle = input.getEndOfCycleTime(planes[pl]);
		if (edtime < 0.0)
			edtime = 0;
		if (endcycle < 0.0)
			endcycle = 1.0;
		unsigned int startFrame = edtime * (numFiles - 1);
		unsigned int endFrame = endcycle * (numFiles - 1);
		viewStart[pl] = startFrame;
		viewEnd[pl] = endFrame;
		planeTransforms[input.getSeriesId(planes[pl])] = input.getTransform(planes[pl]);
		if (!boost::algorithm::starts_with(imageDirectory, "http")) {
			boost::filesystem::path imageDir(imageDirectory);
			for (int i = 0; i < numFiles; i++) {
				//Load the image
				boost::filesystem::path iurl = imageDir / imageURIs[i];
				std::string filename = iurl.string();
				AxisImageFilenames[i] = filename;
				ImageFileReader<ImageType> reader(filename, imageHeight, imageWidth);
				reader.Update();
				AxisImages[i] = reader.GetOutput();
			}
		} else {
			URLFileDownloader downloadManager(workingDir.string());
			std::ostringstream fn;
			for (int i = 0; i < numFiles; i++) {
				fn << "Axis" << i;
				std::string filename = fn.str();
				fn.str("");
				int retval = downloadManager.loadURLToFile(imageDirectory + imageURIs[i], filename);
				if (retval == 0) {
					//Load the image
					ImageFileReader<ImageType> reader(filename, imageHeight, imageWidth);
					reader.Update();
					AxisImages[i] = reader.GetOutput();
					AxisImageFilenames[i] = filename;
				} else {
					std::cerr << "Failed to download uri " << imageURIs[i] << " curl retval " << retval << std::endl;
					throw -1;
				}
			}
		}
		imageURI[planes[pl]] = imageURIs;
		images[planes[pl]] = AxisImages;
		imagefilenames[planes[pl]] = AxisImageFilenames;
	}

	Point3D imgBase = input.getBase();
	Point3D imgApex = input.getApex();

	std::vector<Point3D> edbaseM = input.getEndDistoleMarkers(selectedLongAxisPlane);
	std::vector<Point3D> esbaseM = input.getEndSystoleMarkers(selectedLongAxisPlane);
	double estime = input.getESTime(selectedLongAxisPlane);
	double edtime = input.getEDTime(selectedLongAxisPlane);
	double endcycle = input.getEndOfCycleTime(selectedLongAxisPlane);

	std::vector<std::string> longAxisURIs = imageURI[selectedLongAxisPlane];
	std::vector<ImageType::Pointer> longAxisImages = images[selectedLongAxisPlane];
	unsigned int numFiles = longAxisURIs.size();
	unsigned int startFrame = (numFiles - 1) * edtime;
	unsigned int endFrame = (numFiles - 1) * endcycle;
	unsigned int totalFrames = (endFrame - startFrame + 1);
	double esframe = estime * totalFrames;

	MatrixType transform = input.getTransform(selectedLongAxisPlane);

	//Setup Long axis boundary detection
	SinglePlaneLVHeartMeshModel mesh(totalFrames, FCH, 8); //maxspeckles per side should be multiple of 4
	mesh.setApex(imgApex);
	mesh.setBaseL(edbaseM[0]);
	mesh.setBaseR(edbaseM[2]);
	std::vector<Point3D> rv;
	rv.push_back(edbaseM[0]);
	mesh.setRVInserts(rv);
	std::vector<Point3D> es;
	es.push_back(esbaseM[0]);
	if (esbaseM.size() > 2)
		es.push_back(esbaseM[2]);
	else
		es.push_back(esbaseM[1]);
	mesh.setESFrameData(estime - edtime, es);
	mesh.AlignModel();

	int apexIdx = 0;

	std::vector<Point3D> planeboundary = mesh.getSpeckleInitializers(0.0, &apexIdx, 1.0);

	//Get the markers
	std::map<std::string, std::map<double, std::vector<Point3D> > > meshMarkers;
	std::map<double, std::string> timepoints;
//#define outputtracking
#ifdef outputtracking
	MarkerPlotter<ImageType> plotter;
#endif
	//Note last frame is not used for fitting so skip it
	for (int i = startFrame; i < endFrame; i += framestep) {
		double time = ((double) i) / endFrame;
		std::vector<Point3D> modelBoundary = mesh.getSpeckleInitializers(time, &apexIdx, 0.0);
		unsigned int endpt = modelBoundary.size() - 1;
		ThresholdBasedLongAxisBoundaryDetector<ImageType> detector(longAxisImages[i], modelBoundary[0], modelBoundary[endpt], modelBoundary[apexIdx]);
		detector.setBoundaryPolygon(planeboundary);
		std::vector<Point3D> pts = detector.getEndoBoundary(modelBoundary, apexIdx);

#ifdef outputtracking
		std::ostringstream fn;
		fn << "Detect" << i << ".jpg";
		wd = workingDir / fn.str();
		plotter.plotToFile(wd.string(), longAxisImages[i], pts);
#endif
		std::vector<Point3D> planepts(pts);
		for (int j = 0; j < pts.size(); j++) {
			planepts[j] = transform * pts[j];
		}
		meshMarkers[selectedLongAxisPlane][time] = planepts;
		timepoints[time] = "";
	}

	for (int i = 0; i < numPlanes; i++) {
		if (input.getPlaneType(planes[i]) == SHORT) {
			std::vector<ImageType::Pointer>& saxImages = images[planes[i]];
			std::vector<std::string>& saxImageURI = imageURI[planes[i]];
			MatrixType stransform = input.getTransform(planes[i]);
			RegionOfInterest<ImageType> roiFilter(saxImages);
			std::vector<double> loc = roiFilter.getROIBoundingBox(cutoffSaxradius);
			double radius = std::min(std::max(loc[2] * 1.3, 10.0), 40.0);
			Point3D laxpoi(loc[0], loc[1], 0);
			unsigned int saxStartFrame = viewStart[i];
			unsigned int saxEndFrame = viewEnd[i];

			//Note last frame is not used for fitting so skip it
			for (int j = saxStartFrame; j < saxEndFrame; j += framestep) {
				double time = ((double) (j - saxStartFrame)) / saxEndFrame;
				double meanRadius = 0;
				ThresholdBasedShortAxisBoundaryDetector<ImageType> saxDetector(saxImages[j], laxpoi);
				std::vector<Point3D> sxp = saxDetector.getEndoBoundary(meanRadius, cutoffSaxradius, radius, radius);
				if (meanRadius > cutoffSaxradius) { //Radius below the cutoff radius may be due to edgedetection errors
#ifdef outputtracking
						sxp.push_back(laxpoi);
						std::ostringstream fn;
						fn << "SaxDetect" << planes[i] << "_" << j << ".jpg";
						wd = workingDir / fn.str();
						plotter.plotToFile(wd.string(), saxImages[j], sxp);
						sxp.pop_back();
#endif
					std::vector<Point3D> planepts(sxp);
					for (int k = 0; k < sxp.size(); k++) {
						planepts[k] = stransform * sxp[k];
					}
					meshMarkers[planes[i]][time] = planepts;
					timepoints[time] = "";
				}
			}
		}
	}

	//Setup the LV mesh model

	MatrixType itransform = vnl_matrix_inverse<double>(transform);
	Point3D base = transform * imgBase;
	Point3D apex = transform * imgApex;
	std::vector<Point3D> edbase(2);
	std::vector<Point3D> esbase(2);
	edbase[0] = transform * edbaseM[0];
	edbase[1] = transform * edbaseM[2];
	esbase[0] = transform * esbaseM[0];
	if (esbaseM.size() > 2)
		esbase[1] = transform * esbaseM[2];
	else
		esbase[1] = transform * esbaseM[1];

	LVHeartMeshModel lvmesh(totalFrames * 10); //maxspeckles per side should be multiple of 4
	lvmesh.setApex(apex);
	lvmesh.setBase(base);
	std::vector<Point3D> lvrv;
	std::string selectedShortAxisPlane = input.getSelectedShortAxisPlane();
	if (selectedShortAxisPlane != "") {
		lvrv.push_back(input.getTransform(selectedShortAxisPlane) * input.getAvgRvInsert());
	} else {
		lvrv.push_back(transform * input.getAvgRvInsert());
	}
	lvmesh.setRVInserts(lvrv);
	//Setup baseplane information
	lvmesh.setEDFrameData(edbase);
	lvmesh.setESFrameData(estime - edtime, esbase);
	lvmesh.AlignModel();

	//Get mesh coordinates at select points and output
	RCMeshFitting rcmesh;
	std::vector<std::string> fitMesh;
	for (int i = 0; i < 19; i++) {
		double time = ((double) i) / 18.0;
		std::vector<Point3D> mvp = lvmesh.getRefinedMeshCoordinates(time);
		std::string mesh = rcmesh.getOptimizedMesh(mvp);
		//Change the region to heart as RC mesh operates in root
		boost::replace_all(mesh, "Region: /", "Region: /heart");
		fitMesh.push_back(mesh);
	}


	std::map<std::string, std::vector<RGBImageType::Pointer> > rgbImages;
	RGBImageReaderType::Pointer rgbreader = RGBImageReaderType::New();
	//Load the image files
	for (int i = 0; i < numPlanes; i++) {
		std::vector<std::string>& files = imagefilenames[planes[i]];
		std::vector<RGBImageType::Pointer> images;
		for (int k = 0; k < files.size(); k++) {
			rgbreader->SetFileName(files[i]);
			rgbreader->Update();
			images.push_back(rgbreader->GetOutput());
			images[k]->DisconnectPipeline();
		}
		rgbImages[planes[i]] = images;
	}
	boost::filesystem::path outputDirectory;
	if (input.getOutputDirectory() != "NOT FOUND") {
		outputDirectory = boost::filesystem::path(input.getOutputDirectory());
		if (!boost::filesystem::exists(outputDirectory))
			boost::filesystem::create_directories(outputDirectory);
	} else {
		outputDirectory = boost::filesystem::current_path();
	}

	std::map<std::string, int> seriesTypeMap = input.getSeriesType();
	CMISSDICOMEncapsulator encaps(input.getModelName(), "MRIFIT");
	encaps.setInputXML(input.getXMLString());
	encaps.setMeshParameters(apex, base, lvrv[0], edbase, edtime, endcycle);
	encaps.setESFrameParameters(esbase, estime);
	encaps.setPlaneTransforms(&planeTransforms, &seriesTypeMap);
	encaps.setMeshTargetView(input.getSeriesId(selectedLongAxisPlane));
#ifndef testMesh
	encaps.setMeshMarkers(&meshMarkers);
#endif
	encaps.setMesh(&fitMesh);
	encaps.setViews(&planes, &rgbImages);
	encaps.setViewImageBoundary(viewStart, viewEnd);
	encaps.setWorkingDirectory(outputDirectory.string());
	encaps.buildOutput();

	//Cleanup
	boost::filesystem::remove_all(workingDir);
	return 0;
}

