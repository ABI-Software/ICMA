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


#ifndef POINT3D_H_
#define POINT3D_H_

#include "Coordinate3D.h"
#include "Vector3D.h"

class Point3D: public Coordinate3D {
	bool selected;
public:
	Point3D() :
			Coordinate3D() {
		selected = true;
	}

	Point3D(const Point3D& cp) :
			Coordinate3D(cp.x, cp.y, cp.z) {
		selected = cp.selected;
	}

	Point3D(Real x_, Real y_, Real z_) :
			Coordinate3D(x_, y_, z_) {
		selected = true;
	}


	explicit Point3D(Real p[]) //for compatibility with Cmgui
	:
			Coordinate3D(p[0], p[1], p[2]) {
		selected = true;
	}


	bool isSelected(){
		return selected;
	}

	void setSelection(bool status){
		selected = status;
	}

	/**
	 * Convert the point to an array.  The array returned
	 * must be deleted by the receiver.
	 *
	 * \returns a pointer to an array of 3 Reals.
	 */
	Real* ToArray() {
		Real *array = new Real[3];
		array[0] = x;
		array[1] = y;
		array[2] = z;

		return array;
	}

	/**
	 * Addition operator.
	 *
	 * @param	rhs	The right hand side.
	 *
	 * @return	The result of the operation.
	 */
	Point3D operator+(const Vector3D& rhs) const {
		return Point3D(x + rhs.x, y + rhs.y, z + rhs.z);
	}

	Point3D operator+(const Point3D& rhs) const {
		return Point3D(x + rhs.x, y + rhs.y, z + rhs.z);
	}

	//For convenience
	Point3D operator-(const Vector3D& rhs) const {
		return Point3D(x - rhs.x, y - rhs.y, z - rhs.z);
	}

	Vector3D operator-(const Point3D& rhs) const {
		return Vector3D(x - rhs.x, y - rhs.y, z - rhs.z);
	}

	Point3D operator/(const Real divider) const {
		return Point3D(x / divider, y / divider, z / divider);
	}

	inline Point3D operator *(const Real fScalar) const {
		return Point3D(x * fScalar, y * fScalar, z * fScalar);
	}

	inline bool operator == (const Point3D& rhs){
		return rhs.x==x && rhs.y==y && rhs.z==z;
	}

	Real distance(Point3D& pt) {
		Real dist = (x - pt.x) * (x - pt.x) + (y - pt.y) * (y - pt.y)	+ (z - pt.z) * (z - pt.z);
        return sqrt(dist);
	}

};

inline Point3D operator*(const gtMatrix& m, const Point3D& v) // includes translation
{
	Point3D r;

	Real fInvW = 1.0
			/ (m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3]);

	r.x = (m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3]) * fInvW; // this is 1st row not column
	r.y = (m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3]) * fInvW;
	r.z = (m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3]) * fInvW;

	return r;
}

inline std::istream& operator>>(std::istream& in, Point3D &val) {
	std::string temp;
	in >> temp; // name of the vector
	in >> val.x;
	in >> temp; // trailing character = i
	in >> val.y;
	in >> temp; // trailing character = j
	in >> val.z;
	in >> temp; // trailing character = k

	return in;
}

inline std::ostream& operator<<(std::ostream& out, Point3D &val) {
	out << val.x << ", " << val.y << ", " << val.z << " ";
	return out;
}

#endif /* POINT3D_H_ */
