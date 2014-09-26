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


#include "SpeckleDetector.h"
#include <ctime>

SpeckleDetector::SpeckleDetector(DICOMSliceImageType::Pointer target) :
		targetImage(target), speckleWidth(10), speckleHeight(10), factor(2) {
	this->target = target;
}

SpeckleDetector::~SpeckleDetector() {

}

void SpeckleDetector::setMask(DICOMSliceImageType::Pointer image) {
	mask = image;
	//Create masked target
	MaskFilterType::Pointer maskFilter = MaskFilterType::New();
	maskFilter->SetInput(targetImage);
	maskFilter->SetMaskImage(mask);
	maskFilter->Update();
	target = maskFilter->GetOutput();
	target->DisconnectPipeline();
}

std::vector<Point3D> SpeckleDetector::trackMotion(std::vector<DICOMSliceImageType::Pointer>& speckles, std::vector<Point3D>& origins) {
	unsigned int npts = origins.size();
	if (speckles.size() < npts)
		npts = speckles.size();

	std::vector<Point3D> result(origins);
	DICOMSliceImageType::SizeType speckleSize = speckles[0]->GetLargestPossibleRegion().GetSize();
	DICOMSliceImageType::SpacingType spacing = speckles[0]->GetSpacing();
	speckleWidth = speckleSize[0];
	speckleHeight = speckleSize[1];
	std::vector<DICOMSliceImageType::Pointer> targets = getTargets(origins);
	DICOMSliceImageType::SizeType targetSize = targets[0]->GetLargestPossibleRegion().GetSize();
	for (int i = 0; i < npts; i++) {
		DICOMSliceImageType::IndexType mIdx = matchSpeckleToPatch(speckles[i], speckleSize, targets[i], targetSize);
		result[i].x += mIdx[0];
		result[i].y += mIdx[1];
	}
	return result;
}

std::vector<DICOMSliceImageType::Pointer> SpeckleDetector::getTargets(std::vector<Point3D>& speckleCenters) {
	std::vector<DICOMSliceImageType::Pointer> speckles;
	DICOMSliceImageType::SizeType dSize;
	DICOMSliceImageType::RegionType desiredRegion;
	DICOMSliceImageType::Pointer result;
	DICOMSliceImageType::SizeType targetSize = target->GetLargestPossibleRegion().GetSize();
	unsigned int numSpeckles = speckleCenters.size();
	RoiFilterType::Pointer roiFilter = RoiFilterType::New();


	const double swd = speckleWidth + 10;
	const double shd = speckleHeight + 10;
	//Set the origin to 0,0
	const double origin[2] = { 0.0, 0.0 };

	for (int i = 0; i < numSpeckles; i++) {
		dSize[0] = swd;
		dSize[1] = shd;
		DICOMSliceImageType::IndexType blidx;
		blidx[0] = speckleCenters[i].x - dSize[0] / 2;
		blidx[1] = speckleCenters[i].y - dSize[1] / 2;

		if (speckleCenters[i].x > targetSize[0] || speckleCenters[i].y > targetSize[1]) {
			//The speckle is outside the image size
			//Ignore and create a zero filled image
			DICOMSliceImageType::Pointer dummy = DICOMSliceImageType::New();
			typename DICOMSliceImageType::IndexType corner = {{-swd/2,-shd/2}};
			desiredRegion.SetSize(dSize);
			desiredRegion.SetIndex(corner);
			dummy->SetRegions(desiredRegion);
			dummy->Allocate();
			dummy->FillBuffer(0.0);
			dummy->SetOrigin(origin);
			speckles.push_back(dummy);
			continue;
		}

		try {
			if ((blidx[0] + dSize[0]) > targetSize[0]) {
				dSize[0] = (targetSize[0] - blidx[0]) - 1;
			}
			if ((blidx[1] + dSize[1]) > targetSize[1]) {
				dSize[1] = (targetSize[1] - blidx[1]) - 1;
			}
			if (dSize[0] > targetSize[0]) {
				std::cerr << "for speckle " << speckleCenters[i] << "\t" << i << "\t";
				std::cerr << "speckleBound computing error dsize " << dSize << "\t Blidx " << blidx << "\t Target Size " << targetSize << std::endl;
				dSize[0] = targetSize[0] - 1;
			}
			if (dSize[1] > targetSize[1]) {
				std::cerr << "for speckle " << speckleCenters[i] << "\t" << i << "\t";
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
			roiFilter->SetInput(target);
			roiFilter->Update();
			result = roiFilter->GetOutput();
			result->DisconnectPipeline();

			result->SetOrigin(origin);

		} catch (itk::ExceptionObject& excp) {
#ifdef debug
			std::ostringstream ss;
			ss << "Radius " << std::endl << speckleWidth << ", " << speckleHeight << std::endl
			<< "Desired Size" << std::endl << dSize[0] << ", " << dSize[1] << std::endl;
			ss << "Requesting region \n" << desiredRegion << "Target region \n"
			<< target->GetLargestPossibleRegion() << std::endl;
			ss << "***** Error occurred "<<excp.what()<< std::endl;
			std::cout << ss.str() << std::endl;
#endif
			throw excp;
		}
		speckles.push_back(result);
	}
	return speckles;
}

DICOMSliceImageType::IndexType SpeckleDetector::matchSpeckleToPatch(DICOMSliceImageType::Pointer speckle, DICOMSliceImageType::SizeType speckleSize,
		DICOMSliceImageType::Pointer target, DICOMSliceImageType::SizeType targetSize) {
	itk::ImageKernelOperator<double> kernelOperator;
	kernelOperator.SetImageKernel(speckle);

	itk::Size < 2 > radius = speckleSize;
	radius[0] = (radius[0] - 1) / 2;
	radius[1] = (radius[1] - 1) / 2;

	kernelOperator.CreateToRadius(radius);

	CorrelationFilterType::Pointer correlationFilter = CorrelationFilterType::New();
	correlationFilter->SetInput(target);
	correlationFilter->SetTemplate(kernelOperator);
	correlationFilter->Update();

	MinimumMaximumImageCalculatorType::Pointer minimumMaximumImageCalculatorFilter = MinimumMaximumImageCalculatorType::New();
	minimumMaximumImageCalculatorFilter->SetImage(correlationFilter->GetOutput());
	minimumMaximumImageCalculatorFilter->Compute();

	itk::Index < 2 > maximumCorrelationPatchCenter = minimumMaximumImageCalculatorFilter->GetIndexOfMaximum();
	maximumCorrelationPatchCenter[0] -= speckleWidth;
	maximumCorrelationPatchCenter[1] -= speckleHeight;

	return maximumCorrelationPatchCenter;
}
