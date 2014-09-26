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

#include "itkPolygonSpatialObject.h"
#include "itkSpatialObjectToImageFilter.h"
#include "itkMaskImageFilter.h"
#include "itkOtsuThresholdImageFilter.h"
#include "itkInvertIntensityImageFilter.h"
#include "itkBinaryImageToShapeLabelMapFilter.h"
#include "itkLabelMapToLabelImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"

template<typename ImageType>
ThresholdBasedShortAxisBoundaryDetector<ImageType>::ThresholdBasedShortAxisBoundaryDetector(typename ImageType::Pointer img, Point3D cent, int npts) :
		image(img), centroid(cent), numBdryPts(npts) {
}

template<typename ImageType>
ThresholdBasedShortAxisBoundaryDetector<ImageType>::~ThresholdBasedShortAxisBoundaryDetector() {
}

template<typename ImageType>
std::vector<Point3D> ThresholdBasedShortAxisBoundaryDetector<ImageType>::getEndoBoundary(double& mRadius,double cutoffSaxradius,unsigned int roiw, unsigned int roih) {
	ImageThresholder<ImageType> thresholder(1);

	typedef typename itk::MaskImageFilter<ImageType, ImageType> MDaskImageFilterType;
	typedef itk::PolygonSpatialObject<2> PolygonType;
	PolygonType::Pointer polygon = PolygonType::New();
	PolygonType::PointType point;

	unsigned int xt = round(roiw);
	unsigned int yt = round(roih);

	polygon->ComputeObjectToWorldTransform();
	point[0] = centroid.x - xt;
	point[1] = centroid.y - yt;
	polygon->AddPoint(point);
	point[0] = centroid.x + xt;
	point[1] = centroid.y - yt;
	polygon->AddPoint(point);
	point[0] = centroid.x + xt;
	point[1] = centroid.x + yt;
	polygon->AddPoint(point);
	point[0] = centroid.x - xt;
	point[1] = centroid.x + yt;
	polygon->AddPoint(point);

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

	typedef typename itk::OtsuThresholdImageFilter<ImageType, ImageType> FilterType;
	typename FilterType::Pointer otsuFilter = FilterType::New();
	otsuFilter->SetInput(image);

	typedef typename itk::InvertIntensityImageFilter<ImageType> InvertIntensityImageFilterType;
	typename InvertIntensityImageFilterType::Pointer invertIntensityFilter = InvertIntensityImageFilterType::New();
	invertIntensityFilter->SetMaximum(255);
	invertIntensityFilter->SetInput(otsuFilter->GetOutput());

	mdmasker->SetInput(invertIntensityFilter->GetOutput());
	mdmasker->SetMaskImage(imageFilter->GetOutput());
	mdmasker->Update();

	typedef typename itk::BinaryImageToShapeLabelMapFilter<ImageType> BinaryImageToShapeLabelMapFilterType;
	typename BinaryImageToShapeLabelMapFilterType::Pointer binaryImageToShapeLabelMapFilter = BinaryImageToShapeLabelMapFilterType::New();
	binaryImageToShapeLabelMapFilter->SetInput(mdmasker->GetOutput());
	binaryImageToShapeLabelMapFilter->Update();

	// The output of this filter is an itk::ShapeLabelMap, which contains itk::ShapeLabelObject's
	unsigned int numObj = binaryImageToShapeLabelMapFilter->GetOutput()->GetNumberOfLabelObjects();

	std::vector<double> distance(numObj);
	std::vector<unsigned long> labelsToRemove(numObj);
	// Loop over all of the blobs
	for (unsigned int i = 0; i < numObj; i++) {
		typename BinaryImageToShapeLabelMapFilterType::OutputImageType::LabelObjectType* labelObject = binaryImageToShapeLabelMapFilter->GetOutput()->GetNthLabelObject(i);
		typename BinaryImageToShapeLabelMapFilterType::OutputImageType::LabelObjectType::CentroidType cent = labelObject->GetCentroid();
		Point3D lcent(cent[0], cent[1], 0);
		distance[i] = lcent.distance(centroid);
		labelsToRemove[i] = labelObject->GetLabel();
	}
	const double cutoff = cutoffSaxradius;//10;
	for (unsigned int i = 0; i < numObj; i++) {
		if (distance[i] > cutoff)
			binaryImageToShapeLabelMapFilter->GetOutput()->RemoveLabel(labelsToRemove[i]);
	}

	typedef typename itk::LabelMapToLabelImageFilter<typename BinaryImageToShapeLabelMapFilterType::OutputImageType, ImageType> LabelMapToLabelImageFilterType;
	typename LabelMapToLabelImageFilterType::Pointer labelMapToLabelImageFilter = LabelMapToLabelImageFilterType::New();
	labelMapToLabelImageFilter->SetInput(binaryImageToShapeLabelMapFilter->GetOutput());
	labelMapToLabelImageFilter->Update();

	typedef typename itk::RescaleIntensityImageFilter<ImageType, ImageType> RescaleFilterType;
	typename RescaleFilterType::Pointer rescaleFilter = RescaleFilterType::New();
	rescaleFilter->SetInput(labelMapToLabelImageFilter->GetOutput());
	rescaleFilter->SetOutputMinimum(0);
	rescaleFilter->SetOutputMaximum(255);
	rescaleFilter->Update();
	ActiveContourSegmentation<ImageType> contour;
	typename ImageType::Pointer activec = contour.getActiveContour(rescaleFilter->GetOutput(), centroid);

	binaryImageToShapeLabelMapFilter->SetInput(activec);
	binaryImageToShapeLabelMapFilter->Update();
	numObj = binaryImageToShapeLabelMapFilter->GetOutput()->GetNumberOfLabelObjects();
	std::vector<Point3D> result;
	if (numObj == 1) {
		typename BinaryImageToShapeLabelMapFilterType::OutputImageType::LabelObjectType* labelObject = binaryImageToShapeLabelMapFilter->GetOutput()->GetNthLabelObject(0);
		typename BinaryImageToShapeLabelMapFilterType::OutputImageType::LabelObjectType::VectorType ed = labelObject->GetEquivalentEllipsoidDiameter();
		typename BinaryImageToShapeLabelMapFilterType::OutputImageType::LabelObjectType::CentroidType cent = labelObject->GetCentroid();
		double step = 2 * M_PI / numBdryPts;
		double xd = ed[0] / 2;
		double yd = ed[1] / 2;
		for (double th = 0; th < 2 * M_PI + step; th += step) {
			double x = xd * cos(th) + cent[0];
			double y = yd * sin(th) + cent[1];
			typename ImageType::IndexType pindex;
			pindex[0] = x;
			pindex[1] = y;
			int ctr = 1;
			while (activec->GetPixel(pindex) == 255) {
				pindex[0] = (xd + ctr) * cos(th) + cent[0];
				pindex[1] = (yd + ctr++) * sin(th) + cent[1];
			}
			ctr = 1;
			while (activec->GetPixel(pindex) == 0) {
				pindex[0] = (xd - ctr) * cos(th) + cent[0];
				pindex[1] = (yd - ctr++) * sin(th) + cent[1];
			}
			//Check of the point is within the region of interest
			Point3D dp(pindex[0], pindex[1], 0.0);
			double centr = dp.distance(centroid);
			if (centr <= roih && centr <= roiw)
				result.push_back(Point3D(pindex[0], pindex[1], 0.0));
		}

	} else {
		throw -1;
	}
	//Smooth the result
	//Only if at least more than half boundary points are detected
	if (result.size() > numBdryPts / 2) {
		double theta[128], rad[128];
		Point3D centroid;
		unsigned int numpts = result.size();
		for (int i = 0; i < numpts; i++) {
			centroid = centroid + result[i];
		}
		centroid = centroid * (1.0 / numpts);

		for (int i = 0; i < numpts; i++) {
			theta[i] = atan2((result[i].y - centroid.y), (result[i].x - centroid.x));
			rad[i] = centroid.distance(result[i]);
		}

		alglib::real_1d_array x, y;
		x.setcontent(numpts, theta);
		y.setcontent(numpts, rad);
		alglib::ae_int_t info;
		alglib::spline1dinterpolant s;
		alglib::spline1dfitreport rep;
		const double rho = 10;

		// build penalized regression spline
		try {
			alglib::spline1dfitpenalized(x, y, 50, rho, info, s, rep);
		} catch (alglib::ap_error& err) {
			std::cout << err.msg << std::endl;
		}

		mRadius = 0.0;
		for (int i = 0; i < numpts; i++) {
			double mrad = alglib::spline1dcalc(s,theta[i]);
			result[i].x = mrad*cos(theta[i]) + centroid.x;
			result[i].y = mrad*sin(theta[i]) + centroid.y;
			mRadius +=mrad;
		}
		mRadius /= numpts;
	} else {
		result = std::vector<Point3D>(0);
		mRadius = 0.0;
	}

	return result;
}

