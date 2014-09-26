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

#include "ShortAxisFitting.h"

ShortAxisFitting::ShortAxisFitting(std::vector<std::string> mesh, std::vector<std::vector<Point3D> > markers) :
		inputMesh(mesh), saxMarkers(markers) {
	numberOfModelFrames_ = mesh.size();
	cmiss_nodes.resize(98);
	//Load the mesh
	//Initialise cmgui and get context
	context_ = Cmiss_context_create("saxfit");
	//This required for using the cmiss_context_execute_command
	Cmiss_context_enable_user_interface(context_, NULL);

	/**< Handle to the context */
	Cmiss_region_id root_region = Cmiss_context_get_default_region(context_);

	std::string region_name = "heart";
	// Create a heart region

	Cmiss_region_id heart_region = Cmiss_region_create_child(root_region, region_name.c_str());

	// Read in the heart model spaced over the number of model frames
	for (unsigned int i = 0; i < numberOfModelFrames_ - 1; i++)
	{
		double time = static_cast<double>(i) / numberOfModelFrames_;
		Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
		Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
		Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, mesh[i].c_str(), mesh[i].length());
		Cmiss_stream_information_region_set_resource_attribute_real(stream_information_region, stream_resource, CMISS_STREAM_INFORMATION_REGION_ATTRIBUTE_TIME, time);

		Cmiss_region_read(root_region, stream_information);

		Cmiss_stream_resource_destroy(&stream_resource);
		Cmiss_stream_information_destroy(&stream_information);
		Cmiss_stream_information_region_destroy(&stream_information_region);
	}

	// Wrap the end point add another set of nodes at time 1.0
	{
		Cmiss_stream_information_id stream_information = Cmiss_region_create_stream_information(root_region);
		Cmiss_stream_information_region_id stream_information_region = Cmiss_stream_information_cast_region(stream_information);
		Cmiss_stream_resource_id stream_resource = Cmiss_stream_information_create_resource_memory_buffer(stream_information, mesh[0].c_str(), mesh[0].length());
		int r = Cmiss_stream_information_region_set_resource_attribute_real(stream_information_region, stream_resource, CMISS_STREAM_INFORMATION_REGION_ATTRIBUTE_TIME, 1.0);

		Cmiss_region_read(root_region, stream_information);

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

		int endo[] =
		{ 55, 57, 71, 73, 66, 67, 69, 60, 61, 63, 52, 53, 54, 56, 70, 72, 64, 65, 68, 58, 59, 62, 50, 51, 77, 84, 85, 81, 82, 83, 78, 79, 80, 74, 75, 76, 89, 96, 97, 93, 94, 95,
			90, 91, 92, 86, 87, 88, 98 };
		memcpy(endoNodeIds, endo, sizeof(int) * 49);

		//int sax[] =	{ 73, 74, 75, 76, 83, 84, 80, 81, 82, 77, 78, 79 };
		//int sax[] =		{ 79, 78, 77, 82, 81, 80, 84, 83, 76, 75, 74, 73 };
		int sax[] =	{ 80, 81, 82, 77, 78, 79, 73, 74, 75, 76, 83, 84 };

		memcpy(saxNodes, sax, sizeof(int) * NUMBER_OF_SAX_NODES);

		segmentNodes = new double*[24];
		for (int i = 0; i < 24; i++)
		{
			segmentNodes[i] = new double[2];
		}
		//Store the initial endo coordinates

		const int NUMBER_OF_ENDO_NODES = 48; //Skip apex
		double coord[3];
		for(int frame = 0;frame<numberOfModelFrames_;frame++){
			std::vector<Point3D> frameCoords;
			Cmiss_field_module_begin_change(field_module_);
			{
				double time = ((double) frame) / ((double) numberOfModelFrames_);
				if (frame == (numberOfModelFrames_ - 1))
				{
					time = 1.0;
				}
				Cmiss_field_cache_set_time(cache, time);
				for (int nc = 0; nc < NUMBER_OF_ENDO_NODES; nc++)
				{
					Cmiss_field_cache_set_node(cache, cmiss_nodes[endoNodeIds[nc] - 1]);
					Cmiss_field_evaluate_real(coordinates_rc_, cache, 3, coord);
					Point3D start(coord);
					frameCoords.push_back(start);
				}
				initEndoCoordinates.push_back(frameCoords);
			}
			Cmiss_field_module_end_change(field_module_);
		}



		//Set the segment node ids
		segmentNodes[0][0] = aplaxNodes0;
		segmentNodes[0][1] = aplaxNodes1;
		segmentNodes[1][0] = aplaxNodes1;
		segmentNodes[1][1] = aplaxNodes2;
		segmentNodes[2][0] = aplaxNodes2;
		segmentNodes[2][1] = aplaxNodes3;
		segmentNodes[3][0] = aplaxNodes3;
		segmentNodes[3][1] = aplaxNodes4;
		segmentNodes[4][0] = aplaxNodes4;
		segmentNodes[4][1] = aplaxNodes5;
		segmentNodes[5][0] = aplaxNodes5;
		segmentNodes[5][1] = aplaxNodes6;
		segmentNodes[6][0] = aplaxNodes6;
		segmentNodes[6][1] = aplaxNodes7;
		segmentNodes[7][0] = aplaxNodes7;
		segmentNodes[7][1] = aplaxNodes8;
		segmentNodes[8][0] = tchNodes0;
		segmentNodes[8][1] = tchNodes1;
		segmentNodes[9][0] = tchNodes1;
		segmentNodes[9][1] = tchNodes2;
		segmentNodes[10][0] = tchNodes2;
		segmentNodes[10][1] = tchNodes3;
		segmentNodes[11][0] = tchNodes3;
		segmentNodes[11][1] = tchNodes4;
		segmentNodes[12][0] = tchNodes4;
		segmentNodes[12][1] = tchNodes5;
		segmentNodes[13][0] = tchNodes5;
		segmentNodes[13][1] = tchNodes6;
		segmentNodes[14][0] = tchNodes6;
		segmentNodes[14][1] = tchNodes7;
		segmentNodes[15][0] = tchNodes7;
		segmentNodes[15][1] = tchNodes8;
		segmentNodes[16][0] = fchNodes0;
		segmentNodes[16][1] = fchNodes1;
		segmentNodes[17][0] = fchNodes1;
		segmentNodes[17][1] = fchNodes2;
		segmentNodes[18][0] = fchNodes2;
		segmentNodes[18][1] = fchNodes3;
		segmentNodes[19][0] = fchNodes3;
		segmentNodes[19][1] = fchNodes4;
		segmentNodes[20][0] = fchNodes4;
		segmentNodes[20][1] = fchNodes5;
		segmentNodes[21][0] = fchNodes5;
		segmentNodes[21][1] = fchNodes6;
		segmentNodes[22][0] = fchNodes6;
		segmentNodes[22][1] = fchNodes7;
		segmentNodes[23][0] = fchNodes7;
		segmentNodes[23][1] = fchNodes8;

		//Compute the segment lengths
		initialSegmentLengths = new double[NUMBER_OF_SEGMENTS * numberOfModelFrames_];

		ShortAxisOptimizationInput input;
		input.NUMBER_OF_SEGMENTS = NUMBER_OF_SEGMENTS;
		input.cache = cache;
		input.cmiss_nodes = &cmiss_nodes;
		input.coordinates_rc = coordinates_rc_;
		input.result = &initialSegmentLengths;
		input.numberOfModelFrames = numberOfModelFrames_;
		input.initialSegmentLengths = NULL;
		input.segmentNodes = segmentNodes;

		getSegmentLengths(&input);
		//Compute the average rotation that of the sax markers
		std::vector<Point3D>& iptCoord(saxMarkers[0]);
		unsigned int numSaxMarkers = iptCoord.size();
		Point3D saxCentroid(0, 0, 0);
		for (int i = 0; i < numSaxMarkers; i++)
		{
			saxCentroid += iptCoord[i];
		}
		saxCentroid = saxCentroid*(-1.0/numSaxMarkers);
		initFrame = iptCoord;
		for (int i = 0; i < numSaxMarkers; i++)
		{
			initFrame[i]+=saxCentroid;
		}

		for(int frame = 0;frame<numberOfModelFrames_;frame++){
			std::vector<Point3D>& ipCoord(saxMarkers[frame]);
			unsigned int numSaxMarkers = ipCoord.size();
			Point3D saxcentroid(0, 0, 0);
			for (int i = 0; i < numSaxMarkers; i++)
			{
				saxcentroid += ipCoord[i];
			}
			saxcentroid = saxcentroid*(-1.0/numSaxMarkers);
			std::vector<Point3D> currentFrame(ipCoord);
			for (int i = 0; i < numSaxMarkers; i++)
			{
				currentFrame[i]+=saxcentroid;
			}
			double avgAngle = 0.0;
			for (int i = 0; i < numSaxMarkers; i++)
			{
				avgAngle += (atan2(currentFrame[i].z-initFrame[i].z,currentFrame[i].x-initFrame[i].x));
			}
			avgAngle /=numSaxMarkers;
			targetDeltas.push_back(avgAngle);
			std::cout<<frame<<"\t"<<avgAngle*180/M_PI<<std::endl;
		}
		//The initFrame markers should correspond to the initFrame of the mesh as that is what is used in the comparisons
		initFrame.clear();

		Point3D sax_centroid(0, 0, 0);
		for (int seg = 0; seg < NUMBER_OF_SAX_NODES; seg++)
		{
			Cmiss_field_cache_set_node(cache, cmiss_nodes[saxNodes[seg]]);
			Cmiss_field_evaluate_real(coordinates_rc_, cache, 3, coord);
			Point3D start(coord);
			initFrame.push_back(start);
			sax_centroid += start;
		}
		sax_centroid = sax_centroid * (-1.0 / NUMBER_OF_SAX_NODES);
		for (int seg = 0; seg < NUMBER_OF_SAX_NODES; seg++)
		{
			initFrame[seg] += sax_centroid;
		}

	}
	else
	{
		std::cout << "--- No field module for heart region!!! (Short Axis Fitting)";
		std::cout << "No field module " << std::endl;
		throw -1;
	}

	Cmiss_region_destroy(&heart_region);
	Cmiss_region_destroy(&root_region);
}

