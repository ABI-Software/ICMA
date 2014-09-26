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

#ifndef COORDINATETRANSFORMER_H_
#define COORDINATETRANSFORMER_H_
#include "vnl/vnl_matrix.h"
#include "vnl/algo/vnl_determinant.h"
#include "vnl/vnl_det.h"
#include "vnl/algo/vnl_matrix_inverse.h"
#include "vnl/algo/vnl_symmetric_eigensystem.h"
#include "Point3D.h"
#include <vector>

#define NUMBER_OF_LONG_AXIS_MARKERS 9
#define NUMBER_OF_SHORT_AXIS_MARKERS 8

#define baserid 8
#define baselid 0
#define apexid 4

#define APLAXLEFTINDEX 6
#define APLAXRIGHTINDEX 2

#define TCHLEFTINDEX 5
#define TCHRIGHTINDEX 1

#define FCHLEFTINDEX 3
#define FCHRIGHTINDEX 7

enum MarkerTypes {
	APLAX = 1, TCH, FCH, SAXAPEX, SAXMID, SAXBASE
};

typedef vnl_matrix<double> MatrixType;

class CoordinateTransformer {
public:
	virtual Point3D getApex() = 0;
	virtual Point3D getBase() = 0;
	virtual std::vector<Point3D> getRVInserts() = 0;
	virtual Point3D getSaxCentroid() = 0;
	virtual unsigned int getNumberOfFrames() = 0;
	virtual std::vector<Point3D> getMarkers(unsigned int frame, MarkerTypes type = APLAX, bool transform = true) = 0;
	virtual std::vector<Point3D> getMaterialCoordinates(unsigned int frame, MarkerTypes type = APLAX) = 0;
	virtual std::vector<int> getCMISSElements(unsigned int frame, MarkerTypes type = APLAX) = 0;
	virtual std::vector<Point3D> getAllMarkers(unsigned int frame) = 0;
	virtual std::vector<Point3D> getAllMaterialCoordinates(unsigned int frame) = 0;
	virtual std::vector<int> getAllCMISSElements(unsigned int frame) = 0;
	virtual std::vector<int> getAllMarkerTypes(unsigned int frame) = 0;
	virtual unsigned int getBaseRIndex() = 0;
};

#endif /* COORDINATETRANSFORMER_H_ */
