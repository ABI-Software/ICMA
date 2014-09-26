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

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>

#include "tinyxml2.h"
#include "StrainMeasures.h"

#include "gdcmReader.h"
#include "gdcmWriter.h"
#include "gdcmFile.h"
#include "gdcmDataSet.h"

std::string buildXML(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* mesh, StrainMeasures& measure) {
	tinyxml2::XMLElement* xml;
	tinyxml2::XMLElement* strainElement;
	tinyxml2::XMLElement* strainSegment;

	mesh->DeleteChildren(); // Remove existing mesh elements

	double modelPGS = 0.0;
	double ejectionFraction = 0.0;
	const unsigned int num_walls = 10;
	double wall_xi[num_walls];
	for (unsigned int xi = 0; xi < num_walls; xi++) {
		wall_xi[xi] = ((double) xi) / num_walls + 0.05;
	}
	wall_xi[0] = 0.0;
	wall_xi[num_walls - 1] = 0.99;

	tinyxml2::XMLNode * root = doc.FirstChildElement("CMISSCAPIO");

	xml = root->FirstChildElement("STRAINS");
	if (!xml) {
		xml = doc.NewElement("STRAINS");
		root->InsertEndChild(xml);
	}

	xml->SetAttribute("wallcount", num_walls);
#ifdef linear_distortion
	std::vector<std::string> linearDistortion = measure.getLinearDistortion(
			wall_xi, num_walls);
#endif

	for (unsigned int xi = 0; xi < num_walls; xi++) {

		std::vector<std::string> wallStrains = measure.getSixteenSegmentStrains(wall_xi[xi]);
		std::vector<std::string> fibreStrains = measure.getSixteenSegmentFibreStrains(wall_xi[xi]);
		std::string circumferentialStrain = measure.getCircumferentialStrain(wall_xi[xi]);
		std::vector<std::string> radialStrains = measure.getRadialStrains(wall_xi[xi]);

		std::string LVVolume = measure.getLVVolume(wall_xi[xi]);

		if (xi == 0) {
			modelPGS = measure.getModelPgs();
			ejectionFraction = measure.getEjectionFraction();
		}

		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", xi);
		strainElement->SetAttribute("group", "LINEAR");
		if (measure.isLagrangian())
			strainElement->SetAttribute("type", "LAGRANGIAN");
		else
			strainElement->SetAttribute("type", "NATURAL");

		if (xi == 0) {
			std::string strainName = "LAGRANGIAN";
			if (!measure.isLagrangian())
				strainName = "NATURAL";
			measure.embedStrainOnElements(strainName, wallStrains);
			measure.embedStrainOnElements("FIBRE", fibreStrains);
		}
		strainElement->SetAttribute("count", (int) wallStrains.size());

		for (unsigned int j = 0; j < wallStrains.size(); j++) {
			strainSegment = doc.NewElement("STRAIN");
			strainSegment->SetAttribute("id", j);
			strainSegment->InsertEndChild(doc.NewText(wallStrains[j].c_str()));
			strainElement->InsertEndChild(strainSegment);
		}
		xml->InsertEndChild(strainElement);

		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", xi);
		strainElement->SetAttribute("type", "FIBRE");
		strainElement->SetAttribute("group", "FIBRE");
		strainElement->SetAttribute("count", (int) fibreStrains.size());
		for (unsigned int j = 0; j < fibreStrains.size(); j++) {
			if (fibreStrains[j] != "") {
				strainSegment = doc.NewElement("STRAIN");
				strainSegment->SetAttribute("id", j);
				strainSegment->InsertEndChild(doc.NewText(fibreStrains[j].c_str()));
				strainElement->InsertEndChild(strainSegment);
			}
		}
		xml->InsertEndChild(strainElement);

		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", xi);
		strainElement->SetAttribute("type", "VOLUME");
		strainElement->SetAttribute("count", "1");
		strainElement->SetAttribute("group", "VOLUME");
		strainSegment = doc.NewElement("STRAIN");
		strainSegment->SetAttribute("id", "0");
		strainSegment->InsertEndChild(doc.NewText(LVVolume.c_str()));
		strainElement->InsertEndChild(strainSegment);
		xml->InsertEndChild(strainElement);

		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", xi);
		strainElement->SetAttribute("type", "CIRCUMFERENTIAL");
		strainElement->SetAttribute("count", "1");
		strainElement->SetAttribute("group", "LINEAR");
		strainSegment = doc.NewElement("STRAIN");
		strainSegment->SetAttribute("id", "0");
		strainSegment->InsertEndChild(doc.NewText(circumferentialStrain.c_str()));
		strainElement->InsertEndChild(strainSegment);
		xml->InsertEndChild(strainElement);

		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", xi);
		strainElement->SetAttribute("type", "RADIAL");
		strainElement->SetAttribute("group", "LINEAR");
		strainElement->SetAttribute("count", (int) radialStrains.size());
		for (unsigned int j = 0; j < radialStrains.size(); j++) {
			if (radialStrains[j] != "") {
				strainSegment = doc.NewElement("STRAIN");
				strainSegment->SetAttribute("id", j);
				strainSegment->InsertEndChild(doc.NewText(radialStrains[j].c_str()));
				strainElement->InsertEndChild(strainSegment);
			}
		}

		xml->InsertEndChild(strainElement);

#ifdef linear_distortion
		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", xi);
		strainElement->SetAttribute("type", "LINEARDISTORTION");
		strainElement->SetAttribute("count", "1");
		strainElement->SetAttribute("group", "OTHER");
		strainSegment = doc.NewElement("STRAIN");
		strainSegment->SetAttribute("id", "0");
		strainSegment->InsertEndChild(
				doc.NewText(linearDistortion[xi].c_str()));
		strainElement->InsertEndChild(strainSegment);
		xml->InsertEndChild(strainElement);
#endif
	}

	//Record the myocardialVolume
	{
		std::vector<double> volume = measure.getMyocardialVolumes();
		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", 0); //this is the indexed value and not the actual xi
		strainElement->SetAttribute("type", "MVOL");
		strainElement->SetAttribute("count", "1");
		strainElement->SetAttribute("group", "VOLUME");
		strainSegment = doc.NewElement("STRAIN");
		strainSegment->SetAttribute("id", "0");

		std::ostringstream ss;
		ss << 1.0;
		for (unsigned int j = 1; j < volume.size(); j++) {
			ss << "," << volume[j] / volume[0];
		}
		strainSegment->InsertEndChild(doc.NewText(ss.str().c_str()));
		strainElement->InsertEndChild(strainSegment);
		xml->InsertEndChild(strainElement);
	}

	//Record the PGS
	{
		xml = doc.NewElement("MODELPGS");
		std::ostringstream md;
		md << modelPGS;
		xml->InsertEndChild(doc.NewText(md.str().c_str()));
		root->InsertEndChild(xml);
	}

	//Record the Ejection Fraction
	{
		xml = doc.NewElement("MODELEGF");
		std::ostringstream md;
		md << ejectionFraction;
		xml->InsertEndChild(doc.NewText(md.str().c_str()));
		root->InsertEndChild(xml);
	}

	/*
	 * Note that the reference coordinates field and the reference fibre field
	 * are stored in the mesh region and are out put in the exregion files
	 * these need not be added separately
	 */

	//Load each frame
	std::string filename_prefix = "Heart";
	std::vector<std::string> newMesh = measure.getMesh();
//#define outputMesh
#ifdef outputMesh
	boost::filesystem::path meshdir("./out1");
	if(boost::filesystem::exists(meshdir))
	boost::filesystem::remove_all(meshdir);
	boost::filesystem::create_directories(meshdir);
	std::string dirname = meshdir.string();
	for (unsigned int i = 0; i < newMesh.size(); i++) {
		std::ostringstream ss;
		ss <<dirname<< "/init." << i << ".exregion";
		std::ofstream file(ss.str().c_str(),std::ios::out);
		file<<newMesh[i];
		file.close();
	}
#endif

	for (unsigned int i = 0; i < newMesh.size(); i++) {
		std::ostringstream ss;
		ss << filename_prefix << i << ".exregion";
		tinyxml2::XMLElement* fMesh = doc.NewElement("HEART");
		fMesh->InsertEndChild(doc.NewText(newMesh[i].c_str()));
		fMesh->SetAttribute("filename", ss.str().c_str());
		ss.str("");
		ss << i;
		fMesh->SetAttribute("frame", ss.str().c_str());
		mesh->InsertEndChild(fMesh);
	}

	tinyxml2::XMLPrinter printer;
	doc.Print(&printer);

	std::string result(printer.CStr());
	return result;
}

