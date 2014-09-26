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

#include "LVChamberCircle.h"

LVChamberCircle::LVChamberCircle(std::vector<Point3D> pts) : points(pts), area(0.0) {
	std::vector<Point3D> apts(points);
	apts.push_back(points[0]);
	unsigned int size = apts.size();
	//In the the mesh coordinate system y is along the LV chamber axis
	//Use (z,x)
	Point3D centroid(0,0,0);
	for(int i=0;i<size-1;i++){
		centroid = centroid + apts[i];
	}
	centroid = centroid * (-1.0/size);
	//Translate
	for(int i=0;i<size-1;i++){
		apts[i] = apts[i] + centroid;
	}
	area = 0.0;
	for(int i=0;i<size-1;i++){
		//Compute the area of the rectangle formed by the neighboring points with respect to the coordinate axis
		//This is fine since the points are normalized with centroid zero
		double temp = fabs((apts[i].z-apts[i+1].z)*(apts[i+1].x+apts[i].x)/2.0);
		area += temp;
	}
}

double LVChamberCircle::getArea() {
	return area;
}

double LVChamberCircle::avgHeight(LVChamberCircle& circle) {
	double height = 0.0;
	int npts = points.size();
	if(circle.points.size()<npts)
		npts = circle.points.size();
	for(int i=0;i<npts;i++){
		height += fabs(circle.points[i].y - points[i].y);
	}
	return height/npts;
}

double LVChamberCircle::getVolume(LVChamberCircle& circle) {
	double cArea = circle.getArea();
	return 0.5*(area+cArea)*avgHeight(circle);
}

double LVChamberCircle::getCircumference() {
	double perm = 0.0;
	std::vector<Point3D> apts(points);
	apts.push_back(points[0]);
	unsigned int size = apts.size();
	for(int i=0;i<size;i++){
		perm += apts[i].distance(apts[i+1]);
	}
	return perm;
}

LVChamberCircle::~LVChamberCircle() {

}

