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
#include <string>

inline std::string trim_right_copy(
  const std::string& s,
  const std::string& delimiters = " \f\n\r\t\v" )
{
  return s.substr( 0, s.find_last_not_of( delimiters ) + 1 );
}

inline std::string trim_left_copy(
  const std::string& s,
  const std::string& delimiters = " \f\n\r\t\v" )
{
  return s.substr( s.find_first_not_of( delimiters ) );
}

inline std::string trim(
  const std::string& s,
  const std::string& delimiters = " \f\n\r\t\v" )
{
  return trim_left_copy( trim_right_copy( s, delimiters ), delimiters );
}


XMLInputReader::XMLInputReader(std::string filename) : saveJpeg(false), uploadToDB(false){
	XMLdoc = new tinyxml2::XMLDocument();
	if(XMLdoc->LoadFile(filename.c_str())==tinyxml2::XML_SUCCESS){
		tinyxml2::XMLElement *pRoot, *pParm;
		pRoot = XMLdoc->FirstChildElement("ICMA");
		if (pRoot) {
			pParm = pRoot->FirstChildElement("OUTPUTDIRECTORY");
			outputDirectory= std::string("NOT FOUND");
			if (pParm){
				const char * dir = pParm->GetText();
				std::string dirx(dir);
				outputDirectory = trim(dirx);
				const char * jpeg = pParm->Attribute("JPEGS");
				if(jpeg!=NULL){
					std::string setting(jpeg);
					if(trim(setting)=="true")
						saveJpeg = true;
				}
			}

			pParm = pRoot->FirstChildElement("OUTPUTFILE");
			outputFile= std::string("NOT FOUND");
			if (pParm){
				outputFile = trim(std::string(pParm->GetText()));
				const char * jpeg = pParm->Attribute("JPEGS");
				if(jpeg!=NULL){
					std::string setting(jpeg);
					if(trim(setting)=="true")
							saveJpeg = true;
				}
			}

			pParm = pRoot->FirstChildElement("FILE");
			for( ; pParm; pParm=pParm->NextSiblingElement())
			{
				try{
					tinyxml2::XMLElement* url = pParm->FirstChildElement("URI");
					tinyxml2::XMLElement* name = pParm->FirstChildElement("NAME");

					uri.push_back(std::make_pair(trim(name->GetText()),trim(url->GetText())));
				}catch(std::exception& ex){
					std::cout<<" The input xml file does not seem to be in the appropriate format ";
					std::cout<<ex.what();
					throw ex;
				}
			}
		}

	}else{
		std::cout<<"Unable to load the input file "<<filename<<std::endl;
		throw -1;
	}
}

XMLInputReader::~XMLInputReader() {
	if(XMLdoc)
		delete XMLdoc;
}


std::string XMLInputReader::getOutputDirectory() {
	return outputDirectory;
}

