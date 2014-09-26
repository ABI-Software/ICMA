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


#ifndef LVMESHBASEDSPECKLETRACKING_H_
#define LVMESHBASEDSPECKLETRACKING_H_
#include "DICOMInputManager.h"
#include "LVHeartMeshModel.h"
#include "MotionEstimator.h"
#include "SpeckleLibrary.h"
#include "SpeckleDetector.h"
#include "MarkerPlotter.h"
#include "Point3D.h"
#include "TimeVaryingSmoother.h"
#include "alglib/interpolation.h"

extern int sw;
extern int sh;
extern int MAXSPECKLES;
extern int maxcapframes;
//#define spatialsmoothing

//#define outputSpeckleTrackingImages

#ifdef outputSpeckleTrackingImages
static int numspeckleCalls = 0;
#endif


typedef itk::Array2D<double> DoubleMatrix;
typedef std::vector<Point3D> speckle;

speckle getSpeckle(std::vector<double*>& coords) {
	unsigned int size = coords.size();
	if (size > 0) {
		speckle result(size - 1); //Since the first coord is null
		for (int i = 1; i < size; i++) {
			result[i - 1] = Point3D(coords[i][0], coords[i][1], coords[i][2]);
		}
		return result;
	} else {
		throw -1;
	}
}

void printSpeckle(std::vector<speckle> & spec, std::string prefix) {
	unsigned int size = spec.size();
	std::cout << prefix << std::endl;
	for (int i = 0; i < size; i++) {
		std::cout << "\tTime " << i << " ";
		speckle& sp = spec[i];
		unsigned int ns = sp.size();
		for (int j = 0; j < ns; j++) {
			std::cout << sp[j] << " ";
		}
		std::cout << std::endl;
	}
}

