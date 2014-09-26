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


#ifndef XMLSERIALISER_H_
#define XMLSERIALISER_H_
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include "tinyxml2.h"
#include "boost/filesystem.hpp"
#include "boost/algorithm/string.hpp"
#include "Point3D.h"

//View, Marker Type, frame_number
typedef std::map<std::string, std::map<std::string, std::map<int,std::vector<Point3D> > > > markerContainerType;

class XMLSerialiser {
	Point3D base;
	Point3D apex;
	double targetHeight;
	double targetWidth;
	std::string targetView;
	std::string srcFilename;
	std::vector<Point3D> rvInserts;
	markerContainerType markers;
	std::map<std::string,std::string> timeSampling;
	int numberOfFrames;
	double duration;
	tinyxml2::XMLDocument doc;
	void addCoordinate(tinyxml2::XMLElement* parent, Point3D point);
	void addMARK(tinyxml2::XMLElement* parent, int id, Point3D point);
public:
	XMLSerialiser(std::string filename);
	virtual ~XMLSerialiser();
	void setNumberOfFrames(int noOfFrames);
	void setDuration(double duration);
	void setApex(double x, double y, double z);
	void setBase(double x, double y, double z);
	void addRVInserts(double x, double y, double z);
	void setTimeSample(std::string view, std::string data);
	void addMarkers(std::string view, std::string type, int frame,std::vector<Point3D> points);
	void serialise(std::string modelName, std::string targetDir, std::string filename);
	void setTargetDim(double height, double width);
	void setTargetView(std::string target);
};

#endif /* XMLSERIALISER_H_ */
