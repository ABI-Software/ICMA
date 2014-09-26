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

void optimization(const alglib::real_1d_array& x, alglib::real_1d_array& cost, void* ptr) {
	OptimInput* input = static_cast<OptimInput*>(ptr);
	std::vector<Point3D>* ellipse_markers = input->markers;
	//Ellipse equation
	std::vector<Point3D>::iterator start = ellipse_markers->begin();
	const std::vector<Point3D>::iterator end = ellipse_markers->end();
	double a = x[0];
	double b = x[1];
	double xa = input->centroid[0];
	double ya = input->centroid[1];

	double err = 0;
	for (; start != end; ++start)
	{
		Point3D& mp = *start;
		double x = (mp.x - xa) / a;
		double y = (mp.y - ya) / b;
		err += (x * x + y * y - 1);
	}

	cost[0] = err;
}

MatrixType getEigenVectors(MatrixType& mat) {
	vnl_vector<double> eV(4, 0.0);
	MatrixType eigV(4, 4, 0.0);
	vnl_symmetric_eigensystem_compute(mat, eigV, eV);
	return eigV;
}

void printMatrix(MatrixType& mat, std::string title) {
	std::cout << title << "\n";
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (fabs(mat[i][j]) > 1.0e-4)
				std::cout << mat[i][j] << " ";
			else
				std::cout << "0.0 ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

XMLInputReader::XMLInputReader(std::string filename) :
		transFormCoordinates(true) {
	XMLdoc = new tinyxml2::XMLDocument();
	targetView = -1;
	hasSAXApexView = false;
	hasSAXMidView = false;
	hasSAXBaseView = false;
	//Set gelement centroid to standard gelement
	gelem_centroid.x = 318.0;
	gelem_centroid.y = 212.0;
	gelem_centroid.z = 0.0;
	sax_centroid = Point3D(0, 0, 0);
    strainSelections = std::vector<int>(17,1);
	if (XMLdoc->LoadFile(filename.c_str()) == tinyxml2::XML_SUCCESS)
	{
		tinyxml2::XMLElement *pRoot, *pParm;
		pRoot = XMLdoc->FirstChildElement("SEGMENTATION");
		if (pRoot)
		{
			pParm = pRoot->FirstChildElement("OUTPUTDIRECTORY");
			outputDirectory = std::string("NOT FOUND");
			modelName = std::string("NOT_FOUND");
			if (pParm)
			{
				outputDirectory = std::string(pParm->GetText());

				pParm = pRoot->FirstChildElement("MODELNAME");
				if (pParm)
				{
					modelName = std::string(pParm->GetText());
				}
			}
			pParm = pRoot->FirstChildElement("NUMBEROFFRAMES");
			if (pParm)
			{
				numberOfFrames = atoi(pParm->GetText());
			}
			duration = 0.0;
			pParm = pRoot->FirstChildElement("DURATION");
			if (pParm)
			{
				duration = (double) atof(pParm->GetText());
			}

			pParm = pRoot->FirstChildElement("TRANSFORM");
			if (pParm)
			{
				std::string transform(pParm->GetText());
				boost::to_upper(transform);
				if (transform == "FALSE")
				{
					transFormCoordinates = false;
				}
			}

			pParm = pRoot->FirstChildElement("DICOMFILE");
			if (pParm)
			{
				dicomFile = std::string(pParm->GetText());
			}
			else
			{
				dicomFile = std::string("");
			}

			pParm = pRoot->FirstChildElement("TARGETVIEW");
			if (pParm)
			{
				std::string target(pParm->GetText());
				boost::to_upper(target);
				boost::trim(target);
				if (target == "APLAX")
				{
					targetView = APLAX;
				}
				else if (target == "TCH")
				{
					targetView = TCH;
				}
				else if (target == "FCH")
				{
					targetView = FCH;
				}
				std::cout << "Target View " << target << std::endl;
			}
			pParm = pRoot->FirstChildElement("BASE");
			if (pParm)
			{
				base = getPointData(pParm);
			}
			pParm = pRoot->FirstChildElement("APEX");
			if (pParm)
			{
				apex = getPointData(pParm);
			}

			pParm = pRoot->FirstChildElement("RVINSERTS");
			if (pParm)
			{
				tinyxml2::XMLNode* child = pParm->FirstChild();
				while (child != 0)
				{
					Point3D rvInsert = getPointData(child->ToElement());
					rvInserts.push_back(rvInsert);
					child = child->NextSibling();
				}
			}
			pParm = pRoot->FirstChildElement("GELEM_CENTROID");
			if (pParm)
			{
				gelem_centroid = getPointData(pParm);
			}
			//get strain selection data
			pParm = pRoot->FirstChildElement("SELECTEDSTRAINS");
			if (pParm)
			{
				strainSelections = std::vector<int>(19,0);
				int ctr =0;
				tinyxml2::XMLNode* str = pParm->FirstChild();
				while(str!=0){
					tinyxml2::XMLElement* elemi = str->ToElement();
					if (std::string(elemi->Name()) != "STRAIN"){
						str = str->NextSibling();
						continue;
					}
					int selection = atoi(elemi->GetText());
					strainSelections[selection-1] = 1;
					str = str->NextSibling();
					ctr++;
				}
				if(ctr==0){//Choose all strains
					strainSelections = std::vector<int>(19,1);
				}else{
					strainSelections[18] = 1; //Average
				}
			}

			pParm = pRoot->FirstChildElement("MARKERS");
			if (!pParm)
			{
				std::cout << "Input file does not have markers quitting " << filename << std::endl;
				throw -1;
			}

			for (; pParm; pParm = pParm->NextSiblingElement())
			{
				try
				{
					if (std::string(pParm->Name()) != "MARKERS")
						continue;
					std::string nodetype(pParm->Attribute("VIEW"));
					boost::to_upper(nodetype);
					int frameNumber = 0;
					pParm->QueryIntAttribute("FRAME", &frameNumber);
					int noOfMarkers = 0;
					pParm->QueryIntAttribute("NOOFMARKERS", &noOfMarkers);
					int markerType = 0; // 0 == ENDO (default) 1 == EPI

					if (pParm->Attribute("TYPE") != NULL)
					{
						std::string type(pParm->Attribute("TYPE"));
						boost::to_upper(type);
						if (type == "EPI")
							markerType = 1;
					}
					std::vector<Point3D> fmarkers(noOfMarkers);
					tinyxml2::XMLNode* child = pParm->FirstChild();

					while (child != 0)
					{
						int myID = -1; //marker ids start from 0
						tinyxml2::XMLElement* elem = child->ToElement();
						elem->QueryIntAttribute("ID", &myID);
						if (myID > -1)
						{
							Point3D point = getPointData(elem);
							fmarkers[myID] = point;
						}
						child = child->NextSibling();
					}
					int type = APLAX; //Defaults to APLAX
					if (nodetype == "APLAX")
					{
						type = APLAX;
					}
					else if (nodetype == "TCH")
					{
						type = TCH;
					}
					else if (nodetype == "FCH")
					{
						type = FCH;
					}
					else if (nodetype == "SAXAPEX")
					{
						type = SAXAPEX;
						hasSAXApexView = true;
					}
					else if (nodetype == "SAXMID")
					{
						type = SAXMID;
						hasSAXMidView = true;
					}
					else if (nodetype == "SAXBASE")
					{
						type = SAXBASE;
						hasSAXBaseView = true;
					}

					if (markerTypes.find(frameNumber) == markerTypes.end())
					{
						markerTypes[frameNumber] = new std::vector<int>();
					}
					markerTypes[frameNumber]->push_back(type);

					if (markerType == 0)
					{
						endoMarkerFlags[frameNumber][type] = 1;
						if (markers.find(frameNumber) == markers.end()) //Not found
						{
							std::map<int, std::vector<Point3D>*>* newMap = new std::map<int, std::vector<Point3D>*>();
							(*newMap)[type] = new std::vector<Point3D>(fmarkers);
							markers[frameNumber] = newMap;
						}
						else
						{ //Append to the list
							std::map<int, std::vector<Point3D>*>* Map = markers[frameNumber];
							(*Map)[type] = new std::vector<Point3D>(fmarkers);
						}
					}
					else
					{
						epiMarkerFlags[frameNumber][type] = markerType;
						if (epiMarkers.find(frameNumber) == epiMarkers.end()) //Not found
						{
							std::map<int, std::vector<Point3D>*>* newMap = new std::map<int, std::vector<Point3D>*>();
							(*newMap)[type] = new std::vector<Point3D>(fmarkers);
							epiMarkers[frameNumber] = newMap;
						}
						else
						{ //Append to the list
							std::map<int, std::vector<Point3D>*>* Map = epiMarkers[frameNumber];
							(*Map)[type] = new std::vector<Point3D>(fmarkers);
						}
					}
#ifdef debug
					std::cout<<"Marker frame "<<frameNumber<<" view "<<nodetype<<" Number of Markers "<<noOfMarkers<<std::endl;
					for(int i=0;i<noOfMarkers;i++)
					{
						std::cout<<"["<<i+1<<"] "<<fmarkers[i]<<" ";
					}
					std::cout<<std::endl;
#endif
				} catch (std::exception& ex)
				{
					std::cout << " The input xml file does not seem to be in the appropriate format ";
					std::cout << ex.what() << std::endl;
					throw ex;
				}
			}
			unsigned int numMarkerTypes = markerTypes[0]->size();

			//Check if targetView has been set if not set it to APLAX if available or the first marker type
			if (targetView < 1)
			{
				for (int i = 0; i < numMarkerTypes; i++)
				{ //Check for aplax
					if (markerTypes[0]->at(i) == APLAX)
					{
						targetView = APLAX;
					}
				}
				if (targetView < 1)
				{
					targetView = markerTypes[0]->at(0);
				}
			}

			//Check for FRAMESAMPLING information
			frameSamplingTimes.resize(numberOfFrames);
			pParm = pRoot->FirstChildElement("FRAMEVECTORS");
			if (pParm)
			{
				bool found = false;
				const char* viewnames[] =
				{ "", "APLAX", "TCH", "FCH", "SAXBASE", "SAXMID", "SAXAPEX" };
				std::string targetViewName(viewnames[targetView]);
				pParm = pParm->FirstChildElement("NORMALIZEDFRAMEVECTOR");
				for (; pParm; pParm = pParm->NextSiblingElement())
				{
					std::string fview(pParm->Attribute("VIEW"));
					if (fview == targetViewName)
					{
						std::string time(pParm->GetText());
						std::vector<std::string> values;
						boost::split(values, time, boost::is_any_of(","));
						if (values.size() == numberOfFrames)
						{
							for (int vc = 0; vc < values.size(); vc++)
							{
								frameSamplingTimes[vc] = atof(values[vc].c_str());
							}
							found = true;
						}
					}
					if (!found)
					{
						double denom = numberOfFrames - 1.0;
						for (unsigned int vi = 0; vi < numberOfFrames; vi++)
						{
							frameSamplingTimes[vi] = vi / denom;
						}
					}
				}

			}
			else
			{ //Uniformly sample the frames
				double denom = numberOfFrames - 1.0;
				for (unsigned int vi = 0; vi < numberOfFrames; vi++)
				{
					frameSamplingTimes[vi] = vi / denom;
				}
			}

			//Compute the transformation matrices
			double targetApex[3], targetBaseLeft[3], targetBaseRight[3];
			for (int i = 0; i < numMarkerTypes; i++)
			{
				int type = markerTypes[0]->at(i);
#ifdef print_coord
				std::cout<<"<MARKERS VIEW=\"APLAX\" FRAME=\""<<frame<<"\" NOOFMARKERS=\"9\">"<<std::endl;
#endif

				try
				{
					if (type < SAXAPEX)
					{
						std::vector<Point3D>& ffPoints(*((*markers[0])[type])); //Use the first frame only

						targetApex[0] = ffPoints[apexid].x;
						targetApex[1] = ffPoints[apexid].y;
						targetApex[2] = ffPoints[apexid].z;
						targetBaseLeft[0] = ffPoints[baselid].x;
						targetBaseLeft[1] = ffPoints[baselid].y;
						targetBaseLeft[2] = ffPoints[baselid].z;
						targetBaseRight[0] = ffPoints[baserid].x;
						targetBaseRight[1] = ffPoints[baserid].y;
						targetBaseRight[2] = ffPoints[baserid].z;

						MatrixType transform = getTransformationMatrix(targetApex, targetBaseLeft, targetBaseRight, getRotationAngle(type));

#ifdef print_trans_matrix
						std::cout<<"Transformation matrix "<<type<<std::endl;
						std::cout<<transform[0][0]<<" "<<transform[0][1]<<" "<<transform[0][2]<<" "<<transform[0][3]<<std::endl;
						std::cout<<transform[1][0]<<" "<<transform[1][1]<<" "<<transform[1][2]<<" "<<transform[1][3]<<std::endl;
						std::cout<<transform[2][0]<<" "<<transform[2][1]<<" "<<transform[2][2]<<" "<<transform[2][3]<<std::endl;
						std::cout<<transform[3][0]<<" "<<transform[3][1]<<" "<<transform[3][2]<<" "<<transform[3][3]<<std::endl;
#endif

						MatrixType * myTrans = new MatrixType(transform);
						transforms[type] = myTrans;
						//Store the cmgui format
						std::ostringstream ss;
						double mx[16];
						int mxctr = 0;
						for (int k = 0; k < 4; k++)
						{
							for (int j = 0; j < 4; j++)
							{
								mx[mxctr++] = transform[k][j];
							}
						}

						for (int k = 0; k < 16; k++)
							ss << " " << mx[k];
						planeTransforms[type] = ss.str();
					}
				} catch (std::exception& ex)
				{
					std::string viewName;
					switch (type)
					{
					case APLAX:
						viewName = "APLAX";
						break;
					case TCH:
						viewName = "TCH";
						break;
					case FCH:
						viewName = "FCH";
						break;
					}
					std::cout << "The first frames of each long axis view is expected to have endo cardial markers" << " this input file doesnt seem to have them for " << viewName
							<< ". Terminating";
					throw ex;
				}
			}
			bool saxviews[] = {hasSAXApexView, hasSAXMidView, hasSAXBaseView};
			MarkerTypes saxmarkers[] = {SAXAPEX,SAXMID,SAXBASE};
			for(int svc=0;svc<3;svc++)
			if (saxviews[svc])
			{

				try
				{

					std::vector<Point3D>& ffPoints(*((*markers[0])[saxmarkers[svc]])); //Use the first frame only
					unsigned int nPoints = ffPoints.size();
					sax_centroid = Point3D(0, 0, 0);
					for (int i = 0; i < nPoints; i++)
					{
						sax_centroid = sax_centroid + ffPoints[i];
					}

					sax_centroid = sax_centroid * (1.0/nPoints);

					sax_centroid.z = 0.0;
					MatrixType transform = getSaxTransformationMatrix(saxmarkers[svc]);

#ifdef print_trans_matrix
					std::cout<<"Transformation matrix "<<type<<std::endl;
					std::cout<<transform[0][0]<<" "<<transform[0][1]<<" "<<transform[0][2]<<" "<<transform[0][3]<<std::endl;
					std::cout<<transform[1][0]<<" "<<transform[1][1]<<" "<<transform[1][2]<<" "<<transform[1][3]<<std::endl;
					std::cout<<transform[2][0]<<" "<<transform[2][1]<<" "<<transform[2][2]<<" "<<transform[2][3]<<std::endl;
					std::cout<<transform[3][0]<<" "<<transform[3][1]<<" "<<transform[3][2]<<" "<<transform[3][3]<<std::endl;
#endif


					MatrixType * myTrans = new MatrixType(transform);
					if (transforms.find(saxmarkers[svc]) != transforms.end() && transforms[saxmarkers[svc]] != NULL)
						delete transforms[saxmarkers[svc]];
					transforms[saxmarkers[svc]] = myTrans;
					//Store the cmgui format
					std::ostringstream ss;
					double mx[16];
					int mxctr = 0;
					for (int k = 0; k < 4; k++)
					{
						for (int j = 0; j < 4; j++)
						{
							mx[mxctr++] = transform[k][j];
						}
					}

					for (int k = 0; k < 16; k++)
						ss << " " << mx[k];

					planeTransforms[saxmarkers[svc]] = ss.str();

				} catch (std::exception& ex)
				{
					std::cout << "The first frames of each short axis view is expected to have endo cardial markers this input file doesnt seem to have them" << ". Terminating";
					throw ex;
				}
			}

		}
	}
	else
	{
		std::cout << "Unable to load the input file " << filename << std::endl;
		throw -1;
	}
}

XMLInputReader::~XMLInputReader() {
	if (XMLdoc)
		delete XMLdoc;
	{
		std::map<int, std::map<int, std::vector<Point3D>*>*>::iterator start = markers.begin();
		std::map<int, std::map<int, std::vector<Point3D>*>*>::iterator end = markers.end();
		while (start != end)
		{
			{
				std::map<int, std::vector<Point3D>*>::iterator vstart = start->second->begin();
				std::map<int, std::vector<Point3D>*>::iterator vend = start->second->end();
				while (vstart != vend)
				{
					delete vstart->second;
					++vstart;
				}
			}
			delete start->second;
			++start;
		}
	}
	{
		std::map<int, std::map<int, std::vector<Point3D>*>*>::iterator start = epiMarkers.begin();
		std::map<int, std::map<int, std::vector<Point3D>*>*>::iterator end = epiMarkers.end();
		while (start != end)
		{
			{
				std::map<int, std::vector<Point3D>*>::iterator vstart = start->second->begin();
				std::map<int, std::vector<Point3D>*>::iterator vend = start->second->end();
				while (vstart != vend)
				{
					delete vstart->second;
					++vstart;
				}
			}
			delete start->second;
			++start;
		}
	}
	{
		std::map<int, std::vector<int>*>::iterator start = markerTypes.begin();
		std::map<int, std::vector<int>*>::iterator end = markerTypes.end();
		while (start != end)
		{
			delete start->second;
			++start;
		}
	}

	{
		std::map<int, MatrixType*>::iterator start = transforms.begin();
		std::map<int, MatrixType*>::iterator end = transforms.end();
		while (start != end)
		{
			delete start->second;
			++start;
		}
	}
}

Point3D XMLInputReader::getApex() {
	return apex;
}

Point3D XMLInputReader::getBase() {
	return base;
}

Point3D XMLInputReader::getSaxCentroid() {
	return sax_centroid;
}

std::vector<Point3D> XMLInputReader::getRVInserts() {
	return rvInserts;
}

std::map<int, std::vector<int>*> XMLInputReader::getMarkerTypes() {
	return markerTypes;
}

unsigned int XMLInputReader::getNumberOfFrames() {
	return numberOfFrames;
}

std::vector<Point3D> XMLInputReader::getMarkers(unsigned int frame, MarkerTypes type, bool transformCoord) {
	//Transform the coordinates according to the marker type and target Node
	std::vector<Point3D> result;
	if (transFormCoordinates && transformCoord)
	{
		double tap[3], tbl[3], tbr[3];
		if (endoMarkerFlags[frame][type] == 1)
		{
			{
				MatrixType transform(*(transforms[type]));
//#define checkTranformationError
#ifdef checkTranformationError
				MatrixType inverse = vnl_matrix_inverse<double>(transform);
#endif
				std::vector<Point3D>& myPoints(*((*markers[frame])[type]));
				unsigned int total_markers = myPoints.size();
				for (int j = 0; j < total_markers; j++)
				{
					double x = myPoints[j].x;
					double y = myPoints[j].y;
					double z = myPoints[j].z;
					double mx = transform[0][0] * x + transform[0][1] * y + transform[0][2] * z + transform[0][3];
					double my = transform[1][0] * x + transform[1][1] * y + transform[1][2] * z + transform[1][3];
					double mz = transform[2][0] * x + transform[2][1] * y + transform[2][2] * z + transform[2][3];
					Point3D newPoint(mx, my, mz);

#ifdef checkTranformationError
					//Check the mapping is correct
					x = newPoint.x;
					y = newPoint.y;
					z = newPoint.z;
					mx = inverse[0][0] * x + inverse[0][1] * y
					+ inverse[0][2] * z + inverse[0][3];
					my = inverse[1][0] * x + inverse[1][1] * y
					+ inverse[1][2] * z + inverse[1][3];
					mz = inverse[2][0] * x + inverse[2][1] * y
					+ inverse[2][2] * z + inverse[2][3];
					std::cout<<newPoint<<"\t"<<myPoints[j]<<"\t\t"<<myPoints[j].distance(newPoint)<<std::endl;
					std::cout<<myPoints[j].x<<"\t"<<mx<<"\t\t"<<fabs(mx-myPoints[j].x)<<std::endl;
					std::cout<<myPoints[j].y<<"\t"<<my<<"\t\t"<<fabs(my-myPoints[j].y)<<std::endl;
					std::cout<<myPoints[j].z<<"\t"<<mz<<"\t\t"<<fabs(mz-myPoints[j].z)<<std::endl;

#endif
					newPoint.setSelection(myPoints[j].isSelected());
					result.push_back(newPoint);
#ifdef print_coord
					std::cout<<"<MARK ID=\""<<j<<"\">"<<std::endl;
					std::cout<<"\t<x>"<<mx<<"</x>"<<std::endl;
					std::cout<<"\t<y>"<<my<<"</y>"<<std::endl;
					std::cout<<"\t<z>"<<mz<<"</z>"<<std::endl;
					std::cout<<"</MARK>"<<std::endl;
#endif
#ifdef print_trans_matrix
					std::cout<<" Transformed "<<myPoints[j]<<" -> "<<Point3D(mx,my,mz)<<std::endl;
#endif
				}
				//For the apex ensure that the value is that of the targetView's value
				//Note that since the transformation matrix is determined from the first frame, the apex's do not align in other frames
				if (type < SAXAPEX)
				{
					MatrixType transform(*(transforms[targetView]));
					int j = apexid;
					std::vector<Point3D>& myPoints(*((*markers[frame])[targetView]));
					double x = myPoints[j].x;
					double y = myPoints[j].y;
					double z = myPoints[j].z;
					double mx = transform[0][0] * x + transform[0][1] * y + transform[0][2] * z + transform[0][3];
					double my = transform[1][0] * x + transform[1][1] * y + transform[1][2] * z + transform[1][3];
					double mz = transform[2][0] * x + transform[2][1] * y + transform[2][2] * z + transform[2][3];
					result[j].x = mx;
					result[j].y = my;
					result[j].z = mz;
				}

#ifdef print_coord
				std::cout<<"</MARKERS>"<<std::endl;
#endif
			}
		}
	}
	else
	{
		if (endoMarkerFlags[frame][type] == 1)
		{ //If the frame has endocardial markers
			std::vector<Point3D>* frameMarkers = (*(markers[frame]))[type];
			std::vector<Point3D>& myPoints(*(frameMarkers));
			result.insert(result.begin(), myPoints.begin(), myPoints.end());
		}
	}
	return result;
}

std::string XMLInputReader::getModelName() {
	return modelName;
}

Point3D XMLInputReader::getPointData(tinyxml2::XMLElement* pRoot) {
	Point3D point;
	point.x = 0;
	point.y = 0;
	point.z = 0;
	bool selected = true;
	if (pRoot->QueryBoolAttribute("SELECT", &selected) != tinyxml2::XML_NO_ERROR)
	{
		selected = true;
	}
	point.setSelection(selected);
	tinyxml2::XMLElement *pParm = pRoot->FirstChildElement("x");
	if (pParm)
	{
		point.x = atof(pParm->GetText());
		if (fabs(point.x) < 1.0e-6)
			point.x = 0.0;
	}
	pParm = pRoot->FirstChildElement("y");
	if (pParm)
	{
		point.y = atof(pParm->GetText());
		if (fabs(point.y) < 1.0e-6)
			point.y = 0.0;
	}
	pParm = pRoot->FirstChildElement("z");
	if (pParm)
	{
		point.z = atof(pParm->GetText());
		if (fabs(point.z) < 1.0e-6)
			point.z = 0.0;
	}
	return point;
}

MatrixType XMLInputReader::getSaxTransformationMatrix(MarkerTypes type) {

	double tap[3], tbl[3], tbr[3];
	getTargetApexBase(tap, tbl, tbr);
	Point3D mapex(tap);
	Point3D mbasel(tbl);
	Point3D mbaser(tbr);
	Point3D base = (mbasel+mbaser)*0.5;
	Vector3D apexbase = mapex - base;


	double saxPlaneDistanceFromBase = 0.1;
	if(type==SAXAPEX){
		saxPlaneDistanceFromBase += 2.0/3.0;
	}else if (type==SAXMID){
		saxPlaneDistanceFromBase += 1.0/3.0;
	}
	Point3D poi = base + saxPlaneDistanceFromBase*apexbase;

	//Apex-base is taken to be x-axis
	MatrixType myAxis = getAxisMatrix(tap, tbl, tbr);

	double rotationAngle = M_PI / 2;
	double baseAngle = atan((tbl[1] - tbr[1]) / (tbl[0] - tbr[0]));
	double xrotationAngle = M_PI * 4.5 / 3.0;

	//Determine the pre and post offset
	MatrixType offset(4, 4);
	offset.fill(0.0);
	offset[0][0] = 1.0;
	offset[1][1] = 1.0;
	offset[2][2] = 1.0;
	offset[3][3] = 1.0;

	offset[0][3] = -poi.x;
	offset[1][3] = -poi.y;
	offset[2][3] = -poi.z;

	MatrixType nrotate(4, 4);
	nrotate.fill(0.0);
	//Around z
	nrotate[0][0] = 1.0;
	nrotate[1][1] = cos(rotationAngle);
	nrotate[1][2] = -sin(rotationAngle);
	nrotate[2][1] = sin(rotationAngle);
	nrotate[2][2] = cos(rotationAngle);
	nrotate[3][3] = 1.0;

	MatrixType brotate(4, 4);
	brotate.fill(0.0);
	//Around y - fix the base angle
	brotate[0][0] = 1.0;
	brotate[1][1] = cos(baseAngle);
	brotate[1][2] = -sin(baseAngle);
	brotate[2][1] = sin(baseAngle);
	brotate[2][2] = cos(baseAngle);
	brotate[3][3] = 1.0;


	MatrixType xrot(4, 4);
	xrot.fill(0.0);
	//Around x
	xrot[0][0] = cos(xrotationAngle);
	xrot[0][1] = -sin(xrotationAngle);
	xrot[1][0] = sin(xrotationAngle);
	xrot[1][1] = cos(xrotationAngle);
	xrot[2][2] = 1;
	xrot[3][3] = 1;

	MatrixType roffset = offset;

	roffset[0][3] = sax_centroid.x;
	roffset[1][3] = sax_centroid.y;
	roffset[2][3] = sax_centroid.z;

	MatrixType trans = roffset * brotate * xrot * nrotate * offset;

	MatrixType transformation = vnl_matrix_inverse<double>(trans);



#ifdef matrix_debug
	printMatrix(transformation,"Transformation");
#endif

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
		{
			if (fabs(transformation[i][j]) < 1.0e-6)
			{
				transformation[i][j] = 0.0;
			}
		}

	transforms[type] = new MatrixType(transformation);

	std::vector<std::vector<Point3D> > vMarkers;
	std::vector<Point3D> saxPts = getMarkers(0, type, true);

	for (int j = APLAX; j < SAXAPEX; j++)
	{
		std::vector<Point3D> pts = getMarkers(0, static_cast<MarkerTypes>(j), true);
		if (pts.size() > 0)
		{
			vMarkers.push_back(pts);
		}
	}
	//Transformation puts the image in the apexbase vector direction
	//Scale and translate along x,z to match the coordinates at saxPlaneDistanceFromBase
	SAXBasedCoordinateTransformer st(saxPts, vMarkers, saxPlaneDistanceFromBase);
	MatrixType optDelta = st.getTransform();
	MatrixType trans1 = optDelta * transformation;

	return trans1;
}

MatrixType XMLInputReader::getTransformationMatrix(double* ap, double* bl, double* br, double rotationAngle) {

	double tap[3], tbl[3], tbr[3];

	getTargetApexBase(tap, tbl, tbr);
	//Apex-base is taken to be x-axis
	//Base to RV as the y -axis
	MatrixType referenceAxis = getAxisMatrix(tap, tbl, tbr);
	MatrixType myAxis = getAxisMatrix(ap, bl, br);
	//The determinants of these matrices are 1

	Point3D targetApex = Point3D(tap);
	Point3D targetBase = (Point3D(tbl) + Point3D(tbr)) * 0.5;
	Point3D myApex = Point3D(ap);
	Point3D myBase = (Point3D(bl) + Point3D(br)) * 0.5;

	Vector3D tapv = targetBase - targetApex;
	Vector3D apv = myBase - myApex;
	Vector3D tbasev = Point3D(tbl) - Point3D(tbr);
	Vector3D basev = Point3D(bl) - Point3D(br);

	double abs = apv.Length() / tapv.Length();
	double bs = basev.Length() / tbasev.Length();

	MatrixType scale(4, 4, 0.0);
	scale.fill_diagonal(1.0);
	scale[2][2] = 1.0 / bs; //base change effected in Z axis when myAxis is taken to identity basis
	scale[0][0] = 1.0 / abs; //base change effected in X axis when myAxis is taken to identity basis

	//Determine the rotation required
	MatrixType rot(4, 4);
	rot.fill(0.0);

	//Around x
	rot[0][0] = 1.0;
	rot[1][1] = cos(rotationAngle);
	rot[1][2] = -sin(rotationAngle);
	rot[2][1] = sin(rotationAngle);
	rot[2][2] = cos(rotationAngle);
	rot[3][3] = 1.0;


	//Determine the pre and post offset
	MatrixType offset(4, 4);
	offset.fill(0.0);
	offset[0][0] = 1.0;
	offset[1][1] = 1.0;
	offset[2][2] = 1.0;
	offset[3][3] = 1.0;
	MatrixType roffset = offset;
	offset[0][3] = -referenceAxis[0][3];
	offset[1][3] = -referenceAxis[1][3];
	offset[2][3] = -referenceAxis[2][3];
	roffset[0][3] = referenceAxis[0][3];
	roffset[1][3] = referenceAxis[1][3];
	roffset[2][3] = referenceAxis[2][3];

	//Take myAxis it to Identity basis, scale

	MatrixType transformation = referenceAxis * rot * scale * vnl_matrix_inverse<double>(myAxis);

#ifdef matrix_debug
	printMatrix("Transformation", transformation);
#endif

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
		{
			if (fabs(transformation[i][j]) < 1.0e-6)
			{
				transformation[i][j] = 0.0;
			}
		}
#ifdef matrix_debug
//Compare transformation, does the apex match
	{
		double x =ap[0];
		double y = ap[1];
		double z = ap[2];
		double mx = transformation[0][0] * x + transformation[0][1] * y
		+ transformation[0][2] * z + transformation[0][3];
		double my = transformation[1][0] * x + transformation[1][1] * y
		+ transformation[1][2] * z + transformation[1][3];
		double mz = transformation[2][0] * x + transformation[2][1] * y
		+ transformation[2][2] * z + transformation[2][3];
		std::cout<<rotationAngle<<"\t"<<Point3D(x,y,z)<<"\t"<<Point3D(mx,my,mz)<<std::endl;
	}
#endif

	return transformation;
}


MatrixType XMLInputReader::getAxisMatrix(double* ap, double* bl, double* br) {

	//Determine my axis matrix
	Point3D apexPosition(ap[0], ap[1], 0.0);
	Point3D basePosition(0.5 * (bl[0] + br[0]), 0.5 * (bl[1] + br[1]), 0.0);
	Vector3D xAxis = apexPosition - basePosition;
	xAxis.Normalise();

	Point3D averageOfRVInserts(bl[0], bl[1], 0.0); //Should be the left base else pandora's box opens

	Vector3D yAxis = averageOfRVInserts - basePosition;
	Vector3D zAxis = CrossProduct(xAxis, yAxis);
	zAxis.Normalise();
	yAxis.CrossProduct(zAxis, xAxis);
	yAxis.Normalise();

	// Compute the position of the model coord origin. (1/3 of the way from base to apex)
	Point3D origin = basePosition + (0.3333) * (apexPosition - basePosition);

	MatrixType axisMat(4, 4);
	axisMat[0][0] = xAxis.x;
	axisMat[0][1] = xAxis.y;
	axisMat[0][2] = xAxis.z;
	axisMat[0][3] = origin.x;
	axisMat[1][0] = yAxis.x;
	axisMat[1][1] = yAxis.y;
	axisMat[1][2] = yAxis.z;
	axisMat[1][3] = origin.y;
	axisMat[2][0] = zAxis.x;
	axisMat[2][1] = zAxis.y;
	axisMat[2][2] = zAxis.z;
	axisMat[2][3] = origin.z;
	axisMat[3][0] = 0.0;
	axisMat[3][1] = 0.0;
	axisMat[3][2] = 0.0;
	axisMat[3][3] = 1.0;

	return axisMat;
}

double XMLInputReader::getRotationAngle(int markerType) {
	double result = 0;
	switch (markerType)
	{
	case FCH:
	{
		result = M_PI / 3.0; //120 degrees to APLAX
		break;
	}
	case TCH:
	{
		result = 30 * M_PI / 18;    //75 degrees to APLAX
		break;
	}
	case APLAX:
	{
		result = 0.0;
		break;
	}
	}
	return result;
}

void XMLInputReader::getTargetApexBase(double* tap, double* tbl, double* tbr) {
	std::vector<Point3D>* marks = (*markers[0])[targetView];

	tap[0] = (marks->at(apexid)).x;
	tap[1] = (marks->at(apexid)).y;
	tap[2] = (marks->at(apexid)).z;
	tbl[0] = (marks->at(baselid)).x;
	tbl[1] = (marks->at(baselid)).y;
	tbl[2] = (marks->at(baselid)).z;
	tbr[0] = (marks->at(baserid)).x;
	tbr[1] = (marks->at(baserid)).y;
	tbr[2] = (marks->at(baserid)).z;
}

std::string XMLInputReader::getOutputDirectory() {
	return outputDirectory;
}

std::string XMLInputReader::getDICOMFile() {
	return dicomFile;
}

std::map<int, std::string> XMLInputReader::getImagePlaneTransformations() {
	return planeTransforms;
}

unsigned int XMLInputReader::getBaseRIndex() {
	return baserid;
}

double XMLInputReader::getDuration() {
	return duration;
}

void XMLInputReader::doNotTransformCoordinates() {
	transFormCoordinates = false;
}

void XMLInputReader::transformCoordinates() {
	transFormCoordinates = true;
}

MatrixType* XMLInputReader::getImagePlaneTransformation(MarkerTypes type) {
	if (transforms.find(type) != transforms.end())
	{
		return transforms[type];
	}
	else
	{
		return NULL;
	}
}


bool XMLInputReader::includesSAXView() {
	return hasSAXApexView || hasSAXMidView || hasSAXBaseView;
}


std::vector<int> XMLInputReader::getAllMarkerTypes(unsigned int frame) {
	std::vector<int> result;
	const unsigned int rbaseindex = getBaseRIndex();
	std::vector<int>* types = markerTypes[frame];
	for (int i = 0; i < types->size(); i++)
	{
		int j = types->at(i);
		if (endoMarkerFlags[frame][j] == 1)
		{ //If the frame has endocardial markers
			result.push_back(j);
		}
		if (epiMarkerFlags[frame][j] == 1)
		{ //If the frame has endocardial markers
			result.push_back(j);
		}
	}
	return result;
}

std::vector<double> XMLInputReader::getFrameSamplingTimes() {
	return frameSamplingTimes;
}

std::vector<int> XMLInputReader::getStrainSelections() {
	return strainSelections;
}
