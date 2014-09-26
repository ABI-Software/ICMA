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

#include "LVHeartMeshModel.h"

#ifdef fitAxistoInput
void optimize(const alglib::real_1d_array& x, double& func, void* ptr) {
	struct AxisFittingOptimizationInput* input = static_cast<struct AxisFittingOptimizationInput*>(ptr);
	Cmiss_field_module_id field_module = (input->field_module);
	std::vector<Cmiss_element_id>& elements(*(input->cmiss_elements));
	Cmiss_field_id coordinate = (input->coordinates_rc);
	Cmiss_field_cache_id cache = (input->cache);
	Cmiss_field_id transform_mx = input->transform_mx;

	Point3D apex = input->targetApex;
	Point3D basel = input->targetBasel;
	Point3D baser = input->targetBaser;

	double pgs[16], pgsi[16];
	Cmiss_field_evaluate_real(transform_mx, cache, 16, pgs);

	for (int i = 0; i < 16; i++) {
		pgsi[i] = x[i];
	}
	Cmiss_field_module_begin_change(field_module);
	Cmiss_field_assign_real(transform_mx, cache, 16, pgsi);
	Cmiss_field_module_end_change(field_module);
	double coord[3], vl[3];
	coord[0] = input->apexXi.x;
	coord[1] = input->apexXi.y;
	coord[2] = input->apexXi.z;
	Cmiss_field_cache_set_mesh_location(cache, elements[input->apexEl - 1], 3, coord);
	Cmiss_field_evaluate_real(coordinate, cache, 3, vl);
	Point3D napex(vl);
	coord[0] = input->baselXi.x;
	coord[1] = input->baselXi.y;
	coord[2] = input->baselXi.z;
	Cmiss_field_cache_set_mesh_location(cache, elements[input->baselEl - 1], 3, coord);
	Cmiss_field_evaluate_real(coordinate, cache, 3, vl);
	Point3D nbasel(vl);
	coord[0] = input->baserXi.x;
	coord[1] = input->baserXi.y;
	coord[2] = input->baserXi.z;
	Cmiss_field_cache_set_mesh_location(cache, elements[input->baserEl - 1], 3, coord);
	Cmiss_field_evaluate_real(coordinate, cache, 3, vl);
	Point3D nbaser(vl);

	Cmiss_field_module_begin_change(field_module);
	Cmiss_field_assign_real(transform_mx, cache, 16, pgs);
	Cmiss_field_module_end_change(field_module);
	func = nbasel.distance(basel) + nbaser.distance(baser) + napex.distance(apex);
	input->error = func;
}
#endif

LVHeartMeshModel::LVHeartMeshModel(unsigned int numFrames) :
		focalLength(0.0), bezierToHermiteTransform_(NULL), field_(0), field_module_(0), coordinates_ps_(0), coordinates_rc_(0), coordinates_patient_rc_(0), transform_mx_(0), numberOfModelFrames_(
				numFrames), aligned(0), timeVaryingDataPoints_(134), solverFactory_(new cap::GMMFactory), timeSmoother_() {

	contextName = "CAP";
	timeSamplingInterval = "";
	//Initialise cmgui and get context
	context_ = Cmiss_context_create(contextName.c_str());
	/**< Handle to the context */
	Cmiss_region_id root_region = Cmiss_context_get_default_region(context_);

	//Load the finite element mesh of the heart

	// Create a heart region to stop Cmgui adding lines to the new rendition
	std::string region_name = "heart";
	Cmiss_region_id heart_region = Cmiss_region_create_child(root_region, region_name.c_str());
	//Get the field module of the heart region
	field_module_ = Cmiss_region_get_field_module(heart_region);

	Cmiss_region_destroy(&heart_region);

	esf = -1;

	framesWithDataPoints.resize(numberOfModelFrames_, 0);
	// Read in the heart model spaced over the number of model frames
	double denom = numberOfModelFrames_ - 1;
	for (unsigned int i = 0; i < numberOfModelFrames_; i++) {
		double time = static_cast<double>(i) / denom;
		timePoints.push_back(time);
		Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
		Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
		Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, heartmodel_exnode, heartmodel_exnode_len);
		Cmiss_stream_information_region_set_resource_attribute_real(stream_information_region, stream_resource, CMISS_STREAM_INFORMATION_REGION_ATTRIBUTE_TIME, time);

		Cmiss_region_read(root_region, stream_information);

		Cmiss_stream_resource_destroy(&stream_resource);
		Cmiss_stream_information_destroy(&stream_information);
		Cmiss_stream_information_region_destroy(&stream_information_region);
	}

	//Update handles and define heart fields
	// initialize patientToGlobalTransform_ to identity matrix
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (i == j) {
				patientToGlobalTransform[4 * i + j] = 1.0;
			} else {
				patientToGlobalTransform[4 * i + j] = 0.0;
			}
		}
	}

	if (field_module_ != 0) {
		Cmiss_field_module_begin_change(field_module_);

		// 'coordinates' is an assumed field from the element template file.  It is also assumed to be
		// a prolate spheriodal coordinate system.
		coordinates_ps_ = Cmiss_field_module_find_field_by_name(field_module_, "coordinates");

		// Cannot make all these fields via the Cmgui API yet.
		Cmiss_field_module_define_field(field_module_, "d_ds1", "node_value fe_field coordinates d/ds1");
		Cmiss_field_module_define_field(field_module_, "d_ds2", "node_value fe_field coordinates d/ds2");
		Cmiss_field_module_define_field(field_module_, "d2_ds1ds2", "node_value fe_field coordinates d2/ds1ds2");
		Cmiss_field_module_define_field(field_module_, "coordinates_rc", "coordinate_transformation field coordinates");
		//Get the rectangular cartesian coordinates associated with the mesh
		coordinates_rc_ = Cmiss_field_module_find_field_by_name(field_module_, "coordinates_rc");
		//Create a field which stores the transformation matrix
		transform_mx_ = Cmiss_field_module_create_constant(field_module_, 16, patientToGlobalTransform);
		//Get the coordinates for the mesh in the patient coordinate system
		coordinates_patient_rc_ = Cmiss_field_module_create_projection(field_module_, coordinates_rc_, transform_mx_);
		//Cmiss_field_set_name(coordinates_patient_rc_, "coordinates_patient_rc");
		cache = Cmiss_field_module_create_cache(field_module_);

		Cmiss_field_module_end_change(field_module_);

		//Get the elements
		elements.resize(16);
		Cmiss_mesh_id cmiss_mesh = Cmiss_field_module_find_mesh_by_name(field_module_, "cmiss_mesh_3d");

		Cmiss_element_iterator_id elementIterator = Cmiss_mesh_create_element_iterator(cmiss_mesh);
		Cmiss_element_id element = Cmiss_element_iterator_next(elementIterator);

		//Get the list of mesh elements
		while (element != NULL) {
			int elementId = Cmiss_element_get_identifier(element) - 1; //Cmiss numbering starts at 1
			elements[elementId] = element;
			element = Cmiss_element_iterator_next(elementIterator);
		}
		Cmiss_element_iterator_destroy(&elementIterator);
		Cmiss_mesh_destroy(&cmiss_mesh);

		//Get the nodes
		nodes.resize(40);
		Cmiss_nodeset_id nodeset = Cmiss_field_module_find_nodeset_by_name(field_module_, "cmiss_nodes");

		for (int i = 0; i < 40; i++) // node index starts at 1
				{
			Cmiss_node_id node = Cmiss_nodeset_find_node_by_identifier(nodeset, i + 1);
			nodes[i] = node;
		}
		Cmiss_nodeset_destroy(&nodeset);

	}

	Cmiss_region_destroy(&root_region);

	//Load gmm matrix data
	cap::SolverLibraryFactory& factory = *solverFactory_;

	// Read in S (smoothness matrix)
	std::string tmpFileName = CreateTemporaryEmptyFile();
	WriteCharBufferToFile(tmpFileName, globalsmoothperframematrix_dat, globalsmoothperframematrix_dat_len);
	S_ = factory.CreateSparseMatrixFromFile(tmpFileName);
	RemoveFile(tmpFileName);
	// Read in G (global to local parameter map)
	tmpFileName = CreateTemporaryEmptyFile();
	WriteCharBufferToFile(tmpFileName, globalmapbeziertohermite_dat, globalmapbeziertohermite_dat_len);
	G_ = factory.CreateSparseMatrixFromFile(tmpFileName);
	RemoveFile(tmpFileName);

	// initialize preconditioner and GSMoothAMatrix
	preconditioner_ = factory.CreateDiagonalPreconditioner(*S_);
	aMatrix_ = factory.CreateGSmoothAMatrix(*S_, *G_);

	tmpFileName = CreateTemporaryEmptyFile();
	WriteCharBufferToFile(tmpFileName, prior_dat, prior_dat_len);
	prior_ = factory.CreateVectorFromFile(tmpFileName);
	RemoveFile(tmpFileName);

}

