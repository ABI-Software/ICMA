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

#include "TorsionFitting.h"

TorsionFitting::TorsionFitting(std::vector<std::string> mesh, double focalLength) :
		inputMesh(mesh) {
	numberOfModelFrames_ = mesh.size();
	cmiss_nodes.resize(98);
	//Load the mesh
	//Initialise cmgui and get context
	context_ = Cmiss_context_create("coord");

	/**< Handle to the context */
	Cmiss_region_id root_region = Cmiss_context_get_default_region(context_);

	//Read the mesh and then create torsion free coordinates
	//Currently cmgui allows time sequence merging of fields on nodes only at the root level
	//So create these fields at the root region and get the mesh
	//Load this finally
	std::vector<std::string> loadedMesh;
	for (unsigned int i = 0; i < numberOfModelFrames_; i++)
	{
		double time = static_cast<double>(i) / (numberOfModelFrames_ - 1);
		std::string referenceMesh = std::string(mesh[i]);
		boost::replace_all(referenceMesh, "/heart", "/");
		{
			Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
			Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
			Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, referenceMesh.c_str(), referenceMesh.length());
			Cmiss_stream_information_region_set_resource_attribute_real(stream_information_region, stream_resource, CMISS_STREAM_INFORMATION_REGION_ATTRIBUTE_TIME, time);
			Cmiss_region_read(root_region, stream_information);

			Cmiss_stream_resource_destroy(&stream_resource);
			Cmiss_stream_information_destroy(&stream_information);
			Cmiss_stream_information_region_destroy(&stream_information_region);
		}
		//Change coordinate name to torsionfree_coordinates
		boost::replace_all(referenceMesh, "1) coordinates", "1) torsionfree_coordinates");
		{
			Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
			Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
			Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, referenceMesh.c_str(), referenceMesh.length());
			Cmiss_stream_information_region_set_resource_attribute_real(stream_information_region, stream_resource, CMISS_STREAM_INFORMATION_REGION_ATTRIBUTE_TIME, time);
			Cmiss_region_read(root_region, stream_information);

			Cmiss_stream_resource_destroy(&stream_resource);
			Cmiss_stream_information_destroy(&stream_information);
			Cmiss_stream_information_region_destroy(&stream_information_region);
		}
		{
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
			loadedMesh.push_back(result);
		}
	}

	Cmiss_region_destroy(&root_region);
	Cmiss_context_destroy(&context_);

	context_ = Cmiss_context_create("tf");
	//This required for using the cmiss_context_execute_command
	Cmiss_context_enable_user_interface(context_, NULL);

	/**< Handle to the context */
	root_region = Cmiss_context_get_default_region(context_);

	std::string region_name = "heart";
	// Create a heart region

	Cmiss_region_id heart_region = Cmiss_region_create_child(root_region, region_name.c_str());

	for (unsigned int i = 0; i < numberOfModelFrames_; i++)
	{
		double time = static_cast<double>(i) / (numberOfModelFrames_ - 1);
		{
			Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(heart_region);
			Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
			Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, loadedMesh[i].c_str(), loadedMesh[i].length());
			Cmiss_stream_information_region_set_resource_attribute_real(stream_information_region, stream_resource, CMISS_STREAM_INFORMATION_REGION_ATTRIBUTE_TIME, time);
			Cmiss_region_read(heart_region, stream_information);

			Cmiss_stream_resource_destroy(&stream_resource);
			Cmiss_stream_information_destroy(&stream_information);
			Cmiss_stream_information_region_destroy(&stream_information_region);
		}
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
		//Create the cache
		cache = Cmiss_field_module_create_cache(field_module_);

		std::ostringstream ss;
		ss << "coordinate_system prolate focus " << focalLength << " coordinate_transform field coordinates";

		coordinates_ps_ = Cmiss_field_module_create_field(field_module_, "pscoordinates", ss.str().c_str());
		if (!coordinates_rc_ || !coordinates_ps_)
		{
			std::cout << "--- Coordinates could not be defined for heart region!!! (Torsion fitting)";
			throw -1;
		}

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

		Cmiss_field_module_end_change(field_module_);

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
			Cmiss_field_cache_set_node(cache, cmiss_nodes[endoNodeIds[nc] - 1]);
			temp[0] = temp[1] = temp[2] = 0.0;
			Cmiss_field_evaluate_real(coordinates_ps_, cache, 3, temp);
			initialthetas.push_back(temp[2]);
		}

	}
	else
	{
		std::cout << "--- No field module for heart region!!! (Torison fitting)";
		std::cout << "No field module " << std::endl;
		throw -1;
	}

	Cmiss_region_destroy(&heart_region);
	Cmiss_region_destroy(&root_region);
}

