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

#include "LVHeartMesh.h"
#include <cmath>
//These variable headers should be included once in the object
#include "refinedheart.h"
#include "MeshTopology.h"

LVHeartMesh::LVHeartMesh(std::string name, XMLInputReader& reader) :
		inputReader(reader) {
	numberOfModelFrames_ = inputReader.getNumberOfFrames();
	frameTimes = inputReader.getFrameSamplingTimes();
	baseridx = 8;
	focalLength = 0.0;
	endoNodesUpdated = false;
	field_ = NULL, field_module_ = NULL, coordinates_ps_ = NULL, coordinates_rc_ = NULL;
	coordinates_patient_rc_ = NULL, transform_mx_ = NULL, aligned = false;

	contextName = name;
	//Initialise cmgui and get context
	context_ = Cmiss_context_create(name.c_str());
	/**< Handle to the context */
	Cmiss_region_id root_region = Cmiss_context_get_default_region(context_);

	//Load the finite element mesh of the heart

	// Create a heart region to stop Cmgui adding lines to the new rendition
	std::string region_name = "heart";
	Cmiss_region_id heart_region = Cmiss_region_create_child(root_region, region_name.c_str());
	//Get the field module of the heart region
	field_module_ = Cmiss_region_get_field_module(heart_region);

	Cmiss_region_destroy(&heart_region);
	// Read in the heart model spaced over the number of model frames

	for (unsigned int i = 0; i < numberOfModelFrames_; i++)
	{
		double time = frameTimes[i];
		Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
		Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
		Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, nicemesh_exregion, nicemesh_exregion_len);
		Cmiss_stream_information_region_set_resource_attribute_real(stream_information_region, stream_resource, CMISS_STREAM_INFORMATION_REGION_ATTRIBUTE_TIME, time);

		Cmiss_region_read(root_region, stream_information);

		Cmiss_stream_resource_destroy(&stream_resource);
		Cmiss_stream_information_destroy(&stream_information);
		Cmiss_stream_information_region_destroy(&stream_information_region);
	}

	//Update handles and define heart fields
	// initialize patientToGlobalTransform_ to identity matrix
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			if (i == j)
			{
				patientToGlobalTransform[4 * i + j] = 1.0;
			}
			else
			{
				patientToGlobalTransform[4 * i + j] = 0.0;
			}
		}
	}

	if (field_module_ != 0)
	{
		Cmiss_field_module_begin_change(field_module_);

		// 'coordinates' is an assumed field from the element template file.  It is also assumed to be
		// a prolate spheriodal coordinate system.
		coordinates_ps_ = Cmiss_field_module_find_field_by_name(field_module_, "coordinates");

		// Cannot make all these fields via the Cmgui API yet.
		ds1_ = Cmiss_field_module_create_field(field_module_, "d_ds1", "node_value fe_field coordinates d/ds1");
		ds2_ = Cmiss_field_module_create_field(field_module_, "d_ds2", "node_value fe_field coordinates d/ds2");
		ds1ds2_ = Cmiss_field_module_create_field(field_module_, "d2_ds1ds2", "node_value fe_field coordinates d2/ds1ds2");
		Cmiss_field_module_define_field(field_module_, "coordinates_rc", "coordinate_transformation field coordinates");
		//Get the rectangular cartesian coordinates associated with the mesh
		coordinates_rc_ = Cmiss_field_module_find_field_by_name(field_module_, "coordinates_rc");
		//Create a field which stores the transformation matrix
		transform_mx_ = Cmiss_field_module_create_constant(field_module_, 16, patientToGlobalTransform);

		//Get the coordinates for the mesh in the patient coordinate system
		//Even though the patientToGlobalTransform is identity here
		//The field transform_mx_ is modified in Align model and the patient coordinate system
		//is updated accordingly by cmiss
		coordinates_patient_rc_ = Cmiss_field_module_create_projection(field_module_, coordinates_rc_, transform_mx_);
		Cmiss_field_set_name(coordinates_patient_rc_, "coordinates_patient_rc");

		Cmiss_field_module_end_change(field_module_);
		//Get the cmiss node handles
		cmiss_nodes = std::vector<Cmiss_node_id>(NUMBER_OF_NODES);
		Cmiss_nodeset_id nodeset = Cmiss_field_module_find_nodeset_by_name(field_module_, "cmiss_nodes");
		Cmiss_node_iterator_id nodeIterator = Cmiss_nodeset_create_node_iterator(nodeset);

		Cmiss_node_id node = Cmiss_node_iterator_next(nodeIterator);
		if (node != 0)
		{
			while (node)
			{
				int node_id = Cmiss_node_get_identifier(node);
				cmiss_nodes[node_id - 1] = node;
				node = Cmiss_node_iterator_next(nodeIterator);
			}
		}
		Cmiss_nodeset_destroy(&nodeset);
		Cmiss_node_iterator_destroy(&nodeIterator);

	}
	else
		dbg("--- No field module for heart region!!!");

	Cmiss_region_destroy(&root_region);

	//Create Nodes Maps
	aplaxNodes.resize(9);
	aplaxNodes[8] = aplaxNodes8;
	aplaxNodes[7] = aplaxNodes7;
	aplaxNodes[6] = aplaxNodes6;
	aplaxNodes[5] = aplaxNodes5;
	aplaxNodes[4] = aplaxNodes4;
	aplaxNodes[3] = aplaxNodes3;
	aplaxNodes[2] = aplaxNodes2;
	aplaxNodes[1] = aplaxNodes1;
	aplaxNodes[0] = aplaxNodes0;

	fchNodes.resize(9);
	fchNodes[0] = fchNodes0;
	fchNodes[1] = fchNodes1;
	fchNodes[2] = fchNodes2;
	fchNodes[3] = fchNodes3;
	fchNodes[4] = fchNodes4;
	fchNodes[5] = fchNodes5;
	fchNodes[6] = fchNodes6;
	fchNodes[7] = fchNodes7;
	fchNodes[8] = fchNodes8;

	tchNodes.resize(9);
	tchNodes[0] = tchNodes0;
	tchNodes[1] = tchNodes1;
	tchNodes[2] = tchNodes2;
	tchNodes[3] = tchNodes3;
	tchNodes[4] = tchNodes4;
	tchNodes[5] = tchNodes5;
	tchNodes[6] = tchNodes6;
	tchNodes[7] = tchNodes7;
	tchNodes[8] = tchNodes8;

	hasAPLAX = false;
	hasTCH = false;
	hasFCH = false;
	hasSAXApex = false;
	hasSAXMid = false;
	hasSAXBase = false;

	std::vector<Point3D> pts = inputReader.getMarkers(0, APLAX, false); //False used to reduce computation as we check if the view plane is present and not the actual coords
	if (pts.size() > 0)
	{
		myNodes.insert(myNodes.end(), aplaxNodes.begin(), aplaxNodes.end());
		hasAPLAX = true;
	}
	pts = inputReader.getMarkers(0, TCH, false);
	if (pts.size() > 0)
	{
		myNodes.insert(myNodes.end(), tchNodes.begin(), tchNodes.end());
		hasTCH = true;
	}
	pts = inputReader.getMarkers(0, FCH, false);
	if (pts.size() > 0)
	{
		myNodes.insert(myNodes.end(), fchNodes.begin(), fchNodes.end());
		hasFCH = true;
	}

	pts = inputReader.getMarkers(0, SAXAPEX, false);
	if (pts.size() > 0)
	{
		hasSAXApex = true;
	}
	pts = inputReader.getMarkers(0, SAXMID, false);
	if (pts.size() > 0)
	{
		hasSAXMid = true;
	}
	pts = inputReader.getMarkers(0, SAXBASE, false);
	if (pts.size() > 0)
	{
		hasSAXBase = true;
	}

	//Start initialization
	//Set the apex coordinates
	setApex(reader.getApex());
	//Set the base coordinates
	setBase(reader.getBase());
	//Set the RV insert coordinates to enable computation of coordinate system
	setRVInserts(reader.getRVInserts());
	//Set the array index where the right base coordinate value is stored
	//For instance when there are more than one views, their markers are stored serially and this helps to resolve it
	setBaseRIndex(reader.getBaseRIndex());
	//Set the mesh up to get the PS coordinate values for PLS
	AlignModel();

}

