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

#include "VolumeRegularizer.h"

VolumeRegularizer::VolumeRegularizer(std::vector<std::string> mesh, double focalLength) :
		inputMesh(mesh) {
	apexChange = 1.0;
	ring1Change = 0.75;
	ring2Change = 0.5;
	ring3Change = 0.1;
	numberOfModelFrames_ = mesh.size();
	cmiss_nodes.resize(98);
	//Load the mesh
	//Initialise cmgui and get context
	context_ = Cmiss_context_create("vr");
	//This required for using the cmiss_context_execute_command
	Cmiss_context_enable_user_interface(context_, NULL);

	/**< Handle to the context */
	Cmiss_region_id root_region = Cmiss_context_get_default_region(context_);

	std::string region_name = "heart";
	// Create a heart region

	Cmiss_region_id heart_region = Cmiss_region_create_child(root_region, region_name.c_str());

	// Read in the heart model spaced over the number of model frames
	for (unsigned int i = 0; i < numberOfModelFrames_; i++)
	{
		double time = static_cast<double>(i) / (numberOfModelFrames_ - 1.0);
		Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(heart_region);
		Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
		Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, mesh[i].c_str(), mesh[i].length());
		Cmiss_stream_information_region_set_resource_attribute_real(stream_information_region, stream_resource, CMISS_STREAM_INFORMATION_REGION_ATTRIBUTE_TIME, time);

		Cmiss_region_read(heart_region, stream_information);

		Cmiss_stream_resource_destroy(&stream_resource);
		Cmiss_stream_information_destroy(&stream_information);
		Cmiss_stream_information_region_destroy(&stream_information_region);
	}

	//Cmiss_region_id heart_region = Cmiss_region_find_child_by_name(root_region, region_name.c_str());
	//Get the field module of the heart region
	field_module_ = Cmiss_region_get_field_module(heart_region);

	if (field_module_ != 0)
	{
		Cmiss_field_module_begin_change(field_module_);

		// 'coordinates' is an assumed field in
		// a rc coordinate system.
		coordinates_rc_ = Cmiss_field_module_find_field_by_name(field_module_, "coordinates");
		//Creating a ps coordinate system
		std::ostringstream ss;
		ss << "coordinate_system prolate focus " << focalLength << " coordinate_transform field coordinates";
		coordinates_ps_ = Cmiss_field_module_create_field(field_module_, "pscoordinates", ss.str().c_str());
		if (!coordinates_rc_ || !coordinates_ps_)
		{
			std::cout << "--- Coordinates could not be defined for heart region!!! (Volume regularizer)";
			throw -1;
		}

		//Define fields for getting the volume
		//The fields that get created when the volume is computed create issues when the mesh is output
		//Determine the volume
		Cmiss_field_module_define_field(field_module_, "gauss_location", "finite_element number_of_components 1 field element_xi");
		Cmiss_field_module_define_field(field_module_, "gauss_weight", "finite_element number_of_components 1 field real");
		Cmiss_context_execute_command(context_, "gfx create dgroup gausspoints region /heart");
		Cmiss_context_execute_command(context_,
				"gfx create gauss_points region /heart mesh cmiss_mesh_3d order 2 gauss_location_field gauss_location gauss_point_nodeset gausspoints.cmiss_data gauss_weight_field gauss_weight first_identifier 1");

		Cmiss_field_module_define_field(field_module_, "dX_dxi1", "basis_derivative fe_field coordinates order 1 xi_indices 1");
		Cmiss_field_module_define_field(field_module_, "dX_dxi2", "basis_derivative fe_field coordinates order 1 xi_indices 2");
		Cmiss_field_module_define_field(field_module_, "dX_dxi3", "basis_derivative fe_field coordinates order 1 xi_indices 3");
		//# pack these into a 3x3 matrix, and get differential volume dV from its determinant, in 3-D elements
		Cmiss_field_module_define_field(field_module_, "dX_dxi", "composite dX_dxi1 dX_dxi2 dX_dxi3");
		Cmiss_field_module_define_field(field_module_, "dV", "determinant field dX_dxi");
		//# obtain fields at gauss points:
		Cmiss_field_module_define_field(field_module_, "gauss_coordinates", "embedded element_xi gauss_location field coordinates");
		Cmiss_field_module_define_field(field_module_, "gauss_dV", "embedded element_xi gauss_location field dV");

		//# sum weighted dV over gauss points
		Cmiss_field_module_define_field(field_module_, "weighted_gauss_dV", "multiply fields gauss_weight gauss_dV");
		volume = Cmiss_field_module_create_field(field_module_, "volume", "nodeset_sum field weighted_gauss_dV nodeset gausspoints.cmiss_data");

		//Create the cache
		cache = Cmiss_field_module_create_cache(field_module_);

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

		int epi[] =
		{ 6, 8, 22, 24, 17, 18, 20, 11, 12, 14, 3, 4, 5, 7, 21, 23, 15, 16, 19, 9, 10, 13, 1, 2, 28, 35, 36, 32, 33, 34, 29, 30, 31, 25, 26, 27, 40, 47, 48, 44, 45, 46, 41, 42, 43,
			37, 38, 39, 49 };
		int endo[] =
		{ 55, 57, 71, 73, 66, 67, 69, 60, 61, 63, 52, 53, 54, 56, 70, 72, 64, 65, 68, 58, 59, 62, 50, 51, 77, 84, 85, 81, 82, 83, 78, 79, 80, 74, 75, 76, 89, 96, 97, 93, 94, 95,
			90, 91, 92, 86, 87, 88, 98 };
		memcpy(epiNodeIds, epi, sizeof(int) * 49);
		memcpy(endoNodeIds, endo, sizeof(int) * 49);

		double temp[3]; //Get the initial lambda's
		Cmiss_field_cache_set_time(cache, 0.0);
		for (int nc = 0; nc < 49; nc++)
		{
			double tempx = 0.0;
			Cmiss_field_cache_set_node(cache, cmiss_nodes[epiNodeIds[nc] - 1]);
			temp[0] = temp[1] = temp[2] = 0.0;
			Cmiss_field_evaluate_real(coordinates_ps_, cache, 3, temp);
			initialWallLengths.push_back(temp[0] - tempx);
		}
		for (int i = 0; i < numberOfModelFrames_; i++)
		{
			double time = ((double) i) / ((double) numberOfModelFrames_ - 1.0);
			Cmiss_field_cache_set_time(cache, time);
			temp[0] = 0.0;
			Cmiss_field_cache_set_node(cache, cmiss_nodes[48]);
			Cmiss_field_evaluate_real(volume, cache, 1, temp);
			volumes.push_back(temp[0]);
		}
	}
	else
	{
		std::cout << "--- No field module for heart region!!! (Volume regularizer)";
		std::cout << "No field module " << std::endl;
		throw -1;
	}

	Cmiss_region_destroy(&heart_region);
	Cmiss_region_destroy(&root_region);

}