ShortAxisFitting::~ShortAxisFitting() {
	for (int i = 0; i < NUMBER_OF_NODES; i++)
	{
		Cmiss_node_destroy(&cmiss_nodes[i]);
	}
	Cmiss_field_cache_destroy(&cache);
	Cmiss_field_destroy(&coordinates_rc_);
	Cmiss_field_module_destroy(&field_module_);
	Cmiss_context_destroy(&context_);
	delete[] initialSegmentLengths;
	for (int i = 0; i < 24; i++)
	{
		delete[] segmentNodes[i];
	}
	delete[] segmentNodes;
}

void ShortAxisFitting::getSegmentLengths(ShortAxisOptimizationInput* input) {
	int ctr = 0;
	double* lengthArray = *(input->result);
	double* initialSegmentLengths = input->initialSegmentLengths;
	double coord[3];
	Cmiss_field_id coordinates_rc = input->coordinates_rc;
	Cmiss_field_cache_id cache = input->cache;
	std::vector<Cmiss_node_id>& cmiss_nodes(*input->cmiss_nodes);
	double** segmentNodes = input->segmentNodes;
	int frame = input->frame;
	//for (int frame = 0; frame < input->numberOfModelFrames; frame++)
	{
		double time = ((double) frame) / ((double) input->numberOfModelFrames);
		if (frame == (input->numberOfModelFrames - 1))
		{
			time = 1.0;
		}
		Cmiss_field_cache_set_time(cache, time);
		for (int seg = 0; seg < input->NUMBER_OF_SEGMENTS; seg++)
		{
			Cmiss_field_cache_set_node(cache, cmiss_nodes[segmentNodes[seg][0]]);
			Cmiss_field_evaluate_real(coordinates_rc, cache, 3, coord);
			Point3D start(coord);
			Cmiss_field_cache_set_node(cache, cmiss_nodes[segmentNodes[seg][1]]);
			Cmiss_field_evaluate_real(coordinates_rc, cache, 3, coord);
			Point3D end(coord);
			lengthArray[ctr++] = start.distance(end);
		}
	}
	ctr = 0;
	double error = 0;
	if (initialSegmentLengths != NULL)
	{
		//for (int frame = 0; frame < input->numberOfModelFrames; frame++)
		{
			for (int seg = 0; seg < input->NUMBER_OF_SEGMENTS; seg++)
			{
				error += fabs(initialSegmentLengths[ctr] - lengthArray[ctr]);
				ctr++;
			}
		}
	}
	input->segmentMatchError = error;
}

