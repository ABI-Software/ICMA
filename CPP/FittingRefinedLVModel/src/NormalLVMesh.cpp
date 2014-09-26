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

#include "NormalLVMesh.h"

//These variable headers should be included once in the object
#include "normallvmodel.h"
#include "MeshTopology.h"

NormalLVMesh::NormalLVMesh(Point3D Apex, Point3D base, Point3D rv) :
		focalLength(0.0), field_(0), field_module_(0), coordinates_ps_(0), coordinates_rc_(0), coordinates_patient_rc_(0), transform_mx_(0) {
	apex = Apex;
	this->base = base;
	this->rv = rv;
	contextName = "SCDNormalMale";
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
	numberOfModelFrames_ = NUMBEROFTIMEPOINTS;

	// Read in the heart model spaced over the number of model frames
	for (unsigned int i = 0; i < numberOfModelFrames_; i++)
	{
		double time = static_cast<double>(i) / (numberOfModelFrames_ - 1.0);
		Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
		Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
		Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, meshes[i], mesh_lengths[i]);
		Cmiss_stream_information_region_set_resource_attribute_real(stream_information_region, stream_resource, CMISS_STREAM_INFORMATION_REGION_ATTRIBUTE_TIME, time);

		Cmiss_region_read(root_region, stream_information);

		Cmiss_stream_resource_destroy(&stream_resource);
		Cmiss_stream_information_destroy(&stream_information);
		Cmiss_stream_information_region_destroy(&stream_information_region);
	}

	if (field_module_ != 0)
	{
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
		Point3D apexPosition = apex;
		Point3D basePosition = base;
		Vector3D xAxis = apexPosition - basePosition;
		xAxis.Normalise();

		Vector3D yAxis = rv - basePosition;
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

		cache = Cmiss_field_module_create_cache(field_module_);
		//Create a field which stores the transformation matrix
		transform_mx_ = Cmiss_field_module_create_constant(field_module_, 16, patientToGlobalTransform);

		// Compute FocalLength
		Cmiss_field_id projection_mx = Cmiss_field_module_create_constant(field_module_, 16, patientToGlobalTransform);
		coordinates_patient_rc_ = Cmiss_field_module_create_projection(field_module_, coordinates_rc_, projection_mx);
		Cmiss_field_destroy(&projection_mx);

		double lengthFromApexToBase = (apexPosition - basePosition).Length();
		focalLength = (apexPosition - origin).Length() / cosh(1.0);
		Cmiss_field_set_attribute_real(coordinates_ps_, CMISS_FIELD_ATTRIBUTE_COORDINATE_SYSTEM_FOCUS, focalLength);

		Cmiss_field_module_end_change(field_module_);

		//Get the nodes
		nodes.resize(40);
		Cmiss_nodeset_id nodeset = Cmiss_field_module_find_nodeset_by_name(field_module_, "cmiss_nodes");
		Cmiss_node_iterator_id nodeIterator = Cmiss_nodeset_create_node_iterator(nodeset);

		Cmiss_node_id node = Cmiss_node_iterator_next(nodeIterator);
		if (node != 0)
		{
			while (node)
			{
				int node_id = Cmiss_node_get_identifier(node) - 1;
				nodes[node_id] = node;
				node = Cmiss_node_iterator_next(nodeIterator);
			}
		}
		Cmiss_nodeset_destroy(&nodeset);
		Cmiss_node_iterator_destroy(&nodeIterator);

		//Get the elements
		elements.resize(16);
		Cmiss_mesh_id cmiss_mesh = Cmiss_field_module_find_mesh_by_name(field_module_, "cmiss_mesh_3d");

		Cmiss_element_iterator_id elementIterator = Cmiss_mesh_create_element_iterator(cmiss_mesh);
		Cmiss_element_id element = Cmiss_element_iterator_next(elementIterator);

		//Get the list of mesh elements
		while (element != NULL)
		{
			int elementId = Cmiss_element_get_identifier(element) - 1; //Cmiss numbering starts at 1
			elements[elementId] = element;
			element = Cmiss_element_iterator_next(elementIterator);
		}
		Cmiss_element_iterator_destroy(&elementIterator);
		Cmiss_mesh_destroy(&cmiss_mesh);

	}
	else
		std::cerr << "--- No field module for heart region!!!" << std::endl;

	Cmiss_region_destroy(&root_region);

}

