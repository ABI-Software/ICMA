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


#ifndef SPECKLELIBRARY_H_
#define SPECKLELIBRARY_H_
#include "LVHeartMeshModel.h"
#include "DICOMInputManager.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkStatisticsImageFilter.h"
#include "alglib/interpolation.h"

#define SPECKLESTDCUTOFF 10.0 //This should be commensurate with the speckle size

class SpeckleLibrary
{
  typedef itk::RegionOfInterestImageFilter<DICOMSliceImageType, DICOMSliceImageType> RoiFilterType;
  typedef itk::StatisticsImageFilter<DICOMSliceImageType> StatisticsImageFilterType;
  const int MAXSPECKLES;
  DICOMSliceImageType::Pointer edImage;
  Point3D apex;
  Point3D basel;
  Point3D baser;
  LVHeartMeshModel * mesh;
  std::vector<Point3D> speckleCenters;
  std::vector<Point3D> currentBoundary;
  std::vector<int> cmissNodeIds;
  std::vector<DICOMSliceImageType::Pointer> speckles;
  int speckleHeight;
  int speckleWidth;
  double pixelcutoff;
  double boundarySpeckleFitness;
  double searchExtenstionFactor;
  double heapSpace[2048];
  void
  computeSpeckles();

public:
  SpeckleLibrary(LVHeartMeshModel* lvmesh, DICOMSliceImageType::Pointer image, int maxSpeckles);
  virtual
  ~SpeckleLibrary();
  void
  setApex(Point3D point);
  void
  setBaseL(Point3D point);
  void
  setBaseR(Point3D point);
  void
  setSpeckleSize(int sw, int sh);
  void
  Update();
  std::vector<DICOMSliceImageType::Pointer>&
  getSpeckles();
  std::vector<Point3D>&
  getSpeckleCenters();
  std::vector<Point3D>
  getBorderInitializers(DICOMSliceImageType::Pointer image, double time, int *apexIndex);
  std::vector<Point3D>
  getSpeckleInitializers(std::vector<Point3D>& border,int apidx);
  std::vector<Point3D>
  getCMISSMeshNodes(std::vector<Point3D>& speckles, int apexIndex);
  std::vector<Point3D>
  getCMISSSAXMeshNodes(std::vector<Point3D>& speckles,Point3D origin);
  std::vector<double>
  getModelSegmentLengths(std::vector<Point3D>& speckles);
  static std::vector<std::vector<double> >
  getSegmentStrains(std::vector<std::vector<double> > lengths, bool driftcorrected = true);
  double
  getBoundarySpeckleFitness();
};

#endif /* SPECKLELIBRARY_H_ */
