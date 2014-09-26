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

#include "StrainMeasures.h"
#include "humandtifibre.h"

StrainMeasures::StrainMeasures(LVHeartMesh& mesh, XMLInputReader& input) :
		heart(mesh), inputReader(input), mySegments(NULL) {
	num_level_sets = 10;
	num_elements = 48;
	num_nodes = 98;
	elements.resize(num_elements);
	x_discret = 25;
	y_discret = 25;
	lagrangian = true;
	specklePGS = 0.0;
	modelPGS = 0.0;
	computeModelPGS = false;
	ejectionFraction = 0.0;
	activeViews[0] = activeViews[1] = activeViews[2] = activeViews[3] = false;
	extraNodeStartID = 100000;
	Cmiss_region_id root_region = Cmiss_context_get_default_region(heart.context_);
	field_module = heart.field_module_;
	fieldCache = Cmiss_field_module_create_cache(field_module);
	coordianteField = Cmiss_field_module_find_field_by_name(field_module, "reference_coordinates");

	if (!coordianteField)
	{ //If ref coordinates is not available, define it
	  //Define the necessary fields
		std::string referenceMesh = heart.getMeshAt(0);
		//Change coordinate name to reference_coordinates
		boost::replace_all(referenceMesh, "1) coordinates", "1) reference_coordinates");

		//Load the reference coordiantes mesh
		Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
		Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
		Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, referenceMesh.c_str(), referenceMesh.length());

		Cmiss_region_read(root_region, stream_information);
		Cmiss_stream_resource_destroy(&stream_resource);
		Cmiss_stream_information_destroy(&stream_information);
	}
	//#Calculate the strains
	//fieldModule.defineField("F","gradient coordinate rc_reference_coordinates field rc_coordinates");
	Cmiss_field_module_define_field(field_module, "F", "gradient coordinate reference_coordinates field coordinates");
	//fieldModule.defineField("F_transpose","transpose source_number_of_rows 3 field F");
	Cmiss_field_module_define_field(field_module, "F_transpose", "transpose source_number_of_rows 3 field F");
	//fieldModule.defineField("C","matrix_multiply number_of_rows 3 fields F_transpose F");
	Cmiss_field_module_define_field(field_module, "C", "matrix_multiply number_of_rows 3 fields F_transpose F");
	//fieldModule.defineField("principal_strains","eigenvalues field C");
	Cmiss_field_module_define_field(field_module, "principal_strains", "eigenvalues field C");

	Cmiss_field_destroy(&coordianteField);
	//Assign the coordinates field to the handle for downstream use
	coordianteField = Cmiss_field_module_find_field_by_name(field_module, "coordinates");
	//Define the fibre field
	{
		//Load the reference fibre data and reference coordinates
		{
			if (Cmiss_field_module_find_field_by_name(field_module, "fibres") == NULL)
			{
				Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
				Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
				//Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, reffibre_exregion, reffibre_exregion_len);
				Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, humandtifibre_exregion,
						humandtifibre_exregion_len);

				Cmiss_region_read(root_region, stream_information);
				Cmiss_stream_resource_destroy(&stream_resource);
				Cmiss_stream_information_destroy(&stream_information);
			}

			//Define fibre field

			//#Calculate the deformed fibre axes
			Cmiss_field_module_define_field(field_module, "fibre_axes", "fibre_axes coordinate reference_coordinates fibre fibres");

			Cmiss_field_module_define_field(field_module, "deformed_fibre_axes", "matrix_multiply number_of_rows 3 fields fibre_axes F_transpose");

			Cmiss_field_module_define_field(field_module, "deformed_fibre", "composite deformed_fibre_axes.1 deformed_fibre_axes.2 deformed_fibre_axes.3");

			Cmiss_field_module_define_field(field_module, "principal_fibre_strain1", "composite deformed_fibre_axes.1");
		}
	}
	//Get the element and node handles
	Cmiss_mesh_id cmiss_mesh = Cmiss_field_module_find_mesh_by_name(field_module, "cmiss_mesh_3d");

	Cmiss_element_iterator_id elementIterator = Cmiss_mesh_create_element_iterator(cmiss_mesh);
	Cmiss_element_id element = Cmiss_element_iterator_next(elementIterator);

	while (element != NULL)
	{
		int elementId = Cmiss_element_get_identifier(element) - 1; //Cmiss numbering starts at 1
		elements[elementId] = element;
		element = Cmiss_element_iterator_next(elementIterator);
	}
	Cmiss_element_iterator_destroy(&elementIterator);
	Cmiss_mesh_destroy(&cmiss_mesh);
	Cmiss_region_destroy(&root_region);
}

StrainMeasures::~StrainMeasures() {

	for (unsigned int i = 0; i < num_elements; i++)
	{
		Cmiss_element_destroy(&(elements[i]));
	}
	Cmiss_field_cache_destroy(&fieldCache);
	Cmiss_field_destroy(&coordianteField);
}

void StrainMeasures::setWallSegments(std::vector<WallSegment>* segs) {
	mySegments = segs;
}

/**
 * Computes natural strain rather than lagrangian strain
 *
 * See Hooge et al, "Regional strain and strain rate measurement by Cardiac Ultarsound:
 * Principles, Implementation and Limitations", Eur. J. Echocardiography, 2000, 1, 154-179.
 *
 */

