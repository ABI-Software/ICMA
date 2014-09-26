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

#ifndef LVHEARTMESH_H_
#define LVHEARTMESH_H_

#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <ctime>
#include "Point3D.h"
#include "Vector3D.h"
#include "LongAxisFitting.h"
#include "BundleToDICOMException.h"
#include "XMLInputReader.h"
#include "VolumeRegularizer.h"
#include "ShortAxisFitting.h"
#include "TorsionFitting.h"

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

#include "debug.h"


#include "vnl/vnl_matrix.h"
#include "vnl/algo/vnl_matrix_inverse.h"


typedef vnl_matrix<double> MatrixType;

typedef std::vector<Point3D> PointVector;

/** \class LVHeartMesh
 * \brief A class that takes markers (coordinates, material coordinates, time) as input and produces a LV mesh
 *
 *  Markers that provide coordinate, material coordinate, time information are used to fit LV mesh
 *  The fitting is performed in prolate spheroidal coordinates (essentially lambda coordinate of the mesh is
 *  fit to the specified coordinate), the material coordinate is provided.
 */

class LVHeartMesh
{
  //StrainMeasures is made a friend to access the internal data
  friend class StrainMeasures;
  XMLInputReader& inputReader;
public:
  //Default constructor that takes cmgui context name and the number of LV mesh frames
  LVHeartMesh(std::string name, XMLInputReader& reader);
  virtual
  ~LVHeartMesh();

  //Set the apex coordinate for the LV Mesh
  void
  setApex(Point3D apex);

  //Set the base coordinate for the LV Mesh
  void
  setBase(Point3D base);

  //Set the rv coordinates of the LV Mesh
  void
  setRVInserts(std::vector<Point3D> points);

  //Output the LV Meshes with filenames starting with prefix
  //in the given directory
  void
  outputMesh(std::string prefix, std::string directory);

  //Get the exregion formatted mesh corresponding to the frame number frame
  std::string
  getMeshAt(int frame);
  /**
   * Gets the number of model frames.
   *
   * @return	The number of model frames.
   */
  int
  GetNumberOfModelFrames() const;

  //Get the transformation required to set the position of the mesh in the world coordinates
  std::string
  getTransformation();

  //Get the transformation required to set the position of the mesh in the world coordinates (array size is 16)
  double*
  getTransformationArray();

  //The the focal length associated with prolate spheroidal coordinate system
  double
  GetFocalLength() const;

  /**
   * Align model to given chamber volume based on apex, base and rv inserts information.
   */
  void
  AlignModel();

  /**
   * Converts a nodes rc position into a heart model prolate spheriodal coordinate.
   *
   * @param	node_id	   	The node identifier.
   * @param	region_name	Name of the region.
   *
   * @return	The position in a prolate shperiodal coordinate system.
   */
  std::vector<Point3D>
  ConvertToHeartModelProlateSpheriodalCoordinate(std::vector<Point3D>& point);

  /**
   * Sets a focal lengh.
   *
   * @param	focalLength	Length of the focal.
   */
  void
  SetFocalLength(double focalLength);

  //Marker coordinates are ordered from base_left and base_right, this sequence is repeated for each view
  //serially. The right base index specifies the array index where the right base index for the first view
  //is stored
  void
  setBaseRIndex(unsigned int idx);

  /*
   * Store the input markers in regions corresponding to time
   * /marker{?}
   */
  void
  embedInputMarkers();

  void
  fixWallLengths();

  void
  setPLSLambdas(std::vector<std::vector<double> >& lambdas, bool epiOnly = false);

  /*
   * The meshes are made to fit to the markers exactly, uses the initial fitting results as the starting template
   */
  void
  fixToMarkers();

  std::vector<double>
  getMyocardialVolumes();

  std::vector<double>
  getFrameTimes();

private:
  std::vector<Point3D>
  getMeshNodeValuesAt(int frame);

  void
  fixEndoNodes();

protected:

  int baseridx;
  Point3D apex;
  Point3D base;
  std::vector<Point3D> rvInserts;

  bool hasAPLAX;
  bool hasTCH;
  bool hasFCH;
  bool hasSAXApex;
  bool hasSAXMid;
  bool hasSAXBase;
  bool endoNodesUpdated;
  std::vector<int> aplaxNodes;
  std::vector<int> fchNodes;
  std::vector<int> tchNodes;
  std::vector<int> myNodes;
  std::vector<double> myocardialVolumes;
  std::vector<double> frameTimes;
  const static int NUMBER_OF_NODES = 98;

  double patientToGlobalTransform[16]; /**< The patient to global transform */
  std::string modelName_; /**< Name of the model */
  double focalLength;

  std::vector<Cmiss_node_id> cmiss_nodes;
  Cmiss_context_id context_;
  Cmiss_field_id field_;
  Cmiss_field_module_id field_module_;
  Cmiss_field_id ds1_;
  Cmiss_field_id ds2_;
  Cmiss_field_id ds1ds2_;
  Cmiss_field_id coordinates_ps_;
  Cmiss_field_id coordinates_rc_;
  Cmiss_field_id coordinates_patient_rc_;
  Cmiss_field_id transform_mx_;
  int numberOfModelFrames_; /**< Number of model frames */
  std::string contextName;
  bool aligned;
};

#endif /* LVHEARTMESH_H_ */
