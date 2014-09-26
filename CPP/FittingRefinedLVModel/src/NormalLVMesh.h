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

#ifndef NORMALLVMESH_H_
#define NORMALLVMESH_H_

#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <ctime>
#include "Point3D.h"
#include "Vector3D.h"

extern "C"
{
#include "zn/cmgui_configure.h"
#include "zn/cmiss_context.h"
#include "zn/cmiss_field.h"
#include "zn/cmiss_element.h"
#include "zn/cmiss_region.h"
#include "zn/cmiss_node.h"
#include "zn/cmiss_stream.h"
#include "zn/cmiss_field_module.h"
#include "zn/cmiss_field_constant.h"
#include "zn/cmiss_field_composite.h"
#include "zn/cmiss_field_matrix_operators.h"
#include "zn/cmiss_field_finite_element.h"
#include "zn/cmiss_element.h"
#include "zn/cmiss_time_keeper.h"
#include "zn/cmiss_field_time.h"
}

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "alglib/interpolation.h"

enum ViewTypeEnum {
	LVAPLAX=1, LVTCH, LVFCH, LVSAXBASE, LVSAXMID, LVSAXAPEX
};

class Plane {
public:
	Vector3D normal;
	Point3D position;
};

#include "vnl/vnl_matrix.h"
#include "vnl/algo/vnl_matrix_inverse.h"

typedef vnl_matrix<double> MatrixType;

typedef std::vector<Point3D> PointVector;

class NormalLVMesh {
public:
	NormalLVMesh(Point3D apex, Point3D base, Point3D rv);
	virtual ~NormalLVMesh();
	std::vector<double> getTorsionsAt(double time);
protected:
	Point3D apex;
	Point3D rv;
	Point3D base;
	alglib::spline1dinterpolant apexBaseVariation;

	double patientToGlobalTransform[16]; /**< The patient to global transform */
	std::string modelName_; /**< Name of the model */
	double focalLength;
	std::vector<Cmiss_node_id> nodes;
	std::vector<Cmiss_element_id> elements;
	Cmiss_field_cache_id cache;
	Cmiss_context_id context_;
	Cmiss_field_id field_;
	Cmiss_field_module_id field_module_;
	Cmiss_field_id coordinates_ps_;
	Cmiss_field_id coordinates_rc_;
	Cmiss_field_id coordinates_patient_rc_;
	Cmiss_field_id transform_mx_;
	int numberOfModelFrames_; /**< Number of model frames */
	std::string contextName;
};

#endif /* LVHEARTMESH_H_ */
