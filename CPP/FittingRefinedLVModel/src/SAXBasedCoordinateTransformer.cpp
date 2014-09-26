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

#include "SAXBasedCoordinateTransformer.h"
//#define debug
SAXBasedCoordinateTransformer::SAXBasedCoordinateTransformer(std::vector<Point3D>& sm, std::vector<std::vector<Point3D> >& fm, double saxPlaneDist) :
		saxMarkers(sm), frameMarkers(fm), saxPlaneDistance(saxPlaneDist) {
	saxCentroid.x = 0;
	saxCentroid.y = 0;
	saxCentroid.z = 0;
	rad = new double[saxMarkers.size()];
	theta = new double[saxMarkers.size()];

	computeTransform();
}

std::vector<Point3D> SAXBasedCoordinateTransformer::applyTransform(std::vector<Point3D>& markers) {
	std::vector<Point3D> result(markers);
	unsigned int numMarkers = markers.size();
	for (int i = 0; i < numMarkers; i++)
	{
		double x = result[i].x;
		double y = result[i].y;
		double z = result[i].z;
		double mx = transform[0][0] * x + transform[0][1] * y + transform[0][2] * z + transform[0][3];
		double my = transform[1][0] * x + transform[1][1] * y + transform[1][2] * z + transform[1][3];
		double mz = transform[2][0] * x + transform[2][1] * y + transform[2][2] * z + transform[2][3];
		result[i] = Point3D(mx, my, mz);
	}
	return result;
}

void SAXBasedCoordinateTransformer::computeTransform() {
	std::vector<Point3D> iptCoord(saxMarkers);
	unsigned int numSaxMarkers = saxMarkers.size();
#ifdef debug
	std::cout<<"Sax markers"<<std::endl;
#endif
	for (int i = 0; i < numSaxMarkers; i++)
	{
		saxCentroid += iptCoord[i];
#ifdef debug
	std::cout<<i<<"\t"<<iptCoord[i]<<std::endl;
#endif
	}
	saxCentroid /= numSaxMarkers;

	//Align the apexBase line to be collinear with the saxCentroid and the distance between the sax view markers
	//and viewMarkers is minimum (an isotropic scaling and translation transformation)
	TransformationOptimizationInput input;
	input.saxCentroid = saxCentroid;
	//Use interpolation to find the projection of the left and right basis onto the sax circle
	//Note that the angle is given by the view rotationAngle

	Point3D centre(0, 0, 0);
	for (int i = 0; i < numSaxMarkers; i++)
	{	//Since the first and last markers point to the same location
		Vector3D trans = saxMarkers[i] - saxCentroid;
		rad[i] = trans.Length();
		theta[i] = atan2(trans.z, trans.x); //Since markers have been transformed xz is the rotation axis
	}

	alglib::real_1d_array radius;
	radius.setcontent(numSaxMarkers, rad);
	alglib::real_1d_array index;
	index.setcontent(numSaxMarkers, theta);
	alglib::spline1dinterpolant s;

	// build spline
	try
	{
		alglib::spline1dbuildcubic(index, radius, s);
	} catch (alglib::ap_error& et)
	{
		std::cout << et.msg << std::endl;
		std::cout << radius.tostring(4) << "\t" << index.tostring(4) << std::endl;
		throw et;
	}

	unsigned int numViews = frameMarkers.size();
	std::vector<Point3D> basis(numViews * 2);
	Point3D base(0, 0, 0); //transformed base
	for (int i = 0; i < numViews; i++)
	{
		basis[2 * i] = getLeftSaxInterSection(i);
		basis[2 * i + 1] = getRightSaxInterSection(i);
		base += basis[2 * i];
		base += basis[2 * i + 1];
#ifdef debug
		std::cout<<basis[2*i]<<"\t"<<frameMarkers[i][LBASEINDEX]<<"\t:\t"<<basis[2*i+1]<<"\t"<<frameMarkers[i][RBASEINDEX]<<std::endl;
		std::cout<<basis[2*i].distance(frameMarkers[i][LBASEINDEX])<<"\t:\t"<<basis[2*i+1].distance(frameMarkers[i][RBASEINDEX])<<std::endl;
		std::cout<<basis[2*i].distance(basis[2*i+1])<<"\t:\t"<<frameMarkers[i][LBASEINDEX].distance(frameMarkers[i][RBASEINDEX])<<std::endl;
#endif
	}
	base /= (2 * numViews);
	input.base = base;

#ifdef debug
	std::cout<<"View base "<<base<<"\t"<<saxCentroid<<"\t"<<saxCentroid.distance(base)<<std::endl;
#endif

	for (int i = 0; i < numViews; i++)
	{
		double an1 = atan2(basis[2 * i].z - base.z, basis[2 * i].x - base.x);
		double an2 = atan2(basis[2 * i + 1].z - base.z, basis[2 * i + 1].x - base.x);
		double rd1 = alglib::spline1dcalc(s, an1);
		double rd2 = alglib::spline1dcalc(s, an2);

		Vector3D ml = (basis[2 * i] - base);
		ml.Normalise();
		input.saxl.push_back(base + (rd1 * ml));
		Vector3D mr = (basis[2 * i + 1] - base);
		mr.Normalise();
		input.saxr.push_back(base + rd2 * mr);

		input.viewl.push_back(basis[2 * i]);
		input.viewr.push_back(basis[2 * i + 1]);
#ifdef debug
		std::cout<<input.saxl[i]<<"\t"<<input.viewl[i]<<"\t"<<input.viewl[i].distance(input.saxl[i])<<"\t"<<rd1<<"\t"<<basis[2*i].distance(base)<<"*\t"<<an1<<std::endl;
		std::cout<<input.saxr[i]<<"\t"<<input.viewr[i]<<"\t"<<input.viewr[i].distance(input.saxr[i])<<"\t"<<rd2<<"\t"<<basis[2*i+1].distance(base)<<"*\t"<<an2<<std::endl;
#endif
	}

#ifdef debug
	std::cout<<"Optimizing fit "<<std::endl;
#endif
	//x trans,y trans,z trans,scale
	//Use constrained optimiztion to ensure that the scaling factor is within acceptable limits
	alglib::real_1d_array x = "[0,0,0,0]"; // dx,dz,sx,sz
	x[0] = base.x - saxCentroid.x; //Since we wish to transform sax to base
	x[1] = base.z - saxCentroid.z; //use the difference as initial seed

	alglib::real_1d_array bndl = "[-200,-200, 0.7,0.7]";
	alglib::real_1d_array bndu = "[+200,+200, 1.15,1.15]";
	alglib::minbleicstate state;
	alglib::minbleicreport rep;
	double epsg = 0.0000001;
	double epsf = 0;
	double epsx = 0;
	double epso = 0.00001;
	double epsi = 0.00001;
	double diffstep = 1.0e-6;
	try
	{
		alglib::minbleiccreatef(x, diffstep, state);
		alglib::minbleicsetbc(state, bndl, bndu);
		alglib::minbleicsetinnercond(state, epsg, epsf, epsx);
		alglib::minbleicsetoutercond(state, epso, epsi);
		alglib::minbleicoptimize(state, transformationObjective, NULL, &input);
		alglib::minbleicresults(state, x, rep);
#ifdef debug
		double cost = 0.0;
		transformationObjective(x, cost, &input);
		std::cout << x.tostring(5) << " Cost:" << cost;
		alglib::real_1d_array temp = "[0,0,1,1]";
		transformationObjective(temp, cost, &input);
		std::cout << " (" << cost << ")" << std::endl;
#endif
	} catch (alglib::ap_error& e)
	{
		std::cout << e.msg << std::endl;
		throw e;
	}
	MatrixType trans(4, 4);
	trans.fill(0);

	trans[0][0] = x[2];
	trans[1][1] = 1.0;
	trans[2][2] = x[3];
	trans[3][3] = 1.0;

	trans[0][3] = x[0];
	trans[1][3] = 0.0;
	trans[2][3] = x[1];

	transform = trans;
}

