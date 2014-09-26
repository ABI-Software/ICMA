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


#include "SpeckleLibrary.h"

SpeckleLibrary::SpeckleLibrary(LVHeartMeshModel* lvmesh, DICOMSliceImageType::Pointer image, int maxSpeckles) :
		edImage(image), MAXSPECKLES(maxSpeckles) {
	searchExtenstionFactor = 1.0;
	speckleHeight = 10;
	speckleWidth = 10;
	boundarySpeckleFitness = 0.0;
	pixelcutoff = -1.0;
	mesh = lvmesh;
	apex = mesh->getApex();
	basel = mesh->getBaseL();
	baser = mesh->getBaseR();
}

SpeckleLibrary::~SpeckleLibrary() {

}

void SpeckleLibrary::setApex(Point3D point) {
	apex = point;
}

void SpeckleLibrary::setBaseL(Point3D point) {
	basel = point;
}

void SpeckleLibrary::setBaseR(Point3D point) {
	baser = point;
}

void SpeckleLibrary::setSpeckleSize(int sw, int sh) {
	speckleWidth = sw;
	speckleHeight = sh;
}

void SpeckleLibrary::computeSpeckles() {
	StatisticsImageFilterType::Pointer statisticsImageFilter = StatisticsImageFilterType::New();
	DICOMSliceImageType::SizeType dSize;
	DICOMSliceImageType::RegionType desiredRegion;
	DICOMSliceImageType::Pointer result;
	DICOMSliceImageType::SizeType targetSize = edImage->GetLargestPossibleRegion().GetSize();
	unsigned int numSpeckles = speckleCenters.size();
	RoiFilterType::Pointer roiFilter = RoiFilterType::New();
	int sw2 = speckleWidth / 2;
	int sh2 = speckleHeight / 2;
	boundarySpeckleFitness = 0.0;
	for (int i = 0; i < numSpeckles; i++) {
		try {
			DICOMSliceImageType::IndexType blidx; //This is the bottomLeft corner of the region
			blidx[0] = speckleCenters[i].x - sw2;
			blidx[1] = speckleCenters[i].y - sh2;

			dSize[0] = speckleWidth;
			dSize[1] = speckleHeight;

			if ((blidx[0] + dSize[0]) > targetSize[0]) {
				dSize[0] = (targetSize[0] - blidx[0]) - 1;
			}
			if ((blidx[1] + dSize[1]) > targetSize[1]) {
				dSize[1] = (targetSize[1] - blidx[1]) - 1;
			}
			if (dSize[0] > targetSize[0]) {
				std::cerr << "[LIB] for speckle " << speckleCenters[i] << "\t" << i << "\t";
				std::cerr << "speckleBound computing error dsize " << dSize << "\t Blidx " << blidx << "\t Target Size " << targetSize << std::endl;
				dSize[0] = targetSize[0] - 1;
			}
			if (dSize[1] > targetSize[1]) {
				std::cerr << "[LIB] for speckle " << speckleCenters[i] << "\t" << i << "\t";
				std::cerr << "speckleBound computing error dsize " << dSize << "\t Blidx " << blidx << "\t Target Size " << targetSize << std::endl;
				dSize[1] = targetSize[1] - 1;
			}
			desiredRegion.SetSize(dSize);
			desiredRegion.SetIndex(blidx);
#ifdef debug
			//Due to threading output a single string rather than separate ones
			std::ostringstream ss;
			ss << "Radius " << std::endl << radius[0] << ", " << radius[1] << std::endl
			<< "Desired Size" << std::endl << dSize[0] << ", " << dSize[1]
			<< std::endl;
			ss << "Requesting region \n" << desiredRegion << "Target region \n"
			<< target->GetLargestPossibleRegion() << std::endl;
			std::cout << ss.str() << std::endl;
			std::cout.flush();
#endif

			roiFilter->SetRegionOfInterest(desiredRegion);
			roiFilter->SetInput(edImage);
			roiFilter->Update();
			result = roiFilter->GetOutput();
			result->DisconnectPipeline();
			//Set the origin to 0,0
			double origin[2] = { 0.0, 0.0 };
			result->SetOrigin(origin);

		} catch (itk::ExceptionObject& excp) {
#ifdef debug
			std::ostringstream ss;
			ss << "Radius " << std::endl << radius[0] << ", " << radius[1] << std::endl
			<< "Desired Size" << std::endl << dSize[0] << ", " << dSize[1] << std::endl;
			ss << "Requesting region \n" << desiredRegion << "Target region \n"
			<< target->GetLargestPossibleRegion() << std::endl;
			ss << "***** Error occurred "<<excp.what()<< std::endl;
			std::cout << ss.str() << std::endl;
#endif
			throw excp;
		}
//#define outputSpeckles
#ifdef outputSpeckles
		typedef itk::Image<unsigned char,2> ucharimage;
		typedef itk::CastImageFilter<DICOMSliceImageType,ucharimage> caster;
		typedef itk::ImageFileWriter<ucharimage> writer;
		static int speckleFileCtr = 0;
		std::ostringstream ss;
		ss<<"Speckle"<<speckleFileCtr++<<".jpg";
		writer::Pointer write = writer::New();
		caster::Pointer cast = caster::New();
		cast->SetInput(result);
		write->SetFileName(ss.str());
		write->SetInput(cast->GetOutput());
		write->Update();
#endif
		statisticsImageFilter->SetInput(result);
		statisticsImageFilter->Update();
//#define printspecklestats
#ifdef printspecklestats
		std::cout <<i<< "\tMean: " << statisticsImageFilter->GetMean();
		std::cout << " Std.: " << statisticsImageFilter->GetSigma();
		std::cout << " Min: " << statisticsImageFilter->GetMinimum();
		std::cout << " Max: " << statisticsImageFilter->GetMaximum() << std::endl;
#endif
		if (statisticsImageFilter->GetSigma() > SPECKLESTDCUTOFF) {
			boundarySpeckleFitness += 1.0;
		}
		speckles.push_back(result);
	}
	boundarySpeckleFitness /= numSpeckles;
}

