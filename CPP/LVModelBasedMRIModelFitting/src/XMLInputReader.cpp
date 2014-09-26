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
#include "XMLInputReader.h"

XMLInputReader::XMLInputReader(std::string filename) {
	tinyxml2::XMLDocument* XMLdoc = new tinyxml2::XMLDocument();

	if (XMLdoc->LoadFile(filename.c_str()) == tinyxml2::XML_SUCCESS) {
		tinyxml2::XMLElement *pRoot, *pParm, *pChild, *pFrames, *coord, *mark;
		pRoot = XMLdoc->FirstChildElement("SEGMENTATION");
		if (pRoot == NULL) {
			std::cout << "The input xml file does not have element SEGMENTATION " << filename << std::endl;
			throw -1;
		}
		if (pRoot) {
			outputDirectory = std::string("NOT FOUND");
			modelName = std::string("NOT_FOUND");
			pParm = pRoot->FirstChildElement("OUTPUTDIRECTORY");
			if (pParm) {
				outputDirectory = std::string(pParm->GetText());
			}

			pParm = pRoot->FirstChildElement("MODELNAME");
			if (pParm) {
				modelName = std::string(pParm->GetText());
			}

			pParm = pRoot->FirstChildElement("SERVER");
			if (pParm) {
				imageDirectory = std::string(pParm->GetText());
			}

			pParm = pRoot->FirstChildElement("PLANES");
			for (; pParm; pParm = pParm->NextSiblingElement()) {
				if (std::string(pParm->Name()) != "PLANES") {
					continue; //Only process if the tag is of type "MARKERS"
				}

				double edtime = -1.0, estime = -1.0, endcycle = -1.0;
				std::string planeid, series;
				unsigned int numframes = 0, type = 0;
				pChild = pParm->FirstChildElement("EDTIME");
				if (pChild != 0) {
					edtime = atof(pChild->GetText());
				}
				pChild = pParm->FirstChildElement("ESTIME");
				if (pChild != 0) {
					estime = atof(pChild->GetText());
				}
				pChild = pParm->FirstChildElement("ENDCYCLE");
				if (pChild != 0) {
					endcycle = atof(pChild->GetText());
				}

				pChild = pParm->FirstChildElement("PLANEID");
				if (pChild != 0) {
					planeid = std::string(pChild->GetText());
				}
				pChild = pParm->FirstChildElement("SERIESID");
				if (pChild != 0) {
					series = std::string(pChild->GetText());
				}
				pChild = pParm->FirstChildElement("TYPE");
				if (pChild != 0) {
					std::string tl = std::string(pChild->GetText());
					if (tl == "long" || tl == "LONG") {
						type = 1;
					}
					if (tl == "short" || tl == "SHORT") {
						type = 2;
					}
				}

				this->edtime[planeid] = edtime;
				this->estime[planeid] = estime;
				this->endcycletime[planeid] = endcycle;
				this->type[planeid] = type;
				this->seriesid[planeid] = series;
				pChild = pParm->FirstChildElement("FRAMES");
				if (pChild != 0) {
					numframes = 0;
					pFrames = pChild->FirstChildElement("NUMFRAMES");
					if (pFrames != 0) {
						numframes = atoi(pFrames->GetText());
					}
					if (numframes < 1)
						numframes = 1;
					std::vector<std::string> urls(numframes); // handle numframes is zero
					Point3D tlc, trc, blc, brc;
					double targetHeight = 256.0, targetWidth = 256.0;
					pFrames = pChild->FirstChildElement("FRAME");
					for (; pFrames; pFrames = pFrames->NextSiblingElement()) {
						if (std::string(pFrames->Name()) == "FRAME") {
							int id = -1;
							if (pFrames->Attribute("ID")) {
								id = atoi(pFrames->Attribute("ID"));
							}
							urls[id] = std::string(pFrames->GetText());
						} else {

							if (std::string(pFrames->Name()) == "TRC") {
								trc = getCoordinate(pFrames);
							}
							if (std::string(pFrames->Name()) == "TLC") {
								tlc = getCoordinate(pFrames);
							}
							if (std::string(pFrames->Name()) == "BRC") {
								brc = getCoordinate(pFrames);
							}
							if (std::string(pFrames->Name()) == "BLC") {
								blc = getCoordinate(pFrames);
							}
							if (std::string(pFrames->Name()) == "TARGETHEIGHT") {
								targetHeight = atof(pFrames->GetText());
							}
							if (std::string(pFrames->Name()) == "TARGETWIDTH") {
								targetWidth = atof(pFrames->GetText());
							}
						}
					}
					pFrames = pChild->FirstChildElement("MARKERS");
					this->frames[planeid] = urls;
					this->plane[planeid] = Plane(tlc, trc, blc, brc, targetHeight, targetWidth);
					if (pFrames) {
						std::vector<Point3D> edlm;
						std::vector<Point3D> eslm;
						coord = pFrames->FirstChildElement("EDLM");
						if (coord) {
							mark = coord->FirstChildElement("MARK");
							for (; mark; mark = mark->NextSiblingElement()) {
								edlm.push_back(getCoordinate(mark));
							}
						}
						coord = pFrames->FirstChildElement("ESLM");
						if (coord) {
							mark = coord->FirstChildElement("MARK");
							for (; mark; mark = mark->NextSiblingElement()) {
								eslm.push_back(getCoordinate(mark));
							}
						}
						this->edlm[planeid] = edlm;
						this->eslm[planeid] = eslm;
					}
				}
			}
		}
		std::map<std::string, int>::iterator start, end;
		start = type.begin();
		end = type.end();
		std::vector<std::string> pid;
		std::vector<Point3D> pbase;
		std::vector<Point3D> papex;
		std::vector<Point3D> prv;
		std::vector<double> abl;

		for (; start != end; ++start) {
			std::string planeid = start->first;
			if (start->second == LONG) { //Long Axis
				std::vector<Point3D> ed = edlm[planeid];
				Point3D mbase = (ed[0] + ed[2]) * 0.5;
				Point3D mapex = ed[1];
				double apexbaselength = mbase.distance(mapex);
				pid.push_back(planeid);
				pbase.push_back(mbase);
				papex.push_back(mapex);
				prv.push_back(ed[0]);
				abl.push_back(apexbaselength);
			}
		}
		double temp = abl[0];
		unsigned int ti = 0;
		for (int i = 1; i < abl.size(); i++) {
			if (temp < abl[i]) {
				temp = abl[i];
				ti = i;
			}
		}
		selectedLongAxisPlane = pid[ti];
		selectedShortAxisPlane = "";
		apex = papex[ti];
		base = pbase[ti];
		avgRVInsert = prv[ti];

		//Determine short axis planes
		Plane& laxPlane = plane[selectedLongAxisPlane];
		Vector3D laxNormal(laxPlane.normal);

		Point3D selBase = getTransform(selectedLongAxisPlane) * base;
		Point3D selApex = getTransform(selectedLongAxisPlane) * apex;
		double abdist = selBase.distance(selApex);
		start = type.begin();
		for (; start != end; ++start) {
			std::string planeid = start->first;
			if (start->second == UNKNOWN) { //Short Axis
				Plane& imgPlane = plane[planeid];
				Point3D poi = getPointOfIntersection(planeid, selApex, selBase);
				double dotp = laxNormal * imgPlane.normal;
				if (dotp < 0.2) { //Short axis, for long axis it is close to 1.0
					start->second = SHORTIGNORE;
					dotp = 100.0 * selBase.distance(poi) / abdist;
					if (dotp > 30) {
						dotp = 100.0 * selApex.distance(poi) / abdist;
						if (dotp > 20) {
							start->second = SHORT;
						}
					}
				}
			}
		}

		pid.clear();
		abl.clear();
		papex.clear();

		start = type.begin();
		for (; start != end; ++start) {
			std::string planeid = start->first;
			if (start->second == SHORT) { //Short Axis
				std::vector<Point3D> ed = edlm[planeid];
				if (ed.size() > 0) {
					Point3D rv = (ed[0] + ed[1]) * 0.5;
					double bl = base.distance(rv);
					pid.push_back(planeid);
					abl.push_back(bl);
					papex.push_back(rv);
				}
			}
		}
		if (pid.size() > 0) {
			temp = abl[0]; //Find the closest one to the base
			ti = 0;
			for (int i = 1; i < abl.size(); i++) {
				if (temp > abl[i]) {
					temp = abl[i];
					ti = i;
				}
			}
			avgRVInsert = papex[ti];
			selectedShortAxisPlane = pid[ti];
		}
		tinyxml2::XMLPrinter printer1;
		XMLdoc->Print(&printer1);
		inputXML = std::string(printer1.CStr());
	} else {
		std::cerr << "Unable to read input xml file " << std::endl;
		throw -1;
	}
	delete XMLdoc;
}

