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

StrainMeasures::StrainMeasures(std::vector<std::string> mesh) :
		mySegments(NULL) {
	num_level_sets = 10;
	num_elements = 48;
	num_nodes = 98;
	elements.resize(num_elements);
	x_discret = 25;
	y_discret = 25;
	lagrangian = true;
	modelPGS = 0.0;
	computeModelPGS = false;
	ejectionFraction = 0.0;
	extraNodeStartID = 100000;

	LVMyocardialVolume* lvm = new LVMyocardialVolume(mesh);
	volumes = lvm->getMyocardialVolumes();
	delete lvm;

	//Read the mesh
	context_ = Cmiss_context_create("strainmeasure");
	/**< Handle to the context */
	Cmiss_region_id root_region = Cmiss_context_get_default_region(context_);

	//This required for using the cmiss_context_execute_command
	Cmiss_context_enable_user_interface(context_, NULL);

	//Load the finite element mesh of the heart

	std::string region_name = "heart";
	Cmiss_region_id heart_region = Cmiss_region_create_child(root_region, region_name.c_str());
	//Get the field module of the heart region
	field_module = Cmiss_region_get_field_module(heart_region);

	fieldCache = Cmiss_field_module_create_cache(field_module);
	numberOfModelFrames_ = mesh.size();
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

	{ 	//Define the necessary fields
		std::string referenceMesh = mesh[0];
		//Change coordinate name to reference_coordinates
		boost::replace_all(referenceMesh, "1) coordinates", "1) reference_coordinates");
		//Load the reference coordinates mesh
		Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
		Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
		Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, referenceMesh.c_str(), referenceMesh.length());

		Cmiss_region_read(root_region, stream_information);
		Cmiss_stream_resource_destroy(&stream_resource);
		Cmiss_stream_information_destroy(&stream_information);
	}
	coordianteField = Cmiss_field_module_find_field_by_name(field_module, "reference_coordinates");
	if (!coordianteField) {
		std::cout << "reference_coordinates field not found " << std::endl;
	}

	Cmiss_region_destroy(&heart_region);
	//#Calculate the strains
	Cmiss_field_module_define_field(field_module, "F", "gradient coordinate reference_coordinates field coordinates");
	Cmiss_field_module_define_field(field_module, "F_transpose", "transpose source_number_of_rows 3 field F");
	Cmiss_field_module_define_field(field_module, "C", "matrix_multiply number_of_rows 3 fields F_transpose F");
	Cmiss_field_module_define_field(field_module, "principal_strains", "eigenvalues field C");

	Cmiss_field_destroy(&coordianteField);
	//Assign the coordinates field to the handle for downstream use
	coordianteField = Cmiss_field_module_find_field_by_name(field_module, "coordinates");
	//Define the fibre field
	{
		//Load the reference fibre data and reference coordinates
		{
			if (Cmiss_field_module_find_field_by_name(field_module, "fibres") == NULL) {
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
	//Determine the myocardial volume

	//Get the element and node handles
	Cmiss_mesh_id cmiss_mesh = Cmiss_field_module_find_mesh_by_name(field_module, "cmiss_mesh_3d");

	Cmiss_element_iterator_id elementIterator = Cmiss_mesh_create_element_iterator(cmiss_mesh);
	Cmiss_element_id element = Cmiss_element_iterator_next(elementIterator);

	while (element != NULL) {
		int elementId = Cmiss_element_get_identifier(element) - 1; //Cmiss numbering starts at 1
		elements[elementId] = element;
		element = Cmiss_element_iterator_next(elementIterator);
	}
	Cmiss_element_iterator_destroy(&elementIterator);
	Cmiss_mesh_destroy(&cmiss_mesh);

	Cmiss_region_destroy(&root_region);
}

StrainMeasures::~StrainMeasures() {

	for (unsigned int i = 0; i < num_elements; i++) {
		Cmiss_element_destroy(&(elements[i]));
	}
	Cmiss_field_module_destroy(&field_module);
	Cmiss_field_cache_destroy(&fieldCache);
	Cmiss_field_destroy(&coordianteField);
	Cmiss_context_destroy(&context_);
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
//#define printcoord
#ifdef printcoord
#include "MeshTopology.h"

	std::vector<Cmiss_node_id> myNodes(100);
	Cmiss_nodeset_id nodeset = Cmiss_field_module_find_nodeset_by_name(field_module, "cmiss_nodes");
	Cmiss_node_iterator_id nodeIterator = Cmiss_nodeset_create_node_iterator(nodeset);
	Cmiss_node_id node = Cmiss_node_iterator_next(nodeIterator);
	if (node != 0) {
		double temp_array[3];

		while (node) {
			int node_id = Cmiss_node_get_identifier(node);
			myNodes[node_id - 1] = node;
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
	double denomj = (numberOfModelFrames_ - 1.0);
	for (int i = 0; i < numberOfModelFrames_; i++) {
		std::vector<double> cLengths;
		double time = ((double) i) / denomj;
		Cmiss_field_cache_set_time(fieldCache, time);
#ifdef printcoord
		std::cout<<"Frame "<<i<<"\t"<<time<<"\t"<<mySegments->at(0).xia[2]<<std::endl;
#endif
		for (int j = 0; j < numSegments; j++) {
			WallSegment& seg = mySegments->at(j);
			//The the lengths
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
			{
				int nodeCtr = (j / 8) * 9 + j % 8;
				Cmiss_field_cache_set_node(fieldCache, myNodes[Nodes[nodeCtr]]);
				temp_array1[0] = temp_array1[1] = temp_array1[2] = 0.0;
				Cmiss_field_evaluate_real(field, fieldCache, 3, temp_array1);
				Point3D p3(temp_array1);
				Cmiss_field_cache_set_node(fieldCache, myNodes[Nodes[nodeCtr + 1]]);
				temp_array1[0] = temp_array1[1] = temp_array1[2] = 0.0;
				Cmiss_field_evaluate_real(field, fieldCache, 3, temp_array1);
				Point3D p4(temp_array1);
				std::cout << p1 << "\t" << p2 << " = " << dist << "\t:\t Value at " << Nodes[nodeCtr] + 1 << "\t" << p3 << "\t" << p1.distance(p3) << "\t" << Nodes[nodeCtr + 1] + 1
						<< "\t" << p4 << "\t" << p2.distance(p4) << "\t distance \t " << p3.distance(p4) << std::endl;
			}
#endif
			cLengths.push_back(dist);
		}

		for (int segc = 0; segc < numSegments / 8; segc++) {
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
	std::vector<double> avgStrains(numberOfModelFrames_, 0.0);

	for (int segc = 0; segc < numSegments / 8; segc++) {
		std::vector<std::vector<double> > dstrains;
		std::vector<std::vector<double> > distances;
		if (segc == 0)
			distances = aplaxLengths;
		else if (segc == 1)
			distances = tchLengths;
		else if (segc == 2)
			distances = fchLengths;

		std::vector<double>& initStrain = distances[0];
		for (int frame = 1; frame < numberOfModelFrames_; frame++) { //Compute Strains
			std::vector<double>& curStrainLengths = distances[frame];
			std::vector<double> curStrain;
			double c = 0;
			unsigned int ulimit = initStrain.size();
			for (int j = 0; j < ulimit; j++) {
				c = 100.0 * (curStrainLengths[j] - initStrain[j]) / initStrain[j];
				curStrain.push_back(c);
			}
			dstrains.push_back(curStrain);
		}

		std::vector<std::string> strainSeries;

		for (int j = 0; j < initStrain.size(); j++) {
			std::ostringstream ss;
			ss << 0.0; //For init step

			int denom = numberOfModelFrames_ - 1;
			double maxStrain = dstrains[denom - 1][j];
			//Note that frame goes from 1 to heart.numberOfModelFrames_ when computing strain
			//so shift down by 1
			for (int i = 0; i < denom; i++) { //Compute Strains
											  //Drift compensate
				double stc = dstrains[i][j] - (i + 1) * maxStrain / denom;
				ss << "," << stc;
				avgStrains[i] += stc;
			}
			strainSeries.push_back(ss.str());
		}
		if (segc == 0) {
			strains[2 - 1] = strainSeries[5];
			strains[8 - 1] = strainSeries[4];
			strains[17 - 1] = strainSeries[3];
			strains[18 - 1] = strainSeries[2];
			strains[11 - 1] = strainSeries[1];
			strains[5 - 1] = strainSeries[0];
		} else if (segc == 2) {
			strains[3 - 1] = strainSeries[0];
			strains[9 - 1] = strainSeries[1];
			strains[14 - 1] = strainSeries[2];
			strains[16 - 1] = strainSeries[3];
			strains[12 - 1] = strainSeries[4];
			strains[6 - 1] = strainSeries[5];
		} else if (segc == 1) {
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
	double denom = (numSegments / 8) * 6;

	std::ostringstream ss;
	ss << 0.0; //For init step
	for (int i = 0; i < numberOfModelFrames_ - 1; i++) { //Compute the Average
		ss << "," << avgStrains[i] / denom;
	}
	strains[18] = ss.str();

	//Check if model PGS should be calculated
	if (computeModelPGS) {
		double max = fabs(avgStrains[0]);
		int idx = 0;
		for (int i = 1; i < numberOfModelFrames_ - 1; i++) {
			if (fabs(avgStrains[i]) > max) {
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
	if (mind == 0.0) {
		nan_count++;
	} else {
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
	for (unsigned int i = 0; i <= xdiscret; i++) {
		pointsOfInterest_x[i] = new double[ydiscret + 1];
		pointsOfInterest_y[i] = new double[ydiscret + 1];
	}
	double delx = 1.0 / xdiscret;
	double dely = 1.0 / ydiscret;

	for (unsigned int xd = 0; xd <= xdiscret; xd++) {
		double xinc = xd * delx;
		if (xinc == 0.0)
			xinc = 0.001;
		if (xinc == 1.0)
			xinc = 0.99;
		for (unsigned int yd = 0; yd <= ydiscret; yd++) {
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
	for (unsigned int ls = 0; ls < cnt; ls++) {
		coordSets->clear();
		deformSets->clear();
		for (unsigned int mt = 0; mt < numberOfModelFrames_; mt++) {
			double time = ((double) mt) / (numberOfModelFrames_ - 1.0);
			LevelSet* myCoordLevelSet = new LevelSet(num_elements, xdiscret + 1, ydiscret + 1);
			LevelSet* myDeformLevelSet = new LevelSet(num_elements, xdiscret + 1, ydiscret + 1, 1);

			Cmiss_field_cache_set_time(fieldCache, time);
			nan_count = 0;
			for (unsigned int xd = 0; xd <= xdiscret; xd++) {
				for (unsigned int yd = 0; yd <= ydiscret; yd++) {
					for (unsigned int elementId = 0; elementId < num_elements; elementId++) {
						coordinates[0] = pointsOfInterest_x[xd][yd];
						coordinates[1] = pointsOfInterest_y[xd][yd];
						coordinates[2] = wall_xi[ls];
						//std::cout<<coordinates[0]<<" "<<coordinates[1]<<" "<<coordinates[2]<<std::endl;
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
		for (unsigned int i = 0; i < numpoints; i++) {
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

	for (unsigned int i = 0; i <= xdiscret; i++) {
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
	if (num > 0) {
		//Print time header
		file << "Time";
		for (unsigned int i = 0; i < numberOfModelFrames_; i++) {
			double time = ((double) i) / (numberOfModelFrames_ - 1.0);
			file << "\t" << time;
		}
		file << std::endl;
		for (unsigned int i = 0; i < num; i++) {
			std::string sample = strains[i];
			std::vector<std::string> str1;
			boost::split(str1, sample, boost::is_any_of(","));
			file << "Segment:" << i;
			for (unsigned int j = 0; j < str1.size(); j++) {
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
	if (wall_xi == 0.0) {
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
	int aplaxStrainIds[] = { 1, 7, 12, 14, 10, 4 };
	int tchStrainIds[] = { 2, 8, 13, 15, 11, 5 };
	int fchStrainIds[] = { 0, 6, 9, 3 };
	int activeStrainSegments[16];
	unsigned int totalActiveSegments = 0;
	memset(activeStrainSegments, 0, sizeof(int) * 16);
	for (int i = 0; i < 6; i++)
		activeStrainSegments[aplaxStrainIds[i]] = 1;
	totalActiveSegments += 6;
	for (int i = 0; i < 6; i++)
		activeStrainSegments[tchStrainIds[i]] = 1;
	totalActiveSegments += 6;
	for (int i = 0; i < 4; i++)
		activeStrainSegments[fchStrainIds[i]] = 1;
	totalActiveSegments += 4;

	//Compute the strain for each segment over time
	unsigned int samples = numberOfModelFrames_;
	double* length = new double[samples];
	double* nstrain = new double[numberOfModelFrames_];
	double* avgstrain = new double[numberOfModelFrames_];
	double temp_array[3], temp_array1[3];
	unsigned int numSegments = segments.size();

	memset(avgstrain, 0, numberOfModelFrames_ * sizeof(double));
	std::vector<std::string> strains;
	std::stringstream ss;
	//Setup the header
	for (unsigned int i = 0; i < numSegments; i++) {
		WallSegment& seg = segments[i];
		for (unsigned int dt = 0; dt < samples; dt++) {
			double time = ((double) dt) / (samples - 1.0);

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
		for (unsigned int dt = 1; dt < samples - 1; dt++) {
			nstrain[dt] = length[dt] - length[0];

			ss << "," << nstrain[dt];
			avgstrain[dt] += nstrain[dt] * activeStrainSegments[i];
		}
		ss << "," << nstrain[0];
		avgstrain[numberOfModelFrames_ - 1] += nstrain[0] * activeStrainSegments[i];
		strains.push_back(ss.str());
		//Clear the buffer
		ss.str("");
	}

	ss << avgstrain[0];
	for (unsigned int dt = 1; dt < numberOfModelFrames_; dt++) {
		ss << "," << avgstrain[dt] / totalActiveSegments;
	}
	strains.push_back(ss.str());

	delete[] length;
	delete[] nstrain;
	delete[] avgstrain;
	return strains;
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

	const unsigned int num_strains = numberOfModelFrames_;
	double times[1024];
	for (unsigned int j = 0; j < num_strains; j++) {
		times[j] = static_cast<double>(j) / (num_strains - 1.0);
	}
	Cmiss_time_sequence_id time_sequence = Cmiss_field_module_get_matching_time_sequence(field_module, num_strains, times);

	Cmiss_field_module_begin_change(field_module);

	//Create Node template
	Cmiss_nodeset_id nodeset = Cmiss_field_module_find_nodeset_by_name(field_module, "cmiss_nodes");
	Cmiss_node_template_id nodeTemplate = Cmiss_nodeset_create_node_template(nodeset);
	//Create the field
	Cmiss_field_id strainField = Cmiss_field_module_create_field(field_module, fieldname.c_str(), "finite number_of_components 1 component_names value");
	if (!strainField) {
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

	for (int cptc = 0; cptc < 16; cptc += 2) {
		std::vector<std::string> strain1;
		std::vector<std::string> strain2;
		std::ostringstream ss;
		boost::split(strain1, compute[cptc], boost::is_any_of(","));
		boost::split(strain2, compute[cptc + 1], boost::is_any_of(","));
		for (unsigned int j = 0; j < num_strains - 1; j++) {
			double value1 = atof(strain1[j].c_str());
			double value2 = atof(strain2[j].c_str());
			ss << (value1 + value2) * 0.5 << ",";
		}
		ss << "0.0";
		allStrain.push_back(ss.str());
	}

	for (unsigned int i = 0; i < num_elements; i++) {
		Cmiss_node_id temporaryNode = Cmiss_nodeset_create_node(nodeset, extraNodeStartID++, nodeTemplate);
		Cmiss_element_template_set_node(element_template, 1, temporaryNode); //Link node with the element
		Cmiss_element_merge(elements[i], element_template);
		Cmiss_field_cache_set_node(fieldCache, temporaryNode);
		std::vector<std::string> strains;
		boost::split(strains, allStrain[elemStrainMap[i + 1]], boost::is_any_of(","));
		for (unsigned int j = 0; j < num_strains; j++) {
			double value = atof(strains[j].c_str());
			double time = times[j];
			Cmiss_field_cache_set_time(fieldCache, time);
			Cmiss_field_assign_real(strainField,fieldCache,1,&value);
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

std::string StrainMeasures::getLVVolume(double xi) {

	//Compute the LV circles
	std::vector<std::vector<Point3D> > circleXI;
	std::vector<std::vector<int> > circleElem;

	std::ostringstream lvVolume;
	//Note with respect to the model apex base is x-axis
	//choose y-z as xy
	//Compute the XI values for the circles
	// xi   x(1 inc),height(1 dec),y(=xi)
	const int elemIds[][12] = { { 14, 30, 46, 15, 31, 47, 16, 32, 48, 13, 29, 45 }, { 10, 26, 42, 11, 27, 13, 12, 28, 44, 9, 25, 41 },
			{ 6, 22, 38, 7, 23, 39, 8, 24, 40, 5, 21, 37 }, { 2, 18, 34, 3, 19, 35, 4, 20, 36, 1, 17, 33 } };
	double discretization = 0.05;

	for (int elemDepth = 0; elemDepth < 4; elemDepth++) {

		for (double depth = 1.0; depth >= discretization; depth -= discretization) {
			std::vector<Point3D> cpts;
			std::vector<int> cel;
			for (int eid = 0; eid < 12; eid++) {
				int elemID = elemIds[elemDepth][eid] - 1;
				for (double xs = 0.0; xs < 1.0; xs += discretization) {
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
	for (int frame = 0; frame < numberOfModelFrames_; frame++) {
		double time = ((double) frame) / (numberOfModelFrames_ - 1.0);
		std::vector<LVChamberCircle> circles;
		Cmiss_field_cache_set_time(fieldCache, time);
		//Compute the circles
		for (int i = 0; i < numCircles; i++) {
			std::vector<Point3D>& circlexi = circleXI[i];
			std::vector<int>& cel = circleElem[i];
			std::vector<Point3D> cpts;
			unsigned int npts = circlexi.size();
			double coord[3];
			for (int j = 0; j < npts; j++) {
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
		for (int i = 0; i < numCircles - 1; i++) {
			volume += circles[i].getVolume(circles[i + 1]);
		}
		if (minVolume < 0.0 || volume < minVolume) {
			minVolume = volume;
		}
		endVolume = volume;
		lvVolume << volume << ",";
	}

	if (xi < 0.001) {
		ejectionFraction = (endVolume - minVolume) / endVolume;
	}

	std::string lvstr = lvVolume.str();
	//Remove the last "," seperator
	return lvstr.substr(0, lvstr.length() - 1);
}

double StrainMeasures::getModelPgs() {
	return modelPGS;
}

std::string StrainMeasures::getCircumferentialStrain(double wall_xi, ViewTypeEnum type) {
	std::ostringstream strain;
	//Note with respect to the model apex base is x-axis
	//choose y-z as xy
	//Compute the XI values for the circles
	// xi   x(1 inc),height(1 dec),y(=xi)
	const int elemIds[][12] = { { 14, 30, 46, 15, 31, 47, 16, 32, 48, 13, 29, 45 }, { 10, 26, 42, 11, 27, 13, 12, 28, 44, 9, 25, 41 },
			{ 6, 22, 38, 7, 23, 39, 8, 24, 40, 5, 21, 37 }, { 2, 18, 34, 3, 19, 35, 4, 20, 36, 1, 17, 33 } };
	double discretization = 0.02;

	int elemDepth = 2;
	double depth = 0.0;
	switch (type) {
	case SAXAPEX: {
		elemDepth = 2;
		depth = 0.5;
		break;
	}
	case SAXMID: {
		elemDepth = 1;
		depth = 0.5;
		break;
	}
	case SAXBASE: {
		elemDepth = 0;
		depth = 0.01;
		break;
	}
	}

	std::vector<double*> cpts;
	std::vector<int> cel;
	for (int eid = 0; eid < 12; eid++) {
		int elemID = elemIds[elemDepth][eid] - 1;
		for (double xs = 0.0; xs < 1.0; xs += discretization) {
			Point3D xiCoord(xs, depth, wall_xi);
			cpts.push_back(xiCoord.ToArray());
			cel.push_back(elemID);
		}
	}
	unsigned int npts = cpts.size();
	std::vector<double> circums;
	for (int frame = 0; frame < numberOfModelFrames_; frame++) {
		double time = ((double) frame) / (numberOfModelFrames_ - 1.0);
		Cmiss_field_cache_set_time(fieldCache, time);
		//Compute the circle coordinates
		std::vector<Point3D> pts;
		double coord[3];
		for (int j = 0; j < npts; j++) {
			Cmiss_field_cache_set_mesh_location(fieldCache, elements[cel[j]], 3, cpts[j]);
			Cmiss_field_evaluate_real(coordianteField, fieldCache, 3, coord);
			coord[1] = 0.0;
			pts.push_back(Point3D(coord[0], coord[1], coord[2]));
		}
		LVChamberCircle circle(pts);

		circums.push_back(circle.getCircumference());
	}
	//Free mem
	for (int j = 0; j < npts; j++) {
		delete[] cpts[j];
	}

	double initValue = circums[0];

	for (int frame = 0; frame < numberOfModelFrames_; frame++) {
		strain << (circums[frame] - initValue) / initValue << ",";
	}

	std::string lvstr = strain.str();
	//Remove the last "," seperator
	return lvstr.substr(0, lvstr.length() - 1);
}

std::vector<std::string> StrainMeasures::getRadialStrains(double wall_xi, ViewTypeEnum type) {

	std::vector<std::string> strains(18, "");
	std::vector<double> avgStrains(numberOfModelFrames_, 0.0);
	std::ostringstream strain;
	//Note with respect to the model apex base is x-axis
	//choose y-z as xy
	//Compute the XI values for the circles

	const int allelemIds[][6][3] = { { { 47, 0, 0 }, { 46, 15, 31 }, { 14, 30, 46 }, { 45, 0, 0 }, { 48, 13, 29 }, { 16, 32, 48 } }, { { 43, 0, 0 }, { 42, 11, 27 }, { 14, 30, 46 },
			{ 41, 0, 0 }, { 44, 9, 25 }, { 12, 28, 44 } }, { { 39, 0, 0 }, { 38, 7, 23 }, { 14, 30, 46 }, { 37, 0, 0 }, { 40, 5, 21 }, { 8, 24, 40 } } };

	const double elemStarts[][3] = { { 0, 0, 0 }, { 0.5, 0, 0 }, { 0.0, 0, 0 }, { 0, 0, 0 }, { 0.5, 0, 0 }, { 0, 0, 0 } };

	const double elemEnds[][3] = { { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 0.5 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 0.5 } };

	double discretization = 0.02;

	int typedepth = 0;
	if (type == SAXAPEX)
		typedepth = 2;
	else if (type == SAXMID)
		typedepth = 1;

	double depth = 0.0; //Get the central cross section

	std::vector<std::vector<double*> > eCPTS;
	std::vector<std::vector<int> > eELEM;

	for (int elemDepth = 0; elemDepth < 6; elemDepth++) {
		std::vector<double*> cpts;
		std::vector<int> cel;
		for (int eid = 0; eid < 3; eid++) {
			if (allelemIds[typedepth][elemDepth][eid] > 0) {
				int elemID = allelemIds[typedepth][elemDepth][eid] - 1;
				for (double xs = elemStarts[elemDepth][eid]; xs < elemEnds[elemDepth][eid]; xs += discretization) {
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
	for (int ns = 0; ns < nStrains; ns++) {
		std::vector<double*>& cpts = eCPTS[ns];
		std::vector<int>& cel = eELEM[ns];
		unsigned int npts = cpts.size();
		std::vector<double> circums;
		for (int frame = 0; frame < numberOfModelFrames_; frame++) {
			double time = ((double) frame) / (numberOfModelFrames_ - 1.0);
			Cmiss_field_cache_set_time(fieldCache, time);
			//Compute the circle coordinates
			std::vector<Point3D> pts;
			double coord[3];
			for (int j = 0; j < npts; j++) {
				Cmiss_field_cache_set_mesh_location(fieldCache, elements[cel[j]], 3, cpts[j]);
				Cmiss_field_evaluate_real(coordianteField, fieldCache, 3, coord);
				coord[1] = 0.0;
				pts.push_back(Point3D(coord[0], coord[1], coord[2]));
			}
			double distance = 0;
			for (int j = 1; j < npts; j++) {
				distance += pts[j - 1].distance(pts[j]);
			}
			circums.push_back(distance);
		}
		//Free mem
		for (int j = 0; j < npts; j++) {
			delete[] cpts[j];
		}

		double initValue = circums[0];

		strain.str("");
		for (int frame = 0; frame < numberOfModelFrames_; frame++) {
			double stv = (circums[frame] - initValue) / initValue;
			strain << stv << ",";
			avgStrains[frame] += stv;
		}

		std::string lvstr = strain.str();
		strains[ns] = (lvstr.substr(0, lvstr.length() - 1));
	}

	//Push the average strain
	strain.str("");
	for (int frame = 0; frame < numberOfModelFrames_; frame++) {
		strain << avgStrains[frame] / numberOfModelFrames_ << ",";
	}

	std::string lvstr = strain.str();
	strains[17] = (lvstr.substr(0, lvstr.length() - 1)); //Location of average strain

	return strains;
}

std::vector<std::string> StrainMeasures::getSixteenSegmentTorsionFreeStrains(double wall_xi) {
	return getSixteenSegmentStrains("torsionfree_coordinates", wall_xi);
}

double StrainMeasures::getEjectionFraction() {
	return ejectionFraction;
}

int StrainMeasures::getNumberOfFrames() {
	return numberOfModelFrames_;
}

std::vector<double> StrainMeasures::getMyocardialVolumes() {
	return volumes;
}

std::vector<std::string> StrainMeasures::getMesh() {
	std::vector<std::string> resultx;
	double denom = (numberOfModelFrames_ - 1.0);
	for (int i = 0; i < numberOfModelFrames_; i++) {
		double time = ((double) i) / denom;
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
		if (memory_buffer && memory_buffer_size) {
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
