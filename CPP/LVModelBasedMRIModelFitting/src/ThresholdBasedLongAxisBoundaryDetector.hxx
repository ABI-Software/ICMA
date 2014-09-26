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

#include "ThresholdBasedLongAxisBoundaryDetector.h"
#include "itkImageDuplicator.h"
#include "itkImageFileWriter.h"
#include "itkBinaryContourImageFilter.h"
#include "itkPolygonSpatialObject.h"
#include "itkSpatialObjectToImageFilter.h"
#include "itkMaskImageFilter.h"

template<typename ImageType>
ThresholdBasedLongAxisBoundaryDetector<ImageType>::ThresholdBasedLongAxisBoundaryDetector(typename ImageType::Pointer img, Point3D bl, Point3D br, Point3D ax, int npts) :
		image(img), basel(bl), baser(br), apex(ax), numBdryPts(npts) {

}

template<typename ImageType>
ThresholdBasedLongAxisBoundaryDetector<ImageType>::~ThresholdBasedLongAxisBoundaryDetector() {

}

template<typename ImageType>
typename ImageType::Pointer ThresholdBasedLongAxisBoundaryDetector<ImageType>::getThresholdedImage() {
	return thresholdedImage;
}

template<typename ImageType>
void ThresholdBasedLongAxisBoundaryDetector<ImageType>::setBoundaryPolygon(std::vector<Point3D>& poly) {
	bdryPoly = std::vector<Point3D>(poly);
}

template<typename ImageType>
std::vector<Point3D> ThresholdBasedLongAxisBoundaryDetector<ImageType>::getEndoBoundary(std::vector<Point3D> predictedBdry, int apexIndex) {
	unsigned int maxpts = predictedBdry.size();
	std::vector<Point3D> result(predictedBdry);
	Vector3D basevec = predictedBdry[maxpts - 1] - predictedBdry[0];
	basevec.Normalise();
	Point3D base = (predictedBdry[maxpts - 1] + predictedBdry[0]) * 0.5;
	Vector3D abv = base - predictedBdry[apexIndex];
	abv.Normalise();

	typedef typename itk::OtsuThresholdImageFilter<ImageType, ImageType> FilterType;
	typedef typename itk::InvertIntensityImageFilter<ImageType> InvertIntensityImageFilterType;
	typename FilterType::Pointer otsuFilter = FilterType::New();
	typename InvertIntensityImageFilterType::Pointer invertIntensityFilter = InvertIntensityImageFilterType::New();

	if (bdryPoly.size() > 0) {
		typedef typename itk::MaskImageFilter<ImageType, ImageType> MDaskImageFilterType;
		typedef itk::PolygonSpatialObject<2> PolygonType;
		PolygonType::Pointer polygon = PolygonType::New();
		PolygonType::PointType point;
		polygon->ComputeObjectToWorldTransform();
		for (int ix = 0; ix < bdryPoly.size(); ix++) {
			point[0] = bdryPoly[ix].x;
			point[1] = bdryPoly[ix].y;
			polygon->AddPoint(point);
		} //Close the polygon
		{
			point[0] = bdryPoly[0].x;
			point[1] = bdryPoly[0].y;
			polygon->AddPoint(point);
		}
		typename MDaskImageFilterType::Pointer mdmasker = MDaskImageFilterType::New();
		// create the filter and the output mask image
		typedef typename itk::SpatialObjectToImageFilter<PolygonType, ImageType> XSpatialObjectToImageFilterType;
		typename XSpatialObjectToImageFilterType::Pointer imageFilter = XSpatialObjectToImageFilterType::New();
		imageFilter->SetInput(polygon);
		imageFilter->SetInsideValue(1);
		imageFilter->SetOutsideValue(0);
		polygon->SetThickness(1.0);
		imageFilter->SetSize(image->GetLargestPossibleRegion().GetSize());
		imageFilter->SetOrigin(image->GetOrigin());
		imageFilter->SetSpacing(image->GetSpacing());
		imageFilter->Update();
		mdmasker->SetInput(image);
		mdmasker->SetMaskImage(imageFilter->GetOutput());
		mdmasker->Update();
		otsuFilter->SetInput(mdmasker->GetOutput());
	} else {
		otsuFilter->SetInput(image);
	}
	invertIntensityFilter->SetMaximum(255);
	invertIntensityFilter->SetInput(otsuFilter->GetOutput());
	invertIntensityFilter->Update();
	thresholdedImage = invertIntensityFilter->GetOutput();
	thresholdedImage->DisconnectPipeline();

//#define scanlines
#ifdef scanlines
	typedef typename itk::ImageDuplicator<ImageType> dupicator;
	typename dupicator::Pointer sdup = dupicator::New();
	sdup->SetInputImage(thresholdedImage);
	//sdup->SetInputImage(image);
	sdup->Update();
	typename ImageType::Pointer scan = sdup->GetOutput();
	scan->DisconnectPipeline();
#endif
	double stepSize = 0.5;
	double maxExt = 10.05;
	typename ImageType::IndexType index;
	for (int i = 0; i < apexIndex; i++) {
		int whitespace = 0;
		Point3D sel = predictedBdry[i];
		for (double x = 0; x < maxExt; x += stepSize) {
			Point3D pt = predictedBdry[i] + -x * basevec;
			index[0] = pt.x;
			index[1] = pt.y;
			if (thresholdedImage->GetPixel(index) != 0) {
				whitespace++;
				sel = pt;
#ifdef scanlines
				scan->SetPixel(index, 127);
#endif
			}
		}
		if (whitespace >0 ) {
			result[i] = sel;
		}
#ifdef scanlines
		index[0] = predictedBdry[i].x;
		index[1] = predictedBdry[i].y;
		scan->SetPixel(index, 255);
#endif
	}
	for (int i = apexIndex+1; i < maxpts; i++) {
		int whiteSpace = 0;
		Point3D sel = predictedBdry[i];
		for (double x = 0; x < maxExt; x += stepSize) {
			Point3D pt = predictedBdry[i] + x * basevec;
			index[0] = pt.x;
			index[1] = pt.y;
			if (thresholdedImage->GetPixel(index) != 0) {
				whiteSpace++;
#ifdef scanlines
				scan->SetPixel(index, 127);
#endif
				sel = pt;
			}
		}
		if (whiteSpace > 0) {
			result[i] = sel;
		}
#ifdef scanlines
		index[0] = predictedBdry[i].x;
		index[1] = predictedBdry[i].y;
		scan->SetPixel(index, 255);
#endif
	}

	{
		int whiteSpace = 0;
		Point3D sel = predictedBdry[apexIndex];
		for (double x = 0; x < 3.05; x += 0.5) {
			Point3D pt = predictedBdry[apexIndex] + (x) * abv;
			index[0] = pt.x;
			index[1] = pt.y;
			if (thresholdedImage->GetPixel(index) == 0) {
				whiteSpace++;
#ifdef scanlines
				scan->SetPixel(index, 127);
#endif
				sel = pt;
			}
		}
		if (whiteSpace > 0) {
			result[apexIndex] = sel;
		}
	}
#ifdef scanlines
	static int filectr = 0;
	std::ostringstream ss;
	ss << "RScan" << filectr << ".jpg";
	typedef typename itk::ImageFileWriter<ImageType> WriterType;
	typename WriterType::Pointer writer = WriterType::New();
	writer->SetInput(scan);
	writer->SetFileName(ss.str());
	writer->Update();
	ss.str("");
	filectr++;
#endif
	return result;
}