std::vector<std::string> StrainMeasures::getSegmentStrains(std::string fieldName) {
	std::vector<std::string> strains(19);
	Cmiss_field_id field = Cmiss_field_module_find_field_by_name(field_module, fieldName.c_str());

	//Compute strains
	unsigned int numSegments = mySegments->size();
	double temp_array1[3];
	double temp_array2[3];

#ifdef printcoord
#include "MeshTopology.h"

	std::vector<Cmiss_node_id> myNodes(100);
	Cmiss_nodeset_id nodeset = Cmiss_field_module_find_nodeset_by_name(field_module, "cmiss_nodes");
	Cmiss_node_iterator_id nodeIterator = Cmiss_nodeset_create_node_iterator(nodeset);
	Cmiss_node_id node = Cmiss_node_iterator_next(nodeIterator);
	if (node != 0)
	{
		double temp_array[3];

		while (node)
		{
			int node_id = Cmiss_node_get_identifier(node);
			myNodes[node_id-1] = node;
			node = Cmiss_node_iterator_next(nodeIterator);
		}
	}
	Cmiss_nodeset_destroy(&nodeset);
	Cmiss_node_iterator_destroy(&nodeIterator);

	std::vector<int> Nodes(27);

	Nodes[8] = aplaxNodes8;
	Nodes[7] = aplaxNodes7;
	Nodes[6] = aplaxNodes6;
	Nodes[5] = aplaxNodes5;
	Nodes[4] = aplaxNodes4;
	Nodes[3] = aplaxNodes3;
	Nodes[2] = aplaxNodes2;
	Nodes[1] = aplaxNodes1;
	Nodes[0] = aplaxNodes0;

	Nodes[9] = tchNodes0;
	Nodes[10] = tchNodes1;
	Nodes[11] = tchNodes2;
	Nodes[12] = tchNodes3;
	Nodes[13] = tchNodes4;
	Nodes[14] = tchNodes5;
	Nodes[15] = tchNodes6;
	Nodes[16] = tchNodes7;
	Nodes[17] = tchNodes8;

	Nodes[18] = fchNodes0;
	Nodes[19] = fchNodes1;
	Nodes[20] = fchNodes2;
	Nodes[21] = fchNodes3;
	Nodes[22] = fchNodes4;
	Nodes[23] = fchNodes5;
	Nodes[24] = fchNodes6;
	Nodes[25] = fchNodes7;
	Nodes[26] = fchNodes8;

#endif
	std::vector<std::vector<double> > aplaxLengths;
	std::vector<std::vector<double> > tchLengths;
	std::vector<std::vector<double> > fchLengths;
	std::vector<double>& frameTimes = heart.frameTimes;
	for (int i = 0; i < heart.numberOfModelFrames_; i++)
	{
		std::vector<double> cLengths;
		for (int j = 0; j < numSegments; j++)
		{
			WallSegment& seg = mySegments->at(j);
			//The the lengths
			double time = frameTimes[i];
			Cmiss_field_cache_set_time(fieldCache, time);
			temp_array1[0] = temp_array1[1] = temp_array1[2] = 0.0;
			temp_array2[0] = temp_array2[1] = temp_array2[2] = 0.0;
			//Since cmiss id starts at 1 subtract 1 from seg.elemeid?
			//Note that accessing some computed field (those that involve gradients), with multiple versions leads to gradient set to 0 warning
			Cmiss_field_cache_set_mesh_location(fieldCache, elements[seg.elementida - 1], 3, seg.xia);
			Cmiss_field_evaluate_real(field, fieldCache, 3, temp_array1);
			Cmiss_field_cache_set_mesh_location(fieldCache, elements[seg.elementidb - 1], 3, seg.xib);
			Cmiss_field_evaluate_real(field, fieldCache, 3, temp_array2);
			Point3D p1(temp_array1);
			Point3D p2(temp_array2);
			double dist = p1.distance(p2);
#ifdef printcoord
			if (i == 10)
			{
				int nodeCtr = (j / 8) * 9 + j % 8;
				Cmiss_field_cache_set_node(fieldCache, myNodes[Nodes[nodeCtr]]);
				temp_array1[0] = temp_array1[1] = temp_array1[2] = 0.0;
				Cmiss_field_evaluate_real(field, fieldCache, 3, temp_array1);
				Point3D p3(temp_array1);
				Cmiss_field_cache_set_node(fieldCache, myNodes[Nodes[nodeCtr+1]]);
				temp_array1[0] = temp_array1[1] = temp_array1[2] = 0.0;
				Cmiss_field_evaluate_real(field, fieldCache, 3, temp_array1);
				Point3D p4(temp_array1);
				/*std::cout << j << "\t" << i << "\t"<<seg.elementida<<"\t"<<Point3D(seg.xia)<<"\t"
				 <<seg.elementidb<<"\t"<<Point3D(seg.xib)<<"\t";*/
				std::cout << p1 << "\t" << p2 << " = "
				<< dist << "\t:\t Value at " << Nodes[nodeCtr]+1 << "\t"
				<< p3 << "\t" << p1.distance(p3)<<"\t" << Nodes[nodeCtr+1] +1<<"\t"
				<< p4 << "\t"<< p2.distance(p4)<<"\t distance \t "<<p3.distance(p4) <<std::endl;
			}
#endif
			cLengths.push_back(dist);
		}

		for (int segc = 0; segc < numSegments / 8; segc++)
		{
			int offset = segc * 8;
			std::vector<double> sLengths;

			sLengths.push_back(cLengths[0 + offset] + (1 / 3.0 - 1 / 4.0) * 4 * cLengths[1 + offset]);
			sLengths.push_back((1.0 - (1 / 3.0 - 1 / 4.0)) * 4 * cLengths[1 + offset] + (2 / 3.0 - 1 / 2.0) * 4 * cLengths[2 + offset]);
			sLengths.push_back((1.0 - (2 / 3.0 - 1 / 2.0)) * 4 * cLengths[2 + offset] + cLengths[3 + offset]);
			sLengths.push_back(cLengths[4 + offset] + (1.0 - (2 / 3.0 - 1 / 2.0)) * 4 * cLengths[5 + offset]);
			sLengths.push_back((2 / 3.0 - 1 / 2.0) * 4 * cLengths[5 + offset] + (1.0 - (1 / 3.0 - 1 / 4.0)) * 4 * cLengths[6 + offset]);
			sLengths.push_back((1 / 3.0 - 1 / 4.0) * 4 * cLengths[6 + offset] + cLengths[7 + offset]);

			if (segc == 0)
				aplaxLengths.push_back(sLengths);
			else if (segc == 1)
				tchLengths.push_back(sLengths);
			else
				fchLengths.push_back(sLengths);
		}
	}
	std::vector<double> avgStrains(heart.numberOfModelFrames_, 0.0);
	std::vector<int> selectedStrains  = inputReader.getStrainSelections();

	for (int segc = 0; segc < numSegments / 8; segc++)
	{
		std::vector<std::vector<double> > dstrains;
		std::vector<std::vector<double> > distances;
		if (segc == 0)
			distances = aplaxLengths;
		else if (segc == 1)
			distances = tchLengths;
		else if (segc == 2)
			distances = fchLengths;

		std::vector<double>& initStrain = distances[0];
		for (int frame = 1; frame < heart.numberOfModelFrames_; frame++)
		{ //Compute Strains
			std::vector<double>& curStrainLengths = distances[frame];
			std::vector<double> curStrain;
			double c = 0;
			unsigned int ulimit = initStrain.size();
			for (int j = 0; j < ulimit; j++)
			{
				c = 100.0 * (curStrainLengths[j] - initStrain[j]) / initStrain[j];
				curStrain.push_back(c);
			}
			dstrains.push_back(curStrain);
		}

		std::vector<std::string> strainSeries;
		std::vector<int> selections(6,0);
		if (segc == 0)
		{
			selections[5] = selectedStrains[2 - 1];
			selections[4] = selectedStrains[8 - 1];
			selections[3] = selectedStrains[17 - 1];
			selections[2] = selectedStrains[18 - 1];
			selections[1] = selectedStrains[11 - 1];
			selections[0] = selectedStrains[5 - 1];
		}
		else if (segc == 2)
		{
			selections[0] = selectedStrains[3 - 1];
			selections[1] = selectedStrains[9 - 1];
			selections[2] = selectedStrains[14 - 1];
			selections[3] = selectedStrains[16 - 1];
			selections[4] = selectedStrains[12 - 1];
			selections[5] = selectedStrains[6 - 1];
		}
		else if (segc == 1)
		{
			selections[0] = selectedStrains[4 - 1];
			selections[1] = selectedStrains[10 - 1];
			selections[2] = selectedStrains[15 - 1];
			selections[3] = selectedStrains[13 - 1];
			selections[4] = selectedStrains[7 - 1];
			selections[5] = selectedStrains[1 - 1];
		}
		for (int j = 0; j < initStrain.size(); j++)
		{
			std::ostringstream ss;
			ss << 0.0; //For init step

			int denom = heart.numberOfModelFrames_ - 1;
			double maxStrain = dstrains[denom - 1][j];
			//Note that frame goes from 1 to heart.numberOfModelFrames_ when computing strain
			//so shift down by 1
			for (int i = 0; i < denom; i++)
			{ //Compute Strains
			  //Drift compensate
				double stc = dstrains[i][j] - (i + 1) * maxStrain / denom;
				ss << "," << stc;
				//Add to avg if it was selected
				if(selections[j])
					avgStrains[i] += stc;
			}
			strainSeries.push_back(ss.str());
		}
		if (segc == 0)
		{
			strains[2 - 1] = strainSeries[5];
			strains[8 - 1] = strainSeries[4];
			strains[17 - 1] = strainSeries[3];
			strains[18 - 1] = strainSeries[2];
			strains[11 - 1] = strainSeries[1];
			strains[5 - 1] = strainSeries[0];
		}
		else if (segc == 2)
		{
			strains[3 - 1] = strainSeries[0];
			strains[9 - 1] = strainSeries[1];
			strains[14 - 1] = strainSeries[2];
			strains[16 - 1] = strainSeries[3];
			strains[12 - 1] = strainSeries[4];
			strains[6 - 1] = strainSeries[5];
		}
		else if (segc == 1)
		{
			strains[4 - 1] = strainSeries[0];
			strains[10 - 1] = strainSeries[1];
			strains[15 - 1] = strainSeries[2];
			strains[13 - 1] = strainSeries[3];
			strains[7 - 1] = strainSeries[4];
			strains[1 - 1] = strainSeries[5];
		}
	}
#ifdef printcoord
	std::cout << "Linear 3D " << fieldName << std::endl;
	for (int i = 1; i < 100; i++)
	Cmiss_node_destroy(&myNodes[i]);

#endif
	//Add the average strain
	//Num strain segments depends on the number of active segments (6 strain segments per view, which has 8 active segments)

	double denom = 0;
	for(int i=0;i<18;i++){
			denom +=selectedStrains[i];
	}

	std::ostringstream ss;
	ss << 0.0; //For init step
	for (int i = 0; i < heart.numberOfModelFrames_ - 1; i++)
	{ //Compute the Average
		ss << "," << avgStrains[i] / denom;
	}
	strains[18] = ss.str();
	//Check if model PGS should be calculated
	if (computeModelPGS)
	{
		double max = fabs(avgStrains[0]);
		int idx = 0;
		for (int i = 1; i < heart.numberOfModelFrames_ - 1; i++)
		{
			if (fabs(avgStrains[i]) > max)
			{
				max = fabs(avgStrains[i]);
				idx = i;
			}
		}
		modelPGS = avgStrains[idx] / denom;
		computeModelPGS = false; //set it so that it is not computed in subsequent calls
	}

	Cmiss_field_destroy(&field);

	return strains;
}

std::vector<std::string> StrainMeasures::getSixteenSegmentStrains(std::string fieldName, double wall_xi) {

#ifdef printcoord
	std::cout << "Linear 3D " << fieldName << "\t" << wall_xi << std::endl;
#endif
	std::vector<WallSegment> segments;

	//APLAX 10 22 34 46 46 40 28 16 4
	//      13  9  5  1  1  3  7 11 15
	//Base
	WallSegment seg1(13, 9);
	seg1.setXIA(0, 1, wall_xi);
	seg1.setXIB(0, 1, wall_xi);

	WallSegment seg2(9, 5);
	seg2.setXIA(0, 1, wall_xi);
	seg2.setXIB(0, 1, wall_xi);

	//Mid
	WallSegment seg3(5, 1);
	seg3.setXIA(0, 1, wall_xi);
	seg3.setXIB(0, 1, wall_xi);

	WallSegment seg4(1, 1);
	seg4.setXIA(0, 1, wall_xi);
	seg4.setXIB(0, 0, wall_xi);

	//Apex

	WallSegment seg5(1, 3);
	seg5.setXIA(0, 0, wall_xi);
	seg5.setXIB(0, 1, wall_xi);

	WallSegment seg6(3, 7);
	seg6.setXIA(0, 1, wall_xi);
	seg6.setXIB(0, 1, wall_xi);

	WallSegment seg7(7, 11);
	seg7.setXIA(0, 1, wall_xi);
	seg7.setXIB(0, 1, wall_xi);

	WallSegment seg8(11, 15);
	seg8.setXIA(0, 1, wall_xi);
	seg8.setXIB(0, 1, wall_xi);

	//TCH 12 24 36 48 48 42 30 18 6
	//    45 41 37 33 33 35 39 43 47
	//Base
	WallSegment tseg1(45, 41);
	tseg1.setXIA(0, 1, wall_xi);
	tseg1.setXIB(0, 1, wall_xi);

	WallSegment tseg2(41, 37);
	tseg2.setXIA(0, 1, wall_xi);
	tseg2.setXIB(0, 1, wall_xi);

	//Mid
	WallSegment tseg3(37, 33);
	tseg3.setXIA(0, 1, wall_xi);
	tseg3.setXIB(0, 1, wall_xi);

	WallSegment tseg4(33, 33);
	tseg4.setXIA(0, 1, wall_xi);
	tseg4.setXIB(0, 0, wall_xi);

	//Apex

	WallSegment tseg5(33, 35);
	tseg5.setXIA(0, 0, wall_xi);
	tseg5.setXIB(0, 1, wall_xi);

	WallSegment tseg6(35, 39);
	tseg6.setXIA(0, 1, wall_xi);
	tseg6.setXIB(0, 1, wall_xi);

	WallSegment tseg7(39, 43);
	tseg7.setXIA(0, 1, wall_xi);
	tseg7.setXIB(0, 1, wall_xi);

	WallSegment tseg8(43, 47);
	tseg8.setXIA(0, 1, wall_xi);
	tseg8.setXIB(0, 1, wall_xi);

	//FCH 2 14 26 38 38 44 32 20 8
	//   32 28 24 20 20 18 22 26 30
	//Base
	WallSegment fseg1(32, 28);
	fseg1.setXIA(0, 1, wall_xi);
	fseg1.setXIB(0, 1, wall_xi);

	WallSegment fseg2(28, 24);
	fseg2.setXIA(0, 1, wall_xi);
	fseg2.setXIB(0, 1, wall_xi);

	//Mid
	WallSegment fseg3(24, 20);
	fseg3.setXIA(0, 1, wall_xi);
	fseg3.setXIB(0, 1, wall_xi);

	WallSegment fseg4(20, 20);
	fseg4.setXIA(0, 1, wall_xi);
	fseg4.setXIB(0, 0, wall_xi);

	//Apex

	WallSegment fseg5(20, 18);
	fseg5.setXIA(0, 0, wall_xi);
	fseg5.setXIB(0, 1, wall_xi);

	WallSegment fseg6(18, 22);
	fseg6.setXIA(0, 1, wall_xi);
	fseg6.setXIB(0, 1, wall_xi);

	WallSegment fseg7(22, 26);
	fseg7.setXIA(0, 1, wall_xi);
	fseg7.setXIB(0, 1, wall_xi);

	WallSegment fseg8(26, 30);
	fseg8.setXIA(0, 1, wall_xi);
	fseg8.setXIB(0, 1, wall_xi);

	segments.clear();

	segments.push_back(seg1);
	segments.push_back(seg2);
	segments.push_back(seg3);
	segments.push_back(seg4);
	segments.push_back(seg5);
	segments.push_back(seg6);
	segments.push_back(seg7);
	segments.push_back(seg8);

	segments.push_back(tseg1);
	segments.push_back(tseg2);
	segments.push_back(tseg3);
	segments.push_back(tseg4);
	segments.push_back(tseg5);
	segments.push_back(tseg6);
	segments.push_back(tseg7);
	segments.push_back(tseg8);
	segments.push_back(fseg1);
	segments.push_back(fseg2);
	segments.push_back(fseg3);
	segments.push_back(fseg4);
	segments.push_back(fseg5);
	segments.push_back(fseg6);
	segments.push_back(fseg7);
	segments.push_back(fseg8);

	mySegments = &segments;
	return getSegmentStrains(fieldName);
}

