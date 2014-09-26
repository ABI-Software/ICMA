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

#ifndef SAXBASEDCOORDINATETRANSFORMER_H_
#define SAXBASEDCOORDINATETRANSFORMER_H_
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <iostream>
#include "Point3D.h"
#include "Vector3D.h"
#include "XMLInputReader.h"
#include "alglib/optimization.h"
#include "alglib/interpolation.h"
#include "vnl/vnl_matrix.h"
#include "vnl/algo/vnl_matrix_inverse.h"

#define APEXINDEX 4
#define LBASEINDEX 0
#define RBASEINDEX 8


typedef vnl_matrix<double> MatrixType;

typedef struct {
	Point3D base;
	Point3D saxCentroid;
	std::vector<Point3D> saxl;
	std::vector<Point3D> saxr;
	std::vector<Point3D> viewl;
	std::vector<Point3D> viewr;
} TransformationOptimizationInput;

class SAXBasedCoordinateTransformer {
	std::vector<Point3D>& saxMarkers;
	std::vector<std::vector<Point3D> > frameMarkers;
	double saxPlaneDistance;
	MatrixType transform;
	Point3D saxCentroid;
	double* rad;
	double* theta;
	void computeTransform();
	Point3D getLeftSaxInterSection(int view);
	Point3D getRightSaxInterSection(int view);
	static void transformationObjective(const alglib::real_1d_array &x, double &func, void *ptr);
public:

	SAXBasedCoordinateTransformer(std::vector<Point3D>& sm, std::vector<std::vector<Point3D> >& fm,double saxPlaneDist);
	std::vector<Point3D> applyTransform(std::vector<Point3D>& markers);
	std::vector<std::vector<Point3D>* > getTransformedCoordinates();
	MatrixType getTransform();
	void printTransform();
	virtual ~SAXBasedCoordinateTransformer();
};

#endif /* SAXBASEDCOORDINATETRANSFORMER_H_ */