NormalLVMesh::~NormalLVMesh() {
	for (int i = 0; i < nodes.size(); i++)
		Cmiss_node_destroy(&nodes[i]);

	for (int i = 0; i < elements.size(); i++)
		Cmiss_element_destroy(&elements[i]);

	Cmiss_field_cache_destroy(&cache);
	Cmiss_field_destroy(&coordinates_ps_);
	Cmiss_field_destroy(&coordinates_rc_);
	Cmiss_field_destroy(&coordinates_patient_rc_);
	Cmiss_field_destroy(&transform_mx_);
	Cmiss_field_module_destroy(&field_module_);
	Cmiss_context_destroy(&context_);
}

std::vector<double> NormalLVMesh::getTorsionsAt(double time) {
//For all endo nodes

	std::vector<double> torsion(98);

	int nodeIds[] = {fchNodes0,fchNodes1,fchNodes2,fchNodes3,fchNodes5,fchNodes6,fchNodes7,fchNodes8,
	                  fchaplaxNode00,fchaplaxNode11,fchaplaxNode22,fchaplaxNode33,fchaplaxNode55,fchaplaxNode66,fchaplaxNode77,fchaplaxNode88,
	                  aplaxNodes0,aplaxNodes1,aplaxNodes2,aplaxNodes3,aplaxNodes5,aplaxNodes6,aplaxNodes7,aplaxNodes8,
	                  aplaxtchNode00,aplaxtchNode11,aplaxtchNode22,aplaxtchNode33,aplaxtchNode55,aplaxtchNode66,aplaxtchNode77,aplaxtchNode88,
	                  tchNodes0,tchNodes1,tchNodes2,tchNodes3,tchNodes5,tchNodes6,tchNodes7,tchNodes8,
	                  fchtchNode80,fchtchNode71,fchtchNode62,fchtchNode53,fchtchNode35,fchtchNode26,fchtchNode17,fchtchNode08
	};

	int elemtIds[] = {4,8,12,16,14,10,6,2,
	                  4,8,12,16,14,10,6,2,
	                  4,8,12,16,14,10,6,2,
	                  3,7,11,15,13,9,5,1,
	                  3,7,11,15,13,9,5,1,
	                  3,7,11,15,13,9,5,1
	};

	int offset[]   = {0,0,0,0,0,0,0,0,
	                  0.333,0.333,0.333,0.333,0.333,0.333,0.333,0.333,
	                  0.666,0.666,0.666,0.666,0.666,0.666,0.666,0.666,
	                  0,0,0,0,0,0,0,0,
	                  0.333,0.333,0.333,0.333,0.333,0.333,0.333,0.333,
	                  0.666,0.666,0.666,0.666,0.666,0.666,0.666,0.666
	};

	double coord[3], temp[3];

	Cmiss_field_cache_set_time(cache, 0.0);

	for (int i = 0; i < 48; i++)
	{
		coord[0] = offset[i];
		coord[1] = 1.0;
		coord[2] = 0.0;
		Cmiss_field_cache_set_mesh_location(cache, elements[elemtIds[i] - 1], 3, coord);
		Cmiss_field_evaluate_real(coordinates_ps_, cache, 3, temp);
		torsion[nodeIds[i]] = temp[2];
	}

	Cmiss_field_cache_set_time(cache, time);
	for (int i = 0; i < 48; i++)
	{
		coord[0] = offset[i];
		coord[1] = 1.0;
		coord[2] = 0.0;
		Cmiss_field_cache_set_mesh_location(cache, elements[elemtIds[i] - 1], 3, coord);
		Cmiss_field_evaluate_real(coordinates_ps_, cache, 3, temp);
		torsion[nodeIds[i]] -= temp[2];

	}

	return torsion;
}

