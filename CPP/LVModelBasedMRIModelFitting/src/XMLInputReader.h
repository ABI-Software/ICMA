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
#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include "tinyxml2.h"
#include "Plane.h"

enum PlaneType {
	UNKNOWN = 0, LONG, SHORT, SHORTIGNORE
};

class XMLInputReader {
	std::map<std::string, std::vector<std::string> > frames;
	std::map<std::string, std::string> seriesid;
	std::map<std::string, double> edtime;
	std::map<std::string, double> estime;
	std::map<std::string, double> endcycletime;
	std::map<std::string, int> type;
	std::map<std::string, std::vector<Point3D> > edlm;
	std::map<std::string, std::vector<Point3D> > eslm;
	std::map<std::string, Plane> plane;
	Point3D apex;
	Point3D base;
	Point3D avgRVInsert;
	std::string outputDirectory;
	std::string modelName;
	std::string selectedLongAxisPlane;
	std::string selectedShortAxisPlane;
	std::string imageDirectory;
	std::string inputXML;
	Point3D getCoordinate(tinyxml2::XMLElement *pRoot);
public:
	XMLInputReader(std::string filename);
	virtual ~XMLInputReader();
	std::vector<std::string> getPlaneIds();
	std::vector<std::string> getFrameUrls(std::string planeid);
	std::map<std::string, int> getSeriesType();
	double getEDTime(std::string planeid);
	double getESTime(std::string planeid);
	double getEndOfCycleTime(std::string planeid);
	int getPlaneType(std::string planeid);
	std::vector<Point3D> getEndDistoleMarkers(std::string planeid);
	std::vector<Point3D> getEndSystoleMarkers(std::string planeid);
	Plane getCoordinatePlane(std::string planeid);
	std::string getSeriesId(std::string planeid);
	std::string getImageDirectory();
	std::string getOutputDirectory();
	std::string getModelName();
	std::string getXMLString();
	const Point3D& getApex();
	const Point3D& getAvgRvInsert();
	const Point3D& getBase();
	const std::string& getSelectedLongAxisPlane();
	const std::string& getSelectedShortAxisPlane();
	Point3D getPointOfIntersection(std::string planeid,Point3D apex, Point3D base);
	MatrixType getTransform(std::string planeid);
	double getImageHeight(std::string planeid);
	double getImageWidth(std::string planeid);
	double getTargetHeight(std::string planeid);
	double getTargetWidth(std::string planeid);
};

#endif /* XMLINPUTREADER_H_ */