void VolumeRegularizer::regularize() {
	struct optimizationInput input;
	input.cache = &cache;
	input.field_module_ = &field_module_;
	input.coordinates_ps_ = &coordinates_ps_;
	input.cmiss_nodes = &cmiss_nodes;
	input.endoNodeIDs = endoNodeIds;
	input.epiNodeIDs = epiNodeIds;
	input.initialWallLengths = &initialWallLengths;
	input.volume = &volume;
	input.targetVolume = volumes[0];
	double temp[3];

	//
	// These variables define stopping conditions for the optimizer.
	//
	// We use very simple condition - |g|<=epsg
	//
	//Note that we do not want decimal accuracy, also the minimization problem never seems
	//to reduce the error to zero, rather the order is in 10000.00
	double epsg = 0.10;
	double epsf = 0;
	double epsx = 0;
	double epso = 0.0010;
	double epsi = 0.0010;

	if (true)
	{
		for (int frame = 1; frame < numberOfModelFrames_; frame++)
		{ //For each frame
			input.time = ((double) frame) / ((double) numberOfModelFrames_ - 1.0);
			//Pushing the apex to max seems to put the optimizer far away from the boundary and
			//does not produce any nan's in x
			//The epinodes at the baseplane are fixed across the time sequence
			//As it is assumed that they are rigid, so only the other planes and apex is varied
			alglib::real_1d_array x = "[1.0, 1.01, 1.03]";
			alglib::real_1d_array bndu = "[1.1, 1.3, 1.5]";
			alglib::real_1d_array bndl = "[+1.0, +1.0, +1.0]";
			alglib::minbleicstate state;
			alglib::minbleicreport rep;

			alglib::ae_int_t maxits = 0;

			//
			// This variable contains differentiation step
			//
			double diffstep = 1.0e-4;

			//
			// Now we are ready to actually optimize something:
			// * first we create optimizer
			// * we add boundary constraints
			// * we tune stopping conditions
			// * and, finally, optimize and obtain results...
			//
			try
			{
				alglib::minbleiccreatef(x, diffstep, state);
				alglib::minbleicsetbc(state, bndl, bndu);
				alglib::minbleicsetinnercond(state, epsg, epsf, epsx);
				alglib::minbleicsetoutercond(state, epso, epsi);
				alglib::minbleicoptimize(state, optimize, NULL, &input);
				alglib::minbleicresults(state, x, rep);
			} catch (alglib::ap_error& err)
			{
				std::cout << err.msg << std::endl;
			}
			//
			// ...and evaluate these results
			//
			temp[0] = 0.0;
			Cmiss_field_cache_set_node(cache, cmiss_nodes[48]);
			Cmiss_field_evaluate_real(volume, cache, 1, temp);
			std::cout << "Optimizing volume at " << frame << "\t from " << volumes[frame] << " to " << temp[0] << " target was " << volumes[0] << "\tsolution vector is "
					<< x.tostring(6) << std::endl;
			volumes[frame] = temp[0];
		}
	}

	//Determine the node values to create the mesh from scratch
	std::vector<std::vector<Point3D> > meshNodes;
	for (int frame = 0; frame < numberOfModelFrames_; frame++)
	{
		double time = ((double) frame) / ((double) numberOfModelFrames_ - 1.0);
		Cmiss_field_cache_set_time(cache, time);
		std::vector<Point3D> nodes;
		for (int i = 0; i < 98; i++)
		{
			Cmiss_field_cache_set_node(cache, cmiss_nodes[i]);
			temp[0] = temp[1] = temp[2] = 0.0;
			Cmiss_field_evaluate_real(coordinates_rc_, cache, 3, temp);
			nodes.push_back(Point3D(temp));
		}

		meshNodes.push_back(nodes);
	}

	//Destroy context and recreate
	Cmiss_field_destroy(&coordinates_ps_);
	Cmiss_field_destroy(&coordinates_rc_);

	//Destroy the node handles too
	for (int i = 0; i < NUMBER_OF_NODES; i++)
	{
		Cmiss_node_destroy(&cmiss_nodes[i]);
	}
	Cmiss_field_module_destroy(&field_module_);
	Cmiss_context_destroy(&context_);

	coordinates_ps_ = NULL;

	//Initialise cmgui and get context
	context_ = Cmiss_context_create("vr");
	//This required for using the cmiss_context_execute_command
	Cmiss_context_enable_user_interface(context_, NULL);

	/**< Handle to the context */
	Cmiss_region_id root_region = Cmiss_context_get_default_region(context_);

	std::string region_name = "heart";
	// Create a heart region

	Cmiss_region_id heart_region = Cmiss_region_create_child(root_region, region_name.c_str());

	// Read in the heart model spaced over the number of model frames
	for (unsigned int i = 0; i < numberOfModelFrames_; i++)
	{
		double time = static_cast<double>(i) / (numberOfModelFrames_ - 1.0);
		Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(heart_region);
		Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
		Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, inputMesh[i].c_str(), inputMesh[i].length());
		Cmiss_stream_information_region_set_resource_attribute_real(stream_information_region, stream_resource, CMISS_STREAM_INFORMATION_REGION_ATTRIBUTE_TIME, time);

		Cmiss_region_read(heart_region, stream_information);

		Cmiss_stream_resource_destroy(&stream_resource);
		Cmiss_stream_information_destroy(&stream_information);
		Cmiss_stream_information_region_destroy(&stream_information_region);
	}

	//Cmiss_region_id heart_region = Cmiss_region_find_child_by_name(root_region, region_name.c_str());
	//Get the field module of the heart region
	field_module_ = Cmiss_region_get_field_module(heart_region);

	if (field_module_ != 0)
	{
		Cmiss_field_module_begin_change(field_module_);

		// 'coordinates' is an assumed field in
		// a rc coordinate system.
		coordinates_rc_ = Cmiss_field_module_find_field_by_name(field_module_, "coordinates");
		//Update the nodes to the volume regularized values
		for (int frame = 0; frame < numberOfModelFrames_; frame++)
		{
			double time = ((double) frame) / ((double) numberOfModelFrames_ - 1.0);
			Cmiss_field_cache_set_time(cache, time);
			std::vector<Point3D>& nodes = meshNodes[frame];
			for (int i = 0; i < 98; i++)
			{
				Cmiss_field_cache_set_node(cache, cmiss_nodes[i]);
				temp[0] = nodes[i].x;
				temp[1] = nodes[i].y;
				temp[2] = nodes[i].z;
				Cmiss_field_assign_real(coordinates_rc_, cache, 3, temp);
			}
		}
		Cmiss_field_module_end_change(field_module_);
	}
	else
	{
		std::cout << "--- No field module for heart region!!! (Volume regularizer)";
		std::cout << "No field module " << std::endl;
		throw -1;
	}
	Cmiss_region_destroy(&heart_region);
	Cmiss_region_destroy(&root_region);
}