double StrainMeasures::getLocalDeformation(double* eigenvalues, long * nan_count) {
	double maxd = std::max(std::max(fabs(eigenvalues[0]), fabs(eigenvalues[1])), fabs(eigenvalues[2]));
	double mind = std::min(std::min(fabs(eigenvalues[0]), fabs(eigenvalues[1])), fabs(eigenvalues[2]));

	double value = 0.0;
	if (mind == 0.0)
	{
		nan_count++;
	}
	else
	{
		value = (maxd / mind);
	}
	return value;
}

std::vector<std::string> StrainMeasures::getLinearDistortion(double* wall_xi, unsigned int cnt, double power) {
	unsigned int xdiscret = 25;
	unsigned int ydiscret = 25;
	//Create points of interest array
	double **pointsOfInterest_x = new double*[xdiscret + 1];
	double **pointsOfInterest_y = new double*[xdiscret + 1];
	for (unsigned int i = 0; i <= xdiscret; i++)
	{
		pointsOfInterest_x[i] = new double[ydiscret + 1];
		pointsOfInterest_y[i] = new double[ydiscret + 1];
	}
	double delx = 1.0 / xdiscret;
	double dely = 1.0 / ydiscret;

	for (unsigned int xd = 0; xd <= xdiscret; xd++)
	{
		double xinc = xd * delx;
		if (xinc == 0.0)
			xinc = 0.001;
		if (xinc == 1.0)
			xinc = 0.99;
		for (unsigned int yd = 0; yd <= ydiscret; yd++)
		{
			double yinc = yd * dely;
			if (yinc == 0.0)
				yinc = 0.001;
			if (yinc == 1.0)
				yinc = 0.99;
			pointsOfInterest_x[xd][yd] = xinc;
			pointsOfInterest_y[xd][yd] = yinc;
		}
	}

	Cmiss_field_id principleStarinsField = Cmiss_field_module_find_field_by_name(field_module, "principal_strains");
	double temp_array[3];
	double coordinates[3];
	long nan_count = 0;

	std::vector<std::string> results;

	std::vector<LevelSet*>* coordSets = new std::vector<LevelSet*>();
	std::vector<LevelSet*>* deformSets = new std::vector<LevelSet*>();
	std::vector<double>& frameTimes = heart.frameTimes;
	for (unsigned int ls = 0; ls < cnt; ls++)
	{
		coordSets->clear();
		deformSets->clear();
		for (unsigned int mt = 0; mt < heart.numberOfModelFrames_; mt++)
		{
			//double time = ((double) mt) / heart.numberOfModelFrames_;
			double time = frameTimes[mt];
			LevelSet* myCoordLevelSet = new LevelSet(num_elements, xdiscret + 1, ydiscret + 1);
			LevelSet* myDeformLevelSet = new LevelSet(num_elements, xdiscret + 1, ydiscret + 1, 1);

			Cmiss_field_cache_set_time(fieldCache, time);
			nan_count = 0;
			for (unsigned int xd = 0; xd <= xdiscret; xd++)
			{
				for (unsigned int yd = 0; yd <= ydiscret; yd++)
				{
					for (unsigned int elementId = 0; elementId < num_elements; elementId++)
					{
						coordinates[0] = pointsOfInterest_x[xd][yd];
						coordinates[1] = pointsOfInterest_y[xd][yd];
						coordinates[2] = wall_xi[ls];
						temp_array[0] = temp_array[1] = temp_array[2] = 0.0;

						Cmiss_field_cache_set_mesh_location(fieldCache, elements[elementId], 3, coordinates);
						Cmiss_field_evaluate_real(principleStarinsField, fieldCache, 3, temp_array);
						double result = getLocalDeformation(temp_array, &nan_count);
						myDeformLevelSet->setData(elementId, xd, yd, &result, 1);

						Cmiss_field_evaluate_real(coordianteField, fieldCache, 3, temp_array);
						myCoordLevelSet->setData(elementId, xd, yd, temp_array, 3);
					}
				}
			}
			coordSets->push_back(myCoordLevelSet);
			deformSets->push_back(myDeformLevelSet);
		}
		std::stringstream ss;
		unsigned int numpoints = coordSets->size();
		for (unsigned int i = 0; i < numpoints; i++)
		{
			LevelSet* myCoordLevelSet = coordSets->at(i);
			LevelSet* myDeformLevelSet = deformSets->at(i);
			ss << myDeformLevelSet->getLPSum(0, power) / myCoordLevelSet->getArea();
			if (i < (numpoints - 1))
				ss << ",";
			delete myCoordLevelSet;
			delete myDeformLevelSet;
		}
		results.push_back(ss.str());
	}
	//Clean up
	delete deformSets;
	delete coordSets;

	for (unsigned int i = 0; i <= xdiscret; i++)
	{
		delete[] pointsOfInterest_x[i];
		delete[] pointsOfInterest_y[i];
	}

	delete[] pointsOfInterest_x;
	delete[] pointsOfInterest_y;

	Cmiss_field_destroy(&principleStarinsField);
	return results;
}

void StrainMeasures::Print(std::string filename, std::vector<std::string> strains) {
	std::ofstream file(filename.c_str(), std::ios_base::binary);
	unsigned int num = strains.size();
	if (num > 0)
	{
		//Print time header
		file << "Time";
		std::vector<double>& frameTimes = heart.frameTimes;
		for (unsigned int i = 0; i < heart.numberOfModelFrames_; i++)
		{
			file << "\t" << frameTimes[i];
		}
		file << std::endl;
		for (unsigned int i = 0; i < num; i++)
		{
			std::string sample = strains[i];
			std::vector<std::string> str1;
			boost::split(str1, sample, boost::is_any_of(","));
			file << "Segment:" << i;
			for (unsigned int j = 0; j < str1.size(); j++)
			{
				file << "\t" << str1[j];
			}
			file << std::endl;
		}
	}
	file.close();
}

std::vector<std::string> StrainMeasures::getSegmentStrains() {
	return getSegmentStrains("coordinates");
}

std::vector<std::string> StrainMeasures::getSixteenSegmentStrains(double wall_xi) {
	if (wall_xi == 0.0)
	{
		computeModelPGS = true;
	}
	return getSixteenSegmentStrains("coordinates", wall_xi);
}

std::vector<std::string> StrainMeasures::getSegmentFibreStrains() {
	return getSegmentStrains("deformed_fibre");
}

std::vector<std::string> StrainMeasures::getSixteenSegmentFibreStrains(double wall_xi) {
	return getSixteenSegmentStrains("deformed_fibre", wall_xi);
}

