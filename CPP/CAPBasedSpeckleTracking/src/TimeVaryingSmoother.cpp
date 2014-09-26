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

#include "TimeVaryingSmoother.h"

TimeVaryingSmoother::TimeVaryingSmoother(std::string fVector, double smoothing, bool fixApex) {
	frameVector = fVector;
	numberOfTimeSamples = 0;
	rho = smoothing;
	this->fixApex = fixApex;
	numbasis = 100;
	std::vector<std::string> stimepoints;
	boost::split(stimepoints, frameVector, boost::is_any_of(","));
	for (int p = 0; p < stimepoints.size(); p++) {
		timepoints.push_back(atof(stimepoints[p].c_str()));
	}
}

TimeVaryingSmoother::~TimeVaryingSmoother() {

}

void TimeVaryingSmoother::add(std::vector<Point3D> markers) {
	points.push_back(markers);
	numberOfTimeSamples++;
}

int TimeVaryingSmoother::getNumberOfTimeSamples() {
	return numberOfTimeSamples;
}

std::vector<std::vector<Point3D> > TimeVaryingSmoother::getSmoothedPoints() {
	double heap[2048];
	double *x = heap;
	double *y = heap + numberOfTimeSamples;
	double * index = y + numberOfTimeSamples;
	std::vector<Point3D> intermediate(numberOfTimeSamples);
	std::vector<std::vector<Point3D> > result(points);
	std::vector<Point3D>& firstTime = points[0];
	unsigned int numSpeckles = firstTime.size();

	alglib::ae_int_t info;
	alglib::spline1dinterpolant xs;
	alglib::spline1dinterpolant ys;
	alglib::spline1dfitreport rep;
	int apexIndex = numSpeckles / 2;
	int step = apexIndex / 4.0;

	for (int spc = 0; spc < numSpeckles; spc++) {
		for (int p = 0; p < numberOfTimeSamples; p++) {
			index[p] = timepoints[p];
			x[p] = result[p][spc].x;
			y[p] = result[p][spc].y;
			if (x[p] < 0) {
				std::cout << "Error with marker " << spc << " at " << p << "\t" << result[p][spc] << std::endl;
			}
		}
		alglib::real_1d_array indexv;
		indexv.setcontent(numberOfTimeSamples, index);
		alglib::real_1d_array xv;
		xv.setcontent(numberOfTimeSamples, x);
		alglib::real_1d_array yv;
		yv.setcontent(numberOfTimeSamples, y);
#ifndef penalizedfit
		alglib::ae_int_t m = 7; //polynomial order 6
		double t = 2;
		alglib::ae_int_t info;
		alglib::barycentricinterpolant xs, ys;
		alglib::polynomialfitreport rep;
		try {
			alglib::polynomialfit(indexv, xv, m, info, xs, rep);
			alglib::polynomialfit(indexv, yv, m, info, ys, rep);

#ifdef generatestrainfittingoutput
			if (fixApex)
			{
				std::cout << spc;
				for (int p = 0; p < numberOfTimeSamples; p++)
				{
					double tp = atof(timepoints[p].c_str());
					Point3D ptp(alglib::barycentriccalc(xs, tp), alglib::barycentriccalc(ys, tp), 0.0);
					std::cout << "\t" << ptp;
				}
				std::cout << std::endl;
			}
#endif
			//Reduce the motion near the apex, since the apical speckle strain uses the apex, and the following two nodepoints
			//the motion is reduced in a graded fashion
			double solu[] = { 0.10671, 0.39580, 0.55000 };

			if (fixApex) {

				if (spc > apexIndex - 3 * step - 1 && spc < apexIndex + 3 * step + 1) {
					double factor = solu[0];          //0.12913; //0.07802; //0.1
					if (spc > apexIndex - 2 * step - 1 && spc < apexIndex + 2 * step + 1) {
						factor = solu[1];          //0.54172; //0.33071; //2.0/5.0
						if (spc > apexIndex - step - 1 && spc < apexIndex + step + 1) {
							factor = solu[2];          //0.6; //0.6; //2.0/3.0
						}
					}

					double ifactor = 1.0 - factor;
					result[0][spc].x = alglib::barycentriccalc(xs, 0);
					result[0][spc].y = alglib::barycentriccalc(ys, 0);
					Point3D origin = result[0][spc];
					for (int p = 1; p < numberOfTimeSamples; p++) {
						double tp = timepoints[p];
						Point3D ptp(alglib::barycentriccalc(xs, tp), alglib::barycentriccalc(ys, tp), 0.0);
						result[p][spc] = (origin * factor + ptp * ifactor);
					}
				}
			} else {
				for (int p = 0; p < numberOfTimeSamples; p++) {
					double tp = timepoints[p];
					result[p][spc].x = alglib::barycentriccalc(xs, tp);
					result[p][spc].y = alglib::barycentriccalc(ys, tp);
				}
			}

		} catch (alglib::ap_error& err) {
			std::cout << err.msg << std::endl;
		}
	}

#else
	try
	{

		alglib::spline1dfitpenalized(indexv, xv, numbasis, rho, info, xs, rep);
		alglib::spline1dfitpenalized(indexv, yv, numbasis, rho, info, ys, rep);

#ifdef generatestrainfittingoutput
		if (fixApex)
		{
			std::cout << spc;
			for (int p = 0; p < numberOfTimeSamples; p++)
			{
				double tp = atof(timepoints[p].c_str());
				Point3D ptp(alglib::spline1dcalc(xs, tp), alglib::spline1dcalc(ys, tp), 0.0);
				std::cout << "\t" << ptp;
			}
			std::cout << std::endl;
		}
#endif
		//Reduce the motion near the apex, since the apical speckle strain uses the apex, and the following two nodepoints
		//the motion is reduced in a graded fashion
		double solu[] =
		{	0.10671, 0.39580, 0.55000};

		if (fixApex)
		{

			if (spc > apexIndex - 3 * step - 1 && spc < apexIndex + 3 * step + 1)
			{
				double factor = solu[0];          //0.12913; //0.07802; //0.1
				if (spc > apexIndex - 2 * step - 1 && spc < apexIndex + 2 * step + 1)
				{
					factor = solu[1];          //0.54172; //0.33071; //2.0/5.0
					if (spc > apexIndex - step - 1 && spc < apexIndex + step + 1)
					{
						factor = solu[2];          //0.6; //0.6; //2.0/3.0
					}
				}

				double ifactor = 1.0 - factor;
				result[0][spc].x = alglib::spline1dcalc(xs, 0);
				result[0][spc].y = alglib::spline1dcalc(ys, 0);
				Point3D origin = result[0][spc];
				for (int p = 1; p < numberOfTimeSamples; p++)
				{
					double tp = timepoints[p];
					Point3D ptp(alglib::spline1dcalc(xs, tp), alglib::spline1dcalc(ys, tp), 0.0);
					result[p][spc] = (origin * factor + ptp * ifactor);
				}
			}
		}
		else
		{
			for (int p = 0; p < numberOfTimeSamples; p++)
			{
				double tp = timepoints[p];
				result[p][spc].x = alglib::spline1dcalc(xs, tp);
				result[p][spc].y = alglib::spline1dcalc(ys, tp);
			}
		}

	}
	catch (alglib::ap_error& err)
	{
		std::cout << err.msg << std::endl;
	}
}
#endif
	return result;
}

void TimeVaryingSmoother::biasBoundary() {
	numberOfTimeSamples++;
	points.push_back(points[0]);
	timepoints.push_back(timepoints[timepoints.size() - 1] + 0.1);
}

void TimeVaryingSmoother::setNumBasis(int numBasis) {
	numbasis = numBasis;
}
