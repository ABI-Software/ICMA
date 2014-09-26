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
#ifndef SINGLEPLANELVHEARTMESHMODEL_H_
#define SINGLEPLANELVHEARTMESHMODEL_H_

//These variable headers should be included once in the object
#include "heart.h"
#include "globalsmoothperframematrix.dat.h"
#include "globalmapbeziertohermite.dat.h"
#include "prior.dat.h"

#include "gmmfactory.h"
#include "vnlfactory.h"
#include "basis.h"


#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <ctime>
#include <cmath>
#include "../Point3D.h"
#include "../Vector3D.h"
#include "../Plane.h"

extern "C" {
#include "zn/cmgui_configure.h"
#include "zn/cmiss_context.h"
#include "zn/cmiss_field.h"
#include "zn/cmiss_element.h"
#include "zn/cmiss_region.h"
#include "zn/cmiss_node.h"
#include "zn/cmiss_stream.h"
#include "zn/cmiss_field_module.h"
#include "zn/cmiss_field_constant.h"
#include "zn/cmiss_field_composite.h"
#include "zn/cmiss_field_matrix_operators.h"
#include "zn/cmiss_field_finite_element.h"
#include "zn/cmiss_element.h"
#include "zn/cmiss_time_keeper.h"
#include "zn/cmiss_field_time.h"
}

#include "timesmoother.h"
#include "filesystem.h"

#include "solverlibraryfactory.h"

#include "../alglib/interpolation.h"

#include "vnl/vnl_matrix.h"
#include "vnl/algo/vnl_matrix_inverse.h"

typedef vnl_matrix<double> MatrixType;

typedef std::vector<Point3D> PointVector;

enum ViewTypeEnum {
	APLAX, TCH, FCH, SAXBASE, SAXMID, SAXAPEX
};

class SinglePlaneLVHeartMeshModel {
public:
	//Default constructor
	SinglePlaneLVHeartMeshModel(unsigned int numFrames, int view, unsigned int maxSpecklesPerSide);
	virtual
	~SinglePlaneLVHeartMeshModel();

	//Set the apex coordinate for the LV Mesh
	void
	setApex(Point3D apex);

	//Set the baseL coordinate for the LV Mesh
	void
	setBaseL(Point3D base);

	//Set the baseR coordinate for the LV Mesh
	void
	setBaseR(Point3D base);

	void setESFrameData(double esp, std::vector<Point3D> bpts);

	//Get the apex coordinate for the LV Mesh
	Point3D
	getApex();

	//Get the baseL coordinate for the LV Mesh
	Point3D
	getBaseL();

	//Get the baseR coordinate for the LV Mesh
	Point3D
	getBaseR();

	//Set the rv coordinates of the LV Mesh
	void
	setRVInserts(std::vector<Point3D> points);

	//Output the LV Meshes with filenames starting with prefix
	//in the given directory
	void
	outputMesh(std::string prefix, std::string directory);

	int GetNumberOfModelFrames() const {
		return numberOfModelFrames_;
	}

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
	 * Returns the potential RC coordinates locations at which the speckles for know
	 * xi may be found in the image at frame
	 */
	std::vector<Point3D>
	getSpeckleInitializers(double time, int* apexIndex,double wallxi=0.0);

	//Fit LV Mesh to the input marker data
	void
	FitModel(std::vector<Point3D> points, double time, bool useBoundaryXi = false);

	/**
	 * Returns the normalized times for frame positions to be extracted
	 * that coincide with appropriate base plane variation
	 */
	std::vector<double>
	getFramesFromApexBaseLengthVariation();

	/**
	 * Normalized ratio of apexbase vector length at frame
	 */
	double
	getApexBaseLengthRatioAt(int frame);

	int getViewType() {
		return viewType;
	}

	std::string getTimeSamplingInterval();

protected:
	/**
	 * Sets the lambda parameters for the given time.
	 *
	 * @param	lambdaParams	Options for controlling the lambda.
	 * @param	time			(optional) the time.
	 */
	void SetLambdaAtTime(const std::vector<double>& lambdaParams, double time = 0.0);

	/**
	 * Sets a mu from base plane for frame.
	 *
	 * @param	basePlane  	The base plane.
	 * @param	frameNumber	The frame number.
	 */

	void SetMuFromBasePlaneAtTime(const Plane& basePlane, double time);