void VolumeRegularizer::optimize(const alglib::real_1d_array& x, double& func, void* ptr) {
	struct optimizationInput* input = static_cast<struct optimizationInput*>(ptr);
	Cmiss_field_module_id field_module = *(input->field_module_);
	std::vector<Cmiss_node_id>& cmiss_nodes = *(input->cmiss_nodes);
	std::vector<double> initialWallLengths(*(input->initialWallLengths));
	Cmiss_field_id volume = *(input->volume);
	Cmiss_field_id coordinate = *(input->coordinates_ps_);
	Cmiss_field_cache_id cache = *(input->cache);
	double time = input->time;
	double targetVolume = input->targetVolume;
	int * epiNodeIds = input->epiNodeIDs;
	int * endoNodeIds = input->endoNodeIDs;

	//Trimming to avoid nan's

	//Sometimes x contains NAN in which case reject
	if (x[0] != x[0] || x[1] != x[1] || x[2] != x[2])
	{
		std::cout << "Nan occured " << x.tostring(3) << std::endl;
		func = targetVolume * 10000;
		return;
	}

	unsigned int numNodes = cmiss_nodes.size() / 2 - 1;
	double temp[3];
	int ratioIndex = -1;
	Cmiss_field_module_begin_change(field_module);
	Cmiss_field_cache_set_time(cache, time);
	for (int i = 12; i < numNodes; i++)
	{
		if (i % 12 == 0)
		{
			ratioIndex++;
		}
		double factor = x[ratioIndex];
		Cmiss_field_cache_set_node(cache, cmiss_nodes[epiNodeIds[i] - 1]);
		temp[0] = temp[1] = temp[2] = 0.0;
		Cmiss_field_evaluate_real(coordinate, cache, 3, temp);
		temp[0] = initialWallLengths[i] * factor;
		Cmiss_field_assign_real(coordinate, cache, 3, temp);
	}
	{
		double factor = x[2];
		Cmiss_field_cache_set_node(cache, cmiss_nodes[48]); //Apex
		temp[0] = temp[1] = temp[2] = 0.0;
		Cmiss_field_evaluate_real(coordinate, cache, 3, temp);
		temp[0] = initialWallLengths[48] * factor;
		Cmiss_field_assign_real(coordinate, cache, 3, temp);
	}
	temp[0] = 0.0;
	Cmiss_field_evaluate_real(volume, cache, 1, temp);
	Cmiss_field_module_end_change(field_module);

	func = fabs(targetVolume - temp[0]);
	if (func < 1.0)
		func = 0.0;
}

std::vector<std::string> VolumeRegularizer::getMesh() {
	std::vector<std::string> resultx;
	for (int i = 0; i < numberOfModelFrames_; i++)
	{
		double time = ((double) i) / ((double) numberOfModelFrames_ - 1.0);
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
		resultx.push_back(result);
	}
	return resultx;
}

std::vector<double> VolumeRegularizer::getVolumes() {
	return volumes;
}

VolumeRegularizer::~VolumeRegularizer() {

	for (int i = 0; i < NUMBER_OF_NODES; i++)
	{
		Cmiss_node_destroy(&cmiss_nodes[i]);
	}
	Cmiss_field_cache_destroy(&cache);
	if (coordinates_ps_ != NULL)
		Cmiss_field_destroy(&coordinates_ps_);
	Cmiss_field_destroy(&coordinates_rc_);
	Cmiss_field_module_destroy(&field_module_);
	Cmiss_context_destroy(&context_);
}

