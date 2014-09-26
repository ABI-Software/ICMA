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

#ifndef XMLINPUTREADER_H_
#define XMLINPUTREADER_H_

#include <vector>
#include <algorithm>
#include <map>
#include <cstdlib>
#include <sstream>
#include "tinyxml2.h"
#include "Point3D.h"
#include "Vector3D.h"
#include "alglib/optimization.h"
#include "CoordinateTransformer.h"
#include "SAXBasedCoordinateTransformer.h"
#include <boost/algorithm/string.hpp>

typedef struct {
	std::vector<Point3D>* markers;
	double* centroid;
} OptimInput;

class XMLInputReader {
private:
	tinyxml2::XMLDocument* XMLdoc;
	std::map<int, std::map<int, std::vector<Point3D>* >* > markers;	//Frame Number, View
	std::map<int, std::map<int, std::vector<Point3D>* >* > epiMarkers; //Store epi cardial markers
	std::map<int, std::map<int, int> > endoMarkerFlags;
	std::map<int, std::map<int, int> > epiMarkerFlags;
	std::map<int, std::vector<int>* > markerTypes; //Ordered list of marker type to enable
												  //appropriate assignment of xi locs and elem ids
	std::map<int, MatrixType*> transforms;
	std::map<int, std::string> planeTransforms;

	Point3D apex;
	Point3D base;
	Point3D gelem_centroid;
	Point3D sax_centroid;						//Centroid of the sax plane, computed
	std::vector<Point3D> rvInserts;
	unsigned int numberOfFrames;
	std::string outputDirectory;
	std::string modelName;
	std::string dicomFile;
	int targetView;
	bool transFormCoordinates;
	double duration;
	bool hasSAXApexView;
	bool hasSAXMidView;
	bool hasSAXBaseView;
	std::vector<double> frameSamplingTimes;
	Point3D getPointData(tinyxml2::XMLElement *pRoot);
	double getRotationAngle(int markerType);
	void getTargetApexBase(double* tap, double* tbl, double* tbr);
	MatrixType getTransformationMatrix(double *ap, double *bl, double *br,
			double rotationAngle);
	MatrixType getSaxTransformationMatrix(MarkerTypes type);
	MatrixType getAxisMatrix(double *ap, double *bl, double *br);

public:
	XMLInputReader(std::string filename);
	virtual ~XMLInputReader();

	Point3D getApex();
	Point3D getBase();
	std::vector<Point3D> getRVInserts();
	Point3D getSaxCentroid();
	void doNotTransformCoordinates();
	void transformCoordinates();
	std::vector<int> strainSelections;
	std::vector<double> getFrameSamplingTimes();
	std::map<int, std::vector<int>* > getMarkerTypes();
	unsigned int getNumberOfFrames();
	std::vector<Point3D> getMarkers(unsigned int frame,MarkerTypes type = APLAX,bool transformCoord = true);
	std::vector<int> getAllMarkerTypes(unsigned int frame);
	std::string getModelName();
	std::string getOutputDirectory();
	std::string getDICOMFile();
	std::map<int, std::string> getImagePlaneTransformations();
	MatrixType* getImagePlaneTransformation(MarkerTypes type);
	unsigned int getBaseRIndex();
	double getDuration();
	bool includesSAXView();
	std::vector<int> getStrainSelections();
};

#endif /* XMLINPUTREADER_H_ */
