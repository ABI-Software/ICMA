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


#include "MotionEstimator.h"

MotionEstimator::MotionEstimator(int mtype) :
		type(mtype) {

}

MotionEstimator::~MotionEstimator() {

}

std::vector<Point3D> MotionEstimator::getMotionForPoint(std::vector<Point3D>& init, std::vector<Point3D>& detect) {
	unsigned int npts = init.size();
	std::vector<Point3D> results;
	for (int i = 0; i < npts; i += 3) {
		alglib::rbfmodel modelx, modely;
		alglib::rbfreport repx, repy;
		double vx, vy;
		alglib::rbfcreate(2, 1, modelx);
		std::string matrix = getXRBFInitializer(i, npts, init, detect);
		alglib::real_2d_array xyx(matrix.c_str());
		alglib::rbfsetpoints(modelx, xyx);
		alglib::rbfsetalgomultilayer(modelx, 5.0, 5, 1.0e-3);
		alglib::rbfbuildmodel(modelx, repx);
		vx = alglib::rbfcalc2(modelx, 0.0, 0.0);
		if (fabs(vx) > 50) {
			std::cout << "MOTION ESTIMATOR X\t" << matrix << std::endl;
			vx = 0.0;
		}

		alglib::rbfcreate(2, 1, modely);
		matrix = getYRBFInitializer(i, npts, init, detect);
		alglib::real_2d_array xyy(matrix.c_str());
		alglib::rbfsetpoints(modely, xyy);
		alglib::rbfsetalgomultilayer(modely, 5.0, 5, 1.0e-3);
		alglib::rbfbuildmodel(modely, repy);
		vy = alglib::rbfcalc2(modely, 0.0, 0.0);
		if (fabs(vy) > 50) {
			std::cout << "MOTION ESTIMATOR Y\t" << matrix << std::endl;
			vy = 0.0;
		}
		Point3D mv(vx, vy, 0.0);
		results.push_back(mv);
	}
	return results;
}

std::string MotionEstimator::getXRBFInitializer(int index, int max, std::vector<Point3D>& init, std::vector<Point3D>& detect) {
	std::ostringstream ss;
	int x[] = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
	int y[] = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };

	if (index > 0 && index < max - 3) {
		ss << "[";
		int ctr = 0;
		for (int i = index - 3; i < index + 5; i++) {
			ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].x - detect[i].x << "],";
			ctr++;
		}
		ss << "[" << x[ctr] << "," << y[ctr] << "," << init[index + 5].x - detect[index + 5].x << "]]";
	} else {
		if (type == 0) { //For l
			if (index == 0) {
				ss << "[";
				int ctr = 0;
				for (int i = index; i < index + 3; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].x - detect[i].x << "],";
					ctr++;
				}
				for (int i = index; i < index + 5; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].x - detect[i].x << "],";
					ctr++;
				}
				ss << "[" << x[ctr] << "," << y[ctr] << "," << init[index + 5].x - detect[index + 5].x << "]]";
			} else {
				ss << "[";
				int ctr = 0;
				for (int i = index - 3; i < index + 3; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].x - detect[i].x << "],";
					ctr++;
				}
				for (int i = index; i < index + 2; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].x - detect[i].x << "],";
					ctr++;
				}
				ss << "[" << x[ctr] << "," << y[ctr] << "," << init[index + 2].x - detect[index + 2].x << "]]";
			}
		} else {
			if (index == 0) {
				ss << "[";
				int ctr = 0;
				for (int i = max - 3; i < max; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].x - detect[i].x << "],";
					ctr++;
				}
				for (int i = index; i < index + 5; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].x - detect[i].x << "],";
					ctr++;
				}
				ss << "[" << x[ctr] << "," << y[ctr] << "," << init[index + 5].x - detect[index + 5].x << "]]";
			} else {
				ss << "[";
				int ctr = 0;
				for (int i = index - 3; i < index + 3; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].x - detect[i].x << "],";
					ctr++;
				}
				for (int i = 0; i < 3; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].x - detect[i].x << "],";
					ctr++;
				}
				ss << "[" << x[ctr] << "," << y[ctr] << "," << init[index + 2].x - detect[index + 2].x << "]]";
			}
		}
	}
	return ss.str();
}

std::string MotionEstimator::getYRBFInitializer(int index, int max, std::vector<Point3D>& init, std::vector<Point3D>& detect) {
	std::ostringstream ss;
	int x[] = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
	int y[] = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };
	if (index > 0 && index < max - 3) {
		ss << "[";
		int ctr = 0;
		for (int i = index - 3; i < index + 5; i++) {
			ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].y - detect[i].y << "],";
			ctr++;
		}
		ss << "[" << x[ctr] << "," << y[ctr] << "," << init[index + 5].y - detect[index + 5].y << "]]";
	} else {
		if (type == 0) {
			if (index == 0) {
				ss << "[";
				int ctr = 0;
				for (int i = index; i < index + 3; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].y - detect[i].y << "],";
					ctr++;
				}
				for (int i = index; i < index + 5; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].y - detect[i].y << "],";
					ctr++;
				}
				ss << "[" << x[ctr] << "," << y[ctr] << "," << init[index + 5].y - detect[index + 5].y << "]]";
			} else {
				ss << "[";
				int ctr = 0;
				for (int i = index - 3; i < index + 3; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].y - detect[i].y << "],";
					ctr++;
				}
				for (int i = index; i < index + 2; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].y - detect[i].y << "],";
					ctr++;
				}
				ss << "[" << x[ctr] << "," << y[ctr] << "," << init[index + 2].y - detect[index + 2].y << "]]";
			}
		} else {
			if (index == 0) {
				ss << "[";
				int ctr = 0;
				for (int i = max-3; i < max; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].y - detect[i].y << "],";
					ctr++;
				}
				for (int i = index; i < index + 5; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].y - detect[i].y << "],";
					ctr++;
				}
				ss << "[" << x[ctr] << "," << y[ctr] << "," << init[index + 5].y - detect[index + 5].y << "]]";
			} else {
				ss << "[";
				int ctr = 0;
				for (int i = index - 3; i < index + 3; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].y - detect[i].y << "],";
					ctr++;
				}
				for (int i = 0; i < 3; i++) {
					ss << "[" << x[ctr] << "," << y[ctr] << "," << init[i].y - detect[i].y << "],";
					ctr++;
				}
				ss << "[" << x[ctr] << "," << y[ctr] << "," << init[index + 2].y - detect[index + 2].y << "]]";
			}
		}
	}
	return ss.str();
}