LVHeartMeshModel::~LVHeartMeshModel() {
	Cmiss_field_destroy(&coordinates_ps_);
	Cmiss_field_destroy(&coordinates_rc_);
	Cmiss_field_destroy(&coordinates_patient_rc_);
	Cmiss_field_destroy(&transform_mx_);
	Cmiss_field_module_destroy(&field_module_);
	Cmiss_context_destroy(&context_);
	Cmiss_field_cache_destroy(&cache);

	for (int i = 0; i < elements.size(); i++)
		Cmiss_element_destroy(&elements[i]);

	for (int i = 0; i < nodes.size(); i++)
		Cmiss_node_destroy(&nodes[i]);

	delete aMatrix_;
	delete preconditioner_;
	delete S_;
	delete G_;
	delete prior_;
	delete solverFactory_;
}

void LVHeartMeshModel::setApex(Point3D apex) {
	this->apex = apex;
}

void LVHeartMeshModel::setBase(Point3D base) {
	this->base = base;
}

void LVHeartMeshModel::setRVInserts(std::vector<Point3D> points) {
	PointVector::const_iterator itr = points.begin();
	const PointVector::const_iterator end = points.end();

	while (itr != end) {
		rvInserts.push_back(*itr);
		++itr;
	}
}

void LVHeartMeshModel::SetLambdaAtTime(const std::vector<double>& lambdaParams, double time) {
	Cmiss_field_module_id field_module = field_module_;
	Cmiss_field_module_begin_change(field_module);
	Cmiss_field_id coords_ps = coordinates_ps_;
	Cmiss_field_id d_ds1 = Cmiss_field_module_find_field_by_name(field_module, "d_ds1");
	Cmiss_field_id d_ds2 = Cmiss_field_module_find_field_by_name(field_module, "d_ds2");
	Cmiss_field_id d2_ds1ds2 = Cmiss_field_module_find_field_by_name(field_module, "d2_ds1ds2");

	Cmiss_field_cache_set_time(cache, time);

	for (int i = 0; i < 40; i++) // node index starts at 1
			{
		Cmiss_field_cache_set_node(cache, nodes[i]);
		double loc_ps[3];
		Cmiss_field_evaluate_real(coords_ps, cache, 3, loc_ps);
		loc_ps[0] = lambdaParams[4 * i + 0];
		Cmiss_field_assign_real(coords_ps, cache, 3, loc_ps);
		double loc_d_ds1[] = { 0.0, 0.0, 0.0 };
		loc_d_ds1[0] = lambdaParams[4 * i + 1];
		Cmiss_field_assign_real(d_ds1, cache, 3, loc_d_ds1);
		double loc_d_ds2[] = { 0.0, 0.0, 0.0 };
		loc_d_ds2[0] = lambdaParams[4 * i + 2];
		Cmiss_field_assign_real(d_ds2, cache, 3, loc_d_ds2);
		double loc_d2_ds1ds2[] = { 0.0, 0.0, 0.0 };
		loc_d2_ds1ds2[0] = lambdaParams[4 * i + 3];
		Cmiss_field_assign_real(d2_ds1ds2, cache, 3, loc_d2_ds1ds2);
	}
	Cmiss_field_destroy(&d_ds1);
	Cmiss_field_destroy(&d_ds2);
	Cmiss_field_destroy(&d2_ds1ds2);
	Cmiss_field_module_end_change(field_module);
}