LVHeartMesh::~LVHeartMesh() {
	if (coordinates_ps_ != NULL)
	{
		Cmiss_field_destroy(&ds1_);
		Cmiss_field_destroy(&ds2_);
		Cmiss_field_destroy(&ds1ds2_);
		Cmiss_field_destroy(&coordinates_ps_);
		Cmiss_field_destroy(&coordinates_patient_rc_);
		Cmiss_field_destroy(&transform_mx_);
	}
	for (int i = 0; i < NUMBER_OF_NODES; i++)
	{
		Cmiss_node_destroy(&cmiss_nodes[i]);
	}
	Cmiss_field_destroy(&coordinates_rc_);

	Cmiss_field_module_destroy(&field_module_);
	Cmiss_context_destroy(&context_);
}

void LVHeartMesh::setApex(Point3D apex) {
	this->apex = apex;
}

void LVHeartMesh::setBase(Point3D base) {
	this->base = base;
}

void LVHeartMesh::setRVInserts(std::vector<Point3D> points) {
	PointVector::const_iterator itr = points.begin();
	const PointVector::const_iterator end = points.end();

	while (itr != end)
	{
		rvInserts.push_back(*itr);
		++itr;
	}
}

double LVHeartMesh::GetFocalLength() const {
	return Cmiss_field_get_attribute_real(coordinates_ps_, CMISS_FIELD_ATTRIBUTE_COORDINATE_SYSTEM_FOCUS);
}

void LVHeartMesh::SetFocalLength(double focalLength) {
	this->focalLength = focalLength;
	Cmiss_field_set_attribute_real(coordinates_ps_, CMISS_FIELD_ATTRIBUTE_COORDINATE_SYSTEM_FOCUS, focalLength);
}

double*
LVHeartMesh::getTransformationArray() {
	return patientToGlobalTransform;
}