std::string speckleDetector(DICOMInputManager* manager, std::string viewType, std::vector<Point3D>* markers, std::vector<std::vector<Point3D> >* frameMarkers,
		std::vector<std::vector<double> >* segmentLengths = NULL, double esframe = -1, std::vector<Point3D>* bpts = NULL) {
#ifdef outputSpeckleTrackingImages
	numspeckleCalls++;
#endif
	Point3D Apex;
	Point3D baseL;
	Point3D baseR;

	if (markers->size() == 3) {
		Apex = markers->at(1);
		baseL = markers->at(0);
		baseR = markers->at(2);
	} else if (markers->size() == 9) {
		Apex = markers->at(4);
		baseL = markers->at(0);
		baseR = markers->at(8);
	} else {
		throw new SegmentationAndFittingException("Input boundary markers format not supported, 3 and 8 are supported");
	}

	Point3D base = (baseL + baseR) * 0.5;

	ViewTypeEnum vm = APLAX;
	if (viewType == "FCHENDO")
		vm = FCH;
	else if (viewType == "TCHENDO")
		vm = TCH;

	//Create a LVMesh
	LVHeartMeshModel mesh(maxcapframes, vm, MAXSPECKLES);
	mesh.setApex(Apex);

	mesh.setBaseL(baseL);
	mesh.setBaseR(baseR);

	std::vector<Point3D> rvInserts;
	rvInserts.push_back(baseL);
#define extrapolatebasepts
#ifdef extrapolatebasepts
	int frameNumber;
#endif
	mesh.setRVInserts(rvInserts);
	if (esframe > 0) { //Input esframe data
		mesh.setESFrameData(esframe, *bpts);
#ifdef extrapolatebasepts
		frameNumber = (int) (esframe * maxcapframes + 0.5);
#endif
	}

	//Align the model
	mesh.AlignModel();

	if (markers->size() == 9) {
		//Fit the model to the initial markers
		//true forces the markers to fit to the endo cardial surface
		//as markers size is less than speckle xi
		mesh.FitModel(*markers,0.0,true);
	}

	std::vector<DICOMSliceImageType::Pointer>& images(*(manager->extractSpecifiedFrames(mesh.getFramesFromApexBaseLengthVariation())));
	std::string timesample = mesh.getTimeSamplingInterval();
	std::vector<std::string> timepoints;
	boost::split(timepoints, timesample, boost::is_any_of(","));

	int numImages = images.size();
#ifdef outputSpeckleTrackingImages
	MarkerPlotter plot;
#endif

	SpeckleLibrary lib(&mesh, images[0], MAXSPECKLES);
	lib.setSpeckleSize(sw, sh);
	lib.Update();

	int apexIndex = 0;
	MotionEstimator estimator;
	TimeVaryingSmoother smoother(timesample, 3.25, true);
	smoother.setNumBasis(200);
	for (int i = 0; i < numImages; i++) {
		//Get the time from the time sampling vector
		double time = atof(timepoints[i].c_str());
		std::vector<Point3D> ptx = lib.getBorderInitializers(images[i], time, &apexIndex);
		std::vector<Point3D> spe = lib.getSpeckleInitializers(ptx, apexIndex);
#ifdef outputSpeckleTrackingImages
		std::ostringstream ss;
		ss << "Image_Init" << numspeckleCalls << "_" << i << ".jpg";
		plot.plotToFile(ss.str(), images[i], spe);
#endif

		SpeckleDetector detect1(images[i]);
		std::vector<Point3D> curPos = detect1.trackMotion(lib.getSpeckles(), spe);

		std::vector<Point3D> spex = estimator.getMotionForPoint(spe, curPos);
		unsigned int npts = spex.size();
		for (int p = 0; p < npts; p++) {
			spex[p] += ptx[p];
		}

#ifdef spatialsmoothing
		{
			double rad[1024];
			double index[1024];
			std::vector<Vector3D> pvectors(npts);
			Point3D dbase = (spex[0]+spex[npts-1])*0.5;
			for (int p = 0; p < npts; p++)
			{
				pvectors[p] = spex[p] - dbase;
				rad[p] = pvectors[p].Length();
				pvectors[p].Normalise();
				index[p] = p;
			}
			alglib::real_1d_array xx;
			xx.setcontent(apexIndex, index);
			alglib::real_1d_array yy;
			yy.setcontent(apexIndex, rad);

			alglib::ae_int_t m = 5; //Fit order 4 polynomial
			alglib::ae_int_t info;
			alglib::barycentricinterpolant p;
			alglib::polynomialfitreport rep;

			try {
				alglib::polynomialfit(xx, yy, m, info, p, rep);
			} catch (alglib::ap_error& err) {
				std::cout << err.msg << std::endl;
				std::cout << yy.tostring(1) << std::endl;
			}

			for (int i = 0; i <apexIndex; i++) {
				double len = alglib::barycentriccalc(p, i);
				spex[i] = dbase + pvectors[i]*len;
			}
			xx.setcontent((npts- apexIndex), index+apexIndex);
			yy.setcontent((npts- apexIndex), rad+apexIndex);
			try {
				alglib::polynomialfit(xx, yy, m, info, p, rep);
			} catch (alglib::ap_error& err) {
				std::cout << err.msg << std::endl;
				std::cout << yy.tostring(1) << std::endl;
			}
			for (int i = apexIndex; i <npts; i++) {
				double len = alglib::barycentriccalc(p, i);
				spex[i] = dbase + pvectors[i]*len;
			}
		}
#endif
		std::vector<Point3D> cmissPoints = lib.getCMISSMeshNodes(spex, apexIndex);
#ifdef extrapolatebasepts
		if (esframe > 0) {
			const Point3D esbasel = bpts->at(0);
			const Point3D esbaser = bpts->at(1);
			double weight = 0.0;
			if (frameNumber >= i) {
				weight = (double)(frameNumber - i)/(frameNumber-0.0);
				cmissPoints[0] = (cmissPoints[0] + baseL*weight+esbasel*(1.0-weight))*0.5;
				cmissPoints[cmissPoints.size()-1] = (cmissPoints[cmissPoints.size()-1] + baseR*weight+esbaser*(1.0-weight))*0.5;
			}else{
				weight = (double)(numImages - i-1.0)/(numImages-frameNumber-1.0);
				cmissPoints[0] = (cmissPoints[0] + esbasel*weight+baseL*(1.0-weight))*0.5;
				cmissPoints[cmissPoints.size()-1] = (cmissPoints[cmissPoints.size()-1] + esbaser*weight+baseR*(1.0-weight))*0.5;
			}
		}
#endif
		if (i < numImages - 1) { //Avoid the last frame, CAP code memory overflows causing  munmap_chunk() error
								 //Avoiding this avoid the fault. Also this information is not used subsequently so the step is redundant
			if (i % 3 == 0)
				mesh.FitModel(spex, time, true);
		}
		smoother.add(cmissPoints);
	}
	smoother.biasBoundary();
	std::vector<std::vector<Point3D> > smoothedboundary = smoother.getSmoothedPoints();
	for (int i = 0; i < numImages; i++) {
		std::vector<Point3D>& cmissPoints = smoothedboundary[i];
		frameMarkers->push_back(cmissPoints);
		if (segmentLengths != NULL)
			segmentLengths->push_back(lib.getModelSegmentLengths(cmissPoints));
#ifdef outputSpeckleTrackingImages
		std::ostringstream ssc;
		ssc << "Image_SC" << numspeckleCalls << "_" << i << ".jpg";
		plot.plotToFile(ssc.str(), images[i], cmissPoints);
#endif
	}

	return timesample;
}