std::vector<std::string> StrainMeasures::getSixteenSegmentTorsions(double wall_xi) {

	//The measure of change due to helix rotation between the walls
	//Setup segments
	const double base_start = 1.0;
	const double mid_start = 0.7;
	const double apex_start = 0.5;
	const double apex_end = 0.001;
	const double wall_epi = 1.0;
	if (wall_xi == 1.0)
		wall_xi = 0.99;
	std::vector<WallSegment> segments;
	//APLAX
	const double apexAngle = 0.0;
	//Base
	WallSegment seg1(3, 7); //Segment 2
	seg1.setXIA(apexAngle, 0.5 * (base_start + mid_start), wall_xi);
	seg1.setXIB(apexAngle, 0.5 * (base_start + mid_start), wall_epi);

	WallSegment seg2(5, 1); //Segment 5
	seg2.setXIA(apexAngle, 0.5 * (base_start + mid_start), wall_xi);
	seg2.setXIB(apexAngle, 0.5 * (base_start + mid_start), wall_epi);

	//Mid
	WallSegment seg3(7, 11); //Segment 8
	seg3.setXIA(apexAngle, 0.5 * (mid_start + apex_start), wall_xi);
	seg3.setXIB(apexAngle, 0.5 * (mid_start + apex_start), wall_epi);

	WallSegment seg4(5, 9); //Segment 11
	seg4.setXIA(apexAngle, 0.5 * (mid_start + apex_start), wall_xi);
	seg4.setXIB(apexAngle, 0.5 * (mid_start + apex_start), wall_epi);

	//Apex

	WallSegment seg5(12, 16); //Segment 13
	seg5.setXIA(0.8, 0.5 * (apex_start + apex_end), wall_xi);
	seg5.setXIB(0.8, 0.5 * (apex_start + apex_end), wall_epi);

	WallSegment seg6(9, 13); //Segment 15
	seg6.setXIA(0.1, 0.5 * (apex_start + apex_end), wall_xi);
	seg6.setXIB(0.1, 0.5 * (apex_start + apex_end), wall_epi);

	//TCH
	const double tchAngle = 2.5 / 3.0;
	//Base
	WallSegment tseg1(4, 8); //Segment 3
	tseg1.setXIA(tchAngle, 0.5 * (base_start + mid_start), wall_xi);
	tseg1.setXIB(tchAngle, 0.5 * (base_start + mid_start), wall_epi);

	WallSegment tseg2(6, 2); //Segment 6
	tseg2.setXIA(tchAngle, 0.5 * (base_start + mid_start), wall_xi);
	tseg2.setXIB(tchAngle, 0.5 * (base_start + mid_start), wall_epi);

	//Mid
	WallSegment tseg3(8, 12); //Segment 9
	tseg3.setXIA(tchAngle, 0.5 * (mid_start + apex_start), wall_xi);
	tseg3.setXIB(tchAngle, 0.5 * (mid_start + apex_start), wall_epi);

	WallSegment tseg4(6, 10); //Segment 12
	tseg4.setXIA(tchAngle, 0.5 * (mid_start + apex_start), wall_xi);
	tseg4.setXIB(tchAngle, 0.5 * (mid_start + apex_start), wall_epi);

	//Apex
	WallSegment tseg5(12, 16); //Segment 14
	tseg5.setXIA(tchAngle, 0.5 * (apex_start + apex_end), wall_xi);
	tseg5.setXIB(tchAngle, 0.5 * (apex_start + apex_end), wall_epi);

	WallSegment tseg6(10, 14); //Segment 16
	tseg6.setXIA(tchAngle, 0.5 * (apex_start + apex_end), wall_xi);
	tseg6.setXIB(tchAngle, 0.5 * (apex_start + apex_end), wall_epi);

	//FCH
	const double fchAngle = 1.0 / 3.0;
	//Base
	WallSegment fseg1(1, 5); //Segment 4
	fseg1.setXIA(fchAngle, 0.5 * (base_start + mid_start), wall_xi);
	fseg1.setXIB(fchAngle, 0.5 * (base_start + mid_start), wall_epi);

	WallSegment fseg2(7, 3); //Segment 1
	fseg2.setXIA(fchAngle, 0.5 * (base_start + mid_start), wall_xi);
	fseg2.setXIB(fchAngle, 0.5 * (base_start + mid_start), wall_epi);

	//Mid
	WallSegment fseg3(5, 9); //Segment 10
	fseg3.setXIA(fchAngle, 0.5 * (mid_start + apex_start), wall_xi);
	fseg3.setXIB(fchAngle, 0.5 * (mid_start + apex_start), wall_epi);

	WallSegment fseg4(7, 11); //Segment 7
	fseg4.setXIA(fchAngle, 0.5 * (mid_start + apex_start), wall_xi);
	fseg4.setXIB(fchAngle, 0.5 * (mid_start + apex_start), wall_epi);

	//Apex
	WallSegment fseg5(9, 13); //Segment 14+15
	fseg5.setXIA(fchAngle, apex_start, wall_xi);
	fseg5.setXIB(fchAngle, apex_end, wall_epi);

	WallSegment fseg6(11, 15); //Segment 13+16
	fseg6.setXIA(fchAngle, apex_start, wall_xi);
	fseg6.setXIB(fchAngle, apex_end, wall_epi);

	segments.clear();

	segments.push_back(fseg2); //1
	segments.push_back(seg1); //2
	segments.push_back(tseg1); //3
	segments.push_back(fseg1); //4
	segments.push_back(seg2); //5
	segments.push_back(tseg2); //6
	segments.push_back(fseg4); //7
	segments.push_back(seg3); //8
	segments.push_back(tseg3); //9
	segments.push_back(fseg3); //10
	segments.push_back(seg4); //11
	segments.push_back(tseg4); //12
	segments.push_back(seg5); //13
	segments.push_back(tseg5); //14
	segments.push_back(seg6); //15
	segments.push_back(tseg6); //16

	//Setup the active strain segments
	int aplaxStrainIds[] =
	{ 1, 7, 12, 14, 10, 4 };
	int tchStrainIds[] =
	{ 2, 8, 13, 15, 11, 5 };
	int fchStrainIds[] =
	{ 0, 6, 9, 3 };
	int activeStrainSegments[16];
	unsigned int totalActiveSegments = 0;
	memset(activeStrainSegments, 0, sizeof(int) * 16);
	if (activeViews[0])
	{
		for (int i = 0; i < 6; i++)
			activeStrainSegments[aplaxStrainIds[i]] = 1;
		totalActiveSegments += 6;
	}
	if (activeViews[1])
	{
		for (int i = 0; i < 6; i++)
			activeStrainSegments[tchStrainIds[i]] = 1;
		totalActiveSegments += 6;
	}
	if (activeViews[2])
	{
		for (int i = 0; i < 4; i++)
			activeStrainSegments[fchStrainIds[i]] = 1;
		totalActiveSegments += 4;
	}

	//Compute the strain for each segment over time
	unsigned int samples = heart.numberOfModelFrames_;
	double* length = new double[samples];
	double* nstrain = new double[heart.numberOfModelFrames_];
	double* avgstrain = new double[heart.numberOfModelFrames_];
	double temp_array[3], temp_array1[3];
	unsigned int numSegments = segments.size();

	memset(avgstrain, 0, heart.numberOfModelFrames_ * sizeof(double));
	std::vector<double>& frameTimes = heart.frameTimes;
	std::vector<std::string> strains;
	std::stringstream ss;
	//Setup the header
	for (unsigned int i = 0; i < numSegments; i++)
	{
		WallSegment& seg = segments[i];
		for (unsigned int dt = 0; dt < samples; dt++)
		{
			double time = frameTimes[dt];

			Cmiss_field_cache_set_time(fieldCache, time);
			temp_array1[0] = temp_array1[1] = temp_array1[2] = 0.0;
			//Since cmiss id starts at 1 subtract 1 from seg.elementid?
			Cmiss_field_cache_set_mesh_location(fieldCache, elements[seg.elementida - 1], 3, seg.xia);
			Cmiss_field_evaluate_real(coordianteField, fieldCache, 3, temp_array);
			Cmiss_field_cache_set_mesh_location(fieldCache, elements[seg.elementida - 1], 3, seg.xib);
			Cmiss_field_evaluate_real(coordianteField, fieldCache, 3, temp_array1);

			length[dt] = fabs(temp_array1[2]);
		}


		ss << " 0.0";
		avgstrain[0] = 0.0;
		for (unsigned int dt = 1; dt < samples - 1; dt++)
		{
			nstrain[dt] = length[dt] - length[0];

			ss << "," << nstrain[dt];
			avgstrain[dt] += nstrain[dt] * activeStrainSegments[i];
		}
		ss << "," << nstrain[0];
		avgstrain[heart.numberOfModelFrames_ - 1] += nstrain[0] * activeStrainSegments[i];
		strains.push_back(ss.str());
		//Clear the buffer
		ss.str("");
	}

	ss << avgstrain[0];
	for (unsigned int dt = 1; dt < heart.numberOfModelFrames_; dt++)
	{
		ss << "," << avgstrain[dt] / totalActiveSegments;
	}
	strains.push_back(ss.str());

	delete[] length;
	delete[] nstrain;
	delete[] avgstrain;
	return strains;
}

void StrainMeasures::setActiveViews(bool* views) {
	activeViews[0] = views[0];
	activeViews[1] = views[1];
	activeViews[2] = views[2];
	activeViews[3] = views[3];
}

void StrainMeasures::setLagrangian() {
	lagrangian = true;
}

void StrainMeasures::setNatural() {
	lagrangian = false;
}

bool StrainMeasures::isLagrangian() {
	return lagrangian;
}