void LVHeartMesh::fixToMarkers() {
	fixEndoNodes(); //Ensure that endo nodes match the user input
	std::vector<Point3D> pts;

	std::vector<std::string> fitmesh(numberOfModelFrames_);
	for (int i = 0; i < numberOfModelFrames_; i++)
	{

		LongAxisFitting cmfit(getMeshNodeValuesAt(i), hasAPLAX, hasTCH, hasFCH); //Create a RC mesh with exact values at nodes
		std::vector<Point3D> markers;
		pts = inputReader.getMarkers(i, APLAX, true);
		markers.insert(markers.end(), pts.begin(), pts.end());
		pts = inputReader.getMarkers(i, TCH, true);
		markers.insert(markers.end(), pts.begin(), pts.end());
		pts = inputReader.getMarkers(i, FCH, true);
		markers.insert(markers.end(), pts.begin(), pts.end());
		cmfit.setMarkers(&markers, &myNodes);
		fitmesh[i] = cmfit.getOptimizedMesh();
		//Get the updated node coordinates and set it for current mesh
	}


	//Regularize the volume
	VolumeRegularizer regularizer(fitmesh, focalLength);
	regularizer.regularize();
	fitmesh = regularizer.getMesh();
	myocardialVolumes = regularizer.getVolumes();

	if(hasSAXApex||hasSAXBase||hasSAXMid){
		TorsionFitting tfit(fitmesh,focalLength);
		tfit.setUp(inputReader);
		fitmesh = tfit.getMesh();
	}


	//Destroy current context and recreate with fit mesh
	Cmiss_field_destroy(&ds1_);
	Cmiss_field_destroy(&ds2_);
	Cmiss_field_destroy(&ds1ds2_);
	Cmiss_field_destroy(&coordinates_ps_);
	Cmiss_field_destroy(&coordinates_rc_);
	Cmiss_field_destroy(&coordinates_patient_rc_);
	Cmiss_field_destroy(&transform_mx_);
	//Destroy the node handles too
	for (int i = 0; i < NUMBER_OF_NODES; i++)
	{
		Cmiss_node_destroy(&cmiss_nodes[i]);
	}
	Cmiss_field_module_destroy(&field_module_);
	Cmiss_context_destroy(&context_);

	coordinates_ps_ = NULL;
	coordinates_patient_rc_ = NULL;
	transform_mx_ = NULL;

	//Initialise cmgui and get context
	context_ = Cmiss_context_create(contextName.c_str());
	/**< Handle to the context */
	Cmiss_region_id root_region = Cmiss_context_get_default_region(context_);

	for (unsigned int i = 0; i < numberOfModelFrames_; i++)
	{
		double time = frameTimes[i];
		Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
		Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
		Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, fitmesh[i].c_str(), fitmesh[i].length());
		Cmiss_stream_information_region_set_resource_attribute_real(stream_information_region, stream_resource, CMISS_STREAM_INFORMATION_REGION_ATTRIBUTE_TIME, time);

		Cmiss_region_read(root_region, stream_information);

		Cmiss_stream_resource_destroy(&stream_resource);
		Cmiss_stream_information_destroy(&stream_information);
		Cmiss_stream_information_region_destroy(&stream_information_region);
	}

	Cmiss_region_id heart_region = Cmiss_region_find_child_by_name(root_region, "heart");
	//Get the field module of the heart region
	field_module_ = Cmiss_region_get_field_module(heart_region);

	if (field_module_ != 0)
	{
		Cmiss_field_module_begin_change(field_module_);

		// 'coordinates' is an assumed field in
		// a rc coordinate system.
		coordinates_rc_ = Cmiss_field_module_find_field_by_name(field_module_, "coordinates");

		// Cannot make all these fields via the Cmgui API yet.
		Cmiss_field_module_define_field(field_module_, "d_ds1", "node_value fe_field coordinates d/ds1");
		Cmiss_field_module_define_field(field_module_, "d_ds2", "node_value fe_field coordinates d/ds2");
		Cmiss_field_module_define_field(field_module_, "d2_ds1ds2", "node_value fe_field coordinates d2/ds1ds2");

		if (!coordinates_rc_)
		{
			dbg("--- RC Coordinates could not be defined for heart region!!! (Fit to Markers)");
			std::cout << "No coordinates RC " << std::endl;
		}

		Cmiss_field_module_end_change(field_module_);
		//Get node handles
		Cmiss_nodeset_id nodeset = Cmiss_field_module_find_nodeset_by_name(field_module_, "cmiss_nodes");
		Cmiss_node_iterator_id nodeIterator = Cmiss_nodeset_create_node_iterator(nodeset);

		Cmiss_node_id node = Cmiss_node_iterator_next(nodeIterator);
		if (node != 0)
		{
			while (node)
			{
				int node_id = Cmiss_node_get_identifier(node);
				cmiss_nodes[node_id - 1] = node;
				node = Cmiss_node_iterator_next(nodeIterator);
			}
		}
		Cmiss_nodeset_destroy(&nodeset);
		Cmiss_node_iterator_destroy(&nodeIterator);

		//The heart transformation matrix is changed
		//So set it to be unit
		memset(patientToGlobalTransform, 0, sizeof(double) * 16);
		patientToGlobalTransform[0] = patientToGlobalTransform[5] = patientToGlobalTransform[10] = patientToGlobalTransform[15] = 1.0;
	}
	else
	{
		dbg("--- No field module for heart region!!!");
		std::cout << "No field module " << std::endl;
	}

	Cmiss_region_destroy(&heart_region);
	Cmiss_region_destroy(&root_region);

}

/*
 * Convert the RC coordinates into a prolate Spheriodal coordinates
 * using the mesh information
 */

std::vector<Point3D> LVHeartMesh::ConvertToHeartModelProlateSpheriodalCoordinate(std::vector<Point3D>& point) {

	std::vector<Point3D> result(point);

	Cmiss_field_module_id field_module = field_module_;
	Cmiss_field_module_begin_change(field_module);

	//Get the handles to create temporary node for performing the transformation
	Cmiss_nodeset_id nodeset = Cmiss_field_module_find_nodeset_by_name(field_module, "cmiss_nodes");
	Cmiss_field_id coordinates = Cmiss_field_module_find_field_by_name(field_module, "coordinates");
	Cmiss_node_template_id nodeTemplate = Cmiss_nodeset_create_node_template(nodeset);
	Cmiss_node_template_define_field(nodeTemplate, coordinates);
	Cmiss_field_cache_id cache = Cmiss_field_module_create_cache(field_module);

	//The input coordinates are in rectangular Cartesian coordinates
	//Convert them to prolate spheroidal

	Cmiss_node_id temporaryNode = Cmiss_nodeset_create_node(nodeset, 100005 /*Fix this */, nodeTemplate);

	Cmiss_field_cache_set_node(cache, temporaryNode);

	unsigned int numpoints = point.size();

	for (int i = 0; i < numpoints; i++)
	{
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
	Cmiss_field_cache_destroy(&cache);
	Cmiss_node_template_destroy(&nodeTemplate);
	Cmiss_nodeset_destroy(&nodeset);
	Cmiss_field_module_end_change(field_module);

	return result;

}

void LVHeartMesh::AlignModel() {
	std::ostringstream ss;
	if (!aligned)
	{
		aligned = true;
		Point3D apexPosition = apex;
		Point3D basePosition = base;
		Vector3D xAxis = apexPosition - basePosition;
		xAxis.Normalise();

		std::vector<Point3D>::const_iterator itr = rvInserts.begin();
		std::vector<Point3D>::const_iterator end = rvInserts.end();
		Point3D sum;
		for (; itr != end; ++itr)
		{
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

		gtMatrix patientToGlobalTransform_;

		patientToGlobalTransform_[0][0] = static_cast<float>(xAxis.x);
		patientToGlobalTransform_[0][1] = static_cast<float>(xAxis.y);
		patientToGlobalTransform_[0][2] = static_cast<float>(xAxis.z);
		patientToGlobalTransform_[0][3] = 0.0; //NB this is the first column not row
		patientToGlobalTransform_[1][0] = static_cast<float>(yAxis.x);
		patientToGlobalTransform_[1][1] = static_cast<float>(yAxis.y);
		patientToGlobalTransform_[1][2] = static_cast<float>(yAxis.z);
		patientToGlobalTransform_[1][3] = 0.0;
		patientToGlobalTransform_[2][0] = static_cast<float>(zAxis.x);
		patientToGlobalTransform_[2][1] = static_cast<float>(zAxis.y);
		patientToGlobalTransform_[2][2] = static_cast<float>(zAxis.z);
		patientToGlobalTransform_[2][3] = 0.0;
		patientToGlobalTransform_[3][0] = static_cast<float>(origin.x);
		patientToGlobalTransform_[3][1] = static_cast<float>(origin.y);
		patientToGlobalTransform_[3][2] = static_cast<float>(origin.z);
		patientToGlobalTransform_[3][3] = 1.0;

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				// gtMatrix is column-major and Cmgui wants Row-major we transpose the matrix here.
				patientToGlobalTransform[i + 4 * j] = patientToGlobalTransform_[i][j];
			}
		}

		Cmiss_field_module_begin_change(field_module_);
		Cmiss_field_cache_id cache = Cmiss_field_module_create_cache(field_module_);
		Cmiss_field_assign_real(transform_mx_, cache, 16, patientToGlobalTransform);
		Cmiss_field_cache_destroy(&cache);
		Cmiss_field_module_end_change(field_module_);

		// Compute FocalLength
		double lengthFromApexToBase = (apexPosition - basePosition).Length();
		double focalLength = (apexPosition - origin).Length() / cosh(1.0);

		SetFocalLength(focalLength);
	}
}