void LVHeartMeshModel::SetMuFromBasePlaneAtTime(Plane& basePlane, double time) {

	const Vector3D& normal = basePlane.normal;
	const Point3D& position = basePlane.position;
	// lambda, mu and theta

	Cmiss_field_module_begin_change(field_module_);
	Cmiss_field_id coords_ps = coordinates_ps_;
	Cmiss_field_id coords_patient = coordinates_patient_rc_;

	Cmiss_field_cache_set_time(cache, time);

	// EPI nodes [0-19], ENDO nodes [20-39], node identifiers are base 1.
	// This method follows the CIM method of calculating the model position from the base plane.
	// EPI nodes follow endo
	for (int k = 20; k < 40; k += 20) {
		double mu[4];
		for (int i = 0; i < 4; i++) {
			//Cmiss_node_id node = Cmiss_nodeset_find_node_by_identifier(nodeset, k + i + 1);
			Cmiss_field_cache_set_node(cache, nodes[k + i]);
			double loc[3], loc_ps[3], loc_pat[3];
			Cmiss_field_evaluate_real(coords_ps, cache, 3, loc_ps);
			const double startmu = loc_ps[1];
			mu[i] = loc_ps[1] = 0.0;
			Cmiss_field_assign_real(coords_ps, cache, 3, loc_ps);
			//Cmiss_field_evaluate_real(coords_ps, cache, 3, loc_ps);
			Cmiss_field_evaluate_real(coords_patient, cache, 3, loc_pat);
			Point3D point(loc_pat[0], loc_pat[1], loc_pat[2]);
			Point3D prevPoint;
			double initial = DotProduct(normal, point - position);
			//do while on the same side and less than pi
			do {
				loc_ps[1] += M_PI / 180.0; //one degree increments for mu parameter
				Cmiss_field_assign_real(coords_ps, cache, 3, loc_ps);
				mu[i] = loc_ps[1];
				Cmiss_field_evaluate_real(coords_patient, cache, 3, loc_pat);
				prevPoint = point;
				point = Point3D(loc_pat[0], loc_pat[1], loc_pat[2]);
			} while ((initial * DotProduct(normal, point - position) > 0.0) && (mu[i] <= startmu));
			// We have stepped over the base plane (or we have reached pi), now interpolate between the
			// current point and the previous point
			double z1 = DotProduct(normal, prevPoint - position);
			double z2 = DotProduct(normal, point - position);
			if ((z1 * z2) < 0.0) {
				double zdiff = z2 - z1;
				double s;
				if (fabs(zdiff) < 1.0e-05)
					s = 0.5;
				else
					s = (-z1) / zdiff;
				point = prevPoint + s * (point - prevPoint);
				loc[0] = point.x;
				loc[1] = point.y;
				loc[2] = point.z;
				Cmiss_field_assign_real(coords_patient, cache, 3, loc);
			}

			// Just making sure we keep mu inside it's bounds.
			Cmiss_field_evaluate_real(coords_ps, cache, 3, loc_ps);
			mu[i] = loc_ps[1];
			if (mu[i] > M_PI) {
				mu[i] = M_PI;
				loc_ps[1] = M_PI;
			}
			Cmiss_field_assign_real(coords_ps, cache, 3, loc_ps);
			//EPI nodes follow
			Cmiss_field_cache_set_node(cache, nodes[k + i-20]);
			Cmiss_field_evaluate_real(coords_ps, cache, 3, loc_pat);
			loc_pat[1] = mu[i];
			Cmiss_field_assign_real(coords_ps, cache, 3, loc_pat);
			//Cmiss_node_destroy(&node);
		}

		// Set the remaining mu paramater of the nodes equidistant between mu[i] and 0.0
		for (int j = 1; j < 5; j++) {
			for (int i = 0; i < 4; i++) {
				//Cmiss_node_id node = Cmiss_nodeset_find_node_by_identifier(nodeset, k + (j * 4) + i + 1);
				Cmiss_field_cache_set_node(cache, nodes[k + (j * 4) + i]);
				double loc_ps[3];
				double nmu = mu[i] / 4.0 * (4.0 - static_cast<double>(j));
				Cmiss_field_evaluate_real(coords_ps, cache, 3, loc_ps);
				loc_ps[1] = nmu;
				Cmiss_field_assign_real(coords_ps, cache, 3, loc_ps);
				//Cmiss_node_destroy(&node);
				//EPI nodes follow
				Cmiss_field_cache_set_node(cache, nodes[k + (j * 4) + i - 20]);
				Cmiss_field_evaluate_real(coords_ps, cache, 3, loc_ps);
				loc_ps[1] = nmu;
				Cmiss_field_assign_real(coords_ps, cache, 3, loc_ps);
			}
		}
	}
	Cmiss_field_module_end_change(field_module_);
}

int LVHeartMeshModel::ComputeXi(const Point3D& position, double time, Point3D& xi) const {
	Cmiss_field_module_id field_module = field_module_;
	Cmiss_field_module_begin_change(field_module);

	Cmiss_field_cache_set_time(cache, time);
	double values[3];
	values[0] = position.x;
	values[1] = position.y;
	values[2] = position.z;
	Cmiss_field_id const_position = Cmiss_field_module_create_constant(field_module, 3, values);
	Cmiss_mesh_id mesh = Cmiss_field_module_find_mesh_by_dimension(field_module, 3);
	Cmiss_field_id mesh_location_field = Cmiss_field_module_create_find_mesh_location(field_module, const_position, coordinates_patient_rc_, mesh);
	Cmiss_field_find_mesh_location_id find_mesh_location_field = Cmiss_field_cast_find_mesh_location(mesh_location_field);
	int r = Cmiss_field_find_mesh_location_set_search_mode(find_mesh_location_field, CMISS_FIELD_FIND_MESH_LOCATION_SEARCH_MODE_FIND_NEAREST);
	Cmiss_field_find_mesh_location_destroy(&find_mesh_location_field);

	double xi_values[3];
	Cmiss_element_id el = Cmiss_field_evaluate_mesh_location(mesh_location_field, cache, 3, xi_values);
	xi.x = xi_values[0];
	xi.y = xi_values[1];
	xi.z = xi_values[2];
	int element_id = Cmiss_element_get_identifier(el);

	Cmiss_element_destroy(&el);

	Cmiss_field_module_end_change(field_module);

	Cmiss_mesh_destroy(&mesh);
	Cmiss_field_destroy(&mesh_location_field);
	Cmiss_field_destroy(&const_position);

	return element_id;
}

double LVHeartMeshModel::GetFocalLength() const {
	return Cmiss_field_get_attribute_real(coordinates_ps_, CMISS_FIELD_ATTRIBUTE_COORDINATE_SYSTEM_FOCUS);
}

void LVHeartMeshModel::SetFocalLength(double focalLength) {
	this->focalLength = focalLength;
	Cmiss_field_set_attribute_real(coordinates_ps_, CMISS_FIELD_ATTRIBUTE_COORDINATE_SYSTEM_FOCUS, focalLength);
}

double*
LVHeartMeshModel::getTransformationArray() {
	return patientToGlobalTransform;
}

/*
 * Convert the RC coordinates into a prolate Spheriodal coordinates
 * using the mesh information
 */