void StrainMeasures::embedStrainOnElements(std::string fieldname, std::vector<std::string>& strain) {

	const unsigned int num_strains = heart.numberOfModelFrames_;
	std::vector<double>& frameTimes = heart.frameTimes;

	double times[1024];
	for (unsigned int j = 0; j < num_strains; j++)
	{
		times[j] = frameTimes[j];
	}
	Cmiss_time_sequence_id time_sequence = Cmiss_field_module_get_matching_time_sequence(field_module, num_strains, times);

	Cmiss_field_module_begin_change(field_module);

	//Create Node template
	Cmiss_nodeset_id nodeset = Cmiss_field_module_find_nodeset_by_name(field_module, "cmiss_nodes");
	Cmiss_node_template_id nodeTemplate = Cmiss_nodeset_create_node_template(nodeset);
	//Create the field
	Cmiss_field_id strainField = Cmiss_field_module_create_field(field_module, fieldname.c_str(), "finite number_of_components 1 component_names value");
	if (!strainField)
	{
		std::cout << "Unable to create field " << fieldname << std::endl;
		return;
	}

	Cmiss_node_template_define_field(nodeTemplate, strainField);
	Cmiss_node_template_define_time_sequence(nodeTemplate, strainField, time_sequence);

	//Create Element template
	Cmiss_mesh_id mesh = Cmiss_field_module_find_mesh_by_dimension(field_module, 3);
	Cmiss_element_template_id element_template = Cmiss_mesh_create_element_template(mesh);
	Cmiss_element_template_set_shape_type(element_template, CMISS_ELEMENT_SHAPE_CUBE);
	Cmiss_element_template_set_number_of_nodes(element_template, 1);
	Cmiss_element_basis_id constant_basis = Cmiss_field_module_create_element_basis(field_module, 3, CMISS_BASIS_FUNCTION_CONSTANT);
	const int local_node_index = 1;
	Cmiss_element_template_define_field_simple_nodal(element_template, strainField, -1, constant_basis, 1, &local_node_index);
	Cmiss_element_basis_destroy(&constant_basis);

	//Map strains to elements

	std::map<int, int> elemStrainMap; //Cmiss element no is mapped to strain array number
	//APLAX
	elemStrainMap[3] = elemStrainMap[10] = 1; //Array id base 0
	elemStrainMap[15] = elemStrainMap[22] = 18; //Average of strain ids 2 and 8
	elemStrainMap[27] = elemStrainMap[34] = 12;
	elemStrainMap[39] = elemStrainMap[46] = 17;
	elemStrainMap[40] = elemStrainMap[45] = 17;
	elemStrainMap[28] = elemStrainMap[33] = 10;
	elemStrainMap[16] = elemStrainMap[21] = 19; //Average of strain ids 11 and 5
	elemStrainMap[4] = elemStrainMap[9] = 4;

	//TCH
	elemStrainMap[11] = elemStrainMap[12] = 2; //Array id base 0
	elemStrainMap[23] = elemStrainMap[24] = 20; //Average of strain ids 3 and 9
	elemStrainMap[35] = elemStrainMap[36] = 8;
	elemStrainMap[47] = elemStrainMap[48] = 16;
	elemStrainMap[41] = elemStrainMap[42] = 16;
	elemStrainMap[17] = elemStrainMap[18] = 11;
	elemStrainMap[29] = elemStrainMap[30] = 21; //Average of strain ids 12 and 6
	elemStrainMap[5] = elemStrainMap[6] = 5;

	//FCH
	elemStrainMap[1] = elemStrainMap[2] = 3; //Array id base 0
	elemStrainMap[13] = elemStrainMap[14] = 22; //Average of 10 and 4
	elemStrainMap[25] = elemStrainMap[26] = 4;
	elemStrainMap[37] = elemStrainMap[38] = 23; //Average of strains 14 and 15
	elemStrainMap[43] = elemStrainMap[44] = 24; //Average of strains 16 and 13
	elemStrainMap[31] = elemStrainMap[32] = 6;
	elemStrainMap[19] = elemStrainMap[20] = 25; //Average of strains 7 and 1
	elemStrainMap[7] = elemStrainMap[8] = 0;

	std::vector<std::string> allStrain = strain;

	//Add the additional strains 18 - 23
	std::string s2 = strain[1];
	std::string s8 = strain[7];

	std::string s11 = strain[10];
	std::string s5 = strain[5];

	std::string s9 = strain[8];
	std::string s3 = strain[2];

	std::string s12 = strain[11];
	std::string s6 = strain[6];

	std::string s10 = strain[9];
	std::string s4 = strain[3];

	std::string s14 = strain[13];
	std::string s15 = strain[14];

	std::string s16 = strain[15];
	std::string s13 = strain[12];

	std::string s1 = strain[0];
	std::string s7 = strain[6];

	std::vector<std::string> compute;
	compute.push_back(s2);
	compute.push_back(s8);

	compute.push_back(s11);
	compute.push_back(s5);

	compute.push_back(s9);
	compute.push_back(s3);

	compute.push_back(s12);
	compute.push_back(s6);

	compute.push_back(s10);
	compute.push_back(s4);

	compute.push_back(s14);
	compute.push_back(s15);

	compute.push_back(s16);
	compute.push_back(s13);

	compute.push_back(s7);
	compute.push_back(s1);

	for (int cptc = 0; cptc < 16; cptc += 2)
	{
		std::vector<std::string> strain1;
		std::vector<std::string> strain2;
		std::ostringstream ss;
		boost::split(strain1, compute[cptc], boost::is_any_of(","));
		boost::split(strain2, compute[cptc + 1], boost::is_any_of(","));
		if(strain1.size()==num_strains&&strain2.size()==num_strains){
			for (unsigned int j = 0; j < num_strains - 1; j++)
			{
				double value1 = atof(strain1[j].c_str());
				double value2 = atof(strain2[j].c_str());
				ss << (value1 + value2) * 0.5 << ",";
			}
		}
		//Ensures the element vector mapping is consistent
		ss << "0.0";
		allStrain.push_back(ss.str());
	}

	for (unsigned int i = 0; i < num_elements; i++)
	{
		Cmiss_node_id temporaryNode = Cmiss_nodeset_create_node(nodeset, extraNodeStartID++, nodeTemplate);
		Cmiss_element_template_set_node(element_template, 1, temporaryNode); //Link node with the element
		Cmiss_element_merge(elements[i], element_template);

		Cmiss_field_cache_set_node(fieldCache, temporaryNode);
		std::vector<std::string> strains;
		boost::split(strains, allStrain[elemStrainMap[i + 1]], boost::is_any_of(","));
		if(strains.size()==num_strains){
			for (unsigned int j = 0; j < num_strains; j++)
			{
				double value = atof(strains[j].c_str());
				//double time = static_cast<double>(j) / num_strains;
				double time = frameTimes[j];
				Cmiss_field_cache_set_time(fieldCache, time);
				Cmiss_field_assign_real(strainField,fieldCache,1,&value);
			}
		}
		Cmiss_node_destroy(&temporaryNode);
	}

	Cmiss_field_module_end_change(field_module);
	Cmiss_field_destroy(&strainField);
	Cmiss_element_template_destroy(&element_template);
	Cmiss_node_template_destroy(&nodeTemplate);
	Cmiss_nodeset_destroy(&nodeset);
	//Calling the following leads to the error ERROR: DESTROY(FE_time_sequence).  Positive access_count
	//Cmiss_time_sequence_destroy(&time_sequence);
}