std::string LVHeartMesh::getTransformation() {
	if (!aligned)	//Ensure that the transformation is computed
		AlignModel();
	std::stringstream ss;
	for (int i = 0; i < 16; i++)
		ss << " " << patientToGlobalTransform[i];

	return ss.str();
}

void LVHeartMesh::outputMesh(std::string prefix, std::string directory) {
	Cmiss_region_id root_region = Cmiss_context_get_default_region(context_);
	Cmiss_region_id region = root_region;
	//Cmiss_region_id  region = Cmiss_region_find_child_by_name(root_region,"heart");
	Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(region);
	Cmiss_stream_information_region_id region_stream_information = Cmiss_stream_information_cast_region(stream_information);
	std::ostringstream ss;
	Cmiss_stream_resource_id* stream = new Cmiss_stream_resource_id[numberOfModelFrames_];
	for (unsigned int i = 0; i < numberOfModelFrames_; i++)
	{
		double time = frameTimes[i];
		ss.str("");
		ss << directory << "/" << prefix << "." << i << ".exregion";
		stream[i] = Cmiss_stream_information_create_resource_file(stream_information, ss.str().c_str());
		Cmiss_stream_information_region_set_resource_attribute_real(region_stream_information, stream[i], CMISS_STREAM_INFORMATION_REGION_ATTRIBUTE_TIME, time);
	}

	Cmiss_region_write(region, stream_information);

	for (unsigned int i = 0; i < numberOfModelFrames_; i++)
	{
		Cmiss_stream_resource_destroy(&(stream[i]));
	}
	delete[] stream;
	Cmiss_stream_information_destroy(&stream_information);
	Cmiss_stream_information_region_destroy(&region_stream_information);
	//Cmiss_region_destroy(&region);
	Cmiss_region_destroy(&root_region);
}

std::string LVHeartMesh::getMeshAt(int frame) {

	double time = frameTimes[frame];
	Cmiss_region_id root_region = Cmiss_context_get_default_region(context_);
	Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
	Cmiss_stream_information_region_id region_stream_information = Cmiss_stream_information_cast_region(stream_information);
	Cmiss_stream_resource_id stream = Cmiss_stream_information_create_resource_memory(stream_information);

	Cmiss_stream_information_region_set_resource_attribute_real(region_stream_information, stream, CMISS_STREAM_INFORMATION_REGION_ATTRIBUTE_TIME, time);
	Cmiss_region_write(root_region, stream_information);
	void *memory_buffer = NULL;
	unsigned int memory_buffer_size = 0;
	Cmiss_stream_resource_memory_id memory_resource = Cmiss_stream_resource_cast_memory(stream);
	Cmiss_stream_resource_memory_get_buffer_copy(memory_resource, &memory_buffer, &memory_buffer_size);
	std::string result;
	if (memory_buffer && memory_buffer_size)
	{
		std::string temp((char*) memory_buffer, memory_buffer_size);
		result = temp;
	}
	Cmiss_stream_resource_memory_destroy(&memory_resource);
	Cmiss_stream_resource_destroy(&(stream));

	free(memory_buffer);

	Cmiss_stream_information_destroy(&stream_information);
	Cmiss_stream_information_region_destroy(&region_stream_information);
	return result;
}

void LVHeartMesh::setBaseRIndex(unsigned int idx) {
	baseridx = idx;
}

int LVHeartMesh::GetNumberOfModelFrames() const {
	return numberOfModelFrames_;
}

void LVHeartMesh::embedInputMarkers() {

	Cmiss_region_id root_region = Cmiss_context_get_default_region(context_);
	Cmiss_region_id heart_region = Cmiss_region_find_child_by_name(root_region, "heart");
	Cmiss_region_destroy(&root_region);
	std::vector<Point3D> pts;
	for (int i = 0; i < numberOfModelFrames_ - 1; i++)
	{
		std::vector<Point3D> markers;
		pts = inputReader.getMarkers(i, APLAX, true);
		markers.insert(markers.end(), pts.begin(), pts.end());
		pts = inputReader.getMarkers(i, TCH, true);
		markers.insert(markers.end(), pts.begin(), pts.end());
		pts = inputReader.getMarkers(i, FCH, true);
		markers.insert(markers.end(), pts.begin(), pts.end());
		std::ostringstream regionName;
		regionName << "Marker" << i;
		Cmiss_region_id marker_region = Cmiss_region_create_child(heart_region, regionName.str().c_str());
		Cmiss_field_module_id field_module = Cmiss_region_get_field_module(marker_region);
		Cmiss_region_destroy(&marker_region);
		Cmiss_field_module_begin_change(field_module);

		Cmiss_nodeset_id nodeset = Cmiss_field_module_find_nodeset_by_name(field_module, "cmiss_nodes");
		Cmiss_node_template_id nodeTemplate = Cmiss_nodeset_create_node_template(nodeset);

		//Each node has x,y,z
		Cmiss_field_id composite = Cmiss_field_module_create_field(field_module, "coordinates", "finite number_of_components 3 component_names x y z");

		Cmiss_node_template_define_field(nodeTemplate, composite);
		Cmiss_field_cache_id cache = Cmiss_field_module_create_cache(field_module);

		int endostartNodeId = 1;

		double loc[3];
		unsigned int cmp = markers.size();
		for (int i = 0; i < cmp; i++)
		{
			loc[0] = markers[i].x;
			loc[1] = markers[i].y;
			loc[2] = markers[i].z;
			int nodeID = endostartNodeId++;

			Cmiss_node_id temporaryNode = Cmiss_nodeset_create_node(nodeset, nodeID, nodeTemplate);
			Cmiss_field_cache_set_node(cache, temporaryNode);
			Cmiss_field_assign_real(composite, cache, 3, loc);
			Cmiss_node_destroy(&temporaryNode);
		}
		{
			int nodeID = 1;
			loc[0] = apex.x;
			loc[1] = apex.y;
			loc[2] = apex.z;

			Cmiss_node_id apexNode = Cmiss_nodeset_create_node(nodeset, nodeID, nodeTemplate);
			Cmiss_field_cache_set_node(cache, apexNode);
			Cmiss_field_assign_real(composite, cache, 3, loc);
			Cmiss_node_destroy(&apexNode);

			nodeID++;
			loc[0] = base.x;
			loc[1] = base.y;
			loc[2] = base.z;
			Cmiss_node_id baseNode = Cmiss_nodeset_create_node(nodeset, nodeID, nodeTemplate);
			Cmiss_field_cache_set_node(cache, baseNode);
			Cmiss_field_assign_real(composite, cache, 3, loc);
			Cmiss_node_destroy(&baseNode);
		}

		Cmiss_field_module_end_change(field_module);
		Cmiss_field_destroy(&composite);
		Cmiss_field_cache_destroy(&cache);
		Cmiss_node_template_destroy(&nodeTemplate);
		Cmiss_nodeset_destroy(&nodeset);
		Cmiss_field_module_destroy(&field_module);
	}

	Cmiss_region_destroy(&heart_region);

}