TorsionFitting::~TorsionFitting() {

	Cmiss_field_destroy(&coordinates_rc_);
	Cmiss_field_destroy(&coordinates_ps_);
	for (int i = 0; i < NUMBER_OF_NODES; i++)
	{
		Cmiss_node_destroy(&cmiss_nodes[i]);
	}
	Cmiss_field_cache_destroy(&cache);
	if (coordinates_ps_ != NULL)
		Cmiss_field_destroy(&coordinates_ps_);
	Cmiss_field_module_destroy(&field_module_);
	Cmiss_context_destroy(&context_);
}

void TorsionFitting::setUp(XMLInputReader& reader) {
	int meshSaxNodes[4][12] =
	{
	{ 90, 95, 94, 93, 97, 96, 89, 88, 87, 86, 92, 91 },
		{ 78, 83, 82, 81, 85, 84, 77, 76, 75, 74, 80, 79 },
		{ 58, 68, 65, 64, 72, 70, 56, 54, 51, 50, 62, 59 },
		{ 60, 69, 67, 66, 73, 71, 57, 55, 53, 52, 63, 61 } };
	std::vector<Point3D> saxCentroids;
	std::vector<std::vector<double> > saxangles;
	double coord[3];
	Cmiss_field_module_begin_change(field_module_);
	Cmiss_field_cache_set_time(cache, 0.0);
	std::vector<Point3D> cents;
	for (int i = 0; i < 4; i++)
	{
		Point3D cent(0, 0, 0);
		for (int j = 0; j < 12; j++)
		{
			Cmiss_field_cache_set_node(cache, cmiss_nodes[meshSaxNodes[i][j] - 1]);
			Cmiss_field_evaluate_real(coordinates_rc_, cache, 3, coord);
			cent = cent + Point3D(coord);
		}
		cent = cent * (-1.0 / 12);
		saxCentroids.push_back(cent);
	}

	for (int i = 0; i < 4; i++)
	{
		std::vector<double> angles;
		for (int j = 0; j < 12; j++)
		{
			Cmiss_field_cache_set_node(cache, cmiss_nodes[meshSaxNodes[i][j] - 1]);
			Cmiss_field_evaluate_real(coordinates_rc_, cache, 3, coord);
			Point3D pt = Point3D(coord) + saxCentroids[i];
			double angle = atan2(pt.z, pt.x);
			angles.push_back(angle);
		}
		saxangles.push_back(angles);
	}
	Cmiss_field_module_end_change(field_module_);

	//Load the SAX coordinates and compute the angles
	MarkerTypes shortAxis[] =
	{ SAXAPEX, SAXMID, SAXBASE };
	bool avail[] =
	{ true, true, true };
	std::map<MarkerTypes, std::vector<std::vector<double> > > shortAxisMarkers;
	std::map<MarkerTypes, std::vector<double> > planeInitAngles; //Required for building spline
	unsigned int npts = 0;
	alglib::real_1d_array indexv, values;
	alglib::spline1dfitreport rep;
	alglib::ae_int_t info;
	double index[128];
	for (int i = 0; i < numberOfModelFrames_; i++)
	{
		index[i] = i;
	}
	indexv.setcontent(numberOfModelFrames_, index);
	values.setcontent(numberOfModelFrames_, index);

	for (int i = 0; i < 3; i++)
	{
		std::vector<std::vector<double> > allAngles;
		for (int j = 0; j < numberOfModelFrames_; j++)
		{
			std::vector<Point3D> pts = reader.getMarkers(j, shortAxis[i], true);
			if (pts.size() == 0)
			{
				avail[i] = false;
				break;
			}
			npts = pts.size();
			Point3D pt(0, 0, 0);
			for (int k = 0; k < npts; k++)
			{
				pt = pt + pts[k];
			}
			pt = pt * (-1.0 / npts);
			std::vector<double> angles(npts);

			for (int k = 0; k < npts; k++)
			{
				Point3D ptx = pts[k] + pt;
				angles[k] = atan2(ptx.z, ptx.x);
			}
			allAngles.push_back(angles);
		}
		if (avail[i])
		{
			const double maxAngularChange = M_PI / 18.0;
			//Smooth along time
			for (int k = 0; k < npts; k++)
			{
				for (int j = 0; j < numberOfModelFrames_; j++)
				{
					values[j] = allAngles[j][k];
				}
				alglib::spline1dinterpolant xs;
				alglib::spline1dfitpenalized(indexv, values, 100, 3, info, xs, rep);
				for (int j = 0; j < numberOfModelFrames_; j++)
				{
					allAngles[j][k] = alglib::spline1dcalc(xs, j);
				}
			}
			std::vector<double> initAngles = allAngles[0];
			for (int j = 0; j < numberOfModelFrames_; j++)
			{
				std::vector<double>& pts = allAngles[j];
				for (int k = 0; k < npts; k++)
				{
					pts[k] = fmod(pts[k] - initAngles[k], maxAngularChange);
				}
			}
			shortAxisMarkers[shortAxis[i]] = allAngles;
			planeInitAngles[shortAxis[i]] = initAngles;
		}
	}
	//Build Splines
	nodeValues.resize(npts); //Splines for each base node (12) over number of frames

	double baseDist[] =
	{ 0.0, 0.1, 0.1 + 1 / 3.0, 0.1 + 2 / 3.0, 1.0 };

	indexv.setcontent(4, baseDist);
	values.setcontent(4, baseDist);

	if (avail[0] && avail[1] && avail[2])
	{ //All 3 sax views are available
		std::vector<std::vector<double> >& basept = shortAxisMarkers[SAXBASE];
		std::vector<std::vector<double> >& midpt = shortAxisMarkers[SAXMID];
		std::vector<std::vector<double> >& apexpt = shortAxisMarkers[SAXAPEX];
		for (int i = 0; i < npts; i++)
		{
			std::vector<alglib::spline1dinterpolant> sp(numberOfModelFrames_);
			for (int j = 0; j < numberOfModelFrames_; j++)
			{
				values[0] = 0.0; //No torsion at base
				values[1] = basept[j][i];
				values[2] = midpt[j][i];
				values[3] = apexpt[j][i];
				//Ensure signage
				if (values[1] < 0.0)
					values[1] *= -1.0;
				if (values[3] > 0.0)
					values[3] *= -1.0;
				//Fit node spline
				alglib::spline1dinterpolant xs;
				alglib::spline1dfitpenalized(indexv, values, 100, 3, info, xs, rep);
				sp[j] = xs;
			}
			nodeValues[i] = sp;
		}
	}
	else if (avail[0] && avail[1] && !avail[2])
	{ //APEX and MID available
		std::vector<std::vector<double> >& midpt = shortAxisMarkers[SAXMID];
		std::vector<std::vector<double> >& apexpt = shortAxisMarkers[SAXAPEX];
		for (int i = 0; i < npts; i++)
		{
			std::vector<alglib::spline1dinterpolant> sp(numberOfModelFrames_);
			for (int j = 0; j < numberOfModelFrames_; j++)
			{
				values[0] = 0.0; //No torsion at base
				values[2] = midpt[j][i];
				values[3] = apexpt[j][i];
				//Ensure signage
				if (values[3] > 0.0)
					values[3] *= -1.0;
				values[1] = -values[3] * 2 - values[2];
				if (values[1] < 0.0)
					values[1] *= -1.0;

				//Fit node spline
				alglib::spline1dinterpolant xs;
				alglib::spline1dfitpenalized(indexv, values, 100, 3, info, xs, rep);
				sp[j] = xs;
			}
			nodeValues[i] = sp;
		}
	}
	else if (avail[0] && !avail[1] && avail[2])
	{ //APEX and BASE available
		std::vector<std::vector<double> >& basept = shortAxisMarkers[SAXBASE];
		std::vector<std::vector<double> >& apexpt = shortAxisMarkers[SAXAPEX];
		for (int i = 0; i < npts; i++)
		{
			std::vector<alglib::spline1dinterpolant> sp(numberOfModelFrames_);
			for (int j = 0; j < numberOfModelFrames_; j++)
			{
				values[0] = 0.0; //No torsion at base
				values[1] = basept[j][i];
				values[3] = apexpt[j][i];
				//Ensure signage
				if (values[1] < 0.0)
					values[1] *= -1.0;
				if (values[3] > 0.0)
					values[3] *= -1.0;
				values[2] = (values[1] + values[3]) * 0.5;
				//Fit node spline
				alglib::spline1dinterpolant xs;
				alglib::spline1dfitpenalized(indexv, values, 100, 3, info, xs, rep);
				sp[j] = xs;
			}
			nodeValues[i] = sp;
		}
	}
	else if (!avail[0] && avail[1] && avail[2])
	{ //MID and BASE available
		std::vector<std::vector<double> >& midpt = shortAxisMarkers[SAXMID];
		std::vector<std::vector<double> >& basept = shortAxisMarkers[SAXBASE];
		for (int i = 0; i < npts; i++)
		{
			std::vector<alglib::spline1dinterpolant> sp(numberOfModelFrames_);
			for (int j = 0; j < numberOfModelFrames_; j++)
			{
				values[0] = 0.0; //No torsion at base
				values[1] = basept[j][i];
				values[2] = midpt[j][i];
				//Ensure signage
				if (values[1] < 0.0)
					values[1] *= -1.0;
				values[3] = values[1] * 2 - values[2];
				if (values[3] > 0.0)
					values[3] *= -1.0;

				//Fit node spline
				alglib::spline1dinterpolant xs;
				alglib::spline1dfitpenalized(indexv, values, 100, 3, info, xs, rep);
				sp[j] = xs;
			}
			nodeValues[i] = sp;
		}
	}
	else if (avail[0] && !avail[1] && !avail[2])
	{ //APEX available
		std::vector<std::vector<double> >& pt = shortAxisMarkers[SAXAPEX];
		for (int i = 0; i < npts; i++)
		{
			std::vector<alglib::spline1dinterpolant> sp(numberOfModelFrames_);
			for (int j = 0; j < numberOfModelFrames_; j++)
			{
				values[0] = 0.0; //No torsion at base
				values[3] = pt[j][i];
				//Ensure signage
				if (values[3] > 0.0)
					values[3] *= -1.0;
				values[1] = -0.9 * values[3];
				values[2] = (values[1] + values[3]) * 0.5;
				//Fit node spline
				alglib::spline1dinterpolant xs;
				alglib::spline1dfitpenalized(indexv, values, 100, 3, info, xs, rep);
				sp[j] = xs;
			}
			nodeValues[i] = sp;
		}
	}
	else if (!avail[0] && avail[1] && !avail[2])
	{ //MID available
		std::vector<std::vector<double> >& pt = shortAxisMarkers[SAXMID];
		for (int i = 0; i < npts; i++)
		{
			std::vector<alglib::spline1dinterpolant> sp(numberOfModelFrames_);
			for (int j = 0; j < numberOfModelFrames_; j++)
			{
				values[0] = 0.0; //No torsion at base
				values[2] = pt[j][i];
				values[1] = values[2] / 0.95; //Assumes that base torsion is almost 0.9*apex torsion

				//Ensure signage
				if (values[1] < 0.0)
					values[1] *= -1.0;
				values[3] = -1.1 * values[1];
				//Fit node spline
				alglib::spline1dinterpolant xs;
				alglib::spline1dfitpenalized(indexv, values, 100, 3, info, xs, rep);
				sp[j] = xs;
			}
			nodeValues[i] = sp;
		}
	}
	else if (!avail[0] && !avail[1] && avail[2])
	{ //BASE available
		std::vector<std::vector<double> >& pt = shortAxisMarkers[SAXBASE];
		for (int i = 0; i < npts; i++)
		{
			std::vector<alglib::spline1dinterpolant> sp(numberOfModelFrames_);
			for (int j = 0; j < numberOfModelFrames_; j++)
			{
				values[0] = 0.0; //No torsion at base
				values[1] = pt[j][i];
				//Ensure signage
				if (values[1] < 0.0)
					values[1] *= -1.0;
				values[3] = -1.1 * values[1];
				values[2] = (values[1] + values[3]) * 0.5;
				//Fit node spline
				alglib::spline1dinterpolant xs;
				alglib::spline1dfitpenalized(indexv, values, 100, 3, info, xs, rep);
				sp[j] = xs;
			}
			nodeValues[i] = sp;
		}
	}

	//Compute torsions for model short axis locations
	baseDist[0] = 0.75;
	baseDist[1] = 0.5;
	baseDist[2] = 0.25;
	baseDist[3] = 0.0;

	for (int bc = 0; bc < 3; bc++) //Compute torsions for non base (as torsion at base is set to zero)
	{
		std::vector<double> initAngles = planeInitAngles[shortAxis[bc]];

		unsigned int numSrcPts = initAngles.size();
		if (numSrcPts != 0)
		{
			for (int i = 0; i < numSrcPts; i++)
			{
				index[i] = initAngles[i];
			}
			indexv.setcontent(numSrcPts, index);
			values.setcontent(numSrcPts, index);
		}
		else
		{ //Choose the nonempty shortAxis
			for (int j = 0; j < 3; j++)
			{
				initAngles = planeInitAngles[shortAxis[bc]];
				numSrcPts = initAngles.size();
				if (numSrcPts != 0)
				{
					for (int i = 0; i < numSrcPts; i++)
					{
						index[i] = initAngles[i];
					}
					indexv.setcontent(numSrcPts, index);
					values.setcontent(numSrcPts, index);

				}
				break;
			}
		}
		std::vector<std::vector<double> > planeTorsionAngles;

		for (int i = 0; i < numberOfModelFrames_; i++)
		{
			for (int j = 0; j < numSrcPts; j++)
			{
				values[j] = alglib::spline1dcalc(nodeValues[j][i], baseDist[bc]);
			}
			alglib::spline1dinterpolant xs;
			try
			{
				alglib::spline1dfitpenalized(indexv, values, 50, 5, info, xs, rep);
			} catch (alglib::ap_error& err)
			{
				std::cout << indexv.tostring(3) << "\n" << values.tostring(3) << "\n" << err.msg << std::endl;
			}

			std::vector<double> modelAngles(12);
			std::vector<double>& planeAngle = saxangles[bc];
			for (int j = 0; j < 12; j++)
			{
				try
				{
					modelAngles[j] = alglib::spline1dcalc(xs, planeAngle[j]);
				} catch (alglib::ap_error& err)
				{
					std::cout << "@ j " << j << indexv.tostring(3) << "\n" << values.tostring(3) << "\n" << err.msg << std::endl;
				}
			}
			planeTorsionAngles.push_back(modelAngles);
		}
		torsionAngles.push_back(planeTorsionAngles);
	}

	for (int p = 0; p < 3; p++) //For each sax plane
	{
		std::vector<std::vector<double> >& pta = torsionAngles[p];
		for (int j = 0; j < 12; j++) //For each point, smooth along time
		{
			//Drift compensate
			double factor = pta[numberOfModelFrames_ - 1][j] / (numberOfModelFrames_ - 1.0);

			for (int i = 0; i < numberOfModelFrames_; i++)
			{
				pta[i][j] -= i * factor;
			}
		}
	}

	Cmiss_field_module_begin_change(field_module_);
	for (int i = 0; i < numberOfModelFrames_; i++)
	{
		double time = ((double) i) / ((double) numberOfModelFrames_ - 1);
		Cmiss_field_cache_set_time(cache, time);
		for (int p = 0; p < 3; p++)
		{
			std::vector<double>& ta = torsionAngles[p][i];
			for (int j = 0; j < 12; j++)
			{
				Cmiss_field_cache_set_node(cache, cmiss_nodes[meshSaxNodes[p][j] - 1]);
				Cmiss_field_evaluate_real(coordinates_ps_, cache, 3, coord);
				coord[2] += ta[j];
				Cmiss_field_assign_real(coordinates_ps_, cache, 3, coord);
			}
		}
	}
	Cmiss_field_module_end_change(field_module_);

}