	/**
	 * Projects a point to the model and computes the xi coords and the element id
	 * @param	position	the coordinate of the point
	 * @param	time	The time of the model to use
	 * @param xi the computed xi coord. (output)
	 *
	 * @return Id of the element that the point is projected onto
	 */
	int ComputeXi(const Point3D& position, double time, Point3D& xi) const;

	/**
	 * Sets a focal lengh.
	 *
	 * @param	focalLength	Length of the focal.
	 */
	void SetFocalLength(double focalLength);

	/**
	 * Converts a nodes rc position into a heart model prolate spheriodal coordinate.
	 *
	 * @param	node_id	   	The node identifier.
	 * @param	region_name	Name of the region.
	 *
	 * @return	The position in a prolate shperiodal coordinate system.
	 */
	std::vector<Point3D> ConvertToHeartModelProlateSpheriodalCoordinate(std::vector<Point3D>& point);

	/**
	 * Initialises the Bezier lambda parameters.
	 */
	void InitialiseBezierLambdaParams();

	/**
	 * Sets the theta values of nodes to be the standard values for given time
	 */
	void SetTheta(double time);

	/**
	 * Linear interpolation of plane normals and position along the cycle to capture
	 * base plane variation based on user input
	 */

	Plane InterpolateBasePlane(const std::map<int, Plane>& planes, int frame);

	/**
	 * Fit plane to base plane points.
	 *
	 * @param	basePlanePoints	The base plane points.
	 * @param	xAxis		   	The x coordinate axis.
	 *
	 * @return	The fitted plane.
	 */
	Plane
	FitPlaneToBasePlanePoints(const std::vector<std::pair<Point3D, double> >& basePlanePoints, const Vector3D& xAxis) const;

	/**
	 * Updates the time varying data points.
	 *
	 * @param	x		   	The vector of data points.
	 * @param	frameNumber	The frame number.
	 */
	void
	UpdateTimeVaryingDataPoints(const cap::Vector& x, int frameNumber);

	/**
	 * Updates the time varying model.
	 */
	void
	UpdateTimeVaryingModel();

	/**
	 * Smooth along time.
	 */
	void
	SmoothAlongTime();

	// convert Bezier params to hermite params to they can be fed to Cmgui
	std::vector<double>
	ConvertToHermite(const cap::Vector& bezierParams);

	alglib::spline1dinterpolant apexBaseVariation;

	cap::SparseMatrix* S_; /**< The s */
	cap::SparseMatrix* G_; /**< The g */
	cap::Preconditioner* preconditioner_; /**< The preconditioner */
	cap::GSmoothAMatrix* aMatrix_; /**< The matrix */
	cap::Vector* prior_; /**< The prior */
	cap::SparseMatrix* bezierToHermiteTransform_; /**< The bezier to hermite transform, temporary? */
	cap::SolverLibraryFactory* solverFactory_; /**< The solver factory */
	cap::TimeSmoother timeSmoother_; /**< The time smoother */

	std::vector<Cmiss_element_id> elements;
	std::vector<Cmiss_node_id> nodes;
	Cmiss_field_cache_id cache;
	Cmiss_context_id context_;
	Cmiss_field_id field_;
	Cmiss_field_module_id field_module_;
	Cmiss_field_id coordinates_ps_;
	Cmiss_field_id coordinates_rc_;
	Cmiss_field_id coordinates_patient_rc_;
	Cmiss_field_id transform_mx_;
	Point3D apex;
	Point3D basel;
	Point3D baser;

	std::vector<std::vector<double> > timeVaryingDataPoints_;
	std::vector<int> framesWithDataPoints;
	std::vector<Point3D> esBasePlane;
	std::vector<Point3D> rvInserts;
	std::vector<Point3D> speckleXi;
	std::vector<int> speckleElements;
	std::map<int, std::pair<Point3D, Point3D> > baseplanes;
	std::string contextName;
	std::string timeSamplingInterval;

	double patientToGlobalTransform[16]; /**< The patient to global transform */
	double focalLength;
	bool aligned;
	int baselElem;
	int baserElem;
	int numberOfModelFrames_; /**< Number of model frames */
	int viewType;
	int eOffset;
	double esf;				// Position of es frame in the range [0,1] where = 0 begin rwave, 1 = end rwave
	unsigned int MAXSPECKLESPERSIDE;
	unsigned int speckleApexIndex;
	int elementPtr[8];
};

#endif /* SINGLEPLANELVHEARTMESHMODEL_H_ */