std::vector<Point3D> LVHeartMeshModel::ConvertToHeartModelProlateSpheriodalCoordinate(std::vector<Point3D>& point) {

	std::vector<Point3D> result(point);

	Cmiss_field_module_id field_module = field_module_;
	Cmiss_field_module_begin_change(field_module);

	//Get the handles to create temporary node for performing the transformation
	Cmiss_nodeset_id nodeset = Cmiss_field_module_find_nodeset_by_name(field_module, "cmiss_nodes");
	Cmiss_field_id coordinates = Cmiss_field_module_find_field_by_name(field_module, "coordinates");
	Cmiss_node_template_id nodeTemplate = Cmiss_nodeset_create_node_template(nodeset);
	Cmiss_node_template_define_field(nodeTemplate, coordinates);

	//The input coordinates are in rectangular Cartesian coordinates
	//Convert them to prolate spheroidal

	Cmiss_node_id temporaryNode = Cmiss_nodeset_create_node(nodeset, 100005 /*Fix this */, nodeTemplate);

	Cmiss_field_cache_set_node(cache, temporaryNode);

	unsigned int numpoints = point.size();

	for (int i = 0; i < numpoints; i++) {
		double loc[3];
		loc[0] = point[i].x;
		loc[1] = point[i].y;
		loc[2] = point[i].z;

		Cmiss_field_assign_real(coordinates_patient_rc_, cache, 3, loc);

		loc[0] = loc[1] = loc[2] = 0.0;

		Cmiss_field_evaluate_real(coordinates, cache, 3, loc);
		result[i] = Point3D(loc);
	}

	//Destroy the node so that it is not output with the heart mesh
	Cmiss_nodeset_destroy_node(nodeset, temporaryNode);

	Cmiss_field_destroy(&coordinates);
	Cmiss_node_template_destroy(&nodeTemplate);
	Cmiss_nodeset_destroy(&nodeset);
	Cmiss_field_module_end_change(field_module);

	return result;

}

void LVHeartMeshModel::InitialiseBezierLambdaParams() {
	//Initialise bezier global params for each model
	for (int i = 0; i < 134; i++) {
		timeVaryingDataPoints_[i].resize(numberOfModelFrames_);
		const std::vector<double>& prior = timeSmoother_.GetPrior(i);
		for (int j = 0; j < numberOfModelFrames_; j++) {
			double xi = static_cast<double>(j) / numberOfModelFrames_;

			double lambda = timeSmoother_.ComputeLambda(xi, prior);
			timeVaryingDataPoints_[i][j] = lambda;
		}
	}
}

Plane LVHeartMeshModel::FitPlaneToBasePlanePoints(const std::vector<std::pair<Point3D, double> >& basePlanePoints, const Vector3D& xAxis) const {
	Plane plane;

	if (basePlanePoints.size() == 2) {
		// When only 2 base plane points have been specified
		Vector3D temp1 = basePlanePoints[1].first - basePlanePoints[0].first;
		temp1.Normalise();

		Vector3D temp2 = CrossProduct(temp1, xAxis);

		plane.normal = CrossProduct(temp1, temp2);
		plane.normal.Normalise();

		//plane.position = basePlanePoints[0].first + (0.5 * (basePlanePoints[1].first - basePlanePoints[0].first));
		plane.position = (basePlanePoints[1].first + basePlanePoints[0].first) * 0.5;
	} else if (basePlanePoints.size() == 1) {
		// One base plane point
		plane.position = basePlanePoints[0].first;
		plane.normal = xAxis;
		plane.normal.Normalise();
	} else {
		plane.position = basePlanePoints[0].first;
		plane.normal = xAxis;
		plane.normal.Normalise();
	}

#ifdef debug
	std::ostringstream ss;
	ss<<"plane normal : " <<plane.normal;
	// make sure plane normal is always pointing toward the apex
	dbg(ss.str());
	ss.str("");
	ss<<"xAxis : "<<xAxis;
	dbg(ss.str());
#endif
	if (DotProduct(plane.normal, xAxis) < 0) {
		plane.normal *= -1;
	}

	return plane;
}

void LVHeartMeshModel::AlignModel() {
	std::ostringstream ss;
	if (!aligned) {
		aligned = true;
		Point3D apexPosition = apex;
		Point3D basePosition = base;
		Vector3D xAxis = apexPosition - basePosition;
		xAxis.Normalise();

		std::vector<Point3D>::const_iterator itr = rvInserts.begin();
		std::vector<Point3D>::const_iterator end = rvInserts.end();
		Point3D sum;
		for (; itr != end; ++itr) {
			sum += *itr;
		}

		Point3D averageOfRVInserts = sum / rvInserts.size();

		Vector3D yAxis = averageOfRVInserts - basePosition;
		Vector3D zAxis = CrossProduct(xAxis, yAxis);
		zAxis.Normalise();
		yAxis.CrossProduct(zAxis, xAxis);
#ifdef debug
		ss.str("");
		ss <<"Model coord x axis vector"<<xAxis<<std::endl;
		ss <<"Model coord y axis vector"<<yAxis<<std::endl;
		ss <<"Model coord z axis vector"<<zAxis<<std::endl;
		dbg(ss.str());
#endif
		// Compute the position of the model coord origin. (1/3 of the way from base to apex)
		Point3D origin = basePosition + (0.3333) * (apexPosition - basePosition);

		// Transform heart model using the newly computed axes

		MatrixType axisMat(4, 4);
		axisMat[0][0] = xAxis.x;
		axisMat[0][1] = xAxis.y;
		axisMat[0][2] = xAxis.z;
		axisMat[0][3] = origin.x;
		axisMat[1][0] = yAxis.x;
		axisMat[1][1] = yAxis.y;
		axisMat[1][2] = yAxis.z;
		axisMat[1][3] = origin.y;
		axisMat[2][0] = zAxis.x;
		axisMat[2][1] = zAxis.y;
		axisMat[2][2] = zAxis.z;
		axisMat[2][3] = origin.z;
		axisMat[3][0] = 0.0;
		axisMat[3][1] = 0.0;
		axisMat[3][2] = 0.0;
		axisMat[3][3] = 1.0;

		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				patientToGlobalTransform[i * 4 + j] = axisMat[i][j];
			}
		}

		Cmiss_field_module_begin_change(field_module_);
		Cmiss_field_assign_real(transform_mx_, cache, 16, patientToGlobalTransform);
		Cmiss_field_module_end_change(field_module_);

		// Compute FocalLength
		double lengthFromApexToBase = (apexPosition - basePosition).Length();
#ifdef debug
		ss.str("");
		ss<<"Modeller::AlignModel() : lengthFromApexToBase = "<<lengthFromApexToBase;
		dbg(ss.str());
		ss.str("");
#endif
		double focalLength = (apexPosition - origin).Length() / cosh(1.0);
#ifdef debug
		ss<<"Modeller::AlignModel() : new focal length = "<<focalLength;
		dbg(ss.str());
