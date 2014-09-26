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
#ifndef CMISSDICOMENCAPSULATOR_CPP_
#define CMISSDICOMENCAPSULATOR_CPP_
#include "CMISSDICOMEncapsulator.h"

int CMISSDICOMEncapsulator::composeImages() {

	typename JoinSeriesImageFilterType::Pointer joiner = JoinSeriesImageFilterType::New();
	double origin = 0.0;
	double spacing = 1.0;
	joiner->SetOrigin(origin);
	joiner->SetSpacing(spacing);
	int imgCtr = 0;
	typename ReaderType::Pointer reader = ReaderType::New();
	for (int i = 0; i < views->size(); i++) {
		std::string viewName = views->at(i);
		int start = viewStart[i];
		int end = viewEnd[i];
		std::vector<SliceImageType::Pointer>& images = (*myImages)[viewName];
		for (int f = start; f <= end; f++) {
			joiner->PushBackInput(images[f]);
			imgCtr++;
		}
		unsigned int numFrames = (end - start + 1);
	}
	joiner->Update();
	image = joiner->GetOutput();
	image->DisconnectPipeline();
	return imgCtr;
}

CMISSDICOMEncapsulator::CMISSDICOMEncapsulator(std::string modelname, std::string outputDICOMPrefix) :
		modelName(modelname), fileNamePrefix(outputDICOMPrefix), tagApplicationName("0018|9524"), tagTransducerData("0018|5010") {
	patientID = "UNKNOWN";
	patientName = "UNKNOWN";
	studyID = "UNKNOWN";
	workingDirectory = itksys::SystemTools::GetCurrentWorkingDirectory();
}

std::string CMISSDICOMEncapsulator::buildOutput() {
	int count = composeImages();

	typedef itk::MetaDataDictionary DictionaryType;
	DictionaryType dictionary;

	//Set patientID 0010,0020
	itk::EncapsulateMetaData<std::string>(dictionary, "0010|0020", patientID);

	//Set patientName 0010,0010
	itk::EncapsulateMetaData<std::string>(dictionary, "0010|0010", patientName);

	//Set study instance UID 0020,000D
	itk::EncapsulateMetaData<std::string>(dictionary, "0020|000D", studyID);

	//Set software version 0018,1020
	itk::EncapsulateMetaData<std::string>(dictionary, "0018|1020", "ABI ICMA 0.1");

	//Set ApplicationName
	itk::EncapsulateMetaData<std::string>(dictionary, tagApplicationName, "ABI ICMA 0.1");

	//Create a sop uid for the image, while keeping the series and study ids same
	gdcm::UIDGenerator sopUid;
	sopInstanceUID = sopUid.Generate(); //This should be called before getSerializedMesgData

	itk::EncapsulateMetaData<std::string>(dictionary, "0008|0018", sopInstanceUID);
	itk::EncapsulateMetaData<std::string>(dictionary, "0002|0003", sopInstanceUID);

	std::ostringstream ss;
	ss << count;
	//Set number of frames to be image count
	itk::EncapsulateMetaData<std::string>(dictionary, "0028|0008", ss.str());
	//Set stop trim to be the image count 0008,2143
	itk::EncapsulateMetaData<std::string>(dictionary, "0008|2143", ss.str());

	OutputImageType::SizeType ninputSize = image->GetLargestPossibleRegion().GetSize();

	std::ostringstream in1;
	in1 << ninputSize[0];
	//Set rows 0028,0010
	itk::EncapsulateMetaData<std::string>(dictionary, "0028,0010", in1.str());
	std::ostringstream in2;
	in2 << ninputSize[1];
	//Set columns 0028,0011
	itk::EncapsulateMetaData<std::string>(dictionary, "0028,0011", in2.str());

	//Set pixel spacing 0028,0030
	OutputImageType::SpacingType ninputSpacing = image->GetSpacing();
	std::ostringstream ims;
	ims << ninputSpacing[0] << "\\" << ninputSpacing[1] << "\\" << ninputSpacing[2];
	itk::EncapsulateMetaData<std::string>(dictionary, "0028|0030", ims.str());

	//Image position patient
	std::ostringstream impp;
	impp << 0.0 << "\\" << 0.0 << "\\" << 0.0;
	itk::EncapsulateMetaData<std::string>(dictionary, "0020|0032", impp.str());

	//Image Orientation patient
	std::ostringstream iop;
	iop << 1.0 << "\\" << 0.0 << "\\" << 0.0 << "\\" << 0.0 << "\\" << 1.0 << "\\" << 0.0;
	itk::EncapsulateMetaData<std::string>(dictionary, "0020|0037", iop.str());

	//Store the view names and corresponding image count in the order they are stored in the 3D sequence
	ss.str("");
	for (int i = 0; i < views->size(); i++) {
		int numFiles = viewEnd[i] - viewStart[i] + 1;
		if (numFiles > 0)
			ss << views->at(i) << "|" << numFiles << "#";
	}

	itk::EncapsulateMetaData<std::string>(dictionary, tagTransducerData, ss.str());


	image->SetMetaDataDictionary(dictionary);

	ImageIOType::Pointer gdcmIO = ImageIOType::New();
	// Save as JPEG 2000 Lossless
	// Explicitly specify which compression type to use
	gdcmIO->SetCompressionType(itk::GDCMImageIO::JPEG2000);

	typename WriterType::Pointer seriesWriter = WriterType::New();
	// Request compression of the ImageIO
	seriesWriter->UseCompressionOn();

	boost::filesystem::path dir(workingDirectory);
	std::string fn = (dir / (fileNamePrefix + ".dcm")).string();
	seriesWriter->SetInput(image);
	seriesWriter->SetImageIO(gdcmIO);
	seriesWriter->SetFileName(fn);
	seriesWriter->Update();

	std::ofstream xmlout;
	fn = (dir / (fileNamePrefix + ".xml")).string();
	xmlout.open(fn.c_str(), std::ios::out);
	xmlout << getSerializedMeshData();
	xmlout.close();

	return sopInstanceUID;
}

