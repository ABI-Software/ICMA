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

#include "LVMyocardialVolume.h"

LVMyocardialVolume::LVMyocardialVolume(std::vector<std::string> mesh) {
	//Read the mesh
	Cmiss_context_id context_ = Cmiss_context_create("volumemeasure");
	/**< Handle to the context */
	Cmiss_region_id root_region = Cmiss_context_get_default_region(context_);

	//This required for using the cmiss_context_execute_command
	Cmiss_context_enable_user_interface(context_, NULL);

	//Load the finite element mesh of the heart

	std::string region_name = "heart";
	Cmiss_region_id heart_region = Cmiss_region_create_child(root_region, region_name.c_str());
	//Get the field module of the heart region
	Cmiss_field_module_id field_module = Cmiss_region_get_field_module(heart_region);

	Cmiss_field_cache_id fieldCache = Cmiss_field_module_create_cache(field_module);
	unsigned int numberOfModelFrames_ = mesh.size();
	double denom = numberOfModelFrames_ - 1;
	for (unsigned int i = 0; i < numberOfModelFrames_; i++) {
		double time = static_cast<double>(i) / denom;
		Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
		Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
		Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, mesh[i].c_str(), mesh[i].length());
		Cmiss_stream_information_region_set_resource_attribute_real(stream_information_region, stream_resource, CMISS_STREAM_INFORMATION_REGION_ATTRIBUTE_TIME, time);

		Cmiss_region_read(root_region, stream_information);

		Cmiss_stream_resource_destroy(&stream_resource);
		Cmiss_stream_information_destroy(&stream_information);
		Cmiss_stream_information_region_destroy(&stream_information_region);
	}
	Cmiss_field_module_begin_change(field_module);
	//Determine the myocardial volume
	//Define fields for getting the volume
	//The fields that get created when the volume is computed create issues when the mesh is output
	//Determine the volume
	//# create order 2 Gauss points on both the volume and exterior surface to integrate with
	Cmiss_field_module_define_field(field_module, "gauss_location", "finite_element number_of_components 1 field element_xi");
	Cmiss_field_module_define_field(field_module, "gauss_weight", "finite_element number_of_components 1 field real");
	Cmiss_context_execute_command(context_, "gfx create dgroup gausspoints region /heart");
	Cmiss_context_execute_command(context_,
			"gfx create gauss_points region /heart mesh cmiss_mesh_3d order 2 gauss_location_field gauss_location gauss_point_nodeset gausspoints.cmiss_data gauss_weight_field gauss_weight first_identifier 1");

	Cmiss_field_module_define_field(field_module, "dX_dxi1", "basis_derivative fe_field coordinates order 1 xi_indices 1");
	Cmiss_field_module_define_field(field_module, "dX_dxi2", "basis_derivative fe_field coordinates order 1 xi_indices 2");
	Cmiss_field_module_define_field(field_module, "dX_dxi3", "basis_derivative fe_field coordinates order 1 xi_indices 3");
	//# pack these into a 3x3 matrix, and get differential volume dV from its determinant, in 3-D elements
	Cmiss_field_module_define_field(field_module, "dX_dxi", "composite dX_dxi1 dX_dxi2 dX_dxi3");
	Cmiss_field_module_define_field(field_module, "dV", "determinant field dX_dxi");
	//# obtain fields at gauss points:
	Cmiss_field_module_define_field(field_module, "gauss_coordinates", "embedded element_xi gauss_location field coordinates");
	Cmiss_field_module_define_field(field_module, "gauss_dV", "embedded element_xi gauss_location field dV");

	//# sum weighted dV over gauss points
	Cmiss_field_module_define_field(field_module, "weighted_gauss_dV", "multiply fields gauss_weight gauss_dV");
	Cmiss_field_id volume = Cmiss_field_module_create_field(field_module, "volume", "nodeset_sum field weighted_gauss_dV nodeset gausspoints.cmiss_data");


	Cmiss_field_module_end_change(field_module);

	//Get the element and node handles
	Cmiss_mesh_id cmiss_mesh = Cmiss_field_module_find_mesh_by_name(field_module, "cmiss_mesh_3d");

	Cmiss_element_iterator_id elementIterator = Cmiss_mesh_create_element_iterator(cmiss_mesh);
	Cmiss_element_id element = Cmiss_element_iterator_next(elementIterator);
	std::vector<Cmiss_element_id> elements(48);
	while (element != NULL) {
		int elementId = Cmiss_element_get_identifier(element) - 1; //Cmiss numbering starts at 1
		elements[elementId] = element;
		element = Cmiss_element_iterator_next(elementIterator);
	}
	Cmiss_element_iterator_destroy(&elementIterator);
	Cmiss_mesh_destroy(&cmiss_mesh);

	//Calculate the myocardial volumes
	double temp, coord[3];
	for (int i = 0; i < numberOfModelFrames_; i++) {
		double time = ((double) i) / denom;
		Cmiss_field_cache_set_time(fieldCache, time);
		temp = 0.0;
		coord[0] = 0.0;
		coord[1] = 0.0;
		coord[2] = 0.0;
		Cmiss_field_cache_set_mesh_location(fieldCache, elements[32], 3, coord);
		Cmiss_field_evaluate_real(volume, fieldCache, 1, &temp);
		volumes.push_back(temp);
	}


	for (unsigned int i = 0; i < 48; i++) {
		Cmiss_element_destroy(&(elements[i]));
	}
	Cmiss_field_destroy(&volume);
	Cmiss_field_cache_destroy(&fieldCache);
	Cmiss_field_module_destroy(&field_module);
	Cmiss_region_destroy(&heart_region);
	Cmiss_region_destroy(&root_region);
	Cmiss_context_destroy(&context_);
}

std::vector<double> LVMyocardialVolume::getMyocardialVolumes() {
	return volumes;
}

LVMyocardialVolume::~LVMyocardialVolume() {

}