std::vector<std::string> StrainMeasures::getSpeckleStrains(bool transformed) {
	std::vector<std::string> strains(19); //Currently uses the sixteen segment model (segment numbers start from 0..15, 16 17 correspond to FCH apex segments, 18 is the avg strain)
	MarkerTypes views[3] =
	{ APLAX, TCH, FCH };

#ifdef printcoord
	std::cout << "Speckle 3D " << transformed << std::endl;
#endif
	std::vector<double> avgStrains(heart.numberOfModelFrames_, 0.0);
	int numViews = 0;
	std::vector<int> selectedSeries = inputReader.getStrainSelections();
	std::vector<int> selections(6,0);

	for (int mtype = 0; mtype < 3; mtype++)
	{
		std::vector<std::vector<Point3D> > allMarkers;
		std::vector<Point3D> frame0 = inputReader.getMarkers(0, views[mtype], transformed); //There is a difference in speckle strain values when data is transformed, due to scaling!!

//#define comparedTransformation
#ifdef comparedTransformation
		std::vector<Point3D> cframe0 = inputReader.getMarkers(0,views[mtype],transformed);
		std::vector< std::vector<Point3D> > callMarkers;
#endif
		if (frame0.size() > 0)
		{
			allMarkers.push_back(frame0);
#ifdef comparedTransformation
			callMarkers.push_back(cframe0);
#endif
			for (int frame = 1; frame < heart.numberOfModelFrames_; frame++)
			{
				allMarkers.push_back(inputReader.getMarkers(frame, views[mtype], transformed));
#ifdef comparedTransformation
				callMarkers.push_back(inputReader.getMarkers(frame,views[mtype],!transformed));
#endif
			}
			//Compute the strain lengths
			std::vector<std::vector<double> > distances;

			for (int frame = 0; frame < heart.numberOfModelFrames_; frame++)
			{
				std::vector<double> cLengths;
				std::vector<Point3D>& fValue = allMarkers[frame];
#ifdef printcoord
				std::cout<<frame;
#endif
				for (int i = 0; i < fValue.size() - 1; i++)
				{
					double dist = fValue[i].distance(fValue[i + 1]);
					cLengths.push_back(dist);
#ifdef printcoord
					std::cout<<"\t"<<fValue[i]<<"\t"<<fValue[i + 1]<<"\t"<<dist;
#endif
				}
#ifdef printcoord
				std::cout<<std::endl;
#endif
				//Compute the weighted strain lengths
				std::vector<double> sLengths;


				sLengths.push_back(cLengths[0] + (1 / 3.0 - 1 / 4.0) * 4 * cLengths[1]);
				sLengths.push_back((1.0 - (1 / 3.0 - 1 / 4.0)) * 4 * cLengths[1 ] + (2 / 3.0 - 1 / 2.0) * 4 * cLengths[2]);
				sLengths.push_back((1.0 - (2 / 3.0 - 1 / 2.0)) * 4 * cLengths[2] + cLengths[3]);
				sLengths.push_back(cLengths[4] + (1.0 - (2 / 3.0 - 1 / 2.0)) * 4 * cLengths[5]);
				sLengths.push_back((2 / 3.0 - 1 / 2.0) * 4 * cLengths[5] + (1.0 - (1 / 3.0 - 1 / 4.0)) * 4 * cLengths[6]);
				sLengths.push_back((1 / 3.0 - 1 / 4.0) * 4 * cLengths[6] + cLengths[7]);

				distances.push_back(sLengths);
			}

#ifdef comparedTransformation
			for(int frame = 0;frame<heart.numberOfModelFrames_;frame++)
			{
				std::vector<double> cLengths;
				std::vector<Point3D>& fValue = callMarkers[frame];
				std::vector<Point3D>& cfValue = allMarkers[frame];
				for(int i=0;i<fValue.size()-1;i++)
				{
					double dist = fValue[i].distance(fValue[i+1]);
					std::cout<<frame<<":"<<mtype<<" "<<i<<" ("<<fValue[i]<<") ("<<fValue[i+1]<<") "<<dist<<"\t ("<<cfValue[i]<<") ("<<cfValue[i+1]<<") "<<cfValue[i].distance(cfValue[i+1])<<std::endl;
					cLengths.push_back(dist);
				}
				//Compute the weighted strain lengths
				std::vector<double> sLengths;
				sLengths.push_back(cLengths[0] + cLengths[1] / 12.0);
				sLengths.push_back(11 * cLengths[1] / 12.0 + cLengths[2] / 6.0);
				sLengths.push_back(5 * cLengths[2] / 6.0 + cLengths[3]);
				sLengths.push_back(cLengths[4] + cLengths[5] / 12.0);
				sLengths.push_back(11 * cLengths[5] / 12.0 + cLengths[6] / 6.0);
				sLengths.push_back(5 * cLengths[6] / 6.0 + cLengths[7]);
			}

#endif

			std::vector<std::vector<double> > dstrains;
			std::vector<double>& initStrain = distances[0];
			for (int frame = 1; frame < heart.numberOfModelFrames_; frame++)
			{ //Compute Strains
				std::vector<double>& curStrainLengths = distances[frame];
				std::vector<double> curStrain;
				double c = 0.0;
				unsigned int ulimit = initStrain.size();
				for (int j = 0; j < ulimit; j++)
				{
					c = (curStrainLengths[j] - initStrain[j]) / initStrain[j];
					curStrain.push_back(c);
				}
				dstrains.push_back(curStrain);
			}

			std::vector<std::string> strainSeries;
			if (views[mtype] == APLAX)
			{
				selections[5] = selectedSeries[2 - 1];
				selections[4] = selectedSeries[8 - 1];
				selections[1] = selectedSeries[11 - 1];
				selections[0] = selectedSeries[5 - 1];
				selections[3] = selectedSeries[17 - 1];
				selections[2] = selectedSeries[18 - 1];
			}
			else if (views[mtype] == FCH)
			{
				selections[0] = selectedSeries[3 - 1];
				selections[1] = selectedSeries[9 - 1];
				selections[2] = selectedSeries[14 - 1];
				selections[3] = selectedSeries[16 - 1];
				selections[4] = selectedSeries[12 - 1];
				selections[5] = selectedSeries[6 - 1];
			}
			else if (views[mtype] == TCH)
			{
				selections[0] = selectedSeries[4 - 1];
				selections[1] = selectedSeries[10 - 1];
				selections[2] = selectedSeries[15 - 1];
				selections[3] = selectedSeries[13 - 1];
				selections[4] = selectedSeries[7 - 1];
				selections[5] = selectedSeries[1 - 1];
			}

			for (int j = 0; j < initStrain.size(); j++)
			{
				std::ostringstream ss;
				ss << 0.0; //For init step
				int denom = heart.numberOfModelFrames_ - 1;
				double maxStrain = dstrains[denom - 1][j];
				for (int i = 0; i < denom; i++)
				{ //Compute Strains
				  //Drift compensate
					double stc = dstrains[i][j] - (i + 1) * maxStrain / denom;
					ss << "," << stc;
					//Add to avg
					if(selections[j])
						avgStrains[i] += stc;
				}
				strainSeries.push_back(ss.str());
			}
			if (views[mtype] == APLAX)
			{
				strains[2 - 1] = "2;" + strainSeries[5];
				strains[8 - 1] = "8;" + strainSeries[4];
				strains[11 - 1] = "11;" + strainSeries[1];
				strains[5 - 1] = "5;" + strainSeries[0];
				strains[17 - 1] = "13p14;" + strainSeries[3];
				strains[18 - 1] = "15p16;" + strainSeries[2];
			}
			else if (views[mtype] == FCH)
			{
				strains[3 - 1] = "3;" + strainSeries[0];
				strains[9 - 1] = "9;" + strainSeries[1];
				strains[14 - 1] = "14;" + strainSeries[2];
				strains[16 - 1] = "16;" + strainSeries[3];
				strains[12 - 1] = "12;" + strainSeries[4];
				strains[6 - 1] = "6;" + strainSeries[5];
			}
			else if (views[mtype] == TCH)
			{
				strains[4 - 1] = "4;" + strainSeries[0];
				strains[10 - 1] = "10;" + strainSeries[1];
				strains[15 - 1] = "15;" + strainSeries[2];
				strains[13 - 1] = "13;" + strainSeries[3];
				strains[7 - 1] = "7;" + strainSeries[4];
				strains[1 - 1] = "1;" + strainSeries[5];
			}
			numViews += 6;
		}
	}
#ifdef printcoord
	std::cout << "Speckle 3D " << transformed << std::endl;
#endif

	double denom = 0;
	unsigned int maxSeries = selectedSeries.size();
	if(maxSeries>17)
		maxSeries = 17;
	for(int i=0;i<maxSeries;i++){
		denom += selectedSeries[i];
	}

	//Compute the average
	std::ostringstream ss;
	ss << "18;0.0";
	for (int i = 0; i < heart.numberOfModelFrames_ - 1; i++)
	{
		ss << "," << avgStrains[i] / denom;
	}
	strains[18] = ss.str();

	//compute speckle PGS
	{
		double max = fabs(avgStrains[0]);
		int idx = 0;
		for (int i = 1; i < heart.numberOfModelFrames_ - 1; i++)
		{
			if (fabs(avgStrains[i]) > max)
			{
				max = fabs(avgStrains[i]);
				idx = i;
			}
		}
		specklePGS = avgStrains[idx] / denom;
	}

	return strains;
}

std::string StrainMeasures::getLVVolume(double xi) {

	//Compute the LV circles
	std::vector<std::vector<Point3D> > circleXI;
	std::vector<std::vector<int> > circleElem;

	std::ostringstream lvVolume;
	//Note with respect to the model apex base is x-axis
	//choose y-z as xy
	//Compute the XI values for the circles
	// xi   x(1 inc),height(1 dec),y(=xi)
	/*  const int elemIds[][12] = { { 16, 32, 48, 13, 29, 45, 14, 30, 46, 15, 31, 47 }, { 12, 28, 44, 9, 25, 41, 10, 26, 42, 11, 27, 43 }, { 8, 24, 40, 5, 21, 37, 6, 22, 38, 7, 23, 39 },
	 { 4, 20, 36, 1, 17, 33, 2, 18, 34, 3, 19, 35 } };*/
	const int elemIds[][12] =
	{
	{ 14, 30, 46, 15, 31, 47, 16, 32, 48, 13, 29, 45 },
		{ 10, 26, 42, 11, 27, 13, 12, 28, 44, 9, 25, 41 },
		{ 6, 22, 38, 7, 23, 39, 8, 24, 40, 5, 21, 37 },
		{ 2, 18, 34, 3, 19, 35, 4, 20, 36, 1, 17, 33 } };
	double discretization = 0.05;

	for (int elemDepth = 0; elemDepth < 4; elemDepth++)
	{

		for (double depth = 1.0; depth >= discretization; depth -= discretization)
		{
			std::vector<Point3D> cpts;
			std::vector<int> cel;
			for (int eid = 0; eid < 12; eid++)
			{
				int elemID = elemIds[elemDepth][eid] - 1;
				for (double xs = 0.0; xs < 1.0; xs += discretization)
				{
					cpts.push_back(Point3D(xs, depth, xi));
					cel.push_back(elemID);
				}
			}
			circleXI.push_back(cpts);
			circleElem.push_back(cel);
		}
	}

	unsigned int numCircles = circleXI.size();
	double minVolume = -1.0; //Compute Ejection fraction
	double endVolume = 0.0;  //as using smallest and end volume
	std::vector<double>& frameTimes = heart.frameTimes;
	for (int frame = 0; frame < heart.numberOfModelFrames_; frame++)
	{
		double time = frameTimes[frame];
		std::vector<LVChamberCircle> circles;
		Cmiss_field_cache_set_time(fieldCache, time);
		//Compute the circles
		for (int i = 0; i < numCircles; i++)
		{
			std::vector<Point3D>& circlexi = circleXI[i];
			std::vector<int>& cel = circleElem[i];
			std::vector<Point3D> cpts;
			unsigned int npts = circlexi.size();
			double coord[3];
			for (int j = 0; j < npts; j++)
			{
				double* xcoord = circlexi[j].ToArray();
				Cmiss_field_cache_set_mesh_location(fieldCache, elements[cel[j]], 3, xcoord);
				Cmiss_field_evaluate_real(coordianteField, fieldCache, 3, coord);
				cpts.push_back(Point3D(coord[0], coord[1], coord[2]));
				delete[] xcoord;
			}
			circles.push_back(LVChamberCircle(cpts));
		}
		//Compute the volume for the existing circles
		double volume = 0.0;
		numCircles = circles.size();
		for (int i = 0; i < numCircles - 1; i++)
		{
			volume += circles[i].getVolume(circles[i + 1]);
		}
		if (minVolume < 0.0 || volume < minVolume)
		{
			minVolume = volume;
		}
		endVolume = volume;
		lvVolume << volume << ",";
	}

	if (xi < 0.001)
	{
		ejectionFraction = (endVolume - minVolume) / endVolume;
	}

	std::string lvstr = lvVolume.str();
	//Remove the last "," seperator
	return lvstr.substr(0, lvstr.length() - 1);
}

double StrainMeasures::getModelPgs() {
	return modelPGS;
}

double StrainMeasures::getSpecklePgs() {
	return specklePGS;
}

