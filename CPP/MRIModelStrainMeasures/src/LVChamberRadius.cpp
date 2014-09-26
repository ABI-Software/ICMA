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
#include "LVChamberRadius.h"

LVChamberRadius::LVChamberRadius(std::vector<Point3D> boundary, int apexIndex) {
	unsigned int maxpts = boundary.size();
	apex = boundary[apexIndex];
	Point3D base = (boundary[0] + boundary[maxpts - 1]) * 0.5;
	apexBase = base - apex;
	apexBase.Normalise();
	double index[512], rad[512];

	for (int i = 0; i < apexIndex; i++) {
		Vector3D vec = boundary[i] - apex;
		index[i] = apexBase * vec;
		double vl = vec.Length();
		rad[i] = sqrt(vl * vl - index[i]*index[i]); //Height
	}
	index[apexIndex] = 0.0;
	rad[apexIndex] = 0.0;
	alglib::real_1d_array x, y;
	x.setcontent(apexIndex + 1, index);
	y.setcontent(apexIndex + 1, rad);
	// build spline
	try {
		alglib::spline1dbuildakima(x, y, trad);
	} catch (alglib::ap_error& err) {
		std::cout << err.msg << std::endl;
	}
	index[0] = 0.0;
	rad[0] = 0.0;
	int ctr = 1;
	for (int i = apexIndex + 1; i < maxpts; i++) {
		Vector3D vec = boundary[i] - apex;
		index[ctr] = apexBase * vec;
		double vl = vec.Length();
		rad[ctr] = sqrt(vl * vl - index[ctr]*index[ctr]); //Height
		ctr++;
	}
	alglib::real_1d_array x1, y1;
	x1.setcontent(ctr, index);
	y1.setcontent(ctr, rad);

	// build spline
	try {
		alglib::spline1dbuildakima(x1, y1, brad);
	} catch (alglib::ap_error& err) {
		std::cout << err.msg << std::endl;
	}
}

LVChamberRadius::~LVChamberRadius() {

}

void LVChamberRadius::getRadiiAt(Point3D pt, double& rad1, double& rad2) {
	Vector3D vec = pt - apex;
	double dist = apexBase * vec;
	rad1 = alglib::spline1dcalc(trad,dist);
	rad2 = alglib::spline1dcalc(brad,dist);
}
