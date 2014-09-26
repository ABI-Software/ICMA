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

#ifndef VOLUMEREGULARIZER_H_
#define VOLUMEREGULARIZER_H_

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include "Point3D.h"
#include "Vector3D.h"
#include "alglib/interpolation.h"

extern "C"
{
#include "zn/cmgui_configure.h"
#include "zn/cmiss_context.h"
#include "zn/cmiss_field.h"
#include "zn/cmiss_element.h"
#include "zn/cmiss_region.h"
#include "zn/cmiss_node.h"
#include "zn/cmiss_stream.h"
#include "zn/cmiss_field_module.h"
#include "zn/cmiss_field_composite.h"
#include "zn/cmiss_field_matrix_operators.h"
#include "zn/cmiss_field_finite_element.h"
#include "zn/cmiss_element.h"
#include "zn/cmiss_time_keeper.h"
#include "zn/cmiss_field_time.h"
}

/*
 * Ensures that the mesh myocardial volume is conserved through the cycle
 * The volume of the mesh at time 0 is taken as the objective
 * The wall lengths of elements are modified proportional to their depth
 * with apex allowed 100% change
 *
 * Input mesh is expected to be fitted to endocardial markers and in RC coordinates
 * The focal length for an appropriate PS coordinates should also be provided
 */

class VolumeRegularizer
{
  std::vector<std::string> inputMesh;
  double apexChange;
  double ring1Change;
  double ring2Change;
  double ring3Change;
  unsigned int numberOfModelFrames_;
  std::vector<Cmiss_node_id> cmiss_nodes;
  Cmiss_context_id context_;

  Cmiss_field_module_id field_module_;
  Cmiss_field_id coordinates_ps_;
  Cmiss_field_id coordinates_rc_;
  Cmiss_field_id volume;
  Cmiss_field_cache_id cache;
  const static int NUMBER_OF_NODES = 98;
  std::vector<double> initialWallLengths;
  std::vector<double> volumes;
  int epiNodeIds[49];
  int endoNodeIds[49];
public:
  VolumeRegularizer(std::vector<std::string> mesh, double focalLength);
  void regularize();
  std::vector<std::string> getMesh();
  std::vector<double> getVolumes();
  static void optimize(const alglib::real_1d_array& x, double &func, void* ptr);
  virtual
  ~VolumeRegularizer();
};

struct optimizationInput{
  Cmiss_field_module_id* field_module_;
  std::vector<Cmiss_node_id>* cmiss_nodes;
  std::vector<double>* initialWallLengths;
  Cmiss_field_id* volume;
  Cmiss_field_id* coordinates_ps_;
  Cmiss_field_cache_id* cache;
  int* epiNodeIDs;
  int* endoNodeIDs;
  double time;
  double targetVolume;
};

#endif /* VOLUMEREGULARIZER_H_ */