std::string StrainMeasures::getCircumferentialStrain(double wall_xi, MarkerTypes type) {
	std::ostringstream strain;
	//Note with respect to the model apex base is x-axis
	//choose y-z as xy
	//Compute the XI values for the circles
	// xi   x(1 inc),height(1 dec),y(=xi)
	/*  const int elemIds[][12] = { { 16, 32, 48, 13, 29, 45, 14, 30, 46, 15, 31, 47 }, { 12, 28, 44, 9, 25, 41, 10, 26, 42, 11, 27, 43 }, { 8, 24, 40, 5, 21, 37, 6, 22, 38, 7, 23, 39 },
	 { 4, 20, 36, 1, 17, 33, 2, 18, 34, 3, 19, 35 } };*/
	const int elemIds[][12] =
	{
	{ 14, 30, 46, 15, 31, 47, 16, 32, 48, 13, 29, 45 },
		{ 10, 26, 42, 11, 27, 13, 12, 28, 44, 9, 25, 41 },
		{ 6, 22, 38, 7, 23, 39, 8, 24, 40, 5, 21, 37 },
		{ 2, 18, 34, 3, 19, 35, 4, 20, 36, 1, 17, 33 } };
	double discretization = 0.02;

	int elemDepth = 2;
	double depth = 0.0;
	switch (type)
	{
	case SAXAPEX:
	{
		elemDepth = 2;
		depth = 0.5;
		break;
	}
	case SAXMID:
	{
		elemDepth = 1;
		depth = 0.5;
		break;
	}
	case SAXBASE:
	{
		elemDepth = 0;
		depth = 0.01;
		break;
	}
	}

	std::vector<double*> cpts;
	std::vector<int> cel;
	for (int eid = 0; eid < 12; eid++)
	{
		int elemID = elemIds[elemDepth][eid] - 1;
		for (double xs = 0.0; xs < 1.0; xs += discretization)
		{
			Point3D xiCoord(xs, depth, wall_xi);
			cpts.push_back(xiCoord.ToArray());
			cel.push_back(elemID);
		}
	}
	unsigned int npts = cpts.size();
	std::vector<double> circums;
	std::vector<double>& frameTimes = heart.frameTimes;
	for (int frame = 0; frame < heart.numberOfModelFrames_; frame++)
	{
		double time = frameTimes[frame];
		Cmiss_field_cache_set_time(fieldCache, time);
		//Compute the circle coordinates
		std::vector<Point3D> pts;
		double coord[3];
		for (int j = 0; j < npts; j++)
		{
			Cmiss_field_cache_set_mesh_location(fieldCache, elements[cel[j]], 3, cpts[j]);
			Cmiss_field_evaluate_real(coordianteField, fieldCache, 3, coord);
			coord[1] = 0.0;
			pts.push_back(Point3D(coord[0], coord[1], coord[2]));
		}
		LVChamberCircle circle(pts);

		circums.push_back(circle.getCircumference());
	}
	//Free mem
	for (int j = 0; j < npts; j++)
	{
		delete[] cpts[j];
	}

	double initValue = circums[0];

	for (int frame = 0; frame < heart.numberOfModelFrames_; frame++)
	{
		strain << (circums[frame] - initValue) / initValue << ",";
	}

	std::string lvstr = strain.str();
	//Remove the last "," seperator
	return lvstr.substr(0, lvstr.length() - 1);
}

std::string StrainMeasures::getSpeckleCircumferentialStrain(MarkerTypes type) {
	std::ostringstream strain;
	//Use the same point topology as that used for measuring model circumferential strain
	//Check if the view exists
	{
		std::vector<Point3D> saxMarkers = inputReader.getMarkers(0, type, true);
		if (saxMarkers.size() < 1)
			return "";
	}

	const int elemIds[][12] =
	{
	{ 14, 30, 46, 15, 31, 47, 16, 32, 48, 13, 29, 45 },
		{ 10, 26, 42, 11, 27, 13, 12, 28, 44, 9, 25, 41 },
		{ 6, 22, 38, 7, 23, 39, 8, 24, 40, 5, 21, 37 },
		{ 2, 18, 34, 3, 19, 35, 4, 20, 36, 1, 17, 33 } };
	double discretization = 0.02;

	int elemDepth = 2;
	double depth = 0.0;
	switch (type)
	{
	case SAXAPEX:
	{
		elemDepth = 2;
		depth = 0.5;
		break;
	}
	case SAXMID:
	{
		elemDepth = 1;
		depth = 0.5;
		break;
	}
	case SAXBASE:
	{
		elemDepth = 0;
		depth = 0.01;
		break;
	}
	}

	std::vector<double*> cpts;
	std::vector<int> cel;
	for (int eid = 0; eid < 12; eid++)
	{
		int elemID = elemIds[elemDepth][eid] - 1;
		for (double xs = 0.0; xs < 1.0; xs += discretization)
		{
			Point3D xiCoord(xs, depth, 0.0);
			cpts.push_back(xiCoord.ToArray());
			cel.push_back(elemID);
		}
	}
	unsigned int npts = cpts.size();
	std::vector<double> circums;
	std::vector<double>& frameTimes = heart.frameTimes;
	for (int frame = 0; frame < heart.numberOfModelFrames_; frame++)
	{
		//Create the spline
		std::vector<Point3D> saxMarkers = inputReader.getMarkers(frame, SAXBASE, true);
		unsigned int numSaxMarkers = saxMarkers.size();
		Point3D saxCentroid(0, 0, 0);
		for (int i = 0; i < numSaxMarkers; i++)
		{
			saxCentroid += saxMarkers[i];
		}
		saxCentroid /= numSaxMarkers;

		double* rad = new double[numSaxMarkers];
		double* theta = new double[numSaxMarkers];

		for (int i = 0; i < numSaxMarkers; i++)
		{	//Since the first and last markers point to the same location
			Vector3D trans = saxMarkers[i] - saxCentroid;
			rad[i] = trans.Length();
			theta[i] = atan2(trans.z, trans.x); //Since markers have been transformed xz is the rotation axis
		}

		alglib::real_1d_array radius;
		radius.setcontent(numSaxMarkers, rad);
		alglib::real_1d_array index;
		index.setcontent(numSaxMarkers, theta);
		alglib::spline1dinterpolant s;

		// build spline
		try
		{
			alglib::spline1dbuildcubic(index, radius, s);
		} catch (alglib::ap_error& et)
		{
			std::cout << et.msg << std::endl;
			std::cout << radius.tostring(4) << "\t" << index.tostring(4) << std::endl;
			throw et;
		}

		double time = frameTimes[frame];

		//Use the mesh based topology to order the points

		Cmiss_field_cache_set_time(fieldCache, time);
		//Compute the circle coordinates
		std::vector<Point3D> pts;
		Point3D centroid(0, 0, 0);
		double coord[3];
		for (int j = 0; j < npts; j++)
		{
			Cmiss_field_cache_set_mesh_location(fieldCache, elements[cel[j]], 3, cpts[j]);
			Cmiss_field_evaluate_real(coordianteField, fieldCache, 3, coord);
			coord[1] = 0.0;
			Point3D np(coord[0], coord[1], coord[2]);
			pts.push_back(np);
			centroid = centroid + np;
		}
		std::vector<Point3D> cpts;
		centroid = centroid * (-1.0 / npts);
		for (int j = 0; j < npts; j++)
		{
			Point3D np = pts[j] + centroid;
			double thetax = atan2(np.z, np.x);
			double rd1 = alglib::spline1dcalc(s, thetax);
			Point3D npx(rd1 * cos(thetax), 0, rd1 * sin(thetax));
			cpts.push_back(npx);
		}
		LVChamberCircle circle(cpts);
		circums.push_back(circle.getCircumference());
		delete[] theta;
		delete[] rad;

	}
	//Free mem
	for (int j = 0; j < npts; j++)
	{
		delete[] cpts[j];
	}

	double initValue = circums[0];

	for (int frame = 0; frame < heart.numberOfModelFrames_; frame++)
	{
		strain << (circums[frame] - initValue) / initValue << ",";
	}

	std::string lvstr = strain.str();
	//Remove the last "," seperator
	return lvstr.substr(0, lvstr.length() - 1);
}