int main(int argc, char* argv[]) {

	if (argc < 3) {
		std::cout << "Usage: " << argv[0] << " MRIFIT.xml MRIFIT.dcm " << std::endl;
		return -1;
	}

	tinyxml2::XMLDocument *xmldoc = new tinyxml2::XMLDocument();
	if (xmldoc->LoadFile(argv[1]) == tinyxml2::XML_SUCCESS) {
		std::vector<std::string> mesh;
		tinyxml2::XMLElement* mElem, *pElem, *pHeart;
		mElem = xmldoc->FirstChildElement("CMISSCAPIO");
		if (mElem)
			mElem = mElem->FirstChildElement("MESH");
		if (mElem) {
			int count = 0;
			const char* cval = mElem->Attribute("count");
			if (cval != NULL)
				count = atoi(cval);
			std::string prefix("Heart");
			const char * pre = mElem->Attribute("prefix");
			if (pre != NULL)
				prefix = std::string(pre);
			mesh.resize(count);
			pElem = mElem->FirstChildElement("HEART");
			int frame;
			for (int i = 0; i < count; i++) {
				std::string nm(pElem->GetText());
				const char * fval = pElem->Attribute("frame");
				if (fval != NULL)
					frame = atoi(fval);
				mesh[frame] = nm;
				pElem = pElem->NextSiblingElement();
			}

			//Create the mesh
			StrainMeasures measures(mesh);
			std::string xml = buildXML(*xmldoc, mElem, measures);

			std::string dicomFile(argv[2]);
			gdcm::Reader gdcmReader;
			//Read the dicom file
			gdcmReader.SetFileName(dicomFile.c_str());
			if (!gdcmReader.Read()) {
				std::cerr << "Unable to read dicom file " << dicomFile;
				throw -1;
			}
			gdcm::File &file = gdcmReader.GetFile();
			gdcm::DataSet &ds = file.GetDataSet();

			//Compress the string.. wado seems to have a trouble with ascii data
			boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
			in.push(boost::iostreams::gzip_compressor());
			std::istringstream uncompressed_string(xml);
			in.push(uncompressed_string);
			std::ostringstream compressed_string;
			boost::iostreams::copy(in, compressed_string);
			std::string tagValue = compressed_string.str();

			gdcm::Tag oytag(0x6000, 0x3000);	//Overlay data tag
			ds.Remove(oytag);	//Remove the data associated with the tag else it doesnt get updated
			gdcm::DataElement de;
			de.SetTag(oytag);
			de.SetVR(gdcm::VR::OB);

			de.SetByteValue(tagValue.c_str(), tagValue.length());
			ds.Insert(de);

			//Write the file
			gdcm::Writer writer;
			writer.SetFile(file);
			writer.SetFileName(dicomFile.c_str());
			writer.Write();
		}
	}

	delete xmldoc;
	return 0;
}