#endif
		SetFocalLength(focalLength);

		//If ES base plane data is provided, use it to set the base planes
		if (esf > 0) {
			std::map<int, Plane> planes;
			std::vector<std::pair<Point3D, double> > basePlanePointsInOneFrame;
			basePlanePointsInOneFrame.push_back(std::make_pair(edBasePlane[0], 0.0));
			basePlanePointsInOneFrame.push_back(std::make_pair(edBasePlane[1], 0.0));
			// Fit plane to the points
			Plane edplane = FitPlaneToBasePlanePoints(basePlanePointsInOneFrame, xAxis);
			planes.insert(std::make_pair(0, edplane));
			basePlanePointsInOneFrame.clear();
			//Find the mesh closest to the esf
			double esmt = getNearestMeshTime(esf);
			basePlanePointsInOneFrame.push_back(std::make_pair(esBasePlane[0], esmt));
			basePlanePointsInOneFrame.push_back(std::make_pair(esBasePlane[1], esmt));
			Plane esplane = FitPlaneToBasePlanePoints(basePlanePointsInOneFrame, xAxis);
			int frameNumber = (int) (esmt * numberOfModelFrames_) + 1;
			planes.insert(std::make_pair(frameNumber, esplane));
			for (int fi = 0; fi < numberOfModelFrames_; fi++) {
				double time = ((double) fi) / (numberOfModelFrames_ - 1);
				SetTheta(time);
				Plane fiplane = InterpolateBasePlane(planes, fi);
				SetMuFromBasePlaneAtTime(fiplane, time);
			}
		}

#ifdef showAligmentMatch
		{
			int elem;
			Point3D xi;
			elem = ComputeXi(apex, 0.0, xi);
			std::cout << "Apex " << apex << "\t" << elem << "\t" << xi << computeCoordinates(0.0, elem - 1, xi) << std::endl;
			elem = ComputeXi(edBasePlane[0], 0.0, xi);
			std::cout << "EdBasePlane [0] " << edBasePlane[0]<< "\t" << elem << "\t" << xi << computeCoordinates(0.0, elem - 1, xi) << std::endl;
			elem = ComputeXi(edBasePlane[1], 0.0, xi);
			std::cout << "EDBasePlane [1] " << edBasePlane[1]<< "\t" << elem << "\t" << xi << computeCoordinates(0.0, elem - 1, xi) << std::endl;
		}
#endif
		InitialiseBezierLambdaParams();

		//Call to this is necessary to propagate the baseplane variation
		UpdateTimeVaryingModel();

#ifdef fitAxistoInput
		//Check the error between the input apex, base and model apex base
		//Mostly these points do not coincide and an transform is required
		//to align them
		//Find this transformation using optimization
		{
			AxisFittingOptimizationInput * optInput = new struct AxisFittingOptimizationInput;
			optInput->cache = cache;
			optInput->coordinates_rc = coordinates_patient_rc_;
			optInput->field_module = field_module_;
			optInput->cmiss_elements = &elements;
			optInput->transform_mx = transform_mx_;
			Cmiss_mesh_id mesh = Cmiss_field_module_find_mesh_by_dimension(field_module_, 3);
			optInput->mesh = mesh;
			optInput->targetApex = apex;
			optInput->targetBasel = edBasePlane[0];
			optInput->targetBaser = edBasePlane[1];
			//baser (rv inserts) is along the y coordinate
			//element cmiss id is 4 0,1,0
			//basel cmiss id is 2 0,1,0
			//apex cmiss id is 15 0,0,0

			optInput->apexEl = 15;
			optInput->apexXi = Point3D(0,0,0);
			optInput->baselEl = 4;
			optInput->baselXi = Point3D(0,1,0);
			optInput->baserEl = 2;
			optInput->baserXi = Point3D(0,1,0);

			//Do the optimization
			Cmiss_field_cache_set_time(cache, 0.0);
			alglib::real_1d_array x;
			x.setcontent(16, patientToGlobalTransform);
			alglib::minlbfgsstate state;
			alglib::minlbfgsreport rep;
			alglib::ae_int_t maxits = 0;
			double epsg = 0.0000000001;
			double epsf = 0;
			double epsx = 0;
			double diffstep = 1.0e-6;
			clock_t begin = clock();
			try {
				alglib::minlbfgscreatef(16, x, diffstep, state);
				alglib::minlbfgssetcond(state, epsg, epsf, epsx, maxits);
				alglib::minlbfgsoptimize(state, optimize, NULL, optInput);
				alglib::minlbfgsresults(state, x, rep);
			} catch (alglib::ap_error& err) {
				std::cout << err.msg << std::endl;
			}
#define debugAlignment
#ifdef debugAlignment
			clock_t end = clock();
			double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
			std::cout << "Optimization took " << elapsed_secs << "\t error " << optInput->error << std::endl;
			std::cout << "*******************************************************************" << std::endl;
#endif
			for (int pg = 0; pg < 16; pg++)
			patientToGlobalTransform[pg] = x[pg];

			Cmiss_field_module_begin_change(field_module_);
			Cmiss_field_assign_real(transform_mx_, cache, 16, patientToGlobalTransform);
			Cmiss_field_module_end_change(field_module_);
#ifdef debugAlignment
			Point3D nApex = computeCoordinates(0.0, optInput->apexEl - 1, optInput->apexXi);
			Point3D nbasel = computeCoordinates(0.0, optInput->baselEl - 1, optInput->baselXi);
			Point3D nbaser = computeCoordinates(0.0, optInput->baserEl - 1, optInput->baserXi);

			std::cout << optInput->apexEl << "\t" << apex << "\t" << nApex << "\t" << apex.distance(nApex) << std::endl;
			std::cout << optInput->baselEl << "\t" << edBasePlane[0] << "\t" << nbasel << "\t" << edBasePlane[0].distance(nbasel) << std::endl;
			std::cout << optInput->baserEl << "\t" << edBasePlane[1] << "\t" << nbaser << "\t" << edBasePlane[1].distance(nbaser) << std::endl;
			std::cout << "*******************************************************************" << std::endl;
#endif
			Cmiss_mesh_destroy(&mesh);
			delete optInput;
		}
#endif
		//Since the baseplane points are known at esframe time
		//Fit the model at that time to those points
		if (esf > 0)
			FitModel(esBasePlane, esf, false);
		//Set theta to standard to remove any jumps
		for (int fi = 0; fi < numberOfModelFrames_; fi++) {
			double time = ((double) fi) / (numberOfModelFrames_ - 1);
			SetTheta(time);
		}

		//Compute apex base length variation
		{
			double heap[1024];
			double* lengths = heap;
			double* times = heap + numberOfModelFrames_;
			double denom = numberOfModelFrames_ - 1.0;
			double values[3];
			double coord[3];
			for (unsigned int i = 0; i < numberOfModelFrames_; i++) {
				double time = static_cast<double>(i) / denom;

				Cmiss_field_cache_set_time(cache, time);

				coord[0] = 0;
				coord[1] = 1.0;
				coord[2] = 0.0;
				Cmiss_field_cache_set_mesh_location(cache, elements[4 - 1], 3, coord);
				Cmiss_field_evaluate_real(coordinates_patient_rc_, cache, 3, values);
				Point3D bl(values);

				Cmiss_field_cache_set_mesh_location(cache, elements[2 - 1], 3, coord);
				Cmiss_field_evaluate_real(coordinates_patient_rc_, cache, 3, values);
				Point3D br(values);
				coord[0] = 0;
				coord[1] = 0.0;
				coord[2] = 0.0;

				Cmiss_field_cache_set_mesh_location(cache, elements[13 - 1], 3, coord);
				Cmiss_field_evaluate_real(coordinates_patient_rc_, cache, 3, values);
				Point3D ap(values);

				Point3D base = (bl + br) * 0.5;
				Vector3D abv = ap - base;
				lengths[i] = abv.Length();
				times[i] = time;
			}
			//Normalize lengths
			double initLength = lengths[0];
			for (int i = 0; i < numberOfModelFrames_; i++) {
				lengths[i] /= initLength;
			}

			alglib::real_1d_array x;
			x.setcontent(numberOfModelFrames_, times);
			alglib::real_1d_array y;
			y.setcontent(numberOfModelFrames_, lengths);

			// build spline
			try {
				alglib::spline1dbuildakima(x, y, apexBaseVariation);
			} catch (alglib::ap_error& err) {
				std::cout << err.msg << std::endl;
				std::cout << y.tostring(1) << std::endl;
			}

		}
	}
}

