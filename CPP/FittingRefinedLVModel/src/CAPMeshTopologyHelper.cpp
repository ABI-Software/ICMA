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

#include "CAPMeshTopologyHelper.h"

//Here the cmiss_number is 1 more than the assigned number

const double CAPMeshTopologyHelper::elementAngles[NUMBER_OF_ELEMENTS] = { 0, 270, 180,
		90, 0, 270, 180, 90, 0, 270, 180, 90, 0, 270, 180, 90 };

const int CAPMeshTopologyHelper::leftNeighbor[NUMBER_OF_ELEMENTS] = { 2, 3, 4,
		1, 6, 7, 8, 5, 10, 11, 12, 9, 14, 15, 16, 13 };

const int CAPMeshTopologyHelper::rightNeighbor[NUMBER_OF_ELEMENTS] = { 4, 1, 2,
		3, 8, 5, 6, 7, 12, 9, 10, 11, 16, 13, 14, 15 };

//since LV mesh requires the element_ids to range from 0 - 15, where as cmiss_number starts from 1
//The material point correspondence is from the left base -> apex -> right base
const int CAPMeshTopologyHelper::aplax_cmiss_elements[NUMBER_OF_LONG_AXIS_MARKERS] =
		{ 3, 7, 11, 15, 15, 13, 9, 5, 1 /*, 3, 7, 11, 15, 15, 13, 9, 5, 1*/
		};

const int CAPMeshTopologyHelper::fch_cmiss_elements[NUMBER_OF_LONG_AXIS_MARKERS] =
		{ 2, 6, 10, 14, 14, 12, 8, 4, 0, /*, 2, 6, 10, 14, 14, 16, 12, 8, 4*/
		};

const int CAPMeshTopologyHelper::tch_cmiss_elements[NUMBER_OF_LONG_AXIS_MARKERS] =
		{ 3, 7, 11, 15, 15, 13, 9, 5, 1 /*, 3, 7, 11, 15, 15, 13, 9, 5, 1*/
		};

const int CAPMeshTopologyHelper::saxbase_cmiss_elements[NUMBER_OF_SHORT_AXIS_MARKERS] =
		{ 4,5,5,6,6,7,7,4
		};

int CAPMeshTopologyHelper::getElementNumber(MarkerTypes type,
		unsigned int index) {
	switch (type) {
	case APLAX:
		return aplax_cmiss_elements[index];
	case TCH:
		return tch_cmiss_elements[index];
	case FCH:
		return fch_cmiss_elements[index];
	case SAXBASE:
		return saxbase_cmiss_elements[index];
	case SAXMID:
		return saxmid_cmiss_elements[index];
	case SAXAPEX:
		return saxapex_cmiss_elements[index];

	}
}

Point3D CAPMeshTopologyHelper::getMaterialCoordinate(double angle,
		int* element) {
	int elem = *element; //This is cmiss number -1

	//Angle is expected to be in cmiss element units i.e. 0= 0, 1= M_PI/2
	double myAngle = -angle;
	double normAngle = 0.0;
	if(fabs(myAngle)<1.0e-6)
		myAngle = 0.0;
	if (myAngle >= 0.0) {
		normAngle = myAngle;
		while (normAngle > 1.0) {
			normAngle -= 1.0;
			*element = rightNeighbor[elem] - 1;
			elem = *element;
		}

	} else {
		normAngle = 1.0 + myAngle; //Since myAngle is -ve
		while (normAngle < 0.0) { //Happens when angle is greater than M_PI
			elem = leftNeighbor[elem] - 1;
			normAngle += 1.0;
			elem = *element;
		}
		*element = leftNeighbor[elem] - 1;
	}

	return Point3D(normAngle, 1.0, 0.0);
}

double CAPMeshTopologyHelper::getMCAngle(Point3D& mc, int element) {
	return elementAngles[element] + mc.x*90.0;
}

Point3D CAPMeshTopologyHelper::getMaterialCoordinateForGlobalAngle(double angle,
		int* element) {
	double targetAngle = angle - elementAngles[*element];
	return getMaterialCoordinate(targetAngle,element);
}