void SpeckleLibrary::Update() {
	int apexIndex = 0;

	if (mesh->getViewType() < SAXBASE) {
		std::vector<Point3D> border = getBorderInitializers(edImage, 0.0, &apexIndex);
		speckleCenters = getSpeckleInitializers(border, 0.0);
		int npts = speckleCenters.size();
		std::vector<Point3D> aSpeckles;

		for (int i = 0; i < npts; i += 3) {
			Point3D np = (speckleCenters[i] + speckleCenters[i + 1] + speckleCenters[i + 2]) * (1 / 3.0);
			aSpeckles.push_back(np);
		}
		npts = aSpeckles.size();
		double* dist = heapSpace;
		dist[0] = 0.0;
		for (int i = 1; i < apexIndex + 1; i++) {
			dist[i] = dist[i - 1] + aSpeckles[i - 1].distance(aSpeckles[i]);
		}
		double ldist = dist[apexIndex];
		double step = ldist / 4.0;
		cmissNodeIds.push_back(0); //basel
		for (int ctr = 1; ctr < 4; ctr++) {
			double mstep = ctr * step;
			for (int i = cmissNodeIds[ctr - 1] + 1; i < apexIndex; i++) {
				if (dist[i] > mstep) {
					cmissNodeIds.push_back(i - 1);
					break;
				}
			}
		}
		cmissNodeIds.push_back(apexIndex); //Apex
		dist[apexIndex] = 0;
		for (int i = apexIndex + 1; i < npts; i++) {
			dist[i] = dist[i - 1] + aSpeckles[i - 1].distance(aSpeckles[i]);
		}
		double rdist = dist[npts - 1];
		step = rdist / 4.0;
		for (int ctr = 1; ctr < 4; ctr++) {
			double mstep = ctr * step;
			for (int i = cmissNodeIds[3 + ctr] + 1; i < npts; i++) {
				if (dist[i] > mstep) {
					cmissNodeIds.push_back(i - 1);
					break;
				}
			}
		}
		cmissNodeIds.push_back(npts - 1); //baser
	} else {
		speckleCenters = getBorderInitializers(edImage, 0.0, NULL);
	}
	computeSpeckles();
}