void LVHeartMeshModel::UpdateTimeVaryingDataPoints(const cap::Vector& x, int frameNumber) {
	// Update the (Bezier) parameters for the newly fitted frame
	// This is in turn used as data points for the time varying model in the smoothing step

	for (int i = 0; i < 134; i++) {  //valgrind complains about invald write here
		timeVaryingDataPoints_[i][frameNumber] = x[i];
	}
}

std::string LVHeartMeshModel::getTransformation() {
	std::stringstream ss;
	for (int i = 0; i < 16; i++)
		ss << " " << patientToGlobalTransform[i];

	return ss.str();
}

std::vector<double> LVHeartMeshModel::getFramesFromApexBaseLengthVariation() {
	std::vector<double> result(numberOfModelFrames_);
	//Since update time varying points updates the meshes
	//To capture the time variation of the base
	std::ostringstream ss;
	double denom = numberOfModelFrames_ - 1;
	for (int i = 0; i < numberOfModelFrames_; i++) {
		double tx = i / denom;
		ss << tx << ",";
		result[i] = tx;
	}
	ss << "\b";
	std::string outp = ss.str();
	timeSamplingInterval = outp.substr(0, outp.length() - 2);

	return result;
}

Point3D LVHeartMeshModel::getApex() {
	return apex;
}

Point3D LVHeartMeshModel::getBase() {
	return base;
}

void LVHeartMeshModel::SetTheta(double time) {
	const double thetas[4] = { 0, M_PI_2, M_PI, M_PI_2 * 3.0 };
	const int NUMBER_OF_NODES = nodes.size();
	Cmiss_field_module_begin_change(field_module_);
	Cmiss_field_cache_set_time(cache, time);
	double pscoord[3];
	for (int i = 0; i < NUMBER_OF_NODES; i++) {
		Cmiss_field_cache_set_node(cache, nodes[i]);
		Cmiss_field_evaluate_real(coordinates_ps_, cache, 3, pscoord);
		pscoord[2] = thetas[i % 4];
		Cmiss_field_assign_real(coordinates_ps_, cache, 3, pscoord);
	}
	Cmiss_field_module_end_change(field_module_);
}

Plane LVHeartMeshModel::InterpolateBasePlane(const std::map<int, Plane>& planes, int frame) {
	std::map<int, Plane>::const_iterator itr = planes.begin();

	int prevFrame = 0;
	Plane prevPlane;
	while (itr->first < frame && itr != planes.end()) {
		prevFrame = itr->first;
		prevPlane = itr->second;
		itr++;
	}
	if (itr->first == frame) // Key frame, no interpolation needed
			{
		return itr->second;
	}

	// Handle edge cases where prevFrame > nextFrame (i.e interpolation occurs around the end point)
	int nextFrame;
	Plane nextPlane;
	int maxFrame = numberOfModelFrames_;
	if (itr == planes.end()) {
		nextFrame = planes.begin()->first + maxFrame;
		nextPlane = planes.begin()->second;
	} else {
		nextFrame = itr->first;
		nextPlane = itr->second;
	}

	if (itr == planes.begin()) {
		std::map<int, Plane>::const_reverse_iterator last = planes.rbegin();
		prevFrame = last->first - maxFrame;
		prevPlane = last->second;
	}

	Plane plane;
	double coefficient = (double) (frame - prevFrame) / (nextFrame - prevFrame);

	plane.normal = prevPlane.normal + coefficient * (nextPlane.normal - prevPlane.normal);

	plane.position = prevPlane.position + coefficient * (nextPlane.position - prevPlane.position);

	return plane;
}

std::vector<double> LVHeartMeshModel::ConvertToHermite(const cap::Vector& bezierParams) {
	cap::Vector* hermiteParams = (*G_).mult(bezierParams);

	int indices[128] = { 26, 25, 22, 21, 6, 5, 2, 1, 27, 26, 23, 22, 7, 6, 3, 2, 28, 27, 24, 23, 8, 7, 4, 3, 25, 28, 21, 24, 5, 8, 1, 4, 30, 29, 26, 25, 10, 9, 6, 5, 31, 30, 27,
			26, 11, 10, 7, 6, 32, 31, 28, 27, 12, 11, 8, 7, 29, 32, 25, 28, 9, 12, 5, 8, 34, 33, 30, 29, 14, 13, 10, 9, 35, 34, 31, 30, 15, 14, 11, 10, 36, 35, 32, 31, 16, 15, 12,
			11, 33, 36, 29, 32, 13, 16, 9, 12, 38, 37, 34, 33, 18, 17, 14, 13, 39, 38, 35, 34, 19, 18, 15, 14, 40, 39, 36, 35, 20, 19, 16, 15, 37, 40, 33, 36, 17, 20, 13, 16 };

	int invertedIndices[40];

	for (int i = 0; i < 128; i++) {
		invertedIndices[indices[i] - 1] = i;
	}

	std::vector<double> temp(160);

	for (int i = 0; i < 40; i++) {
		temp[i * 4] = (*hermiteParams)[invertedIndices[i] * 4];
		temp[i * 4 + 1] = (*hermiteParams)[invertedIndices[i] * 4 + 1];
		temp[i * 4 + 2] = (*hermiteParams)[invertedIndices[i] * 4 + 2];
		temp[i * 4 + 3] = (*hermiteParams)[invertedIndices[i] * 4 + 3];
	}

	delete hermiteParams;
	return temp;
}

void LVHeartMeshModel::UpdateTimeVaryingModel() {
	double denom = numberOfModelFrames_ - 1;
	for (int j = 0; j < numberOfModelFrames_; j++) {
		double time = static_cast<double>(j) / denom; //--(double)j/heartModel_.GetNumberOfModelFrames();
		cap::Vector* x = solverFactory_->CreateVector(134);
		for (int i = 0; i < 134; i++) {
			(*x)[i] = timeVaryingDataPoints_[i][j];
		}
		const std::vector<double>& hermiteLambdaParams = ConvertToHermite(*x);
		SetLambdaAtTime(hermiteLambdaParams, time);
		delete x;
	}
}

