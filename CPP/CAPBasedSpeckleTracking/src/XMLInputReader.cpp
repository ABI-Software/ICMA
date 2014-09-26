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

XMLInputReader::XMLInputReader(std::string filename)
{
  XMLdoc = new tinyxml2::XMLDocument();
  targetView = -1;
  targetWidth = 0.0;
  targetHeight = 0.0;
  createImages = false;
  inputXMLFile = filename;
  if (XMLdoc->LoadFile(filename.c_str()) == tinyxml2::XML_SUCCESS)
    {
      tinyxml2::XMLElement *pRoot, *pParm;
      pRoot = XMLdoc->FirstChildElement("SEGMENTATION");
      if (pRoot == NULL)
        {
          std::cout << "The input xml file does not have element SEGMENTATION " << filename << std::endl;
          throw -1;
        }
      if (pRoot)
        {
          outputDirectory = std::string("NOT FOUND");
          modelName = std::string("NOT_FOUND");
          pParm = pRoot->FirstChildElement("OUTPUTDIRECTORY");
          if (pParm)
            {
              outputDirectory = std::string(pParm->GetText());
              const char* attr = pParm->Attribute("JPEGS");
              if (attr != NULL)
                {
                  if (std::string(attr) == "TRUE" || std::string(attr) == "true")
                    {
                      createImages = true;
                    }
                }
            }

          pParm = pRoot->FirstChildElement("MODELNAME");
          if (pParm)
            {
              modelName = std::string(pParm->GetText());
            }

          pParm = pRoot->FirstChildElement("NUMBEROFFRAMES");
          numberOfFrames = maxcapframes;
          if (pParm)
            {
              numberOfFrames = atoi(pParm->GetText());
            }
          pParm = pRoot->FirstChildElement("TARGETHEIGHT");
          if (pParm)
            {
              targetHeight = atof(pParm->GetText());
            }
          pParm = pRoot->FirstChildElement("TARGETWIDTH");
          if (pParm)
            {
              targetWidth = atof(pParm->GetText());
            }
          pParm = pRoot->FirstChildElement("TARGETVIEW");
          if (pParm)
            {
              targetView = std::string(pParm->GetText());
            }

          pParm = pRoot->FirstChildElement("BASE");
          if (pParm)
            {
              double * b = getPointData(pParm);
              base[0] = b[0];
              base[1] = b[1];
              base[2] = b[2];
              delete[] b;
            }
          pParm = pRoot->FirstChildElement("APEX");
          if (pParm)
            {
              double * a = getPointData(pParm);
              apex[0] = a[0];
              apex[1] = a[1];
              apex[2] = a[2];
              delete[] a;
            }
          pParm = pRoot->FirstChildElement("RVINSERTS");
          if (pParm)
            {
              tinyxml2::XMLNode* child = pParm->FirstChild();
              while (child != 0)
                {
                  double* rvInsert = getPointData(child->ToElement());
                  rvInserts.push_back(rvInsert);
                  child = child->NextSibling();
                }
            }
          pParm = pRoot->FirstChildElement("MARKERS"); //Returns all xml elements from the first element names MARKERS
          for (; pParm; pParm = pParm->NextSiblingElement())
            {
              if (std::string(pParm->Name()) != "MARKERS")
                {
                  continue; //Only process if the tag is of type "MARKERS"
                }

              try
                {
                  bool edset = false;
                  bool ecset = false;

                  std::string viewType = boost::to_upper_copy(std::string(pParm->Attribute("VIEW")));
                  std::string markerType("ENDO");
                  if (pParm->Attribute("TYPE"))
                    {
                      markerType = boost::to_upper_copy(std::string(pParm->Attribute("TYPE")));
                    }
                  std::string recordType = viewType + markerType;
                  this->viewType[recordType] = viewType;
                  this->markerType[recordType] = markerType;
                  if (pParm->Attribute("ED"))
                    {
                      ed[recordType] = atoi(pParm->Attribute("ED"));
                      edset = true;
                    }
                  if (pParm->Attribute("EC"))
                    {
                      ec[recordType] = atoi(pParm->Attribute("EC"));
                      ecset = true;
                    }

                  if (pParm->Attribute("BPM"))
                    {
                      bpm[recordType] = (double) atof(pParm->Attribute("BPM"));
                      ecset = true;
                    }

                  if (edset)
                    {
                      if (!ecset)
                        std::cout << " EC has not been set for " << recordType << " program may fail" << std::endl;
                    }
                  if (ecset)
                    {
                      if (!edset)
                        std::cout << " ED has not been set for " << recordType << " program may fail" << std::endl;
                    }

                  tinyxml2::XMLElement* uri = pParm->FirstChildElement("URI");
                  if (uri)
                    {
                      fileuri[recordType] = std::string(uri->GetText());
                    }
                  else
                    {
                      std::ostringstream ss;
                      ss << "FATAL: No URI associated with view " << viewType << " marker type " << markerType;
                      throw SegmentationAndFittingException(ss.str());
                    }

                  std::vector<double*> fmarkers(100); //Resize to correct no of markers
                  tinyxml2::XMLElement* child = pParm->FirstChildElement("MARKS");
                  int ctr = 0;
                  while (child != 0)
                    {
                	  if (std::string(child->Name()) == "MARKS"){
						  int myID = -1; //marker ids start from 1
						  child->QueryIntAttribute("ID", &myID);
						  if (myID > -1)
							{
							  double* point = getPointData(child);
							  fmarkers[myID] = point;
							  ctr++;
							}
						  else
							{
							  std::cout << "Failed to load the node "<< std::cout;
							  fflush(stdout);
							}
                	  }
					  child = child->NextSiblingElement();
                    }
                  fmarkers.resize(ctr + 1);
                  markers[recordType] = fmarkers;
                  //Check if ESframe information is available and if so load it
                  child = pParm->FirstChildElement("ESFRAME");
                  if(child!=0){
                	  int esframe = -1;
                	  child->QueryIntAttribute("ESFRAMENO", &esframe);
                	  std::vector<double*> baseplane(2);
                	  tinyxml2::XMLElement* fchild = child->FirstChildElement("MARKS");
                      while (fchild != 0)
                        {
                    	  if (std::string(fchild->Name()) == "MARKS"){
    						  int myID = -1; //marker ids start from 1
    						  fchild->QueryIntAttribute("ID", &myID);
    						  if (myID > -1)
    							{
    							  double* point = getPointData(fchild);
    							  baseplane[myID-1] = point;
    							}
    						  else
    							{
    							  std::cout << "Failed to load the node " << std::cout;
    							  fflush(stdout);
    							}
                    	  }
						  fchild = fchild->NextSiblingElement();
                        }
                      es[recordType] = esframe;
                      esbaseplane[recordType] = baseplane;
                  }

#ifdef debug
                  std::cout<<"Markers for "<<viewType<<"\t";
                  for(int i=1;i<ctr;i++)
                    {
                      std::cout<<fmarkers[i][0]<<" ,"<<fmarkers[i][1]<<" ";
                    }
                  std::cout<<std::endl;
#endif
                }
              catch (std::exception& ex)
                {
                  std::cout << " The input xml file does not seem to be in the appropriate format ";
                  std::cout << ex.what();
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

XMLInputReader::~XMLInputReader()
{
  if (XMLdoc)
    delete XMLdoc;
    {
      std::map<std::string, std::vector<double*> >::iterator start = markers.begin();
      std::map<std::string, std::vector<double*> >::iterator end = markers.end();
      while (start != end)
        {
            {
              std::vector<double*>& vec = start->second;
              for (int i = 0; i < vec.size(); i++)
                {
                  if (vec[i])
                    delete[] vec[i];
                }
            }
          ++start;
        }
    }
    {
      std::vector<double*>::iterator start = rvInserts.begin();
      const std::vector<double*>::iterator end = rvInserts.end();
      while (start != end)
        {
          delete[] (*start);
          ++start;
        }

    }
}

double*
XMLInputReader::getApex()
{
  return apex;
}

double*
XMLInputReader::getBase()
{
  return base;
}

std::vector<double*>&
XMLInputReader::getRVInserts()
{
  return rvInserts;
}

unsigned int
XMLInputReader::getNumberOfFrames()
{
  return numberOfFrames;
}

std::map<std::string, std::vector<double*> >
XMLInputReader::getMarkers()
{
  //return *(markers[frame]);
  //Transform the coordinates according to the marker type and target Node


  return markers;
}

std::string
XMLInputReader::getModelName()
{
  return modelName;
}

double*
XMLInputReader::getPointData(tinyxml2::XMLElement* pRoot)
{
  double* point = new double[3];
  point[0] = 0;
  point[1] = 0;
  point[2] = 0;
  tinyxml2::XMLElement *pParm = pRoot->FirstChildElement("x");
  if (pParm)
    {
      point[0] = atof(pParm->GetText());
      if (fabs(point[0]) < 1.0e-6)
        point[0] = 0.0;
    }
  pParm = pRoot->FirstChildElement("y");
  if (pParm)
    {
      point[1] = atof(pParm->GetText());
      if (fabs(point[1]) < 1.0e-6)
        point[1] = 0.0;
    }
  pParm = pRoot->FirstChildElement("z");
  if (pParm)
    {
      point[2] = atof(pParm->GetText());
      if (fabs(point[2]) < 1.0e-6)
        point[2] = 0.0;
    }
  return point;
}

std::string
XMLInputReader::getOutputDirectory()
{
  return outputDirectory;
}

std::string
XMLInputReader::getUri(std::string view)
{
  return fileuri[view];
}

double
XMLInputReader::getBPM(std::string view)
{
  if (bpm.find(view) != bpm.end())
    {
      return bpm[view];
    }
  else
    {
      throw -1;
    }
}

double
XMLInputReader::getTargetHeight()
{
  return targetHeight;
}

double
XMLInputReader::getTargetWidth()
{
  return targetWidth;
}

int
XMLInputReader::getED(std::string view)
{
  if (ed.find(view) != ed.end())
    {
      return ed[view];
    }
  return -1;
}

int
XMLInputReader::getES(std::string view)
{
  if (ec.find(view) != ec.end())
    {
      return ec[view];
    }
  return -1;
}

std::vector<std::string>
XMLInputReader::getRecords()
{
  std::vector<std::string> result;
  std::map<std::string, std::vector<double*> >::iterator start = markers.begin();
  std::map<std::string, std::vector<double*> >::iterator end = markers.end();
  while (start != end)
    {
      result.push_back(start->first);
      ++start;
    }
  return result;
}

std::vector<std::string>
XMLInputReader::getViews()
{
  std::vector<std::string> result;
  std::map<std::string, std::string>::iterator start = viewType.begin();
  std::map<std::string, std::string>::iterator end = viewType.end();
  while (start != end)
    {
      result.push_back(start->second);
      ++start;
    }
  return result;
}

std::vector<std::string>
XMLInputReader::getMarkerTypes()
{
  std::vector<std::string> result;
  std::map<std::string, std::string>::iterator start = markerType.begin();
  std::map<std::string, std::string>::iterator end = markerType.end();
  while (start != end)
    {
      result.push_back(start->second);
      ++start;
    }
  return result;
}

int XMLInputReader::getESFrame(std::string view) {
	if (es.find(view) != es.end())
	    {
	      return es[view];
	    }
	  return -1;
}

std::vector<double*> XMLInputReader::getESBaseplane(std::string view) {
	if (esbaseplane.find(view) != esbaseplane.end())
	    {
	      return esbaseplane[view];
	    }
	  return std::vector<double*>(0);
}
