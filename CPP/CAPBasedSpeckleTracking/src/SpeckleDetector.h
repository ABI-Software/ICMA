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


#ifndef SPECKLEDETECTOR_H_
#define SPECKLEDETECTOR_H_
#include "itkMaskImageFilter.h"
#include "DICOMInputManager.h"
#include "Point3D.h"
#include "itkMinimumMaximumImageCalculator.h"
#include "itkNormalizedCorrelationImageFilter.h"
#include "itkImageKernelOperator.h"
#include "itkRegionOfInterestImageFilter.h"
#include "alglib/interpolation.h"

#include "MarkerPlotter.h"


class SpeckleDetector
{
  typedef itk::RegionOfInterestImageFilter<DICOMSliceImageType, DICOMSliceImageType> RoiFilterType;
  typedef itk::MaskImageFilter<DICOMSliceImageType,DICOMSliceImageType> MaskFilterType;
  typedef itk::NormalizedCorrelationImageFilter<DICOMSliceImageType, DICOMSliceImageType, DICOMSliceImageType> CorrelationFilterType;
  typedef itk::MinimumMaximumImageCalculator<DICOMSliceImageType> MinimumMaximumImageCalculatorType;
  int speckleWidth;
  int speckleHeight;
  const double factor;
  DICOMSliceImageType::Pointer targetImage;
  DICOMSliceImageType::Pointer mask;
  DICOMSliceImageType::Pointer target;
  std::vector<DICOMSliceImageType::Pointer>
  getTargets(std::vector<Point3D>& origins);
  DICOMSliceImageType::IndexType
  matchSpeckleToPatch(DICOMSliceImageType::Pointer speckle, DICOMSliceImageType::SizeType speckleSize, DICOMSliceImageType::Pointer target,
      DICOMSliceImageType::SizeType targetSize);
public:
  SpeckleDetector(DICOMSliceImageType::Pointer target);
  virtual void
  setMask(DICOMSliceImageType::Pointer image);
  virtual std::vector<Point3D>
  trackMotion(std::vector<DICOMSliceImageType::Pointer>& speckles, std::vector<Point3D>& origins);
  virtual
  ~SpeckleDetector();

};

#endif /* SPECKLEDETECTOR_H_ */
