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
 *  Contributor(s): J Chung
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


#ifndef CAPTOTALLEASTSQUARES_H_
#define CAPTOTALLEASTSQUARES_H_

#include "vnl/vnl_matrix.h"
#include "vnl/vnl_sparse_matrix.h"
#include "vnl/algo/vnl_svd.h"
#include "vnl/vnl_sparse_matrix_linear_system.h"
#include "vnl/algo/vnl_lsqr.h"
#include "vnl/vnl_linear_system.h"

#include "Point3D.h"
#include "Vector3D.h"

namespace cap
{

inline Point3D ComputeCentroid(const std::vector<std::pair<Point3D, double> >& points)
{
	Point3D centroid;
	for (std::vector<std::pair<Point3D, double> >::const_iterator i = points.begin();
			i != points.end(); ++i)
	{
		centroid += i->first;
	}
	centroid /= points.size();
	return centroid;
}

inline Plane FitPlaneUsingTLS(const std::vector<std::pair<Point3D, double> >& points)
{
	// 1. Compute centroid
	Point3D centroid = ComputeCentroid(points);
	
	// 2. Build m*3 matrix from (data points - centroid)
	vnl_matrix<double> M(points.size(), 3);
	for (size_t i = 0; i < points.size(); ++i)
	{
		Vector3D diff = points[i].first - centroid;
		M(i,0) = diff.x;
		M(i,1) = diff.y;
		M(i,2) = diff.z;
	}
	
	// 3. perform svd
	vnl_svd<double> svd(M);
	
	int index = svd.rank() - 1; // index of smallest non-zero singular value
	vnl_vector<double> v = svd.V().get_column(index); // and the corresponding vector
	
	Plane plane;
	plane.normal = Vector3D(v(0), v(1), v(2));
	plane.position = centroid;
	
	return plane;
}


inline void TestLSQR()
{
	std::cout << __func__ << "\n";
	{
		vnl_sparse_matrix<double> m(2,2);
		m(0,0) = 1; m(0,1) = 2;
		m(1,0) = 3; m(1,1) = 4;
		
		vnl_vector<double> b(2);
		b[0] = 3;
		b[1] = 7;
		
		vnl_sparse_matrix_linear_system<double> ls(m,b);
		vnl_lsqr lsqr(ls);
		lsqr.set_max_iterations(100);
		vnl_vector<double> x(2);
		lsqr.minimize(x);
		lsqr.diagnose_outcome(std::cout);
		std::cout << x << std::endl;
	}
	
	{
		vnl_sparse_matrix<double> m(3,2);
		m(0,0) = 1; m(0,1) = 2;
		m(1,0) = 3; m(1,1) = 4;
		m(2,0) = 1; m(2,1) = 3;
		
		vnl_vector<double> b(3);
		b[0] = 3;
		b[1] = 7;
		b[2] = 3;
		
		vnl_sparse_matrix_linear_system<double> ls(m,b);
		vnl_lsqr lsqr(ls);
		lsqr.set_max_iterations(100);
		vnl_vector<double> x(2);
		lsqr.minimize(x);
		lsqr.diagnose_outcome(std::cout);
		std::cout << x << std::endl;
	}
}

} // end namespace cap
#endif /* CAPTOTALLEASTSQUARES_H_ */
