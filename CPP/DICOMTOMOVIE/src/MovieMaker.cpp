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


#include "MovieMaker.h"

int MovieMaker::id = 0;

MovieMaker::MovieMaker(DCMTKUtils& DCMtkutil, std::string dcmURI,
		std::string jprefix, std::string ffmpeg, bool saveJpegs) :
		myId(id++), stopProcessing(false), prefix(jprefix), dicomuri(
				dcmURI), ffmpegEXEC(ffmpeg), saveImages(saveJpegs), targetHeight(
				0.0), targetWidth(0.0), dcmtkutil(DCMtkutil) {
	boost::filesystem::path pwd = boost::filesystem::current_path();
	workingDir = pwd.string();
	outputDir = workingDir;
	frameCounter = NULL;
	input = new DICOMInputManager(dcmtkutil, dicomuri, workingDir);
	initialized = false;
}

MovieMaker::~MovieMaker() {
	delete input;
}


void MovieMaker::makeMovie() {
	if(!initialized){
		Initialize();
	}
	if (!stopProcessing) {
		if (!input->isICMAInstance()) {
			processInstance();
		} else { //Perform ICMA related processing
				 //Output jpeg files
			unsigned int numFrames = input->getNumberOfFrames();
			*frameCounter = numFrames; //Store for xml string description

			boost::filesystem::path dir(workingDir);
			boost::filesystem::path odir(outputDir);
			std::ostringstream ss;

			try {
				//Extract Overlay data and get mesh information
				gdcm::Reader gdcmReader;
				//Read the dicom file
				std::string dicomFile(input->getDICOMFile());
				gdcmReader.SetFileName(dicomFile.c_str());
				if (!gdcmReader.Read()) {
					std::cout << "Unable to read file " << dicomFile;
					processInstance();
					return;
				}
				gdcm::File &file = gdcmReader.GetFile();
				gdcm::DataSet &ds = file.GetDataSet();
				gdcm::Tag oytag(0x6000, 0x3000);	//Overlay data tag
				const gdcm::DataElement &de = ds.GetDataElement(oytag);
				const gdcm::ByteValue *bv = de.GetByteValue();

				std::ostringstream cpd;
				cpd.write(bv->GetPointer(), bv->GetLength());

				std::string imageViewEncoding = input->getTransducerData();

				std::vector<std::string> views;
				boost::split(views, imageViewEncoding, boost::is_any_of("#"));

				if (views.size() > 0) {
					std::string targetDir = (odir / prefix).string();
					boost::filesystem::create_directories(targetDir);
					boost::filesystem::path mydir(targetDir);
					std::string metaDataFile = "";

					try {
						std::stringstream compressed;
						compressed << cpd.str();
						std::stringstream decompressed;
						boost::iostreams::filtering_streambuf<
								boost::iostreams::input> in;
						in.push(boost::iostreams::gzip_decompressor());
						in.push(compressed);
						boost::iostreams::copy(in, decompressed);

						metaDataFile = (mydir / "metaData.xml").string();
						std::ofstream metaData(metaDataFile.c_str(),
								std::ios::out);
						metaData << decompressed.str();
						metaData.close();
					} catch (std::exception& gzipe) {
						//Copy of the dicom file is set as meta data
						metaDataFile = (mydir / "dicom.dcm").string();
						boost::filesystem::copy_file(
								input->getDICOMFile().c_str(),
								metaDataFile.c_str(),
								boost::filesystem::copy_option::overwrite_if_exists);
					}

					//Create images
					int imageIndex = 0;
					for (int i = 0; i < views.size(); i++) {
						std::string vrec = views[i];
						boost::trim(vrec);
						if (vrec.length() > 0) {
							int idx = vrec.find('|');
							std::string viewName = vrec.substr(0, idx);
							int frameCount = atoi(vrec.substr(idx + 1).c_str());
							std::string viewDir = (mydir / viewName).string();
							boost::filesystem::create_directories(viewDir);
							boost::filesystem::path vdPath(viewDir);
							for (int j = 0; j < frameCount; j++) {
								ss.str("");
								ss << viewName << std::setfill('0')
										<< std::setw(3) << j << ".jpg";
								std::string fn = (vdPath / ss.str()).string();

								input->saveFrameAsJpeg(fn, j + imageIndex);
							}
							imageIndex += frameCount;
						}
					}
					std::string fn = prefix + ".properties";
					std::ofstream properties((odir / fn).string().c_str(),
							std::ios::out);
					properties << "ICMA_FEM = TRUE" << std::endl;
					properties << "METADATAFILE = " << metaDataFile
							<< std::endl;
					properties << "VIEWENCODING = " << imageViewEncoding
							<< std::endl;
					properties << "SOPIUID = " << input->getSopInstanceUID()
							<< std::endl;
					//TODO Add target ED EC data to mark the cardiac cycle times
					properties.close();
				}
			}
			catch (std::exception& e) {
				std::cout << e.what() << std::endl;
			}
		}
	}

}