std::vector<std::string> StrainMeasures::getRadialStrains(double wall_xi, MarkerTypes type) {

	std::vector<std::string> strains(18, "");
	std::vector<double> avgStrains(heart.numberOfModelFrames_, 0.0);
	std::ostringstream strain;
	//Note with respect to the model apex base is x-axis
	//choose y-z as xy
	//Compute the XI values for the circles

	const int allelemIds[][6][3] =
	{
	{
	{ 47, 0, 0 },
		{ 46, 15, 31 },
		{ 14, 30, 46 },
		{ 45, 0, 0 },
		{ 48, 13, 29 },
		{ 16, 32, 48 } },
		{
		{ 43, 0, 0 },
			{ 42, 11, 27 },
			{ 14, 30, 46 },
			{ 41, 0, 0 },
			{ 44, 9, 25 },
			{ 12, 28, 44 } },
		{
		{ 39, 0, 0 },
			{ 38, 7, 23 },
			{ 14, 30, 46 },
			{ 37, 0, 0 },
			{ 40, 5, 21 },
			{ 8, 24, 40 } } };

	const double elemStarts[][3] =
	{
	{ 0, 0, 0 },
		{ 0.5, 0, 0 },
		{ 0.0, 0, 0 },
		{ 0, 0, 0 },
		{ 0.5, 0, 0 },
		{ 0, 0, 0 } };

	const double elemEnds[][3] =
	{
	{ 1, 1, 1 },
		{ 1, 1, 1 },
		{ 1, 1, 0.5 },
		{ 1, 1, 1 },
		{ 1, 1, 1 },
		{ 1, 1, 0.5 } };

	double discretization = 0.02;

	int typedepth = 0;
	if (type == SAXAPEX)
		typedepth = 2;
	else if (type == SAXMID)
		typedepth = 1;

	double depth = 0.0; //Get the central cross section

	std::vector<std::vector<double*> > eCPTS;
	std::vector<std::vector<int> > eELEM;

	for (int elemDepth = 0; elemDepth < 6; elemDepth++)
	{
		std::vector<double*> cpts;
		std::vector<int> cel;
		for (int eid = 0; eid < 3; eid++)
		{
			if (allelemIds[typedepth][elemDepth][eid] > 0)
			{
				int elemID = allelemIds[typedepth][elemDepth][eid] - 1;

				for (double xs = elemStarts[elemDepth][eid]; xs < elemEnds[elemDepth][eid]; xs += discretization)
				{
					Point3D xiCoord(xs, depth, wall_xi);
					cpts.push_back(xiCoord.ToArray());
					cel.push_back(elemID);
				}
			}
		}
		eCPTS.push_back(cpts);
		eELEM.push_back(cel);
	}
	unsigned int nStrains = eCPTS.size();
	std::vector<double>& frameTimes = heart.frameTimes;
	for (int ns = 0; ns < nStrains; ns++)
	{
		std::vector<double*>& cpts = eCPTS[ns];
		std::vector<int>& cel = eELEM[ns];
		unsigned int npts = cpts.size();
		std::vector<double> circums;
		for (int frame = 0; frame < heart.numberOfModelFrames_; frame++)
		{

			double time = frameTimes[frame];
			Cmiss_field_cache_set_time(fieldCache, time);
			//Compute the circle coordinates
			std::vector<Point3D> pts;
			double coord[3];
			for (int j = 0; j < npts; j++)
			{
				Cmiss_field_cache_set_mesh_location(fieldCache, elements[cel[j]], 3, cpts[j]);
				Cmiss_field_evaluate_real(coordianteField, fieldCache, 3, coord);
				coord[1] = 0.0;
				pts.push_back(Point3D(coord[0], coord[1], coord[2]));
			}
			double distance = 0;
			for (int j = 1; j < npts; j++)
			{
				distance += pts[j - 1].distance(pts[j]);
			}
			circums.push_back(distance);
		}
		//Free mem
		for (int j = 0; j < npts; j++)
		{
			delete[] cpts[j];
		}

		double initValue = circums[0];

		strain.str("");
		for (int frame = 0; frame < heart.numberOfModelFrames_; frame++)
		{
			double stv = (circums[frame] - initValue) / initValue;
			strain << stv << ",";
			avgStrains[frame] += stv;
		}

		std::string lvstr = strain.str();
		strains[ns] = (lvstr.substr(0, lvstr.length() - 1));
	}

	//Push the average strain
	strain.str("");
	for (int frame = 0; frame < heart.numberOfModelFrames_; frame++)
	{
		strain << avgStrains[frame] / heart.numberOfModelFrames_ << ",";
	}

	std::string lvstr = strain.str();
	strains[17] = (lvstr.substr(0, lvstr.length() - 1)); //Location of average strain

	return strains;
}

std::vector<std::string> StrainMeasures::getRadialSpeckleStrains(MarkerTypes type) {
	std::vector<std::string> strains(18, "");
	//Check if the view exists
	{
		std::vector<Point3D> saxMarkers = inputReader.getMarkers(0, type, true);
		if (saxMarkers.size() < 1)
			return strains;
	}
	std::vector<double> avgStrains(heart.numberOfModelFrames_, 0.0);
	std::ostringstream strain;
	//Note with respect to the model apex base is x-axis
	//choose y-z as xy
	//Compute the XI values for the circles

	const int allelemIds[][6][3] =
	{
	{
	{ 47, 0, 0 },
		{ 46, 15, 31 },
		{ 14, 30, 46 },
		{ 45, 0, 0 },
		{ 48, 13, 29 },
		{ 16, 32, 48 } },
		{
		{ 43, 0, 0 },
			{ 42, 11, 27 },
			{ 14, 30, 46 },
			{ 41, 0, 0 },
			{ 44, 9, 25 },
			{ 12, 28, 44 } },
		{
		{ 39, 0, 0 },
			{ 38, 7, 23 },
			{ 14, 30, 46 },
			{ 37, 0, 0 },
			{ 40, 5, 21 },
			{ 8, 24, 40 } } };

	const double elemStarts[][3] =
	{
	{ 0, 0, 0 },
		{ 0.5, 0, 0 },
		{ 0.0, 0, 0 },
		{ 0, 0, 0 },
		{ 0.5, 0, 0 },
		{ 0, 0, 0 } };

	const double elemEnds[][3] =
	{
	{ 1, 1, 1 },
		{ 1, 1, 1 },
		{ 1, 1, 0.5 },
		{ 1, 1, 1 },
		{ 1, 1, 1 },
		{ 1, 1, 0.5 } };

	int typedepth = 0;
	if (type == SAXAPEX)
		typedepth = 2;
	else if (type == SAXMID)
		typedepth = 1;

	double discretization = 0.02;

	double depth = 0.0; //Get the central cross section

	std::vector<std::vector<double*> > eCPTS;
	std::vector<std::vector<int> > eELEM;

	for (int elemDepth = 0; elemDepth < 6; elemDepth++)
	{
		std::vector<double*> cpts;
		std::vector<int> cel;
		for (int eid = 0; eid < 3; eid++)
		{
			if (allelemIds[typedepth][elemDepth][eid] > 0)
			{
				int elemID = allelemIds[typedepth][elemDepth][eid] - 1;
				for (double xs = elemStarts[elemDepth][eid]; xs < elemEnds[elemDepth][eid]; xs += discretization)
				{
					Point3D xiCoord(xs, depth, 0.0);
					cpts.push_back(xiCoord.ToArray());
					cel.push_back(elemID);
				}
			}
		}
		eCPTS.push_back(cpts);
		eELEM.push_back(cel);
	}
	unsigned int nStrains = eCPTS.size();
	std::vector<double>& frameTimes = heart.frameTimes;
	for (int ns = 0; ns < nStrains; ns++)
	{
		std::vector<double*>& cpts = eCPTS[ns];
		std::vector<int>& cel = eELEM[ns];
		unsigned int npts = cpts.size();
		std::vector<double> circums;
		for (int frame = 0; frame < heart.numberOfModelFrames_; frame++)
		{
			//Compute the spline
			std::vector<Point3D> saxMarkers = inputReader.getMarkers(frame, type, true);
			unsigned int numSaxMarkers = saxMarkers.size();
			Point3D saxCentroid(0, 0, 0);
			for (int i = 0; i < numSaxMarkers; i++)
			{
				saxCentroid += saxMarkers[i];
			}
			saxCentroid /= numSaxMarkers;

			double* rad = new double[numSaxMarkers];
			double* theta = new double[numSaxMarkers];

			for (int i = 0; i < numSaxMarkers; i++)
			{	//Since the first and last markers point to the same location
				Vector3D trans = saxMarkers[i] - saxCentroid;
				rad[i] = trans.Length();
				theta[i] = atan2(trans.z, trans.x); //Since markers have been transformed xz is the rotation axis
			}

			alglib::real_1d_array radius;
			radius.setcontent(numSaxMarkers, rad);
			alglib::real_1d_array index;
			index.setcontent(numSaxMarkers, theta);
			alglib::spline1dinterpolant s;

			// build spline
			try
			{
				alglib::spline1dbuildcubic(index, radius, s);
			} catch (alglib::ap_error& et)
			{
				std::cout << et.msg << std::endl;
				std::cout << radius.tostring(4) << "\t" << index.tostring(4) << std::endl;
				throw et;
			}

			double time = frameTimes[frame];
			//Done this way to match the data computed for the model
			Cmiss_field_cache_set_time(fieldCache, time);
			//Compute the circle coordinates
			std::vector<Point3D> pts;
			double coord[3];
			for (int j = 0; j < npts; j++)
			{
				Cmiss_field_cache_set_mesh_location(fieldCache, elements[cel[j]], 3, cpts[j]);
				Cmiss_field_evaluate_real(coordianteField, fieldCache, 3, coord);
				Point3D np(coord[0], coord[1], coord[2]);
				double thetax = atan2(np.z, np.x);
				double rd1 = alglib::spline1dcalc(s, thetax);
				Point3D npx(rd1 * cos(thetax), 0, rd1 * sin(thetax));
				pts.push_back(npx);
			}

			double distance = 0;
			for (int j = 1; j < npts; j++)
			{
				distance += pts[j - 1].distance(pts[j]);
			}
			circums.push_back(distance);

			delete[] rad;
			delete[] theta;
		}
		//Free mem
		for (int j = 0; j < npts; j++)
		{
			delete[] cpts[j];
		}

		double initValue = circums[0];

		strain.str("");
		for (int frame = 0; frame < heart.numberOfModelFrames_; frame++)
		{
			double stv = (circums[frame] - initValue) / initValue;
			strain << stv << ",";
			avgStrains[frame] += stv;
		}

		std::string lvstr = strain.str();
		strains[ns] = (lvstr.substr(0, lvstr.length() - 1));
	}

	//Push the average strain
	strain.str("");
	for (int frame = 0; frame < heart.numberOfModelFrames_; frame++)
	{
		strain << avgStrains[frame] / heart.numberOfModelFrames_ << ",";
	}

	std::string lvstr = strain.str();
	strains[17] = (lvstr.substr(0, lvstr.length() - 1)); //Location of average strain

	return strains;
}

std::vector<std::string> StrainMeasures::getSixteenSegmentTorsionFreeStrains(double wall_xi) {
	if(heart.hasSAXApex||heart.hasSAXMid||heart.hasSAXBase)
		return getSixteenSegmentStrains("torsionfree_coordinates", wall_xi);
	else
		return std::vector<std::string>(0);
}

double StrainMeasures::getEjectionFraction() {
	return ejectionFraction;
}

std::vector<int> StrainMeasures::getStrainSelections() {
	return inputReader.getStrainSelections();
}