std::vector<DICOMSliceImageType::Pointer>&
SpeckleLibrary::getSpeckles() {
	return speckles;
}

std::vector<Point3D>&
SpeckleLibrary::getSpeckleCenters() {
	return speckleCenters;
}

std::vector<Point3D> SpeckleLibrary::getBorderInitializers(DICOMSliceImageType::Pointer image, double time, int *apexIndex) {
	if (apexIndex != NULL) {
		std::vector<Point3D> boundaryI = mesh->getSpeckleInitializers(time, apexIndex);
		DICOMSliceImageType::SizeType imgSize = image->GetLargestPossibleRegion().GetSize();

		std::vector<Point3D> boundary(boundaryI);
		int apidx = *apexIndex;
		Point3D basel1 = basel;
		Point3D baser1 = baser;
		Vector3D basev = baser1 - basel1;
		basev.Normalise();
		unsigned int npts = boundary.size();
		if (pixelcutoff < 0.0) { //Set it based on edImage
			StatisticsImageFilterType::Pointer statisticsImageFilter = StatisticsImageFilterType::New();
			statisticsImageFilter->SetInput(edImage);
			statisticsImageFilter->Update();
			pixelcutoff = statisticsImageFilter->GetMean() + statisticsImageFilter->GetSigma() / 10.0;

			DICOMSliceImageType::IndexType idx;
			idx[0] = boundary[apidx].x;
			idx[1] = boundary[apidx].y;
			double aval = edImage->GetPixel(idx);
			if (aval < pixelcutoff)
				aval = pixelcutoff;
			idx[0] = basel1.x;
			idx[1] = basel1.y;
			double bval = edImage->GetPixel(idx);
			if (bval < pixelcutoff)
				bval = pixelcutoff;
			idx[0] = baser1.x;
			idx[1] = baser1.y;
			double bvar = edImage->GetPixel(idx);
			if (bvar < pixelcutoff)
				bvar = pixelcutoff;
			pixelcutoff = (aval + bval + bvar) / 3.0;
		}
		//Move the predicted points to the endo boundary
		{
			Point3D base = (basel1 + baser1) * 0.5;
			Vector3D baseL = basel1 - base;
			baseL.Normalise();
			Vector3D baseR = baser1 - base;
			baseR.Normalise();
			Vector3D apexBase = boundary[apidx] - base;
			apexBase.Normalise();

			std::vector<Point3D> abvp(npts);
			double* distance = heapSpace;
			double* index = heapSpace + npts;
			double bdist = 0.0;
			for (int i = 0; i < apidx; i++) {
				Point3D& pt = boundary[i];
				DICOMSliceImageType::IndexType idx;
				Vector3D pvec = pt - base;
				Point3D poabv = base + (pvec * apexBase) * apexBase;

				double dist = pt.distance(poabv);

				int bspace = 0;
				double init = 0;
				for (double dx = init; dx < dist * searchExtenstionFactor; dx += 1.0) {
					Point3D np = poabv + dx * baseL;
					idx[0] = np.x;
					idx[1] = np.y;
					if (image->GetPixel(idx) < pixelcutoff) {
						bspace++;
					}
				}
				if (bspace < dist * 0.5) //If the border is in the middle of a white field, then
					bspace = dist;    //the black space counting method is incorrect
				abvp[i] = poabv;
				distance[i] = bspace + init;
				index[i] = i;
				if (i == 0) {
					bdist = dist;
				}
			}
			for (int i = 0; i < 2; i++)
				distance[i] = bdist;

			//Smooth using alglib
			double rho = 2.75;
			alglib::real_1d_array x;
			x.setcontent(apidx, index);
			alglib::real_1d_array y;
			y.setcontent(apidx, distance);
			alglib::ae_int_t info;
			alglib::spline1dinterpolant s;
			alglib::spline1dfitreport rep;
			try {
				alglib::spline1dfitpenalized(x, y, 50, rho, info, s, rep);
			} catch (alglib::ap_error& err) {
				std::cout << err.msg << std::endl;
				std::cout << y.tostring(1) << std::endl;
			}

			for (int i = 0; i < apidx; i++) {
				Point3D pt = abvp[i] + alglib::spline1dcalc(s, i) * baseL;
				boundary[i] = pt;
			}

			distance[apidx] = 0;
			index[apidx] = apidx;
			for (int i = apidx + 1; i < npts; i++)
			{
				Point3D& pt = boundary[i];
				DICOMSliceImageType::IndexType idx;
				Vector3D pvec = pt - base;
				Point3D poabv = base + (pvec * apexBase) * apexBase;
				double dist = pt.distance(poabv);
				int bspace = 0;
				double init = 0.0;
				for (double dx = init; dx < dist * searchExtenstionFactor; dx += 1.0) {
					Point3D np = poabv + dx * baseR;
					idx[0] = np.x;
					idx[1] = np.y;
					if (image->GetPixel(idx) < pixelcutoff) {
						bspace++;
					}
				}
				if (bspace < dist * 0.5) //If the border is in the middle of a white field, then
					bspace = dist;    //the black space counting method is incorrect
				abvp[i] = poabv;
				distance[i] = bspace + init;
				index[i] = i;
				if (i == npts - 1)
					bdist = dist;
			}
			for (int i = 1; i < 3; i++)
				distance[npts - i] = bdist;
			x.setcontent((npts - apidx), index + apidx - 1);
			y.setcontent((npts - apidx), distance + apidx - 1);
			try {
				alglib::spline1dfitpenalized(x, y, 50, rho, info, s, rep);
			} catch (alglib::ap_error& err) {
				std::cout << err.msg << std::endl;
				std::cout << y.tostring(1) << std::endl;
			}
			for (int i = apidx + 1; i < npts; i++)
			{
				Point3D pt = abvp[i] + alglib::spline1dcalc(s, i) * baseR;
				//boundary[i] = boundary[i]*0.5+pt*0.5;
				boundary[i] = pt;
			}
			//The boundary values should be smoothed
			basel1 = boundary[0];
			baser1 = boundary[boundary.size() - 1];
			basev = baser1 - basel1;
			basev.Normalise();
		}
		currentBoundary = boundary;

		return currentBoundary;
	} else {
		int idx;
		currentBoundary = mesh->getSpeckleInitializers(time, &idx);
		return currentBoundary;
	}
}
std::vector<Point3D>