std::string MovieMaker::getXMLDescriptor() {
	std::ostringstream ss;
	ss << "<FILE name=\"" + prefix + "\">";
	ss << "<POSTER type=\"jpg\">" << prefix << "POSTER.jpg" << "</POSTER>";
#ifdef phase_field
	ss<<"<FIELD  type=\"jpg\">"<<prefix<<"field.jpg"<<"</FIELD>";
#endif
	if (saveImages) {
		ss << "<FRAMES count=\"" << *frameCounter << "\">";
		for (int i = 0; i < *frameCounter; i++) {
			ss << "<FRAME id=\"" << i << "\" type=\"jpg\">";
			ss << prefix << std::setfill('0') << std::setw(3) << i << ".jpg";
			ss << "</FRAME>";
		}
		ss << "</FRAMES>";
	}
	ss << "</FILE>";

	return ss.str();
}

void MovieMaker::Initialize() {
	if (targetWidth != 0.0)
		input->setTargetWidth(targetWidth);
	if (targetHeight != 0.0)
		input->setTargetHeight(targetHeight);
	input->Initialize();
	initialized = true;
}

void MovieMaker::processInstance() {
	//Output jpeg files
	unsigned int numFrames = input->getNumberOfFrames();
	*frameCounter = numFrames; //Store for xml string description
	boost::filesystem::path dir(workingDir);
	boost::filesystem::path odir(outputDir);

	if (numFrames > 1) {
		//Save a poster image
		int rWave = input->rWaveFrame();
		std::ostringstream ss;
		ss << prefix << "POSTER.jpg";
		std::string fn = (odir / ss.str()).string();
		input->saveFrameAsJpeg(fn, rWave);

		//Save attributes
		ss.str("");
		ss << prefix << ".properties";
		fn = (odir / ss.str()).string();
		input->saveAttributesToFile(fn);
		std::ofstream propFile;
		propFile.open(fn.c_str(), std::ios_base::app);


#ifdef phase_field
		//Save the phase field
		PhaseField field(fn);
		ss.str("");
		ss << prefix << "field.jpg";
		fn = (odir / ss.str()).string();
		field.saveToFile(fn);*/
#endif

		std::vector<std::string> filenames(numFrames);
		for (int i = 0; i < numFrames; i++) {
			ss.str("");
			ss << prefix << myId << std::setfill('0') << std::setw(3) << i
					<< ".jpg";
			fn = (dir / ss.str()).string();
			filenames[i] = fn;
			input->saveFrameAsJpeg(fn, i);
			if (saveImages) {
				ss.str("");
				ss << prefix << std::setfill('0') << std::setw(3) << i
						<< ".jpg";
				fn = (odir / ss.str()).string();
				input->saveFrameAsJpeg(fn, i);
			}
		}

		//Call ffmpeg to convert the images to movie
		bool allFound = true;
		// Launch ffmpeg
		if (!stopProcessing) { //Since threaded execution allows for concurrent access to member
			std::string filePrefix = (dir / prefix).string();
			std::string ofilePrefix = (odir / prefix).string();

			std::ostringstream ffmpeg_cmdline;
			ffmpeg_cmdline << ffmpegEXEC << " -loglevel panic ";

			ffmpeg_cmdline << "-r " << (int) (floor(input->getFrameRate()))
					<< " -i " << filePrefix << myId << "%03d.jpg"
					<< " -dframes " << numFrames
					<< " -b:v 1500k -vcodec libx264 -an -g 30 " << ofilePrefix
					<< ".mp4";

			//Check if file with the name already exists, if so ffmpeg will wait for user input
			//Remove the file
			if (boost::filesystem::exists(ofilePrefix + ".mp4"))
				boost::filesystem::remove(ofilePrefix + ".mp4");

			int returnc = system(ffmpeg_cmdline.str().c_str());
			if (!boost::filesystem::exists(ofilePrefix + ".mp4")) {
				std::cout << "!!!!!!!!!!!!!!!!!!!!!" << ofilePrefix + ".mp4"
						<< "Not found";
				allFound = false;
			}

			//Create ogg and webm files
			ffmpeg_cmdline.str("");

			ffmpeg_cmdline << ffmpegEXEC << " -loglevel panic ";
			ffmpeg_cmdline << "-r " << (int) (floor(input->getFrameRate()))
					<< " -i " << filePrefix << myId << "%03d.jpg"
					<< " -dframes " << numFrames
					<< " -b:v 1500k -vcodec libvpx -an -f webm -g 30 "
					<< ofilePrefix << ".webm";

			if (boost::filesystem::exists(ofilePrefix + ".webm"))
				boost::filesystem::remove(ofilePrefix + ".webm");

			returnc = system(ffmpeg_cmdline.str().c_str());
			if (!boost::filesystem::exists(ofilePrefix + ".webm")) {
				std::cout << "!!!!!!!!!!!!!!!!!!!!!" << ofilePrefix + ".webm"
						<< "Not found";
				allFound = false;
			}

			ffmpeg_cmdline.str("");

			ffmpeg_cmdline << ffmpegEXEC << " -loglevel panic ";
			ffmpeg_cmdline << "-r " << (int) (floor(input->getFrameRate()))
					<< " -i " << filePrefix << myId << "%03d.jpg"
					<< " -dframes " << numFrames
					<< " -b:v 1500k -vcodec libtheora -an -g 30 " << ofilePrefix
					<< ".ogv";

			if (boost::filesystem::exists(ofilePrefix + ".ogv"))
				boost::filesystem::remove(ofilePrefix + ".ogv");

			returnc = system(ffmpeg_cmdline.str().c_str());
			if (!boost::filesystem::exists(ofilePrefix + ".ogv")) {
				std::cout << "!!!!!!!!!!!!!!!!!!!!!" << ofilePrefix + ".ogv"
						<< "Not found";
				allFound = false;
			}
		}

		//Check to ensure the files have been created
		//else retry at least 3 times with a delay of 1 min
		//This to ensure that ffmpeg writes complete. Sometimes there is a delay between completion and file availability
		int retry = 3;
		while (retry > 0 && allFound == false) {
			std::cout << "!!!!!!!!!!! Retry " << retry << " left " << std::endl;
			boost::this_thread::sleep(boost::posix_time::seconds(60));
			if (!stopProcessing) { //Since threaded execution allows for concurrent access to member
				std::string filePrefix = (dir / prefix).string();
				std::string ofilePrefix = (odir / prefix).string();

				std::ostringstream ffmpeg_cmdline;
				ffmpeg_cmdline << ffmpegEXEC << " -loglevel panic ";

				ffmpeg_cmdline << "-r " << (int) (floor(input->getFrameRate()))
						<< " -i " << filePrefix << myId << "%03d.jpg"
						<< " -dframes " << numFrames
						<< " -b:v 1500k -vcodec libx264 -an -g 30 "
						<< ofilePrefix << ".mp4";

				//Check if file with the name already exists
				if (!boost::filesystem::exists(ofilePrefix + ".mp4")) {
					int returnc = system(ffmpeg_cmdline.str().c_str());
				}
				if (!boost::filesystem::exists(ofilePrefix + ".mp4")) {
					allFound = false;
				}
				//Create ogg and webm files
				ffmpeg_cmdline.str("");

				ffmpeg_cmdline << ffmpegEXEC << " -loglevel panic ";
				ffmpeg_cmdline << "-r " << (int) (floor(input->getFrameRate()))
						<< " -i " << filePrefix << myId << "%03d.jpg"
						<< " -dframes " << numFrames
						<< " -b:v 1500k -vcodec libvpx -an -f webm -g 30 "
						<< ofilePrefix << ".webm";

				if (!boost::filesystem::exists(ofilePrefix + ".webm")) {
					int returnc = system(ffmpeg_cmdline.str().c_str());
				}
				if (!boost::filesystem::exists(ofilePrefix + ".webm")) {
					allFound = false;
				}
				ffmpeg_cmdline.str("");

				ffmpeg_cmdline << ffmpegEXEC << " -loglevel panic ";
				ffmpeg_cmdline << "-r " << (int) (floor(input->getFrameRate()))
						<< " -i " << filePrefix << myId << "%03d.jpg"
						<< " -dframes " << numFrames
						<< " -b:v 1500k -vcodec libtheora -an -g 30 "
						<< ofilePrefix << ".ogv";

				if (!boost::filesystem::exists(ofilePrefix + ".ogv")) {
					int returnc = system(ffmpeg_cmdline.str().c_str());
				}
				if (!boost::filesystem::exists(ofilePrefix + ".ogv")) {
					allFound = false;
				}
			} else {
				break;
			}
			if (allFound)
				break;
			retry--;
		}
		//Delete the jpeg files

		for (int i = 0; i < numFrames; i++) {
			boost::filesystem::remove(filenames[i]);
		}
	} else {
		//Save a poster image
		std::ostringstream ss;
		ss << prefix << "POSTER.jpg";
		std::string fn = (odir / ss.str()).string();
		input->saveFrameAsJpeg(fn, 0);

		//Save attributes
		ss.str("");
		ss << prefix << ".properties";
		fn = (odir / ss.str()).string();
		input->saveAttributesToFile(fn);
	}
}
