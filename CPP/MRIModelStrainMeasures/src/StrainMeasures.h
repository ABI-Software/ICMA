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
#ifndef STRAINMEASURES_H_
#define STRAINMEASURES_H_
#include "WallSegment.h"
#include "LevelSet.h"
#include "LVMyocardialVolume.h"
#include <vector>
#include <boost/algorithm/string.hpp>
#include <fstream>

#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <assert.h>
#include <cmath>
#include <cstdlib>
#include "Point3D.h"
#include "LVChamberCircle.h"

extern "C" {
#include <zn/cmgui_configure.h>
#include "zn/cmiss_field_vector_operators.h"
#include "zn/cmiss_context.h"
#include "zn/cmiss_graphic.h"
#include "zn/cmiss_core.h"
#include "zn/cmiss_graphics_filter.h"
#include "zn/cmiss_differential_operator.h"
#include "zn/cmiss_graphics_material.h"
#include "zn/cmiss_element.h"
#include "zn/cmiss_graphics_module.h"
#include "zn/cmiss_fdio.h"
#include "zn/cmiss_idle.h"
#include "zn/cmiss_field_alias.h"
#include "zn/cmiss_interactive_tool.h"
#include "zn/cmiss_field_arithmetic_operators.h"
#include "zn/cmiss_node.h"
#include "zn/cmiss_field_composite.h"
#include "zn/cmiss_optimisation.h"
#include "zn/cmiss_field_conditional.h"
#include "zn/cmiss_region.h"
#include "zn/cmiss_field_finite_element.h"
#include "zn/cmiss_rendition.h"
#include "zn/cmiss_field_group.h"
#include "zn/cmiss_scene.h"
#include "zn/cmiss_field.h"
#include "zn/cmiss_scene_viewer.h"
#include "zn/cmiss_field_image.h"
#include "zn/cmiss_selection.h"
#include "zn/cmiss_field_image_processing.h"
#include "zn/cmiss_spectrum.h"
#include "zn/cmiss_field_logical_operators.h"
#include "zn/cmiss_status.h"
#include "zn/cmiss_field_matrix_operators.h"
#include "zn/cmiss_stream.h"
#include "zn/cmiss_field_module.h"
#include "zn/cmiss_tessellation.h"
#include "zn/cmiss_field_nodeset_operators.h"
#include "zn/cmiss_time.h"
#include "zn/cmiss_field_scene_viewer_projection.h"
#include "zn/cmiss_time_keeper.h"
#include "zn/cmiss_field_subobject_group.h"
#include "zn/cmiss_time_sequence.h"
#include "zn/cmiss_field_time.h"
#include "zn/cmiss_field_trigonometry.h"
}

enum ViewTypeEnum {
	APLAX, TCH, FCH, SAXBASE, SAXMID, SAXAPEX
};

class StrainMeasures {
	int x_discret;
	int y_discret;
	bool lagrangian;
	int extraNodeStartID;
	double modelPGS;
	double ejectionFraction;
	bool computeModelPGS;
	int num_level_sets;
	unsigned int num_elements;
	unsigned int num_nodes;
	unsigned int numberOfModelFrames_;
	std::vector<double> volumes;
	std::vector<WallSegment>* mySegments;
	std::vector<Cmiss_element_id> elements;
	Cmiss_field_module_id field_module;
	Cmiss_field_cache_id fieldCache;
	Cmiss_field_id coordianteField;
	Cmiss_context_id context_;
	double getLocalDeformation(double* eigenvalues, long * nan_count);
	std::vector<std::string> getSegmentStrains(std::string fieldName);
	std::vector<std::string> getSixteenSegmentStrains(std::string fieldName, double wall_xi);

public:
	StrainMeasures(std::vector<std::string> mesh);
	virtual ~StrainMeasures();
	void setWallSegments(std::vector<WallSegment>* segs);

	std::string getLVVolume(double xi = 0.0);
	//Returns mySegments->size()+1 values, the last one being the average strain
	std::vector<std::string> getSegmentStrains();
	//Returns mySegments->size()+1 values, the last one being the average strain
	std::vector<std::string> getSixteenSegmentStrains(double wall_xi = 0.0);
	//Returns mySegments->size()+1 values, the last one being the average strain
	//if the torsion free coordinates exist else the vector is of size zero
	std::vector<std::string> getSixteenSegmentTorsionFreeStrains(double wall_xi = 0.0);
	//Returns mySegments->size()+1 values, the last one being the average strain
	std::vector<std::string> getSegmentFibreStrains();
	//Returns mySegments->size()+1 values, the last one being the average strain
	std::vector<std::string> getSixteenSegmentFibreStrains(double wall_xi = 0.0);
	std::vector<std::string> getSixteenSegmentTorsions(double wall_xi = 0.0);
	std::vector<std::string> getLinearDistortion(double* wall_xi, unsigned int cnt, double power = 2.0);
	//Returns the circumferential strain over a cycle
	std::string getCircumferentialStrain(double wall_xi = 0.0, ViewTypeEnum type = SAXBASE);
	//Returns radial strain
	std::vector<std::string> getRadialStrains(double wall_xi = 0.0, ViewTypeEnum type = SAXBASE);

	void embedStrainOnElements(std::string fieldname, std::vector<std::string>& strain);
	void Print(std::string filename, std::vector<std::string> strains);
	void setLagrangian();
	void setNatural();
	bool isLagrangian();
	double getModelPgs();
	double getEjectionFraction();
	int getNumberOfFrames();
	std::vector<double> getMyocardialVolumes();
	std::vector<std::string> getMesh();
};

#endif /* STRAINMEASURES_H_ */