void LVHeartMesh::setPLSLambdas(std::vector<std::vector<double> >& lambdas, bool epiOnly) {

	Cmiss_field_module_begin_change(field_module_);

	//Get the node handles
	std::vector<Cmiss_node_id>& jpc = cmiss_nodes;

	Cmiss_field_cache_id cache = Cmiss_field_module_create_cache(field_module_);
	double lv[3];
	for (int i = 0; i < numberOfModelFrames_; i++)
		{
			double time = frameTimes[i];
			Cmiss_field_cache_set_time(cache, time);
			std::vector<double> myLams = lambdas[i];
			unsigned int nLimit = NUMBER_OF_NODES;
			unsigned int lamdbaOffset = 0;
			if (epiOnly)
				nLimit = 49; //Update the epicardial nodes alone 1-49
			for (int node = 0; node < nLimit; node++)
			{
				Cmiss_field_cache_set_node(cache, jpc[node]);
				Cmiss_field_evaluate_real(coordinates_ps_, cache, 3, lv);
				lv[0] = myLams[lamdbaOffset++];
				Cmiss_field_assign_real(coordinates_ps_, cache, 3, lv);
				//Set the ds1, ds2, ds1ds2 fields
				//Even though only one value is required, the api expects 3 values (consistent with the number of coordinate component)
				lv[0] = myLams[lamdbaOffset++] / 3.0;
				lv[1] = 0.0;
				lv[2] = 0.0;
				Cmiss_field_assign_real(ds1_, cache, 3, lv);
				lv[0] = myLams[lamdbaOffset++] / 3.0;
				lv[1] = 0.0;
				lv[2] = 0.0;
				Cmiss_field_assign_real(ds2_, cache, 3, lv);
				lv[0] = myLams[lamdbaOffset++] / 3.0;
				lv[1] = 0.0;
				lv[2] = 0.0;
				Cmiss_field_assign_real(ds1ds2_, cache, 3, lv);
			}
		}

	Cmiss_field_cache_destroy(&cache);

	Cmiss_field_module_end_change(field_module_);

}

std::vector<Point3D> LVHeartMesh::getMeshNodeValuesAt(int frame) {
	std::vector<Point3D> result(NUMBER_OF_NODES);
	Cmiss_field_module_begin_change(field_module_);

	//Get the node handles
	std::vector<Cmiss_node_id>& jpc = cmiss_nodes;

	Cmiss_field_cache_id cache = Cmiss_field_module_create_cache(field_module_);
	double lv[3];

	double time = frameTimes[frame];
	Cmiss_field_cache_set_time(cache, time);
	unsigned int nLimit = NUMBER_OF_NODES;
	for (int node = 0; node < nLimit; node++)
	{
		Cmiss_field_cache_set_node(cache, jpc[node]);
		Cmiss_field_evaluate_real(coordinates_patient_rc_, cache, 3, lv);
		result[node] = Point3D(lv);
	}

	Cmiss_field_cache_destroy(&cache);

	Cmiss_field_module_end_change(field_module_);

	return result;
}

