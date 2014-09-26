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

#include <iostream>
#include <fstream>

#include "XMLInputReader.h"
#include "LVHeartMesh.h"
#include "StrainMeasures.h"
#include "XMLSerialiser.h"
#include "BundleToDICOMException.h"

#include "boost/filesystem.hpp"
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>

#include "gdcmReader.h"
#include "gdcmWriter.h"
#include "gdcmFile.h"
#include "gdcmDataSet.h"
#include "gdcmUIDGenerator.h"

void transform(double* x, double* y, double* z, MatrixType* ts) {
	double x1 = *x, y1 = *y, z1 = *z;
	MatrixType& transformation = *ts;
	double mx = transformation[0][0] * x1 + transformation[0][1] * y1 + transformation[0][2] * z1 + transformation[0][3];
	double my = transformation[1][0] * x1 + transformation[1][1] * y1 + transformation[1][2] * z1 + transformation[1][3];
	double mz = transformation[2][0] * x1 + transformation[2][1] * y1 + transformation[2][2] * z1 + transformation[2][3];

	*x = mx;
	*y = my;
	*z = mz;
}

void printISOSurfaceGFX(MatrixType* transformation, std::string view) {
	double x1 = 0.0, x2 = 800.0, x3 = 800.0, y1 = 0.0, y2 = 0.0, y3 = 600.0, z1 = 0.0, z2 = 0.0, z3 = 0.0;

	transform(&x1, &y1, &z1, transformation);
	transform(&x2, &y2, &z2, transformation);
	transform(&x3, &y3, &z3, transformation);

	double A = y1 * (z2 - z3) + y2 * (z3 - z1) + y3 * (z1 - z2);
	double B = z1 * (x2 - x3) + z2 * (x3 - x1) + z3 * (x1 - x2);
	double C = x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2);
	double D = -(x1 * (y2 * z3 - y3 * z2) + x2 * (y3 * z1 - y1 * z3) + x3 * (y1 * z2 - y2 * z1));

	std::cout << "gfx define field /heart/iso_scalar" << view << " coordinate_system rectangular_cartesian dot_product fields proj_coordinates [" << A << " " << B << " " << C
			<< "]" << std::endl;

	double delta = 1.0e+6;
	std::cout << "gfx modify g_element /heart/ iso_surfaces as iso_scalarSurf" << view << " coordinate proj_coordinates tessellation default LOCAL iso_scalar iso_scalar" << view
			<< " range_number_of_iso_values 2 first_iso_value " << (-D - delta) << "  last_iso_value " << (-D + delta)
			<< " use_elements no_select material default data LAGRANGIAN spectrum longaxisstrain selected_material default_selected render_shaded" << std::endl;

}