void ShortAxisFitting::fit() {
	//Compute the segment lengths
	double *tempSpace = new double[NUMBER_OF_SEGMENTS * numberOfModelFrames_];
	memcpy(tempSpace, initialSegmentLengths, sizeof(double) * NUMBER_OF_SEGMENTS * numberOfModelFrames_);

	ShortAxisOptimizationInput* input = new struct ShortAxisOptimizationInput;
	input->numberOfModelFrames = numberOfModelFrames_;
	input->NUMBER_OF_SEGMENTS = NUMBER_OF_SEGMENTS;
	input->NUMBER_OF_SAX_NODES = NUMBER_OF_SAX_NODES;
	input->field_module = field_module_;
	input->coordinates_rc = coordinates_rc_;
	input->cache = cache;
	input->cmiss_nodes = &cmiss_nodes;
	input->targetDeltas = &targetDeltas;
	input->endoNodesIds = endoNodeIds;
	input->segmentNodes = segmentNodes;
	input->saxNodes = saxNodes;
	input->initialSegmentLengths = initialSegmentLengths;
	input->result = &tempSpace;
	input->segmentMatchError = 0.0;
	input->initFrame = &initFrame;
	input->initEndoCoordinates = &initEndoCoordinates;

	int nodegroups[][12] = {
	   {aplaxNodes0,aplaxNodes8,tchNodes0,tchNodes8,fchNodes0,fchNodes8,aplaxtchNode00,aplaxtchNode88,fchaplaxNode00,fchaplaxNode88,fchtchNode08,fchtchNode80},
	   {aplaxNodes1,aplaxNodes7,tchNodes1,tchNodes7,fchNodes1,fchNodes7,aplaxtchNode11,aplaxtchNode77,fchaplaxNode11,fchaplaxNode77,fchtchNode17,fchtchNode71},
	   {aplaxNodes2,aplaxNodes6,tchNodes2,tchNodes6,fchNodes2,fchNodes6,aplaxtchNode22,aplaxtchNode66,fchaplaxNode22,fchaplaxNode66,fchtchNode26,fchtchNode62},
	   {aplaxNodes3,aplaxNodes5,tchNodes3,tchNodes5,fchNodes3,fchNodes5,aplaxtchNode33,aplaxtchNode55,fchaplaxNode33,fchaplaxNode55,fchtchNode35,fchtchNode53}
	};

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

	alglib::real_1d_array x;
	alglib::real_1d_array bndl;
	alglib::real_1d_array bndu;

	//Create the initial and boundary vectors
	int parameters = 4;
	double* tempValues = new double[parameters];

	for (int i = 0; i < parameters; i++)
	{
		tempValues[i] = -0.25;
	}
	bndl.setcontent(parameters, tempValues);
	for (int i = 0; i < parameters; i++)
	{
		tempValues[i] = +0.25;
	}
	bndu.setcontent(parameters, tempValues);

	alglib::minbleicstate state;
	alglib::minbleicreport rep;

	alglib::ae_int_t maxits = 0;

	//
	// This variable contains differentiation step
	//
	double diffstep = 1.0e-4;

	for (int frame = 0; frame < numberOfModelFrames_; frame++)
	{
		input->frame = frame;
		clock_t begin = clock();


		tempValues[0] = 0.0;
		tempValues[2] = targetDeltas[frame];
		tempValues[1] = -targetDeltas[frame]/2;
		tempValues[3] = -targetDeltas[frame]/2;

		x.setcontent(parameters,tempValues);
		//Compute the centroids
		{
			double coord[3];
			double time = ((double) frame) / ((double) numberOfModelFrames_);
			if (frame == (numberOfModelFrames_ - 1))
			{
				time = 1.0;
			}
			Cmiss_field_cache_set_time(cache, time);
			//Compute the centroids
			Point3D centroids[4];
			for(int sc=0;sc<4;sc++){
				for(int nc = 0;nc<12;nc++){
					Cmiss_field_cache_set_node(cache, cmiss_nodes[nodegroups[sc][nc]]);
					Cmiss_field_evaluate_real(coordinates_rc_, cache, 3, coord);
					Point3D start(coord);
					centroids[sc] += start;
				}
				input->centroids[sc] = centroids[sc]*(-1.0/12);
			}

		}


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
			alglib::minbleicoptimize(state, optimize, NULL, input);
			alglib::minbleicresults(state, x, rep);
			setThetaDelta(input, x);

			{
				double coord[3];
				{
					double time = ((double) frame) / ((double) numberOfModelFrames_);
					if (frame == (numberOfModelFrames_ - 1))
					{
						time = 1.0;
					}
					Cmiss_field_cache_set_time(cache, time);
					std::vector<Point3D> sax;
					Point3D sax_centroid(0, 0, 0);
					for (int seg = 0; seg < NUMBER_OF_SAX_NODES; seg++)
					{
						Cmiss_field_cache_set_node(cache, cmiss_nodes[saxNodes[seg]]);
						Cmiss_field_evaluate_real(coordinates_rc_, cache, 3, coord);
						Point3D start(coord);
						sax.push_back(start);
						sax_centroid += start;
					}
					sax_centroid = sax_centroid * (1.0 / NUMBER_OF_SAX_NODES);
					double avg = 0.0;
					for (int seg = 0; seg < NUMBER_OF_SAX_NODES; seg++)
					{
						Vector3D trans = sax[seg] - sax_centroid;
						double denom = trans.x- initFrame[seg].x;
						double nuem  = trans.z- initFrame[seg].z;
						double ltheta = atan2(nuem, denom);
						if(fabs(denom)<1.0e-6 && fabs(nuem)<1.0e-6){
							ltheta = 0.0;
						}
						avg+=ltheta;
					}
					avg/=NUMBER_OF_SAX_NODES;
					std::cout << "Frame " << frame << "\t" << x.tostring(5)<<"\t Result "<<avg*180/M_PI<<"\t Target "<<targetDeltas[frame]*180/M_PI<< std::endl;
				}

			}

		} catch (alglib::ap_error& err)
		{
			std::cout << err.msg << std::endl;
		}

		clock_t end = clock();
		double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
		std::cout << "Optimization took " << elapsed_secs << std::endl;
		std::cout <<"Total Error "<< input->totalError << "\t segment error " << input->segmentMatchError << "\t thetaError " << input->thetaError << std::endl;
		std::cout<<"*******************************************************************"<<std::endl;
	}
	delete[] tempSpace;
	throw -1;
}