void LVHeartMesh::fixEndoNodes() {
	if (!endoNodesUpdated)
	{
		//Get the node handles
		std::vector<Cmiss_node_id>& jpc = cmiss_nodes;

		Cmiss_field_module_begin_change(field_module_);
		Cmiss_field_cache_id cache = Cmiss_field_module_create_cache(field_module_);

		{
			//Set the missing endoNode data based on PS approximation as such an approximation is more complex in RC
			double temp_array[3];
			Point3D init;
			for (int i = 0; i < numberOfModelFrames_; i++)
			{
				std::vector<Point3D> pscoord(NUMBER_OF_NODES);
				std::vector<Point3D> markers;
				std::vector<Point3D> pts;

				pts = inputReader.getMarkers(i, APLAX, true);
				markers.insert(markers.end(), pts.begin(), pts.end());
				pts = inputReader.getMarkers(i, TCH, true);
				markers.insert(markers.end(), pts.begin(), pts.end());
				pts = inputReader.getMarkers(i, FCH, true);
				markers.insert(markers.end(), pts.begin(), pts.end());

				pts = ConvertToHeartModelProlateSpheriodalCoordinate(markers);
				unsigned int numMarkers = pts.size();
				for (int j = 0; j < numMarkers; j++)
				{
					pscoord[myNodes[j]] = pts[j];
				}

				bool lhasTCH = hasTCH;
				bool lhasFCH = hasFCH;
				bool lhasAPLAX = hasAPLAX;

				if (!lhasTCH)
				{
					if (lhasFCH && lhasAPLAX)
					{
						pscoord[tchNodes0] = (pscoord[fchNodes0] + pscoord[aplaxNodes0]) * 0.5;
						pscoord[tchNodes1] = (pscoord[fchNodes1] + pscoord[aplaxNodes1]) * 0.5;
						pscoord[tchNodes2] = (pscoord[fchNodes2] + pscoord[aplaxNodes2]) * 0.5;
						pscoord[tchNodes3] = (pscoord[fchNodes3] + pscoord[aplaxNodes3]) * 0.5;
						pscoord[tchNodes5] = (pscoord[fchNodes5] + pscoord[aplaxNodes5]) * 0.5;
						pscoord[tchNodes6] = (pscoord[fchNodes6] + pscoord[aplaxNodes6]) * 0.5;
						pscoord[tchNodes7] = (pscoord[fchNodes7] + pscoord[aplaxNodes7]) * 0.5;
						pscoord[tchNodes8] = (pscoord[fchNodes8] + pscoord[aplaxNodes8]) * 0.5;
					}
					else if (!lhasAPLAX && lhasFCH)
					{
						pscoord[tchNodes0] = pscoord[fchNodes0];
						pscoord[tchNodes1] = pscoord[fchNodes1];
						pscoord[tchNodes2] = pscoord[fchNodes2];
						pscoord[tchNodes3] = pscoord[fchNodes3];
						pscoord[tchNodes5] = pscoord[fchNodes5];
						pscoord[tchNodes6] = pscoord[fchNodes6];
						pscoord[tchNodes7] = pscoord[fchNodes7];
						pscoord[tchNodes8] = pscoord[fchNodes8];
					}
					else if (lhasAPLAX && !lhasFCH)
					{
						pscoord[tchNodes0] = pscoord[aplaxNodes0];
						pscoord[tchNodes1] = pscoord[aplaxNodes1];
						pscoord[tchNodes2] = pscoord[aplaxNodes2];
						pscoord[tchNodes3] = pscoord[aplaxNodes3];
						pscoord[tchNodes5] = pscoord[aplaxNodes5];
						pscoord[tchNodes6] = pscoord[aplaxNodes6];
						pscoord[tchNodes7] = pscoord[aplaxNodes7];
						pscoord[tchNodes8] = pscoord[aplaxNodes8];
					}
					lhasTCH = true;
				}
				if (!lhasFCH)
				{
					if (lhasAPLAX && lhasTCH)
					{
						pscoord[fchNodes0] = (pscoord[tchNodes0] + pscoord[aplaxNodes0]) * 0.5;
						pscoord[fchNodes1] = (pscoord[tchNodes1] + pscoord[aplaxNodes1]) * 0.5;
						pscoord[fchNodes2] = (pscoord[tchNodes2] + pscoord[aplaxNodes2]) * 0.5;
						pscoord[fchNodes3] = (pscoord[tchNodes3] + pscoord[aplaxNodes3]) * 0.5;
						pscoord[fchNodes5] = (pscoord[tchNodes5] + pscoord[aplaxNodes5]) * 0.5;
						pscoord[fchNodes6] = (pscoord[tchNodes6] + pscoord[aplaxNodes6]) * 0.5;
						pscoord[fchNodes7] = (pscoord[tchNodes7] + pscoord[aplaxNodes7]) * 0.5;
						pscoord[fchNodes8] = (pscoord[tchNodes8] + pscoord[aplaxNodes8]) * 0.5;
					}
					else if (!lhasTCH && lhasAPLAX)
					{
						pscoord[fchNodes0] = pscoord[aplaxNodes0];
						pscoord[fchNodes1] = pscoord[aplaxNodes1];
						pscoord[fchNodes2] = pscoord[aplaxNodes2];
						pscoord[fchNodes3] = pscoord[aplaxNodes3];
						pscoord[fchNodes5] = pscoord[aplaxNodes5];
						pscoord[fchNodes6] = pscoord[aplaxNodes6];
						pscoord[fchNodes7] = pscoord[aplaxNodes7];
						pscoord[fchNodes8] = pscoord[aplaxNodes8];
					}
					else if (lhasTCH && !lhasAPLAX)
					{
						pscoord[fchNodes0] = pscoord[tchNodes0];
						pscoord[fchNodes1] = pscoord[tchNodes1];
						pscoord[fchNodes2] = pscoord[tchNodes2];
						pscoord[fchNodes3] = pscoord[tchNodes3];
						pscoord[fchNodes5] = pscoord[tchNodes5];
						pscoord[fchNodes6] = pscoord[tchNodes6];
						pscoord[fchNodes7] = pscoord[tchNodes7];
						pscoord[fchNodes8] = pscoord[tchNodes8];
					}
					lhasFCH = true;
				}
				if (!lhasAPLAX)
				{
					if (lhasFCH && lhasTCH)
					{
						pscoord[aplaxNodes0] = (pscoord[tchNodes0] + pscoord[fchNodes0]) * 0.5;
						pscoord[aplaxNodes1] = (pscoord[tchNodes1] + pscoord[fchNodes1]) * 0.5;
						pscoord[aplaxNodes2] = (pscoord[tchNodes2] + pscoord[fchNodes2]) * 0.5;
						pscoord[aplaxNodes3] = (pscoord[tchNodes3] + pscoord[fchNodes3]) * 0.5;
						pscoord[aplaxNodes5] = (pscoord[tchNodes5] + pscoord[fchNodes5]) * 0.5;
						pscoord[aplaxNodes6] = (pscoord[tchNodes6] + pscoord[fchNodes6]) * 0.5;
						pscoord[aplaxNodes7] = (pscoord[tchNodes7] + pscoord[fchNodes7]) * 0.5;
						pscoord[aplaxNodes8] = (pscoord[tchNodes8] + pscoord[fchNodes8]) * 0.5;
					}
					else if (!lhasTCH && lhasFCH)
					{
						pscoord[aplaxNodes0] = pscoord[fchNodes0];
						pscoord[aplaxNodes1] = pscoord[fchNodes1];
						pscoord[aplaxNodes2] = pscoord[fchNodes2];
						pscoord[aplaxNodes3] = pscoord[fchNodes3];
						pscoord[aplaxNodes5] = pscoord[fchNodes5];
						pscoord[aplaxNodes6] = pscoord[fchNodes6];
						pscoord[aplaxNodes7] = pscoord[fchNodes7];
						pscoord[aplaxNodes8] = pscoord[fchNodes8];
					}
					else if (lhasTCH && !lhasFCH)
					{
						pscoord[aplaxNodes0] = pscoord[tchNodes0];
						pscoord[aplaxNodes1] = pscoord[tchNodes1];
						pscoord[aplaxNodes2] = pscoord[tchNodes2];
						pscoord[aplaxNodes3] = pscoord[tchNodes3];
						pscoord[aplaxNodes5] = pscoord[tchNodes5];
						pscoord[aplaxNodes6] = pscoord[tchNodes6];
						pscoord[aplaxNodes7] = pscoord[tchNodes7];
						pscoord[aplaxNodes8] = pscoord[tchNodes8];
					}
					lhasAPLAX = true;
				}

				//Average the inputs for unknown nodes
				//if (false)
				{
					pscoord[fchtchNode08] = (pscoord[fchNodes0] + pscoord[tchNodes8]) * 0.5;
					pscoord[fchtchNode17] = (pscoord[fchNodes1] + pscoord[tchNodes7]) * 0.5;
					pscoord[fchtchNode26] = (pscoord[fchNodes2] + pscoord[tchNodes6]) * 0.5;
					pscoord[fchtchNode35] = (pscoord[fchNodes3] + pscoord[tchNodes5]) * 0.5;
					pscoord[fchaplaxNode88] = (pscoord[fchNodes8] + pscoord[aplaxNodes8]) * 0.5;
					pscoord[fchaplaxNode77] = (pscoord[fchNodes7] + pscoord[aplaxNodes7]) * 0.5;
					pscoord[fchaplaxNode66] = (pscoord[fchNodes6] + pscoord[aplaxNodes6]) * 0.5;
					pscoord[fchaplaxNode55] = (pscoord[fchNodes5] + pscoord[aplaxNodes5]) * 0.5;

					pscoord[fchaplaxNode00] = (pscoord[aplaxNodes0] + pscoord[fchNodes0]) * 0.5;
					pscoord[fchaplaxNode11] = (pscoord[aplaxNodes1] + pscoord[fchNodes1]) * 0.5;
					pscoord[fchaplaxNode22] = (pscoord[aplaxNodes2] + pscoord[fchNodes2]) * 0.5;
					pscoord[fchaplaxNode33] = (pscoord[aplaxNodes3] + pscoord[fchNodes3]) * 0.5;

					pscoord[aplaxtchNode00] = (pscoord[aplaxNodes0] + pscoord[tchNodes0]) * 0.5;
					pscoord[aplaxtchNode11] = (pscoord[aplaxNodes1] + pscoord[tchNodes1]) * 0.5;
					pscoord[aplaxtchNode22] = (pscoord[aplaxNodes2] + pscoord[tchNodes2]) * 0.5;
					pscoord[aplaxtchNode33] = (pscoord[aplaxNodes3] + pscoord[tchNodes3]) * 0.5;

					pscoord[fchtchNode80] = (pscoord[tchNodes0] + pscoord[fchNodes8]) * 0.5;
					pscoord[fchtchNode71] = (pscoord[tchNodes1] + pscoord[fchNodes7]) * 0.5;
					pscoord[fchtchNode62] = (pscoord[tchNodes2] + pscoord[fchNodes6]) * 0.5;
					pscoord[fchtchNode53] = (pscoord[tchNodes3] + pscoord[fchNodes5]) * 0.5;

					pscoord[aplaxtchNode88] = (pscoord[aplaxNodes8] + pscoord[tchNodes8]) * 0.5;
					pscoord[aplaxtchNode77] = (pscoord[aplaxNodes7] + pscoord[tchNodes7]) * 0.5;
					pscoord[aplaxtchNode66] = (pscoord[aplaxNodes6] + pscoord[tchNodes6]) * 0.5;
					pscoord[aplaxtchNode55] = (pscoord[aplaxNodes5] + pscoord[tchNodes5]) * 0.5;
				}

				pts = ConvertToHeartModelProlateSpheriodalCoordinate(pscoord);
				//double time = (double) i / ((double) numberOfModelFrames_);
				double time = frameTimes[i];
				Cmiss_field_cache_set_time(cache, time);
				//std::cout << "Time " << i << std::endl;
				for (int j = 0; j < NUMBER_OF_NODES; j++)
				{
					if (!(pscoord[j] == init))
					{
						Cmiss_field_cache_set_node(cache, jpc[j]);
						//Ensure that theta is not changed
						Cmiss_field_evaluate_real(coordinates_ps_, cache, 3, temp_array);
						//std::cout << Point3D(temp_array) << "\t" << pscoord[j] << std::endl;
						temp_array[0] = pscoord[j].x;
						temp_array[1] = pscoord[j].y;
						//temp_array[2] = pscoord[j].z;
						Cmiss_field_assign_real(coordinates_ps_, cache, 3, temp_array);
					}
				}
			}

		}

		Cmiss_field_cache_destroy(&cache);

		Cmiss_field_module_end_change(field_module_);
		endoNodesUpdated = true;
	}
}