CMISSDICOMEncapsulator::~CMISSDICOMEncapsulator() {

}

void CMISSDICOMEncapsulator::addCoordinate(tinyxml2::XMLElement* parent, Point3D point) {
	std::string names[] = { "x", "y", "z" };
	double values[] = { point.x, point.y, point.z };
	std::ostringstream ss;
	for (int i = 0; i < 3; i++) {
		tinyxml2::XMLElement * x = doc.NewElement(names[i].c_str());
		ss.str("");
		ss << values[i];
		x->InsertEndChild(doc.NewText(ss.str().c_str()));
		parent->InsertEndChild(x);
	}

}

void CMISSDICOMEncapsulator::addMARK(tinyxml2::XMLElement* parent, int id, Point3D point) {
	tinyxml2::XMLElement * mark = doc.NewElement("MARK");
	std::ostringstream ss;
	ss << id;
	mark->SetAttribute("ID", ss.str().c_str());
	addCoordinate(mark, point);
	parent->InsertEndChild(mark);
}

std::string CMISSDICOMEncapsulator::getSerializedMeshData() {
	//Create the CAP model xml file
	tinyxml2::XMLElement* image;
	tinyxml2::XMLElement* point;
	tinyxml2::XMLElement* coordValue;
	tinyxml2::XMLElement* xml;
	doc.Clear();

	tinyxml2::XMLDeclaration* decl = doc.NewDeclaration("xml version=\"1.0\" encoding=\"UTF-8\" ");
	doc.InsertEndChild(decl);

	tinyxml2::XMLElement * root = doc.NewElement("CMISSCAPIO");
	doc.InsertEndChild(root);

	xml = doc.NewElement("TRANSFORMATIONMATRIX");
	xml->InsertEndChild(doc.NewText("1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0"));
	root->InsertEndChild(xml);

	std::ostringstream msg;
	const boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
	boost::posix_time::time_facet * const f = new boost::posix_time::time_facet("%Y-%m-%d %H:%M:%S%F%Q");
	msg.imbue(std::locale(msg.getloc(), f));
	msg << now;

	xml = doc.NewElement("MODELCREATIONDATE");
	xml->InsertEndChild(doc.NewText(msg.str().c_str()));
	root->InsertEndChild(xml);

	xml = doc.NewElement("MODELNAME");
	xml->InsertEndChild(doc.NewText(modelName.c_str()));
	root->InsertEndChild(xml);

	msg.str("");
	msg<<(endcycle-edtime);
	xml = doc.NewElement("MODELDURATION");
	xml->InsertEndChild(doc.NewText(msg.str().c_str()));
	root->InsertEndChild(xml);

	msg.str("");
	msg<<(edtime);
	xml = doc.NewElement("MODELEDTIME");
	xml->InsertEndChild(doc.NewText(msg.str().c_str()));
	root->InsertEndChild(xml);

	msg.str("");
	msg<<(estime);
	xml = doc.NewElement("MODELESTIME");
	xml->InsertEndChild(doc.NewText(msg.str().c_str()));
	root->InsertEndChild(xml);

	xml = doc.NewElement("TARGETVIEW");
	xml->InsertEndChild(doc.NewText(targetView.c_str()));
	root->InsertEndChild(xml);

	xml = doc.NewElement("SOPINSTANCEUID");
	xml->InsertEndChild(doc.NewText(sopInstanceUID.c_str()));
	root->InsertEndChild(xml);


	if (planeTransforms != NULL) {
		std::map<std::string, MatrixType>::iterator it = planeTransforms->begin();
		const std::map<std::string, MatrixType>::iterator itend = planeTransforms->end();
		while (it != itend) {
			std::string viewType;
			xml = doc.NewElement("VIEWPLANETRANSFORMATIONMATRIX");
			xml->SetAttribute("VIEW", it->first.c_str());
			std::ostringstream view;
			MatrixType& mat = it->second;
			for (int x = 0; x < 4; x++)
				for (int y = 0; y < 4; y++) {
					double val = mat[x][y];
					if (fabs(val) < 1.0e-9)
						val = 0.0;
					view << " " << val;
				}

			std::string vm = view.str();
			boost::trim(vm);

			xml->InsertEndChild(doc.NewText(vm.c_str()));
			root->InsertEndChild(xml);
			++it;
		}
	}

	if (types != NULL) {
		std::map<std::string, int>::iterator it = types->begin();
		const std::map<std::string, int>::iterator itend = types->end();
		while (it != itend) {
			std::string viewType;
			xml = doc.NewElement("VIEWPLANETYPE");
			xml->SetAttribute("VIEW", it->first.c_str());
			std::string mytype("LONG");
			if (it->second > 1)
				mytype = "SHORT";
			xml->InsertEndChild(doc.NewText(mytype.c_str()));
			root->InsertEndChild(xml);
			++it;
		}
	}

	//Load mesh for each frame
	std::string filename_prefix = "Heart";
	tinyxml2::XMLElement* meshes = doc.NewElement("MESH");
	meshes->SetAttribute("prefix", filename_prefix.c_str());
	int numModelFrames = mesh->size();
	meshes->SetAttribute("count", numModelFrames);
	for (unsigned int i = 0; i < numModelFrames; i++) {
		std::ostringstream ss;
		ss << filename_prefix << i << ".exregion";
		tinyxml2::XMLElement* fMesh = doc.NewElement("HEART");
		fMesh->InsertEndChild(doc.NewText(mesh->at(i).c_str()));
		fMesh->SetAttribute("filename", ss.str().c_str());
		ss.str("");
		ss << i;
		fMesh->SetAttribute("frame", ss.str().c_str());
		meshes->InsertEndChild(fMesh);
	}
	root->InsertEndChild(meshes);

	//Entry for downstream process
	xml = doc.NewElement("STRAINS");
	root->InsertEndChild(xml);

	//Data that is sent to the CAP for building model
	tinyxml2::XMLElement * sroot = doc.NewElement("SEGMENTATION");

	sroot->SetAttribute("chamber", "LV");
	sroot->SetAttribute("name", modelName.c_str());

	tinyxml2::XMLElement * dir = doc.NewElement("OUTPUTDIRECTORY");
	dir->InsertEndChild(doc.NewText("."));
	sroot->InsertEndChild(dir);

	tinyxml2::XMLElement * dcm = doc.NewElement("DICOMFILE");
	std::string dcmFileName = "./" + modelName + ".dcm";
	dcm->InsertEndChild(doc.NewText(dcmFileName.c_str()));
	sroot->InsertEndChild(dcm);

	tinyxml2::XMLElement * name = doc.NewElement("MODELNAME");
	name->InsertEndChild(doc.NewText(modelName.c_str()));
	sroot->InsertEndChild(name);

	tinyxml2::XMLElement * noff = doc.NewElement("NUMBEROFFRAMES");
	std::ostringstream ss;
	ss << mesh->size();
	noff->InsertEndChild(doc.NewText(ss.str().c_str()));
	sroot->InsertEndChild(noff);

	if (targetView != "") {
		tinyxml2::XMLElement * tView = doc.NewElement("TARGETVIEW");
		tView->InsertEndChild(doc.NewText(targetView.c_str()));
		sroot->InsertEndChild(tView);
	}

	tinyxml2::XMLElement * baseE = doc.NewElement("BASE");
	addCoordinate(baseE, base);
	sroot->InsertEndChild(baseE);

	tinyxml2::XMLElement * apexE = doc.NewElement("APEX");
	addCoordinate(apexE, apex);
	sroot->InsertEndChild(apexE);

	tinyxml2::XMLElement * rvInsert = doc.NewElement("RVINSERTS");
	tinyxml2::XMLElement * insert = doc.NewElement("INSERT");
	addCoordinate(insert, avgRvInsert);
	rvInsert->InsertEndChild(insert);
	sroot->InsertEndChild(rvInsert);

	std::map<std::string, std::map<double, std::vector<Point3D> > >::iterator vstart = meshMarkers->begin();
	const std::map<std::string, std::map<double, std::vector<Point3D> > >::iterator vend = meshMarkers->end();
	while (vstart != vend) {
		std::string viewType = vstart->first;
		std::map<double, std::vector<Point3D> >& typeMarkers = vstart->second;
		std::map<double, std::vector<Point3D> >::iterator start = typeMarkers.begin();
		const std::map<double, std::vector<Point3D> >::iterator end = typeMarkers.end();
		int frameCounter = 0;
		while (start != end) {
			double time = start->first;
			std::vector<Point3D>& mrks = start->second;

			tinyxml2::XMLElement * marker = doc.NewElement("MARKERS");
			marker->SetAttribute("VIEW", viewType.c_str());
			marker->SetAttribute("TYPE", "ENDO");
			ss.str("");
			ss << frameCounter++;
			marker->SetAttribute("FRAME", ss.str().c_str());
			ss.str("");
			ss << mrks.size();
			marker->SetAttribute("NOOFMARKERS", ss.str().c_str());
			ss.str("");
			ss << time;
			marker->SetAttribute("FRAMETIME", ss.str().c_str());

			for (int i = 0; i < mrks.size(); i++) {
				addMARK(marker, i, mrks[i]);
			}
			sroot->InsertEndChild(marker);
			++start;
		}
		++vstart;
	}

	tinyxml2::XMLPrinter printer1;
	doc.Print(&printer1);
	std::string xmlstr(printer1.CStr());
	int capioindex = xmlstr.length() - std::string("</CMISSCAPIO>").length();
	std::string openxml = xmlstr.substr(0, capioindex - 1);
	std::string inputLine(inputXML);
	boost::replace_all(inputLine, "</SEGMENTATION>", "");
	std::string tag("<SEGMENTATION>");
	int index = inputLine.find(tag);
	std::string inputT = inputLine.substr(index + tag.length());
	std::string fullxml = openxml + "<SPECKLETRACKINGINPUT>" + inputT + "</SPECKLETRACKINGINPUT></CMISSCAPIO>";

	return fullxml;
}

#endif