int main(int argc, char* argv[]) {

	if (argc < 2) {
		std::cout << "Usage: SpeckleFitting xmlinputfile [outputdirectory]" << std::endl;
		return -1;
	}

	//Read the input file and get marker details
	XMLInputReader reader(argv[1]);

	//Get the transformation matrix related to the various views
	std::map<int, std::string> transforms = reader.getImagePlaneTransformations();

	if (argc > 2) {
		if (std::string(argv[2]) == ".") {
			std::cout << "ISO Surfaces " << std::endl;
			std::string viewnames[] = { "", "aplax", "tch", "fch", "saxapex", "saxmid", "saxbase" };
			for (int i = 1; i < 7; i++) {
				if (transforms.find(i) != transforms.end()) {
					printISOSurfaceGFX(reader.getImagePlaneTransformation(static_cast<MarkerTypes>(i)), viewnames[i]);
				}
			}
			std::cout << "Transformations " << std::endl;
			for (int i = 1; i < 7; i++) {
				if (transforms.find(i) != transforms.end()) {
					std::cout << viewnames[i] << "\t" << transforms[i] << std::endl;
				}
			}

		}
	}
	std::cout << "WORKLOAD:3" << std::endl;

	std::cout << "STEP TRANFORMATION completed" << std::endl;

	unsigned int numberOfFrames = reader.getNumberOfFrames();
	//Create a LVMesh, let the cmgui context name be CMISS and create a LVmesh with numberOfFrames meshes
	LVHeartMesh mesh("CMISS", reader);

	mesh.fixWallLengths();
	//Create a RV mesh that follows the user input and also ensure that the myocardial volume is conserved
	mesh.fixToMarkers();


	std::cout << "STEP FIT completed" << std::endl;
	//Compute various strain measures
	StrainMeasures measure(mesh, reader);
	//Compute the measures for all long axis views, note the long axis information
	//for all the views may not be provided, even though the resulting mesh is 3D
	bool views[] = { true, true, true, false };
	measure.setActiveViews(views);

	//If a directory is specified, output the mesh and strain data to that directory
	if (argc > 2) {
		if (std::string(argv[2]) == ".") {
			std::cout << "Heart\t" << mesh.getTransformation() << std::endl;

			std::vector<std::string> strains = measure.getSixteenSegmentStrains();
			std::vector<std::string> speckleStrains = measure.getSpeckleStrains(true);
			//std::vector<std::string> fibreStrains = measure.getSixteenSegmentFibreStrains(0.0);
			measure.Print("strain.csv", strains);
			measure.Print("speckle.csv", speckleStrains);
			mesh.outputMesh("test", argv[2]);
		}
	}

	std::cout << "STEP MEASUREMENT completed" << std::endl;

	//If DICOM filename is not provided create the files in the given output directory
	if (reader.getDICOMFile() == "") {
		//Output mesh, strains, transformation in an xml file
		boost::filesystem::path dir(reader.getOutputDirectory());
		//Create a file name with modelname in the specified directory
		std::string filename = reader.getModelName() + ".xml";
		boost::filesystem::path qfilename = dir / filename;
		//Output the result in an xml formatted form
		std::ofstream file(qfilename.string().c_str(), std::ios_base::binary);

		std::string output(XMLSerialiser::buildXMLString(mesh, measure, reader.getDuration(), &transforms));
		boost::replace_all(output, "</CMISSCAPIO>", "");
		boost::trim(output);

		file << output << std::endl;
		file << "<SPECKLETRACKINGOUTPUT>" << std::endl;
		tinyxml2::XMLDocument* XMLdoc = new tinyxml2::XMLDocument();
		if (XMLdoc->LoadFile(argv[1]) == tinyxml2::XML_SUCCESS) {
			tinyxml2::XMLPrinter printer;
			XMLdoc->Print(&printer);
			std::string inputLine(printer.CStr());
			boost::replace_all(inputLine, "</SEGMENTATION>", "");
			std::string tag("<SEGMENTATION>");
			int index = inputLine.find(tag);
			std::string inputT = inputLine.substr(index + tag.length());
			boost::trim(inputT);
			file << inputT << std::endl;
		}
		file << "</SPECKLETRACKINGOUTPUT>" << std::endl;
		file << "</CMISSCAPIO>";
		file.close();
	} else {	//If a dicom file is specified - store the data in the dicom file (in the overlay tag)
		gdcm::Reader gdcmReader;
		//Read the dicom file
		std::string dicomFile(reader.getDICOMFile());
		gdcmReader.SetFileName(dicomFile.c_str());
		if (!gdcmReader.Read()) {
			std::stringstream msg;
			msg << "Unable to read file " << dicomFile;
			throw new BundleToDICOMException(msg.str());
		}
		gdcm::File &file = gdcmReader.GetFile();
		gdcm::DataSet &ds = file.GetDataSet();

		std::string output(XMLSerialiser::buildXMLString(mesh, measure, reader.getDuration(), &transforms));
		boost::replace_all(output, "</CMISSCAPIO>", "");
		boost::trim(output);
		std::ostringstream sfx;
		sfx << output << std::endl;
		sfx << "<SPECKLETRACKINGOUTPUT>" << std::endl;
		tinyxml2::XMLDocument* XMLdoc = new tinyxml2::XMLDocument();
		if (XMLdoc->LoadFile(argv[1]) == tinyxml2::XML_SUCCESS) {
			tinyxml2::XMLPrinter printer;
			XMLdoc->Print(&printer);
			std::string inputLine(printer.CStr());
			std::string tag("<SEGMENTATION");
			int index = inputLine.find(tag);
			std::string inputT = inputLine.substr(index);
			boost::trim(inputT);
			sfx << inputT << std::endl;
		}
		sfx << "</SPECKLETRACKINGOUTPUT>" << std::endl;
		sfx << "</CMISSCAPIO>";

		//Compress the string.. wado seems to have a trouble with ascii data
		boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
		in.push(boost::iostreams::gzip_compressor());
		std::istringstream uncompressed_string(sfx.str());
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

	std::cout << argv[0] << ":completed" << std::endl;
	return 0;
}