std::vector<std::vector<Point3D>*> SAXBasedCoordinateTransformer::getTransformedCoordinates() {
	unsigned int numViews = frameMarkers.size();
	std::vector<std::vector<Point3D>*> result(numViews);
	for (int i = 0; i < numViews; i++)
	{
		result[i] = new std::vector<Point3D>(frameMarkers[i]);
	}

	for (int j = 0; j < numViews; j++)
	{
		std::vector<Point3D>& tvec = *(result[j]);
		unsigned int numMarkers = tvec.size();
		for (int i = 0; i < numMarkers; i++)
		{
			double x = tvec[i].x;
			double y = tvec[i].y;
			double z = tvec[i].z;
			double mx = transform[0][0] * x + transform[0][1] * y + transform[0][2] * z + transform[0][3];
			double my = transform[1][0] * x + transform[1][1] * y + transform[1][2] * z + transform[1][3];
			double mz = transform[2][0] * x + transform[2][1] * y + transform[2][2] * z + transform[2][3];
			tvec[i] = Point3D(mx, my, mz);
		}
	}
	return result;
}

void SAXBasedCoordinateTransformer::transformationObjective(const alglib::real_1d_array& x, double& func, void* ptr) {

	TransformationOptimizationInput* input = static_cast<TransformationOptimizationInput*>(ptr);
	MatrixType trans(4, 4);
	trans.fill(0);

	trans[0][0] = x[2];
	trans[1][1] = 1.0;
	trans[2][2] = x[3];
	trans[3][3] = 1.0;

	trans[0][3] = x[0];
	trans[1][3] = 0.0;
	trans[2][3] = x[1];


	std::vector<Point3D> myPoints;
	myPoints.push_back(input->saxCentroid);
	myPoints.insert(myPoints.end(), input->saxl.begin(), input->saxl.end());
	myPoints.insert(myPoints.end(), input->saxr.begin(), input->saxr.end());

	unsigned int targetPoints = myPoints.size();

	myPoints.push_back(input->base);
	myPoints.insert(myPoints.end(), input->viewl.begin(), input->viewl.end());
	myPoints.insert(myPoints.end(), input->viewr.begin(), input->viewr.end());

	double temp = 0.0;

	for (int i = 0; i < targetPoints; i++)
	{
		double x = myPoints[i].x;
		double y = myPoints[i].y;
		double z = myPoints[i].z;
		double mx = trans[0][0] * x + trans[0][1] * y + trans[0][2] * z + trans[0][3];
		double my = trans[1][0] * x + trans[1][1] * y + trans[1][2] * z + trans[1][3];
		double mz = trans[2][0] * x + trans[2][1] * y + trans[2][2] * z + trans[2][3];
		myPoints[i] = Point3D(mx, my, mz);
		double xd = myPoints[i].x - myPoints[i + targetPoints].x;
		double zd = myPoints[i].z - myPoints[i + targetPoints].z;
		temp += (xd * xd) + (zd * zd);
	}
	func = temp;
}