SpeckleLibrary::getSpeckleInitializers(std::vector<Point3D>& border, int apidx) {
	std::vector<Point3D> boundary;
	Point3D origin;
	double lv = speckleWidth;
	double rv = -lv;
	int npts = border.size();
	if (apidx > 0) {
		Point3D& basel = border[0];
		Point3D& baser = border[npts - 1];
		Point3D& apex = border[apidx];
		Point3D base = (basel + baser) * 0.5;
		origin = base + (0.3333) * (apex - base);
	} else {
		//Find centroid
		Point3D centroid;
		for (int i = 0; i < npts; i++) {
			centroid = centroid + border[i];
		}
		origin = centroid * (1.0 / npts);
	}
	for (int i = 0; i < npts; i++) {
		Point3D& pt = border[i];
		Vector3D vec = pt - origin;
		vec.Normalise();
		Point3D left = pt + lv * vec;
		boundary.push_back(left);
		left = pt + (2 * lv) * vec;
		boundary.push_back(left);
		left = pt + (3 * lv) * vec;
		boundary.push_back(left);
	}

	return boundary;
}

std::vector<Point3D> SpeckleLibrary::getCMISSMeshNodes(std::vector<Point3D>& aSpeckles, int apexIndex) {
	std::vector<Point3D> result(9);

	unsigned int npts = aSpeckles.size();

	double* index = heapSpace;
	double* xcoord = index + npts;
	double* ycoord = xcoord + npts;

	for (int i = 0; i < npts; i++) {
		index[i] = i;
		xcoord[i] = aSpeckles[i].x;
		ycoord[i] = aSpeckles[i].y;
	}
	double rho = 2.5;
	alglib::real_1d_array x;
	x.setcontent(apexIndex + 1, index);
	alglib::real_1d_array y;
	y.setcontent(apexIndex + 1, xcoord);
	alglib::real_1d_array z;
	z.setcontent(apexIndex + 1, ycoord);
	alglib::ae_int_t info;
	alglib::spline1dinterpolant s;
	alglib::spline1dinterpolant bp;
	alglib::spline1dfitreport rep;
	try {
		alglib::spline1dfitpenalized(x, y, 100, rho, info, s, rep);
		alglib::spline1dfitpenalized(x, z, 100, rho, info, bp, rep);
	} catch (alglib::ap_error& err) {
		std::cout << err.msg << std::endl;
		std::cout << y.tostring(1) << std::endl;
	}
	for (int i = 0; i < apexIndex; i++) {
		aSpeckles[i].x = alglib::spline1dcalc(s, i);
		aSpeckles[i].y = alglib::spline1dcalc(bp, i);
	}
	x.setcontent(npts - apexIndex, index + apexIndex);
	y.setcontent(npts - apexIndex, xcoord + apexIndex);
	z.setcontent(npts - apexIndex, ycoord + apexIndex);
	try {
		alglib::spline1dfitpenalized(x, y, 100, rho, info, s, rep);
		alglib::spline1dfitpenalized(x, z, 100, rho, info, bp, rep);
	} catch (alglib::ap_error& err) {
		std::cout << err.msg << std::endl;
		std::cout << y.tostring(1) << std::endl;
	}
	for (int i = apexIndex + 1; i < npts; i++) {
		aSpeckles[i].x = alglib::spline1dcalc(s, i);
		aSpeckles[i].y = alglib::spline1dcalc(bp, i);
	}

#ifdef simpledivision
	double step = apexIndex/4.0;
	//result[0] = aSpeckles[0];
	for(int i=0;i<4;i++)
	{
		double aidx = i*step;
		result[i] = aSpeckles[(int)(aidx)];
		result[8-i] = aSpeckles[(int)(npts-aidx-1)];
	}
	result[4] = aSpeckles[apexIndex];
	//result[8] = baser;
#else
	//Speckle density changes between the sides based on their length
	for (int i = 0; i < 9; i++) {
		result[i] = aSpeckles[cmissNodeIds[i]];
	}
#endif
	return result;
}