XMLInputReader::~XMLInputReader() {

}

Point3D XMLInputReader::getCoordinate(tinyxml2::XMLElement * pFrames) {
	double point[3];
	tinyxml2::XMLElement* coord;

	point[0] = 0;
	point[1] = 0;
	point[2] = 0;
	coord = pFrames->FirstChildElement("x");
	if (coord) {
		point[0] = atof(coord->GetText());
		if (fabs(point[0]) < 1.0e-6)
			point[0] = 0.0;
	}
	coord = pFrames->FirstChildElement("y");
	if (coord) {
		point[1] = atof(coord->GetText());
		if (fabs(point[1]) < 1.0e-6)
			point[1] = 0.0;
	}
	coord = pFrames->FirstChildElement("z");
	if (coord) {
		point[2] = atof(coord->GetText());
		if (fabs(point[2]) < 1.0e-6)
			point[2] = 0.0;
	}
	return Point3D(point);

}

std::vector<std::string> XMLInputReader::getPlaneIds() {
	std::map<std::string, std::vector<std::string> >::iterator start, end;
	start = frames.begin();
	end = frames.end();
	std::vector<std::string> ids;
	while (start != end) {
		ids.push_back(start->first);
		++start;
	}
	return ids;
}

std::vector<std::string> XMLInputReader::getFrameUrls(std::string planeid) {
	return frames[planeid];
}

