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

#ifndef VECTOR3D_H_
#define VECTOR3D_H_

#include "Coordinate3D.h"

class Vector3D: public Coordinate3D {
	bool normalized;
public:
	Vector3D() :
			Coordinate3D() {
		normalized = false;
	}

	Vector3D(const Vector3D& cp) :
				Coordinate3D(cp.x, cp.y, cp.z) {
			normalized = cp.normalized;
	}

	Vector3D(Real x_, Real y_, Real z_) :
			Coordinate3D(x_, y_, z_) {
		normalized = false;
	}

	Real Length() const {
		return sqrt(x * x + y * y + z * z);
	}

	void Normalise() {
		if(!normalized){
			Real length = Length();
			x /= length;
			y /= length;
			z /= length;
			normalized = true;
		}
	}

	void CrossProduct(const Vector3D& vec1, const Vector3D& vec2) {
		x = (vec1).y * (vec2).z - (vec1).z * (vec2).y;
		y = (vec1).z * (vec2).x - (vec1).x * (vec2).z;
		z = (vec1).x * (vec2).y - (vec1).y * (vec2).x;
	}

	// arithmetic operations
	inline Vector3D operator +(const Vector3D& rkVector) const {
		return Vector3D(x + rkVector.x, y + rkVector.y, z + rkVector.z);
	}

	Vector3D operator-(const Vector3D& rhs) const {
		return Vector3D(x - rhs.x, y - rhs.y, z - rhs.z);
	}

	inline Vector3D operator *(const Real fScalar) const {
		return Vector3D(x * fScalar, y * fScalar, z * fScalar);
	}

	inline Real operator *(const Vector3D& rhs) const {
		return x * rhs.x + y * rhs.y + z * rhs.z;
	}
};

inline Vector3D operator*(Real scalar, const Vector3D& rhs) {
	return Vector3D(scalar * rhs.x, scalar * rhs.y, scalar * rhs.z);
}
;

inline Vector3D operator*(const gtMatrix& m, const Vector3D& v) // no translation
		{
	Vector3D r;

	r.x = (m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z); // + m[0][3] );
	r.y = (m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z); // + m[1][3] );
	r.z = (m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z); // + m[2][3] );

	return r;
}
;

inline Vector3D CrossProduct(const Coordinate3D& vec1,
		const Coordinate3D& vec2) {
	return Vector3D((vec1).y * (vec2).z - (vec1).z * (vec2).y,
			(vec1).z * (vec2).x - (vec1).x * (vec2).z,
			(vec1).x * (vec2).y - (vec1).y * (vec2).x);
}


template<typename V>
inline
double ComputeVolumeOfTetrahedron(const V& a, const V& b, const V& c,
		const V& d)

		{
	double vol = DotProduct((a - d), CrossProduct((b - d), (c - d)));
	return fabs(vol);
}

template<typename V>
inline V& operator*=(V& v, double number) {
	v.x *= number;
	v.y *= number;
	v.z *= number;
	return v;
}

template<typename V>
inline V& operator/=(V& v, double number) {
	v.x /= number;
	v.y /= number;
	v.z /= number;
	return v;
}

template<typename V>
inline V& operator+=(V& v, const V& rhs) {
	v.x += rhs.x;
	v.y += rhs.y;
	v.z += rhs.z;
	return v;
}

/**
 * Solve a*sine(x coordinate) + b*cosine(x coordinate) = c.  For 0 < x < 180.
 *
 * @param	a	a.
 * @param	b	The b.
 * @param	c	The c.
 *
 * @return	.
 */
inline double SolveASinXPlusBCosXIsEqualToC(double a, double b, double c) {
	// This solves a sin(x) + b cos(x) = c for 0 < x < 180;

	// use the fact the eqn is equivalent to
	// C cos(x - y) = c
	// where
	double C = std::sqrt(a * a + b * b);

	double y = std::atan(a / b);

	double xMinusY = std::acos(c / C);

	double x = xMinusY + y;
	while (x < 0) {
		x += 2.0 * M_PI;
	}
	while (x > 2.0 * M_PI) {
		x -= 2.0 * M_PI;
	}

	if (x < M_PI) {
		return x;
	}

	// try the other solution
	x = (-xMinusY) + y;

	while (x < 0) {
		x += 2.0 * M_PI;
	}
	while (x > 2.0 * M_PI) {
		x -= 2.0 * M_PI;
	}
	return x;
}

#endif /* VECTOR3D_H_ */