std::vector<double> SpeckleLibrary::getModelSegmentLengths(std::vector<Point3D>& speckles) {
	std::vector<double> slengths;
	for (int i = 0; i < 8; i++) {
		slengths.push_back(speckles[i].distance(speckles[i + 1]));
	}

	std::vector<double> lengths;
	lengths.push_back(slengths[0] + (1 / 3.0 - 1 / 4.0) * 4 * slengths[1]);
	lengths.push_back((1.0 - (1 / 3.0 - 1 / 4.0)) * 4 * slengths[1] + (2 / 3.0 - 1 / 2.0) * 4 * slengths[2]);
	lengths.push_back((1.0 - (2 / 3.0 - 1 / 2.0)) * 4 * slengths[2] + slengths[3]);
	lengths.push_back(slengths[4] + (1.0 - (2 / 3.0 - 1 / 2.0)) * 4 * slengths[5]);
	lengths.push_back((2 / 3.0 - 1 / 2.0) * 4 * slengths[5] + (1.0 - (1 / 3.0 - 1 / 4.0)) * 4 * slengths[6]);
	lengths.push_back((1 / 3.0 - 1 / 4.0) * 4 * slengths[6] + slengths[7]);
	return lengths;
}

std::vector<std::vector<double> > SpeckleLibrary::getSegmentStrains(std::vector<std::vector<double> > lengths, bool driftcorrected) {
	unsigned int numImages = lengths.size();
	std::vector<std::vector<double> > strains(6);
	std::vector<double>& initLengths = lengths[0];
	for (int i = 0; i < numImages; i++) {
		std::vector<double>& curLengths = lengths[i];
		for (int j = 0; j < 6; j++) {
			double temp = (curLengths[j] - initLengths[j]) / initLengths[j];
			strains[j].push_back(temp);
		}
	}

	double heap[500]; //Function is static so cannot use object heapSpace
	//Get data at multiple points
	double* indexs = heap;
	double* values = heap + numImages;
	double denom = (numImages - 1.0);
	for (int i = 0; i < numImages; i++) {
		indexs[i] = i / denom;
	}
	alglib::real_1d_array x;
	x.setcontent(numImages, indexs);

	std::vector<std::vector<double> > result;
	for (int j = 0; j < 6; j++) {
		std::vector<double>& curStrain = strains[j];
		for (int i = 0; i < numImages; i++) {
			values[i] = curStrain[i];
		}
		alglib::real_1d_array y;
		y.setcontent(numImages, values);
		alglib::spline1dinterpolant s;
		// build spline
		try {
			alglib::spline1dbuildakima(x, y, s);
		} catch (alglib::ap_error& err) {
			std::cout << err.msg << "\t@\t" << j << std::endl;
			std::cout << y.tostring(2) << std::endl;
			std::cout << x.tostring(2) << std::endl;
		}
		std::vector<double> intStrain;
		for (int i = 0; i < numImages - 1; i++) {
			intStrain.push_back(alglib::spline1dcalc(s, i / denom));
			intStrain.push_back(alglib::spline1dcalc(s, (i + 0.5) / denom));
		}
		intStrain.push_back(curStrain[numImages - 1]); //The boundary
		result.push_back(intStrain);
	}

	if (driftcorrected) {
		int numStrains = result[0].size();
		for (int j = 0; j < 6; j++) {
			std::vector<double>& curStrain = result[j];
			double maxV = curStrain[numStrains - 1];
			if (maxV != 0.0) {
				double denom = static_cast<double>(numStrains - 1);
				for (int i = 1; i < numStrains; i++) {
					curStrain[i] -= maxV * static_cast<double>(i) / denom;
				}
			}
		}
	}

	return result;
}