std::vector<std::string> TorsionFitting::getMesh() {
	std::vector<std::string> resultx;
	for (int i = 0; i < numberOfModelFrames_; i++)
	{
		double time = ((double) i) / ((double) numberOfModelFrames_ - 1);
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
	//Create reference coordinates
	{

		Cmiss_context_id context = Cmiss_context_create("refcoord");
		/**< Handle to the context */
		Cmiss_region_id root_region = Cmiss_context_get_default_region(context);

		//Read the mesh and then create reference coordinates
		//Currently cmgui allows time sequence merging of fields on nodes only at the root level
		//So create these fields at the root region and get the mesh
		//Load this finally
		std::string referenceMesh = std::string(resultx[0]);
		std::string result;
		boost::replace_all(referenceMesh, "/heart", "/");
		{
			Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
			Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
			Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, referenceMesh.c_str(), referenceMesh.length());
			Cmiss_region_read(root_region, stream_information);

			Cmiss_stream_resource_destroy(&stream_resource);
			Cmiss_stream_information_destroy(&stream_information);
			Cmiss_stream_information_region_destroy(&stream_information_region);
		}
		//Change coordinate name to torsionfree_coordinates
		boost::replace_all(referenceMesh, "1) coordinates", "1) reference_coordinates");
		{
			Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
			Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
			Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, referenceMesh.c_str(), referenceMesh.length());
			Cmiss_region_read(root_region, stream_information);

			Cmiss_stream_resource_destroy(&stream_resource);
			Cmiss_stream_information_destroy(&stream_information);
			Cmiss_stream_information_region_destroy(&stream_information_region);
		}
		{
			Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
			Cmiss_stream_information_region_id region_stream_information = Cmiss_stream_information_cast_region(stream_information);
			Cmiss_stream_resource_id stream = Cmiss_stream_information_create_resource_memory(stream_information);

			Cmiss_region_write(root_region, stream_information);
			void *memory_buffer = NULL;
			unsigned int memory_buffer_size = 0;
			Cmiss_stream_resource_memory_id memory_resource = Cmiss_stream_resource_cast_memory(stream);
			Cmiss_stream_resource_memory_get_buffer_copy(memory_resource, &memory_buffer, &memory_buffer_size);

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
		}
		Cmiss_region_destroy(&root_region);
		Cmiss_context_destroy(&context);
		//Load the mesh into heart region
		context = Cmiss_context_create("refcoord");
		/**< Handle to the context */
		root_region = Cmiss_context_get_default_region(context);
		Cmiss_region_id heart_region = Cmiss_region_create_child(root_region, "heart");
		{
			Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(heart_region);
			Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
			Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, result.c_str(), result.length());
			Cmiss_region_read(heart_region, stream_information);

			Cmiss_stream_resource_destroy(&stream_resource);
			Cmiss_stream_information_destroy(&stream_information);
			Cmiss_stream_information_region_destroy(&stream_information_region);
		}
		{
			Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
			Cmiss_stream_information_region_id region_stream_information = Cmiss_stream_information_cast_region(stream_information);
			Cmiss_stream_resource_id stream = Cmiss_stream_information_create_resource_memory(stream_information);

			Cmiss_region_write(root_region, stream_information);
			void *memory_buffer = NULL;
			unsigned int memory_buffer_size = 0;
			Cmiss_stream_resource_memory_id memory_resource = Cmiss_stream_resource_cast_memory(stream);
			Cmiss_stream_resource_memory_get_buffer_copy(memory_resource, &memory_buffer, &memory_buffer_size);

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
		}
		Cmiss_region_destroy(&heart_region);
		Cmiss_region_destroy(&root_region);
		Cmiss_context_destroy(&context);
		resultx[0] = result;
	}
	return resultx;
}
