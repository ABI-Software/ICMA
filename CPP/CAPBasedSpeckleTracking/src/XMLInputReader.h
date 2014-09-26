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
#include <cmath>
#include <sstream>
#include <cstdlib>
#include <boost/algorithm/string.hpp>
#include "SegmentationAndFittingException.h"
#include "tinyxml2.h"


#define NUMBER_OF_LONG_AXIS_MARKERS 25

extern int maxcapframes;

class XMLInputReader {
private:

	tinyxml2::XMLDocument* XMLdoc;
	std::map<std::string,std::vector<double*> > markers;
	std::map<std::string,std::string > viewType;
	std::map<std::string,std::string > markerType;
	std::map<std::string,std::string> fileuri;
	std::map<std::string,int> ed;
	std::map<std::string,int> ec;
	std::map<std::string,int> es;
	std::map<std::string,std::vector<double*> > esbaseplane;
	std::map<std::string,double> bpm;
	double apex[3];
	double base[3];
	double targetWidth;
	double targetHeight;
	std::vector<double*> rvInserts;
	unsigned int numberOfFrames;
	std::string outputDirectory;
	std::string modelName;
	std::string targetView;
	std::string metaData;
	bool stripMetaTag;
	std::string recordIdentifier;
	bool createImages;
	std::string inputXMLFile;
	double*    getPointData(tinyxml2::XMLElement *pRoot);

public:
	XMLInputReader(std::string filename);
	virtual ~XMLInputReader();

	double* getApex();
	double* getBase();
	std::vector<double*>& getRVInserts();
	unsigned int getNumberOfFrames();
	std::map<std::string,std::vector<double*> > getMarkers();
	std::string getModelName();
	std::string getOutputDirectory();
	std::string getUri(std::string view);
	double getBPM(std::string view);
	double getTargetHeight();
	double getTargetWidth();
	int getED(std::string view);
	int getES(std::string view);
	int getESFrame(std::string view);
	std::vector<double*> getESBaseplane(std::string view);
	std::vector<std::string> getRecords();
	std::vector<std::string> getViews();
	std::vector<std::string> getMarkerTypes();

	bool createViewImages(){
		return createImages;
	}
	std::string getRecordIdentifier(){
		return recordIdentifier;
	}
	std::string getInputXMLFile(){
	  return inputXMLFile;
	}

};


#endif /* XMLINPUTREADER_H_ */