double XMLInputReader::getEDTime(std::string planeid) {
	return edtime[planeid];
}

double XMLInputReader::getESTime(std::string planeid) {
	return estime[planeid];
}

double XMLInputReader::getEndOfCycleTime(std::string planeid) {
	return endcycletime[planeid];
}

int XMLInputReader::getPlaneType(std::string planeid) {
	return type[planeid];
}

std::vector<Point3D> XMLInputReader::getEndDistoleMarkers(std::string planeid) {
	return edlm[planeid];
}

std::vector<Point3D> XMLInputReader::getEndSystoleMarkers(std::string planeid) {
	return eslm[planeid];
}

Plane XMLInputReader::getCoordinatePlane(std::string planeid) {
	return plane[planeid];
}

std::string XMLInputReader::getImageDirectory() {
	return imageDirectory;
}

std::string XMLInputReader::getOutputDirectory() {
	return outputDirectory;
}

std::string XMLInputReader::getModelName() {
	return modelName;
}

const Point3D& XMLInputReader::getApex() {
	return apex;
}

const Point3D& XMLInputReader::getAvgRvInsert() {
	return avgRVInsert;
}

const Point3D& XMLInputReader::getBase() {
	return base;
}

const std::string& XMLInputReader::getSelectedLongAxisPlane() {
	return selectedLongAxisPlane;
}

const std::string& XMLInputReader::getSelectedShortAxisPlane() {
	return selectedShortAxisPlane;
}

MatrixType XMLInputReader::getTransform(std::string planeid) {
	return plane[planeid].transform;
}

double XMLInputReader::getImageHeight(std::string planeid) {
	return plane[planeid].iheight;
}

double XMLInputReader::getImageWidth(std::string planeid) {
	return plane[planeid].iwidth;
}

double XMLInputReader::getTargetHeight(std::string planeid) {
	return plane[planeid].pheight;
}

double XMLInputReader::getTargetWidth(std::string planeid) {
	return plane[planeid].pwidth;
}

Point3D XMLInputReader::getPointOfIntersection(std::string planeid, Point3D apex, Point3D base) {
	Plane& saxPlane = plane[planeid];
	Vector3D pn = saxPlane.normal;
	Point3D pp = saxPlane.position;
	Vector3D apexbase = base - apex;

	double tx = pn.x * (base.x - apex.x);
	double tcx = pn.x * (apex.x - pp.x);
	double ty = pn.y * (base.y - apex.y);
	double tcy = pn.y * (apex.y - pp.y);
	double tz = pn.z * (base.z - apex.z);
	double tcz = pn.z * (apex.z - pp.z);

	double rhs = tcx + tcy + tcz;
	double lhs = tx + ty + tz;
	double t = -rhs / lhs;

	return (apex + t * apexbase);
}

std::string XMLInputReader::getXMLString() {
	return inputXML;
}

std::string XMLInputReader::getSeriesId(std::string planeid) {
	return seriesid[planeid];
}

std::map<std::string, int> XMLInputReader::getSeriesType() {
	std::map<std::string, int> stypes;
	std::map<std::string, int>::iterator start, end;
	start = type.begin();
	end = type.end();
	while(start!=end){
		stypes[seriesid[start->first]] = start->second;
		++start;
	}
	return stypes;
}