void LVHeartMeshModel::FitModel(std::vector<Point3D> points, double stime, bool useBoundaryXi) {
	double time = getNearestMeshTime(stime);
	unsigned int numPoints = points.size();
	if (numPoints > 0) {

		if (!aligned)
			AlignModel();
		std::vector<Point3D>* xivec;
		std::vector<int>* elvec;
		std::vector<Point3D> xit;
		std::vector<int> elem_idt;
		if (useBoundaryXi) {
			xivec = &speckleXi;
			elvec = &speckleElements;
		} else {
			for (int i = 0; i < numPoints; i++) {
				Point3D xip;
				//Since ComputeXi returns cmiss element id at base 1, subtract 1 to translate it to base zero as the element id is used for
				//index calculations
				int id = ComputeXi(points[i], time, xip) - 1;
				xip.z = 0.0;
				xit.push_back(xip);
				elem_idt.push_back(id);
			}
			xivec = &xit;
			elvec = &elem_idt;
		}

		std::vector<Point3D>& xi = *xivec;
		std::vector<int>& elem_id = *elvec;

		// Compute P
		// 1. Map xi coords for each data point in prolate spheroidal coordinates
		std::vector<Point3D> pscoord = ConvertToHeartModelProlateSpheriodalCoordinate(points);

		std::vector<Point3D>::iterator itr = pscoord.begin();
		cap::Vector* guidePointLambda = solverFactory_->CreateVector(numPoints);

		for (int i = 0; itr != pscoord.end(); ++itr, i++) {
			//Map the coordinate value to the xi location
			//Modify h2p function to take a point, use predefined node and coordinates
			(*guidePointLambda)[i] = itr->x; // x = lambda, y = mu, z = theta
		}

		// 2. evaluate basis at the xi coords
		double psi[32]; //FIX 32?
		std::vector<cap::Entry> entries;
		cap::BiCubicHermiteLinearBasis basis;
		std::vector<Point3D>::iterator itr_xi = xi.begin();
		std::vector<Point3D>::const_iterator end_xi = xi.end();

		unsigned int maxColIndex = 512; //The element_id's should range from 0 - 15, note that cmiss ranges from 1-16

		for (int xiIndex = 0; itr_xi != end_xi; ++itr_xi, ++xiIndex) {
			//Do endo
			double temp[3];
			temp[0] = itr_xi->x;
			temp[1] = itr_xi->y;
			temp[2] = itr_xi->z;
			basis.Evaluate(psi, temp);
			for (int nodalValueIndex = 0; nodalValueIndex < 32; nodalValueIndex++) {
				cap::Entry e;
				e.value = psi[nodalValueIndex];
				e.colIndex = 32 * (elem_id[xiIndex]) + nodalValueIndex;
				e.rowIndex = xiIndex;
				entries.push_back(e);
			}

		}

		// 3. construct P
		cap::SparseMatrix* P = solverFactory_->CreateSparseMatrix(numPoints, maxColIndex, entries);

		aMatrix_->UpdateData(*P);

		// Compute RHS - GtPt(dataLamba - priorLambda)

		cap::Vector* lambda = G_->mult(*prior_);

		cap::Vector* p = P->mult(*lambda);

		*guidePointLambda -= *p;
		// rhs = GtPt p
		cap::Vector* temp = P->trans_mult(*guidePointLambda);
		cap::Vector* rhs = G_->trans_mult(*temp);

		// Solve Normal equation
		const double tolerance = 1.0e-3;
		const int maximumIteration = 1000;

		cap::Vector* x = solverFactory_->CreateVector(134); //FIX magic number

		solverFactory_->CG(*aMatrix_, *x, *rhs, *preconditioner_, maximumIteration, tolerance);

		*x += *prior_;

		const std::vector<double>& hermiteLambdaParams = ConvertToHermite(*x);

		SetLambdaAtTime(hermiteLambdaParams, time);
		int mframe = time * numberOfModelFrames_;
		UpdateTimeVaryingDataPoints(*x, mframe); //Bezier

		delete P;
		delete lambda;
		delete p;
		delete guidePointLambda;
		delete temp;
		delete rhs;
		delete x;

		//Ensure that update timevarying data points knows about this frame
		framesWithDataPoints[mframe] = 1.0;
		SmoothAlongTime();
	}

}

void LVHeartMeshModel::SmoothAlongTime() {
	// For each global parameter in the per frame model
#ifdef debug
	clock_t before = clock();
#endif
	double denom = numberOfModelFrames_ - 1.0;
	for (int i = 0; i < 134; i++) // FIX magic number
	{
		std::vector<double> lambdas = timeSmoother_.FitModel(i, timeVaryingDataPoints_[i], framesWithDataPoints);

		for (int j = 0; j < numberOfModelFrames_; j++) //FIX duplicate code
				{
			double xi = static_cast<double>(j) / denom;
			double lambda = timeSmoother_.ComputeLambda(xi, lambdas);
			timeVaryingDataPoints_[i][j] = lambda;
		}
	}

#ifdef debug
	clock_t after = clock();
	std::ostringstream ss;
	ss<<solverFactory_->GetName() << " Smoothing time = " <<((after - before) / static_cast<double>(CLOCKS_PER_SEC));
	dbg(ss.str());
#endif
	// feed the results back to Cmgui
	UpdateTimeVaryingModel();
}

void LVHeartMeshModel::outputMesh(std::string prefix, std::string directory) {
	Cmiss_region_id root_region = Cmiss_context_get_default_region(context_);
	Cmiss_region_id region = root_region;
	Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(region);
	Cmiss_stream_information_region_id region_stream_information = Cmiss_stream_information_cast_region(stream_information);
	std::ostringstream ss;
	Cmiss_stream_resource_id* stream = new Cmiss_stream_resource_id[numberOfModelFrames_];
	double denom = numberOfModelFrames_ - 1;
	for (unsigned int i = 0; i < numberOfModelFrames_; i++) {
		double time = ((double) i) / denom;
		ss.str("");
		ss << directory << "/" << prefix << "." << i << ".exregion";
		stream[i] = Cmiss_stream_information_create_resource_file(stream_information, ss.str().c_str());
		Cmiss_stream_information_region_set_resource_attribute_real(region_stream_information, stream[i], CMISS_STREAM_INFORMATION_REGION_ATTRIBUTE_TIME, time);
	}

	Cmiss_region_write(region, stream_information);

	for (unsigned int i = 0; i < numberOfModelFrames_; i++) {
		Cmiss_stream_resource_destroy(&(stream[i]));
	}
	delete[] stream;
	Cmiss_stream_information_destroy(&stream_information);
	Cmiss_stream_information_region_destroy(&region_stream_information);
	Cmiss_region_destroy(&root_region);
}