void ShortAxisFitting::setThetaDelta(ShortAxisOptimizationInput* input, const alglib::real_1d_array& x) {
	Cmiss_field_module_id field_module = (input->field_module);
	std::vector<Cmiss_node_id>& cmiss_nodes(*(input->cmiss_nodes));
	Cmiss_field_id coordinate = (input->coordinates_rc);
	Cmiss_field_cache_id cache = (input->cache);
	double* initialSegmentLengths = input->initialSegmentLengths;
	int* saxNodes = input->saxNodes;
	int numberOfModelFrames = input->numberOfModelFrames;
	int NUMBER_OF_SEGMENTS = input->NUMBER_OF_SEGMENTS;
	int NUMBER_OF_SAX_NODES = input->NUMBER_OF_SAX_NODES;
	double** result = input->result; //This is expected to be preallocated for heap efficiency
	int* endoNodeIds = input->endoNodesIds;
	const int NUMBER_OF_ENDO_NODES = 48; //Ignore the apex
	int ctr = 0;
	double coord[3];
	int frame = input->frame;
	Point3D centroids[4];
	centroids[0] = input->centroids[0];
	centroids[1] = input->centroids[1];
	centroids[2] = input->centroids[2];
	centroids[3] = input->centroids[3];


	//First 12 endo nodes saxcentroid 0, next 12 1...

	//Set the theta variation in x to the mesh
	{
		Cmiss_field_module_begin_change(field_module);
		{
			double time = ((double) frame) / ((double) numberOfModelFrames);
			if (frame == (numberOfModelFrames - 1))
			{
				time = 1.0;
			}
			Cmiss_field_cache_set_time(cache, time);


			for (int nc = 0; nc < NUMBER_OF_ENDO_NODES; nc++)
			{
				ctr = nc/12;
				double deltaTheta = ((int)(x[ctr]*100000))/100000.0;
				Cmiss_field_cache_set_node(cache, cmiss_nodes[endoNodeIds[nc] - 1]);
				Cmiss_field_evaluate_real(coordinate, cache, 3, coord);
				Point3D start(coord);
				start = start + centroids[nc/12];
				double nTheta = atan2(start.z, start.x) + deltaTheta;
				double dist = sqrt(start.x * start.x + start.z * start.z);
				coord[0] = dist * cos(nTheta) - centroids[nc/12].x;
				coord[2] = dist * sin(nTheta) - centroids[nc/12].z;

				Cmiss_field_assign_real(coordinate, cache, 3, coord);
			}
		}
		Cmiss_field_module_end_change(field_module);
	}

}