void SAXBasedCoordinateTransformer::printTransform() {
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			std::cout << transform[i][j] << " ";
		}
		std::cout << std::endl;
	}
}

MatrixType SAXBasedCoordinateTransformer::getTransform() {
	return transform;
}

SAXBasedCoordinateTransformer::~SAXBasedCoordinateTransformer() {
	delete[] rad;
	delete[] theta;
}

Point3D SAXBasedCoordinateTransformer::getLeftSaxInterSection(int view) {
	Point3D base = (frameMarkers[view][LBASEINDEX] + frameMarkers[view][RBASEINDEX]) * 0.5;
	Point3D apex = frameMarkers[view][APEXINDEX];

	Vector3D r = (apex - base);
	double denom = r * r;
	for (int i = LBASEINDEX; i < APEXINDEX; i++)
	{
		double lambda = (frameMarkers[view][i] - base) * r / denom;
		Point3D s = base + r * lambda; //Point perpendicular to apex-base line from marker
		rad[i] = s.distance(frameMarkers[view][i]); //Get the height
		theta[i] = fabs(lambda);
	}

	rad[APEXINDEX] = 0.0;
	theta[APEXINDEX] = 1.0;
	int numItems = APEXINDEX - LBASEINDEX + 1;
	alglib::real_1d_array radius;
	radius.setcontent(numItems, rad);
	alglib::real_1d_array index;
	index.setcontent(numItems, theta);
	alglib::spline1dinterpolant s;
	// build spline
	try
	{
		alglib::spline1dbuildakima(index, radius, s);
	} catch (alglib::ap_error& et)
	{
		std::cout << et.msg << std::endl;
		throw et;
	}

	double height = alglib::spline1dcalc(s, saxPlaneDistance);
	r.Normalise();
	Point3D inter = base + r * saxPlaneDistance; //Point perpendicular to apex-base line from marker
	Vector3D rperp = (frameMarkers[view][LBASEINDEX] - base);
	rperp.Normalise();
	Point3D result = inter + rperp * height;
	return result;
}

Point3D SAXBasedCoordinateTransformer::getRightSaxInterSection(int view) {
	Point3D base = (frameMarkers[view][LBASEINDEX] + frameMarkers[view][RBASEINDEX]) * 0.5;
	Point3D apex = frameMarkers[view][APEXINDEX];

	Vector3D r = (apex - base);
	double denom = r * r;
	int ctr = 0;
	for (int i = RBASEINDEX; i > APEXINDEX; i--)
	{
		double lambda = (frameMarkers[view][i] - base) * r / denom;
		Point3D s = base + r * lambda; //Point perpendicular to apex-base line from marker
		rad[ctr] = s.distance(frameMarkers[view][i]);
		theta[ctr++] = fabs(lambda);
	}
	rad[APEXINDEX] = 0.0;
	theta[APEXINDEX] = 1.0;
	int numItems = APEXINDEX - LBASEINDEX + 1;
	alglib::real_1d_array radius;
	radius.setcontent(numItems, rad);
	alglib::real_1d_array index;
	index.setcontent(numItems, theta);
	alglib::spline1dinterpolant s;
	// build spline
	try
	{
		alglib::spline1dbuildakima(index, radius, s);
	} catch (alglib::ap_error& et)
	{
		std::cout << et.msg << std::endl;
		throw et;
	}
	double height = alglib::spline1dcalc(s, saxPlaneDistance);
	r.Normalise();
	Point3D inter = base + r * saxPlaneDistance; //Point perpendicular to apex-base line from marker
	Vector3D rperp = (frameMarkers[view][RBASEINDEX] - base);
	rperp.Normalise();
	Point3D result = inter + rperp * height;
	return result;
}