std::string saxSpeckleDetector(DICOMInputManager* manager, std::string viewType, Point3D apex, Point3D basel, Point3D baser, Point3D gelemCentroid, std::vector<Point3D>* markers,
		std::vector<std::vector<Point3D> >* frameMarkers, std::vector<std::vector<double> >* segmentLengths = NULL) {
#ifdef outputSpeckleTrackingImages
	numspeckleCalls++;
#endif
	Point3D Apex(apex);
	Point3D baseL(basel);
	Point3D baseR(baser);
	Point3D base = (baseL + baseR) * 0.5;

	Point3D bl, br;
	if (markers->size() == 2) {
		bl = markers->at(0);
		br = markers->at(1);
	}
	if (markers->size() == 8) {
		bl = markers->at(0);
		br = markers->at(3); //Note that the last one will be closer the start
	}

	ViewTypeEnum vm = SAXBASE;
	if (viewType == "SAXMIDENDO")
		vm = SAXMID;
	else if (viewType == "SAXAPEXENDO")
		vm = SAXAPEX;

	const unsigned int MAXSAXSPECKLES = MAXSPECKLES/2;

	//Create a LVMesh
	LVHeartMeshModel mesh(maxcapframes, vm, MAXSAXSPECKLES);
	mesh.setApex(Apex);

	mesh.setBaseL(baseL);
	mesh.setBaseR(baseR);

	std::vector<Point3D> rvInserts;
	rvInserts.push_back(baseL);

	mesh.setRVInserts(rvInserts);

	//Align the model
	mesh.AlignModel();

	std::vector<DICOMSliceImageType::Pointer>& images(*(manager->extractSpecifiedFrames(mesh.getFramesFromApexBaseLengthVariation())));
	std::string timesampling = mesh.getTimeSamplingInterval();
	std::vector<std::string> timepoints;
	boost::split(timepoints, timesampling, boost::is_any_of(","));

	int numImages = images.size();
#ifdef outputSpeckleTrackingImages
	MarkerPlotter plot;
#endif
	SpeckleLibrary lib(&mesh, images[0], MAXSAXSPECKLES);
	lib.setSpeckleSize(sw, sh);
	lib.Update();

	int apexIndex = 0;
	Point3D origin = (bl + br) * 0.5;
	MotionEstimator estimator(vm);
	for (int i = 0; i < numImages; i++) {
		//Get the time from the time sampling vector
		double time = atof(timepoints[i].c_str());
		std::vector<Point3D> ptx = lib.getBorderInitializers(images[i], time, NULL);
		std::vector<Point3D> spe = lib.getSpeckleInitializers(ptx, -1);

		SpeckleDetector detect1(images[i]);
		std::vector<Point3D> curPos = detect1.trackMotion(lib.getSpeckles(), spe);

		std::vector<Point3D> spex = estimator.getMotionForPoint(spe, curPos);
		unsigned int npts = spex.size();
		for (int p = 0; p < npts; p++) {
			spex[p] += ptx[p];
		}
		std::vector<Point3D> cmissPoints = lib.getCMISSSAXMeshNodes(spex, origin);

		if (i < numImages - 1) { //Avoid the last frame, CAP code memory overflows causing  munmap_chunk() error
								 //Avoiding this avoid the fault. Also this information is not used subsequently so the step is redundant
			if (i % 3 == 0)
				mesh.FitModel(spex, time, true);
		}

		frameMarkers->push_back(cmissPoints);

#ifdef outputSpeckleTrackingImages
		std::ostringstream ss;
		ss << "Image_SC" << numspeckleCalls << "_" << i << ".jpg";
		plot.plotToFile(ss.str(), images[i], cmissPoints);
#endif
	}

	//Compute torsions
	{
		std::vector<std::vector<double> > angles;
		for (int i = 0; i < numImages; i++) {
			std::vector<double> myAngles;
			std::vector<Point3D> result = frameMarkers->at(i);
			Point3D centroid(0, 0, 0);
			unsigned int numSpeckles = result.size();
			for (int j = 0; j < numSpeckles; j++) {
				centroid += result[j];
			}
			centroid = centroid * (1.0 / numSpeckles);
			for (int j = 0; j < numSpeckles; j++) {
				Vector3D op = result[j] - centroid;
				double theta = atan2(op.y, op.x);
				myAngles.push_back(theta);
			}
			angles.push_back(myAngles);
		}

		//std::cout << viewType << std::endl;
		std::vector<double> initAngles = angles[0];
		unsigned int numSpeckles = initAngles.size();
		for (int i = 0; i < numImages; i++) {
			std::vector<double>& myAngles = angles[i];
			for (int j = 0; j < numSpeckles; j++) {
				myAngles[j] -= initAngles[j];
			}
		}
		//Drift compensate
		for (int j = 0; j < numSpeckles; j++) {
			double factor = angles[numImages - 1][j] / (numImages - 1.0);
			for (int i = 0; i < numImages; i++) {
				angles[i][j] -= i * factor;
				if (angles[i][j] < -M_PI) {
					angles[i][j] = 2 * M_PI + angles[i][j];
				}
			}
		}
	}
	return timesampling;
}

#endif /* LVMESHBASEDSPECKLETRACKING_H_ */
