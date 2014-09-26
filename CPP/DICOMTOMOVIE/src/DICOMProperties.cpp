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


#include "DICOMProperties.h"
#include "itkImageSeriesReader.h"
#include "itkGDCMImageIO.h"
#include "boost/algorithm/string/predicate.hpp"
#include "boost/algorithm/string/find.hpp"

DICOMProperties::DICOMProperties(DCMTKUtils& DCMtkutil, std::string file) : icmaDicom(false), myFile(file), dcmtkutil(DCMtkutil) {

	typedef signed short PixelType;

	typedef itk::Image< PixelType, 3 >    InputImageType;
	typedef itk::ImageSeriesReader< InputImageType >	    ReaderType;
	ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName(file);
    reader->Update();
	ReaderType::DictionaryRawPointer dictionary = (*(reader->GetMetaDataDictionaryArray()))[0];
	//Read the dictionary for DICOM parameters
	typedef itk::MetaDataDictionary DictionaryType;

	DictionaryType::ConstIterator itr = dictionary->Begin();
	DictionaryType::ConstIterator end = dictionary->End();

	typedef itk::MetaDataObject<std::string> MetaDataStringType;

	while (itr != end)
	{
		itk::MetaDataObjectBase::Pointer entry = itr->second;

		MetaDataStringType::Pointer entryvalue = dynamic_cast<MetaDataStringType *>(entry.GetPointer());
		if (entryvalue)
		{
			std::string tagkey(itr->first);
			std::string result(tagkey); //trim
			result.erase(result.find_last_not_of(" ") + 1);
			result.erase(0, result.find_first_not_of(" "));
			tagkey = result;
			std::string tagvalue = entryvalue->GetMetaDataObjectValue();
			std::string value = tagvalue;
			boost::replace_all(value, "^", " ");
			if(boost::starts_with(value,"ABI ICMA")){
				icmaDicom = true;
			}
		    std::string labelId;
		    bool found =  itk::GDCMImageIO::GetLabelFromTag( tagkey, labelId );


			if(found && value.length()<400 && value.length() > 0){
				header.insert(std::make_pair(boost::erase_all_copy(labelId, " "),value));
			}
		}
		++itr;
	}
}

std::map<std::string, std::string> DICOMProperties::getProperties() {
	return header;
}

void DICOMProperties::loadElement(tinyxml2::XMLNode* pParent) {
	tinyxml2::XMLElement *pChild, *pParm;
	pParm = pParent->ToElement();
	if(std::string(pParm->Name())=="element"){
		std::string name  = std::string(pParm->Attribute("name"));
		boost::trim(name);
		const char* val = pParm->GetText();
		std::string value;
		if(val!=NULL){
			value = std::string(val);
		}else{
			value = "";
		}
		boost::replace_all(value, "^", " ");
		if(!(boost::starts_with(name,"Unknown Tag") || name == "PrivateCreator" || name == "PixelData" || name == "OverlayData")){
			header.insert(std::make_pair(name,value));
			if(boost::starts_with(name,"SoftwareVersions") && boost::starts_with(value,"ABI ICMA") ){
				icmaDicom = true;
			}
		}
	}
	else{//Done this way as element do not have children
		for (  tinyxml2::XMLNode* pC = pParm->FirstChild(); pC != 0; pC = pC->NextSibling())
		{
			loadElement(pC);
		}
	}
}

std::string DICOMProperties::getValueOf(std::string tag) {
	if(header.find(tag)!=header.end()){
		return header[tag];
	}else{
		return "NOT FOUND";
	}
}

bool DICOMProperties::isICMADICOM(){
	return icmaDicom;
}

DICOMProperties::~DICOMProperties() {

}