//Scan the to create the blob of interest and then find the boundary of the blob
template<typename ImageType>
std::vector<Point3D> ThresholdBasedLongAxisBoundaryDetector<ImageType>::getEndoBoundary(int* apexIndex) {
	typedef typename itk::OtsuThresholdImageFilter<ImageType, ImageType> FilterType;
	typedef typename itk::InvertIntensityImageFilter<ImageType> InvertIntensityImageFilterType;
	typename FilterType::Pointer otsuFilter = FilterType::New();
	typename InvertIntensityImageFilterType::Pointer invertIntensityFilter = InvertIntensityImageFilterType::New();

	if (bdryPoly.size() > 0) {
		typedef typename itk::MaskImageFilter<ImageType, ImageType> MDaskImageFilterType;
		typedef itk::PolygonSpatialObject<2> PolygonType;
		PolygonType::Pointer polygon = PolygonType::New();
		PolygonType::PointType point;
		polygon->ComputeObjectToWorldTransform();
		for (int ix = 0; ix < bdryPoly.size(); ix++) {
			point[0] = bdryPoly[ix].x;
			point[1] = bdryPoly[ix].y;
			polygon->AddPoint(point);
		} //Close the polygon
		{
			point[0] = bdryPoly[0].x;
			point[1] = bdryPoly[0].y;
			polygon->AddPoint(point);
		}
		typename MDaskImageFilterType::Pointer mdmasker = MDaskImageFilterType::New();
		// create the filter and the output mask image
		typedef typename itk::SpatialObjectToImageFilter<PolygonType, ImageType> XSpatialObjectToImageFilterType;
		typename XSpatialObjectToImageFilterType::Pointer imageFilter = XSpatialObjectToImageFilterType::New();
		imageFilter->SetInput(polygon);
		imageFilter->SetInsideValue(1);
		imageFilter->SetOutsideValue(0);
		polygon->SetThickness(1.0);
		imageFilter->SetSize(image->GetLargestPossibleRegion().GetSize());
		imageFilter->SetOrigin(image->GetOrigin());
		imageFilter->SetSpacing(image->GetSpacing());
		imageFilter->Update();
		mdmasker->SetInput(image);
		mdmasker->SetMaskImage(imageFilter->GetOutput());
		mdmasker->Update();
		otsuFilter->SetInput(mdmasker->GetOutput());
	} else {
		otsuFilter->SetInput(image);
	}
	invertIntensityFilter->SetMaximum(255);
	invertIntensityFilter->SetInput(otsuFilter->GetOutput());
	invertIntensityFilter->Update();
	thresholdedImage = invertIntensityFilter->GetOutput();
	thresholdedImage->DisconnectPipeline();

	typedef typename ImageType::SizeType ImageSizeType;
	typedef typename ImageType::SpacingType ImageSpacingType;
	typedef typename ImageType::IndexType ImageIndexType;
	typedef typename itk::ImageDuplicator<ImageType> dupicator;
	typename dupicator::Pointer dup = dupicator::New();
	dup->SetInputImage(thresholdedImage);
	dup->Update();
	typename ImageType::Pointer lvchamber = dup->GetOutput();
	lvchamber->DisconnectPipeline();
	lvchamber->FillBuffer(0);
//#define scanlines
#ifdef scanlines
	dup->SetInputImage(thresholdedImage);
	dup->Update();
	typename ImageType::Pointer scan = dup->GetOutput();
	scan->DisconnectPipeline();
	scan->FillBuffer(0);
#endif

	const unsigned int blackSpaceBreak = 3;
	Point3D base = (baser + basel) * 0.5;
	Vector3D baseVector = baser - basel;
	double radius = baseVector.Length();
	Vector3D apexBase = apex - base;
	double apexBaseLength = apexBase.Length();
	apexBase.Normalise();
	Vector3D baseL = basel - base;
	baseL.Normalise();
	Vector3D baseR = baser - base;
	baseR.Normalise();

	ImageSizeType size = thresholdedImage->GetLargestPossibleRegion().GetSize();
	ImageSpacingType spacing = thresholdedImage->GetSpacing();
	ImageIndexType index;
	double heapSpace[1024];
	double* leftx = heapSpace;
	double* rightx = heapSpace + size[1];
	memset(leftx, 0, sizeof(double) * size[1] * 2);

	const double stepSize = 0.5;
	int baseEndY = -1, apexEndY = size[1];
	for (double x = 0; x <= apexBaseLength; x += stepSize) { //Move from base to apex, checking on either side to get boundaries
		Point3D abvp = base + x * apexBase;
		index[0] = abvp.x;
		index[1] = abvp.y;

		double initDist = 0.0;
		int blackSpace = 0;
		double l = 0;
		for (l = initDist; l < radius; l += stepSize) {
			Point3D left = abvp + l * baseL;
			index[0] = left.x;
			index[1] = left.y;

			if (thresholdedImage->GetPixel(index) == 0) {
				blackSpace++;
			}
			if (blackSpace > blackSpaceBreak) {
				blackSpace = 0;
				break;
			}
			lvchamber->SetPixel(index, 255);
#ifdef scanlines
			scan->SetPixel(index, 127);
#endif
		}
		if (l >= radius) //If line search went all the way to the end, track back until the first white spot
				{
			for (l = radius; l > 0; l -= stepSize) {
				Point3D left = abvp + l * baseL;
				index[0] = left.x;
				index[1] = left.y;

				if (thresholdedImage->GetPixel(index) != 0) {
					lvchamber->SetPixel(index, 0);
				} else {
					break;
				}

#ifdef scanlines
				if (scan->GetPixel(index) == 127)
					scan->SetPixel(index, 0);
#endif
			}
		}
		blackSpace = 0;
		for (l = initDist; l < radius; l += stepSize) {
			Point3D right = abvp + l * baseR;
			index[0] = right.x;
			index[1] = right.y;

			if (thresholdedImage->GetPixel(index) == 0) {
				blackSpace++;
			}
			if (blackSpace > blackSpaceBreak) {
				break;
			}
			lvchamber->SetPixel(index, 255);
#ifdef scanlines
			scan->SetPixel(index, 127);
#endif
		}
		if (l >= radius) {
			for (l = radius; l > 0; l -= stepSize) {
				Point3D right = abvp + l * baseR;
				index[0] = right.x;
				index[1] = right.y;

				if (thresholdedImage->GetPixel(index) != 0) {
					lvchamber->SetPixel(index, 0);
				} else {
					break;
				}
#ifdef scanlines
				if (scan->GetPixel(index) == 127)
					scan->SetPixel(index, 0);
#endif
			}
		}
	}

	ActiveContourSegmentation<ImageType> contour;
	Point3D seed = (apex + base) * 0.5;
	typename ImageType::Pointer activec = contour.getActiveContour(lvchamber, seed);
	typedef typename itk::BinaryImageToLabelMapFilter<ImageType> BinaryImageToLabelMapFilterType;
	typename BinaryImageToLabelMapFilterType::Pointer binaryImageToLabelMapFilter = BinaryImageToLabelMapFilterType::New();
	binaryImageToLabelMapFilter->SetInput(activec);
	binaryImageToLabelMapFilter->Update();

	unsigned int numObjects = binaryImageToLabelMapFilter->GetOutput()->GetNumberOfLabelObjects();

	//Get the largest object and determine the extreme left and right points
	int lsize = 0, idx = 0;
	for (int oc = 0; oc < numObjects; oc++) {
		typename BinaryImageToLabelMapFilterType::OutputImageType::LabelObjectType* labelObject = binaryImageToLabelMapFilter->GetOutput()->GetNthLabelObject(oc);
		if (labelObject->Size() > lsize) {
			lsize = labelObject->Size();
			idx = oc;
		}
	}
	typename BinaryImageToLabelMapFilterType::OutputImageType::LabelObjectType* labelObject = binaryImageToLabelMapFilter->GetOutput()->GetNthLabelObject(idx);
	typename BinaryImageToLabelMapFilterType::OutputImageType::IndexType lidx;
	for (unsigned int pixelId = 0; pixelId < labelObject->Size(); pixelId++) {
		lidx = labelObject->GetIndex(pixelId);
		unsigned int yc = lidx[1];
		if (leftx[yc] == 0) {
			leftx[yc] = lidx[0];
		}
		if (leftx[yc] > lidx[0])
			leftx[yc] = lidx[0];
		if (rightx[yc] < lidx[0])
			rightx[yc] = lidx[0];
	}

#ifdef scanlines
	static int filectr = 0;
	std::ostringstream ss;
	ss << "RScan" << filectr << ".jpg";
	typedef typename itk::ImageFileWriter<ImageType> WriterType;
	typename WriterType::Pointer writer = WriterType::New();
	writer->SetInput(scan);
	writer->SetFileName(ss.str());
	writer->Update();
	ss.str("");
	ss << "Active" << filectr++ << ".jpg";
	writer->SetInput(activec);
	writer->SetFileName(ss.str());
	writer->Update();

#endif

	std::vector<Point3D> result;
	unsigned int endl = basel.y, endr = baser.y;
	while (leftx[endl++] != 0)
		;
	while (rightx[endr++] != 0)
		;

	Vector3D xaxis(baseL);
	int tpts = 0;
	double theapSpace[2048];
	double* rad = theapSpace;
	double* tta = rad + 1024;
	int ctr = 0;
	rad[ctr] = radius / 2;
	tta[ctr++] = acos(baseL * xaxis);

	for (int i = basel.y + 1; i < endl; i++) {
		if (leftx[i] != 0) {
			Point3D bpts(leftx[i], i, 0);
			Vector3D bvec = bpts - base;
			double len = bvec.Length();
			bvec.Normalise();
			double theta = acos(bvec * xaxis);
			if (theta > tta[ctr - 1]) {
				rad[ctr] = len;
				tta[ctr++] = theta;
			}
		}
	}
	unsigned int end = baser.y - 2;
	for (int i = endr; i > end; i--) {
		if (rightx[i] != 0) {
			Point3D bpts(rightx[i], i, 0);
			Vector3D bvec = bpts - base;
			double len = bvec.Length();
			bvec.Normalise();
			double theta = acos(bvec * xaxis);
			if (theta > tta[ctr - 1]) {
				rad[ctr] = len;
				tta[ctr++] = theta;
			}
		}
	}
	//last point
	{
		double theta = acos(baseR * xaxis);
		rad[ctr] = radius / 2;
		tta[ctr++] = theta;
	}

	alglib::real_1d_array x, y;
	x.setcontent(ctr, tta);
	y.setcontent(ctr, rad);
	alglib::spline1dinterpolant s;

	// build spline
	try {
		alglib::spline1dbuildakima(x, y, s);
	} catch (alglib::ap_error& err) {
		std::cout << err.msg << std::endl;
	}
	double step = (tta[ctr - 1] - tta[0]) / numBdryPts;
	double tOffset = acos(-xaxis.x); //Since baseL vector is pointing in the opposite direction to image x-axis
	for (double st = tta[0]; st < tta[ctr - 1] + step; st += step) {
		double rad = spline1dcalc(s, st);
		double theta = st + tOffset;
		Point3D pts = Point3D(rad * cos(theta), rad * sin(theta), 0) + base;
		result.push_back(pts);
	}

	return result;

}