/**
 * Sets the wall thickness to be 10% the base length of the
 * first frame
 */
void LVHeartMesh::fixWallLengths() {
	if (coordinates_ps_ != NULL)
	{
		fixEndoNodes();
		Cmiss_field_module_begin_change(field_module_);
		Cmiss_field_cache_id cache = Cmiss_field_module_create_cache(field_module_);
		double avgRadius = 0.0;
		double temp[3];
		std::vector<int> nodeIds;
		if (hasAPLAX)
		{
			nodeIds.push_back(aplaxNodes0);
			nodeIds.push_back(aplaxNodes8);
		}
		if (hasTCH)
		{
			nodeIds.push_back(tchNodes0);
			nodeIds.push_back(tchNodes8);
		}
		if (hasFCH)
		{
			nodeIds.push_back(fchNodes0);
			nodeIds.push_back(fchNodes8);
		}
		int numNodes = nodeIds.size();
		for (int ctr = 0; ctr < numNodes; ctr++)
		{
			Cmiss_field_cache_set_node(cache, cmiss_nodes[nodeIds[ctr]]);
			temp[0] = temp[1] = temp[2] = 0.0;
			Cmiss_field_evaluate_real(coordinates_ps_, cache, 3, temp);
			avgRadius += temp[0]; //Get the lambda
		}
		avgRadius /= numNodes;

		double wallThickness = avgRadius * 0.25;
		/*int epiNodeIds[] =
		 { 6, 8, 22, 24, 17, 18, 20, 11, 12, 14, 3, 4,  5, 7, 21, 23, 15, 16, 19, 9, 10, 13, 1, 2, 28, 35, 36, 32, 33, 34, 29, 30, 31, 25, 26, 27, 40, 47, 48, 44, 45, 46, 41, 42, 43,
		 37, 38, 39, 49 };*/
		int epiNodeIds[] =
		{
		fchaplaxNodesEpi00,
			aplaxNodesEpi0, aplaxtchNodesEpi00, tchNodesEpi0, fchtchNodesEpi80,
			fchNodesEpi8,
			fchaplaxNodesEpi88, aplaxNodesEpi8, aplaxtchNodesEpi88, tchNodesEpi8,
			fchtchNodesEpi08,
			fchNodesEpi0, fchaplaxNodesEpi11, aplaxNodesEpi1, aplaxtchNodesEpi11,
			tchNodesEpi1,
			fchtchNodesEpi71, fchNodesEpi7, fchaplaxNodesEpi77, aplaxNodesEpi7,
			aplaxtchNodesEpi77,
			tchNodesEpi7, fchtchNodesEpi17, fchNodesEpi1, aplaxNodesEpi2,
			aplaxtchNodesEpi22,
			tchNodesEpi2, fchtchNodesEpi62, fchNodesEpi6, fchaplaxNodesEpi66,
			aplaxNodesEpi6,
			aplaxtchNodesEpi66, tchNodesEpi6, fchtchNodesEpi26, fchNodesEpi2,
			fchaplaxNodesEpi22,
			aplaxNodesEpi3, aplaxtchNodesEpi33, tchNodesEpi3, fchtchNodesEpi53,
			fchNodesEpi5,
			fchaplaxNodesEpi55, aplaxNodesEpi5, aplaxtchNodesEpi55, tchNodesEpi5,
			fchtchNodesEpi35,
			fchNodesEpi3, fchaplaxNodesEpi33, aplaxNodesEpi4 };
		/*int endoNodeIds[] =
		 { 55, 57, 71, 73, 66, 67, 69, 60, 61, 63, 52, 53, 54, 56, 70, 72, 64, 65, 68, 58, 59, 62, 50, 51, 77, 84, 85, 81, 82, 83, 78, 79, 80, 74, 75, 76, 89, 96, 97, 93, 94, 95,
		 90, 91, 92, 86, 87, 88, 98 };*/
		int endoNodeIds[] =
		{
		fchaplaxNode00,
			aplaxNodes0, aplaxtchNode00, tchNodes0, fchtchNode80,
			fchNodes8,
			fchaplaxNode88, aplaxNodes8, aplaxtchNode88, tchNodes8,
			fchtchNode08,
			fchNodes0, fchaplaxNode11, aplaxNodes1, aplaxtchNode11,
			tchNodes1,
			fchtchNode71, fchNodes7, fchaplaxNode77, aplaxNodes7,
			aplaxtchNode77,
			tchNodes7, fchtchNode17, fchNodes1, aplaxNodes2,
			aplaxtchNode22,
			tchNodes2, fchtchNode62, fchNodes6, fchaplaxNode66,
			aplaxNodes6,
			aplaxtchNode66, tchNodes6, fchtchNode26, fchNodes2,
			fchaplaxNode22,
			aplaxNodes3, aplaxtchNode33, tchNodes3, fchtchNode53,
			fchNodes5,
			fchaplaxNode55, aplaxNodes5, aplaxtchNode55, tchNodes5,
			fchtchNode35,
			fchNodes3, fchaplaxNode33, aplaxNodes4 };
		double endoLambdas[49];
		double epiLambdas[49];
		double epiMus[49];
		double epiThetas[49];
		//set the epi lamdbas to remain the same for all time points
		//set the epi mu to be a weighhted sum of endo mu in the first frame and the current frame
		//The epi thetas remain constant across the frames
		//Epi changes will be handled in the volume conservation step
		Cmiss_field_cache_set_time(cache, 0.0);
		double wallWeight[] =
		{ 1.0, 0.85, 0.75, 0.7, 0.65 };
		for (int nc = 0; nc < 49; nc++)
		{
			Cmiss_field_cache_set_node(cache, cmiss_nodes[endoNodeIds[nc]]);
			temp[0] = temp[1] = temp[2] = 0.0;
			Cmiss_field_evaluate_real(coordinates_ps_, cache, 3, temp);
			double newLambda = temp[0] + wallThickness * wallWeight[nc / 12];
			endoLambdas[nc] = temp[0];
			epiMus[nc] = temp[1];
			epiThetas[nc] = temp[2];
			Cmiss_field_cache_set_node(cache, cmiss_nodes[epiNodeIds[nc]]);
			temp[0] = temp[1] = temp[2] = 0.0;
			Cmiss_field_evaluate_real(coordinates_ps_, cache, 3, temp);
			epiLambdas[nc] = newLambda;
		}
		//Set the values
		for (int i = 0; i < numberOfModelFrames_; i++)
		{
			double time = frameTimes[i];
			Cmiss_field_cache_set_time(cache, time);
			for (int nc = 0; nc < 49; nc++)
			{
				Cmiss_field_cache_set_node(cache, cmiss_nodes[endoNodeIds[nc]]);
				temp[0] = temp[1] = temp[2] = 0.0;
				Cmiss_field_evaluate_real(coordinates_ps_, cache, 3, temp);

				Cmiss_field_cache_set_node(cache, cmiss_nodes[epiNodeIds[nc]]);
				temp[0] = epiLambdas[nc];
				temp[1] = (epiMus[nc] * 0.25 + temp[1] * 0.75);
				temp[2] = epiThetas[nc];
				Cmiss_field_assign_real(coordinates_ps_, cache, 3, temp);
			}
		}

		Cmiss_field_cache_destroy(&cache);
		Cmiss_field_module_end_change(field_module_);
	}
	else
	{
		std::cerr << "Mesh already in RC, fixing wall lengths aborted" << std::endl;
	}
}

std::vector<double> LVHeartMesh::getMyocardialVolumes() {
	return myocardialVolumes;
}

std::vector<double> LVHeartMesh::getFrameTimes() {
	return frameTimes;
}
