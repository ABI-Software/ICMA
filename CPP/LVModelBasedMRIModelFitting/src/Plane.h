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

#ifndef PLANE_H_
#define PLANE_H_
#include "Vector3D.h"
#include "Point3D.h"

#include "vnl/vnl_matrix.h"
#include "vnl/algo/vnl_matrix_inverse.h"


typedef vnl_matrix<double> MatrixType;

class Plane {
public:
	Vector3D normal;
	double d;
	Point3D position;
	MatrixType transform;
	double pheight;	//Height of image plane
	double pwidth; //Width of image plane
	double iheight; //Height of image plane at which landmarks were determined
	double iwidth;  //Width of image plane at which landmarks were determined
	Plane();
	Plane(const Plane& pl);
	Plane(Point3D tlc, Point3D trc, Point3D blc, Point3D brc, double height, double width);
	void transformFrom(std::vector<Point3D> bls,Point3D apex);
};

inline Plane::Plane() {
	pwidth=0;
	pheight=0;
	iheight=0;
	iwidth=0;
	d=0;
}

inline Plane::Plane(const Plane& pl) {
	normal = pl.normal;
	d = pl.d;
	position = pl.position;
	pheight = pl.pheight;
	pwidth = pl.pwidth;
	iheight = pl.iheight;
	iwidth = pl.iwidth;
}

inline Plane::Plane(Point3D tlc, Point3D trc, Point3D blc, Point3D brc, double height, double width) {
	Vector3D v1 = trc - tlc;
	Vector3D v2 = blc - tlc;
	normal = CrossProduct(v1, v2);
	normal.Normalise();
	position = (tlc + trc + blc + brc) * 0.25; //Midpoint of the plane
	d = (blc-Point3D(0,0,0))*normal;
    pheight  = tlc.distance(blc);
    pwidth   = tlc.distance(trc);
    iheight = height;
    iwidth = width;
	//Determine the coordinate transformation
	MatrixType mat(4, 4), target(4, 4);
	mat.fill(1.0);
	target.fill(1.0);

	Point3D TLC(0,0,0), BLC(0,iheight,0), BRC(iwidth,iheight,0);

	Vector3D al = (blc - tlc);
	Vector3D ar = (brc - tlc);
	Vector3D Cp = CrossProduct(al, ar);
	Point3D c1 = tlc + Cp;

	Vector3D Al = (BLC - TLC);
	Vector3D Ar = (BRC - TLC);
	Vector3D cp = CrossProduct(Al, Ar);
	Point3D c2 = TLC + cp;


	mat[0][0] = TLC.x;
	mat[0][1] = BLC.x;
	mat[0][2] = BRC.x;
	mat[0][3] = c2.x;
	mat[1][0] = TLC.y;
	mat[1][1] = BLC.y;
	mat[1][2] = BRC.y;
	mat[1][3] = c2.y;
	mat[2][0] = TLC.z;
	mat[2][1] = BLC.z;
	mat[2][2] = BRC.z;
	mat[2][3] = c2.z;

	target[0][0] = tlc.x;
	target[0][1] = blc.x;
	target[0][2] = brc.x;
	target[0][3] = c1.x;
	target[1][0] = tlc.y;
	target[1][1] = blc.y;
	target[1][2] = brc.y;
	target[1][3] = c1.y;
	target[2][0] = tlc.z;
	target[2][1] = blc.z;
	target[2][2] = brc.z;
	target[2][3] = c1.z;

	transform = target * vnl_matrix_inverse<double>(mat);
}



#endif /* PLANE_H_ */