double SpeckleLibrary::getBoundarySpeckleFitness() {
	return boundarySpeckleFitness;
}

std::vector<Point3D> SpeckleLibrary::getCMISSSAXMeshNodes(std::vector<Point3D>& speckles, Point3D origin) {
	const unsigned int maxcmissnodes = 12;
	std::vector<Point3D> result(maxcmissnodes);
	Point3D centroid;
	int numSpeckles = speckles.size();
	for (int i = 0; i < numSpeckles; i++)
		centroid += speckles[i];
	centroid = centroid * (1.0 / numSpeckles);
	double* radius = heapSpace;
	double* theta = heapSpace + numSpeckles;
	double* index = heapSpace + 2 * numSpeckles;
	double denom = numSpeckles - 1.0;
	for (int i = 0; i < numSpeckles; i++) {
		radius[i] = centroid.distance(speckles[i]);
		Vector3D op = speckles[i] - centroid;
		theta[i] = atan2(op.y, op.x);
		index[i] = i / denom;
	}
	alglib::real_1d_array x, rad, the;
	x.setcontent(numSpeckles, index);
	rad.setcontent(numSpeckles, radius);
	the.setcontent(numSpeckles, theta);

	alglib::spline1dinterpolant rads, thetas;
	// build spline

	alglib::ae_int_t info;
	alglib::spline1dinterpolant s;
	alglib::spline1dfitreport rep;
	try {
		alglib::spline1dfitpenalized(x, rad, 50, 2.5, info, rads, rep);
	} catch (alglib::ap_error& err) {
		std::cout << err.msg << std::endl;
		std::cout << rad.tostring(2) << std::endl;
	}
	alglib::spline1dbuildakima(x, the, thetas);

	double step = 1.0 / (maxcmissnodes - 1);

	for (int i = 0; i < maxcmissnodes; i++) {
		double nradius = alglib::spline1dcalc(rads, i * step);
		double angle = alglib::spline1dcalc(thetas, i * step);
		Point3D newp = origin;
		newp.x += nradius * cos(angle);
		newp.y += nradius * sin(angle);
		result[i] = newp;
	}

	return result;

}