void ShortAxisFitting::optimize(const alglib::real_1d_array& x, double& func, void* ptr) {
	struct ShortAxisOptimizationInput* input = static_cast<struct ShortAxisOptimizationInput*>(ptr);
	Cmiss_field_module_id field_module = (input->field_module);
	std::vector<Cmiss_node_id>& cmiss_nodes(*(input->cmiss_nodes));
	std::vector<double>& targetDeltas(*(input->targetDeltas));
	Cmiss_field_id coordinate = (input->coordinates_rc);
	Cmiss_field_cache_id cache = (input->cache);
	double* initialSegmentLengths = input->initialSegmentLengths;
	int* saxNodes = input->saxNodes;
	int numberOfModelFrames = input->numberOfModelFrames;
	int NUMBER_OF_SEGMENTS = input->NUMBER_OF_SEGMENTS;
	int NUMBER_OF_SAX_NODES = input->NUMBER_OF_SAX_NODES;
	double** result = input->result; //This is expected to be preallocated for heap efficiency
	int* endoNodeIds = input->endoNodesIds;
	const int NUMBER_OF_ENDO_NODES = 48; //Ignore the apex
	int ctr = 0;
	double coord[3];
	int frame = input->frame;
	std::vector<Point3D>& resetCoord(input->initEndoCoordinates->at(frame));
	std::vector<Point3D>& initFrame(*input->initFrame);

	setThetaDelta(input, x);

	//Compute the theta error
	double thetaError = 0.0;
	//for (int frame = 0; frame < numberOfModelFrames; frame++)
	{
		double time = ((double) frame) / ((double) numberOfModelFrames);
		if (frame == (numberOfModelFrames - 1))
		{
			time = 1.0;
		}
		Cmiss_field_cache_set_time(cache, time);
		std::vector<Point3D> sax;
		Point3D sax_centroid(0, 0, 0);
		for (int seg = 0; seg < NUMBER_OF_SAX_NODES; seg++)
		{
			Cmiss_field_cache_set_node(cache, cmiss_nodes[saxNodes[seg]]);
			Cmiss_field_evaluate_real(coordinate, cache, 3, coord);
			Point3D start(coord);
			sax.push_back(start);
			sax_centroid += start;
		}
		sax_centroid = sax_centroid * (1.0 / NUMBER_OF_SAX_NODES);
		double avg = 0.0;
		for (int seg = 0; seg < NUMBER_OF_SAX_NODES; seg++)
		{
			Vector3D trans = sax[seg] - sax_centroid;
			double denom = trans.x- initFrame[seg].x;
			double nuem  = trans.z- initFrame[seg].z;
			double ltheta = atan2(nuem, denom);
			if(fabs(denom)<1.0e-6 && fabs(nuem)<1.0e-6){
					ltheta = 0.0;
			}
			avg += ltheta;
		}
		avg /=NUMBER_OF_SAX_NODES;
		thetaError +=fabs(avg-targetDeltas[frame]);
		input->thetaError = avg;
	}


	//Compute the segment error
	getSegmentLengths(input);
	double segmentMatchError = input->segmentMatchError;
	func = segmentMatchError + thetaError;
	if(segmentMatchError>2.0)
		func = 1.0e+30;
	input->totalError = func;
	//Reset coordinates to original
	{
		Cmiss_field_module_begin_change(field_module);

		{
			double time = ((double) frame) / ((double) numberOfModelFrames);
			if (frame == (numberOfModelFrames - 1))
			{
				time = 1.0;
			}
			Cmiss_field_cache_set_time(cache, time);
			for (int nc = 0; nc < NUMBER_OF_ENDO_NODES; nc++)
			{
				coord[0] = resetCoord[nc].x;
				coord[1] = resetCoord[nc].y;
				coord[2] = resetCoord[nc].z;
				Cmiss_field_cache_set_node(cache, cmiss_nodes[endoNodeIds[nc] - 1]);
				Cmiss_field_assign_real(coordinate, cache, 3, coord);
			}
		}
		Cmiss_field_module_end_change(field_module);
	}
}

std::vector<std::string> ShortAxisFitting::getMesh() {
	std::vector<std::string> resultx;
	for (int i = 0; i < numberOfModelFrames_; i++)
	{
		double time = ((double) i) / ((double) numberOfModelFrames_);
		if (i == (numberOfModelFrames_ - 1))
			time = 1.0;
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
