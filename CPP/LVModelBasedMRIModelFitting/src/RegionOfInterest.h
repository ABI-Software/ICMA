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
#ifndef REGIONOFINTEREST_H_
#define REGIONOFINTEREST_H_
#include "itkAddImageFilter.h"
#include "itkSubtractImageFilter.h"
#include "itkDivideImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkStatisticsImageFilter.h"
#include "itkHoughTransform2DCirclesImageFilter.h"
#include "itkImageDuplicator.h"
#include "itkMedianImageFilter.h"
#include "itkMaskImageFilter.h"
#include "itkPolygonSpatialObject.h"
#include "itkSpatialObjectToImageFilter.h"
#include <vector>

template<typename ImageType>
class RegionOfInterest {
	typedef itk::Image<float,2> VImageType;
	VImageType::Pointer variance;
	std::vector<typename ImageType::Pointer> images;
	const double maxrad;
public:
	RegionOfInterest(std::vector<typename ImageType::Pointer> img);
	virtual ~RegionOfInterest(){};
	VImageType::Pointer getVarianceImage();
	std::vector<double> getROIBoundingBox(double cutoffRadius=10.0);
};



#ifndef ITK_MANUAL_INSTANTIATION
#include "RegionOfInterest.hxx"
#endif

#endif /* REGIONOFINTEREST_H_ */