std::string LVHeartMeshModel::getTimeSamplingInterval() {
	if (aligned) {
		if (timeSamplingInterval != "") {
			getFramesFromApexBaseLengthVariation();
		}
	}
	return timeSamplingInterval;
}

void LVHeartMeshModel::setESFrameData(double esp, std::vector<Point3D> bpts) {
	esf = esp;
	esBasePlane = bpts;
}

void LVHeartMeshModel::setEDFrameData(std::vector<Point3D> bpts) {
	edBasePlane = bpts;
}

Point3D LVHeartMeshModel::computeCoordinates(double time, int elementid, Point3D& xi) {
	Cmiss_field_cache_set_time(cache, time);
	double xiv[3], temp[3];
	xiv[0] = xi.x;
	xiv[1] = xi.y;
	xiv[2] = xi.z;
	Cmiss_field_cache_set_mesh_location(cache, elements[elementid], 3, xiv);
	Cmiss_field_evaluate_real(coordinates_patient_rc_, cache, 3, temp);
	return Point3D(temp);
}

double LVHeartMeshModel::getNearestMeshTime(double time) {
	int idx = time * numberOfModelFrames_;
	double esmt = time;
	if (idx > -1 && idx < numberOfModelFrames_ - 1) {
		while (timePoints[idx++] <= time)
			;
		esmt = timePoints[idx - 1];
	} else if (idx >= numberOfModelFrames_ - 1) {
		esmt = timePoints[numberOfModelFrames_ - 1];
	}
	return esmt;
}

std::vector<Point3D> LVHeartMeshModel::computePlaneBoundary(Point3D bl, Point3D br, double time, double wallxi) {
	std::vector<Point3D> result;
	const int ep1[] = { 2, 6, 10, 14, 12, 4, 8, 0 };
	const int ep2[] = { 1, 5, 9, 13, 15, 11, 7, 3 };
	const int bep1[] = { 0, 8, 4, 12, 14, 10, 6, 2 };
	const int bep2[] = { 3, 7, 11, 15, 13, 9, 5, 1 };
	Point3D blxi, brxi;
	int blid = ComputeXi(bl, time, blxi) - 1;
	int brid = ComputeXi(br, time, brxi) - 1;
	int const * elementlist = ep1;
	if (blid == ep1[0] || brid == ep1[7]) {
		elementlist = ep1;
	} else if (blid == ep2[0] || brid == ep2[7]) {
		elementlist = ep2;
	} else if (blid == bep1[0] || brid == bep1[7]) {
		elementlist = bep1;
	} else if (blid == bep2[0] || brid == bep2[7]) {
		elementlist = bep2;
	} else {
		std::cerr << "Plane boundary cannot be found" << std::endl;
		std::cerr << " BLID " << blid << "\t" << " BRID " << brid << std::endl;
		throw -1;
	}
	std::cout << "BL " << blid + 1 << "\t" << blxi << "\t" << bl << "\t" << computeCoordinates(time, blid, blxi) << std::endl;
	std::cout << "BR " << brid + 1 << "\t" << brxi << "\t" << br << "\t" << computeCoordinates(time, brid, brxi) << std::endl;

	double xi[3], coord[3];
	xi[0] = blxi.x;
	xi[2] = wallxi;
	//Vary xi[1] from 1 to 0 for bl
	const double estep = 0.5;
	Cmiss_field_cache_set_time(cache, time);
	for (int e = 0; e < 4; e++) {
		for (double xiv = 1.0; xiv > -0.01; xiv -= estep) {
			xi[1] = xiv;
			Cmiss_field_cache_set_mesh_location(cache, elements[elementlist[e]], 3, xi);
			Cmiss_field_evaluate_real(coordinates_patient_rc_, cache, 3, coord);
			std::cout << elementlist[e] << "\t" << Point3D(xi) << "\t" << Point3D(coord) << std::endl;
			result.push_back(Point3D(coord));
		}
	}
	//Vary xi[1] from 0 to 1 for br
	for (int e = 4; e < 8; e++) {
		for (double xiv = 0.0; xiv < 1.01; xiv += estep) {
			xi[1] = xiv;
			Cmiss_field_cache_set_mesh_location(cache, elements[elementlist[e]], 3, xi);
			Cmiss_field_evaluate_real(coordinates_patient_rc_, cache, 3, coord);
			std::cout << elementlist[e] << "\t" << Point3D(xi) << "\t" << Point3D(coord) << std::endl;
			result.push_back(Point3D(coord));
		}
	}
	return result;
}

std::vector<Point3D> LVHeartMeshModel::getRefinedMeshCoordinates(double time) {
	//First provide endo markers followed by epi markers
	//Each element is split into 3 along the xi1
	const unsigned int elemids[] = { 1, 2, 3, 0, 5, 6, 7, 4, 9, 10, 11, 8, 13, 14, 15, 12 };
	std::vector<Point3D> points;
	double endoXi[3] = { 0, 1, 0 };
	double temp[3];
	double xistep = 1.0 / 3.0;
	Cmiss_field_cache_set_time(cache, time);
	for (int i = 0; i < 16; i++) {
		for (double xi = 1.0; xi > 0.05; xi -= xistep) {
			endoXi[0] = xi;
			Cmiss_field_cache_set_mesh_location(cache, elements[elemids[i]], 3, endoXi);
			Cmiss_field_evaluate_real(coordinates_patient_rc_, cache, 3, temp);
			points.push_back(Point3D(temp));
		}
	}
	//Get apex value
	endoXi[0] = 0;
	endoXi[1] = 0;
	endoXi[2] = 0;
	Cmiss_field_cache_set_mesh_location(cache, elements[13], 3, endoXi);
	Cmiss_field_evaluate_real(coordinates_patient_rc_, cache, 3, temp);
	points.push_back(Point3D(temp));
	//For epi
	endoXi[0] = 0;
	endoXi[1] = 1;
	endoXi[2] = 1;
	for (int i = 0; i < 16; i++) {
		for (double xi = 1.0; xi > 0.05; xi -= xistep) {
			endoXi[0] = xi;
			Cmiss_field_cache_set_mesh_location(cache, elements[elemids[i]], 3, endoXi);
			Cmiss_field_evaluate_real(coordinates_patient_rc_, cache, 3, temp);
			points.push_back(Point3D(temp));
		}
	}
	//Get the apex
	endoXi[0] = 0;
	endoXi[1] = 0;
	endoXi[2] = 1;
	Cmiss_field_cache_set_mesh_location(cache, elements[13], 3, endoXi);
	Cmiss_field_evaluate_real(coordinates_patient_rc_, cache, 3, temp);
	points.push_back(Point3D(temp));

	return points;
}
