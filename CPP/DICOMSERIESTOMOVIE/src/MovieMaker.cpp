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

MovieMaker::MovieMaker(DCMTKUtils& DCMtkutil, std::vector<std::string> dcmURI, std::string jprefix, std::string ffmpeg, bool saveJpegs) :
		myId(id++), stopProcessing(false), prefix(jprefix), dicomuri(dcmURI), ffmpegEXEC(ffmpeg), saveImages(saveJpegs), targetHeight(0.0), targetWidth(0.0), dcmtkutil(DCMtkutil) {
	boost::filesystem::path pwd = boost::filesystem::current_path();
	workingDir = pwd.string();
	outputDir = workingDir;
	frameCounter = NULL;
	input = NULL;
	initialized = false;
}

MovieMaker::~MovieMaker() {
}

void MovieMaker::makeMovie() {
	boost::filesystem::path urlfilesDir;
	if (!stopProcessing) {
		ImageIOType::Pointer gdcmIO = ImageIOType::New();
		SeriesReaderType::Pointer reader = SeriesReaderType::New();
		reader->SetImageIO(gdcmIO);

		boost::filesystem::path dir(workingDir);
		boost::filesystem::path qfilename;

		std::vector<boost::filesystem::path> files;
		urlfilesDir = dir / "urlfiles";
		if (!boost::filesystem::exists(urlfilesDir)) {
			boost::filesystem::create_directories(urlfilesDir);
		}

		unsigned int numFiles = dicomuri.size();

		if (numFiles > 1) {
			URLFileDownloader uriFile(urlfilesDir.string());
			for (int f = 0; f < numFiles; f++) {
				if (boost::starts_with(dicomuri[f], "http")) { //If it is a url download it
					std::ostringstream ss;
					ss << "MRI" << f << ".dcm";
					std::string fn(ss.str());

					if (uriFile.loadURLToFile(dicomuri[f], fn) != 0) {
						SAFTHROW("Unable to load URL " + dicomuri[f])
					}
					files.push_back(urlfilesDir / fn);
				} else {
					boost::filesystem::path mypath(dicomuri[f]);
					if (!boost::filesystem::is_directory(mypath)) {
						files.push_back(mypath);
					} else {
						std::copy(boost::filesystem::recursive_directory_iterator(mypath), boost::filesystem::recursive_directory_iterator(), std::back_inserter(files));
					}
				}
			}
		} else { //Should be a directory
			std::copy(boost::filesystem::recursive_directory_iterator(dicomuri[0]), boost::filesystem::recursive_directory_iterator(), std::back_inserter(files));
		}

//Load the dicom files into a map
		typedef std::map<std::string, DICOMImage*> DicomFileTableType;
		DicomFileTableType dicomFileTable_;
		BOOST_FOREACH (boost::filesystem::path const& filename , files){
		std::string fullpath = filename.string();
		try
		{
			DICOMImage* dicomFile = new DICOMImage(fullpath);
			dicomFileTable_.insert(std::make_pair(fullpath, dicomFile));
		} catch (std::exception& e)
		{
			// This is not a DICOM file
		}
	}
		typedef std::pair<int, double> SliceKeyType;
		typedef std::map<SliceKeyType, std::vector<DICOMImage*> > slicesType;
		slicesType slices;
		BOOST_FOREACH (DicomFileTableType::value_type const& value , dicomFileTable_){
		DICOMImage* dicomFile = value.second;
		int seriesNum = dicomFile->GetSeriesNumber();
		Vector3D v = dicomFile->GetImagePosition() - Point3D(0, 0, 0);
		double distanceFromOrigin = v.Length();

		SliceKeyType key = std::make_pair(seriesNum, distanceFromOrigin);
		slicesType::iterator itr = slices.find(key);
		if (itr != slices.end())
		{
			itr->second.push_back(dicomFile);
		}
		else
		{
			std::vector<DICOMImage*> v(1, dicomFile);
			slices.insert(std::make_pair(key, v));
		}
	}

	// Sort the dicom images in a slice/series by the instance Number
		BOOST_FOREACH (slicesType::value_type& slice , slices){
		std::vector<DICOMImage*>& images = slice.second;
		// use stable_sort to preserve the order of images in case the instance number element is missing
		// (in which case the GetInstanceNumber returns -1)
		std::stable_sort(images.begin(), images.end(),
				boost::bind(std::less<int>(), boost::bind(&DICOMImage::GetInstanceNumber, _1), boost::bind(&DICOMImage::GetInstanceNumber, _2)));
	}

		std::vector<std::vector<std::string> > filelist;
		std::vector<std::vector<Point3D> > imagePlanePosition;
		std::vector<double > imageHeight;
		std::vector<double > imageWidth;

		BOOST_FOREACH (slicesType::value_type& slice , slices){
		std::vector<DICOMImage*>& images = slice.second;
		std::vector<std::string> names;
		imagePlanePosition.push_back(images[0]->getImagePlaneCoordinates());
		imageHeight.push_back(images[0]->GetImageHeight());
		imageWidth.push_back(images[0]->GetImageWidth());
		BOOST_FOREACH (DICOMImage* image , images)
		{
			names.push_back(image->GetFilename());
			delete image;
		}
		filelist.push_back(names);
	}
		std::string saveprefix = prefix;
		std::ostringstream ss;
		int sctr = 1;
		std::ostringstream planes;
		unsigned int numfilelists = filelist.size();
		//BOOST_FOREACH (std::vector<std::string>& files , filelist){
		for (int fctr = 0; fctr < numfilelists; fctr++) {
			std::vector<std::string>& files = filelist[fctr];
			input = new DICOMInputManager(dcmtkutil, files, workingDir);
			input->Initialize();
			ss.str("");
			ss << saveprefix << sctr++;
			prefix = ss.str();
			processInstance();
			planes << "PLANE" << (sctr - 1) << " = " << prefix << std::endl;
			ss.str("");
			ss << imagePlanePosition[fctr][0] << "#" << imagePlanePosition[fctr][1] << "#" << imagePlanePosition[fctr][2] << "#" << imagePlanePosition[fctr][3];
			//Add the target height and with information which is based on the num rows and cols
			ss<<"#"<<imageHeight[fctr]<<"#"<<imageWidth[fctr];
			planes << "COORD" << (sctr - 1) << " = " << ss.str() << std::endl;
#ifdef printfileunitdetails
			std::cout<<"Files in unit "<<prefix<<std::endl;
			for(int i=0;i<files.size();i++)
			std::cout<<files[i]<<std::endl;
			std::cout<<"******************************"<<std::endl;
#endif
			delete input;
		}
		//Output details to a file
		prefix = saveprefix;
		boost::filesystem::path odir(outputDir);
		std::string fn = (odir / (prefix + ".properties")).string();
		std::ofstream seriesDes(fn.c_str(), std::ios::out);
		seriesDes << "NUMPLANES = " << sctr << std::endl << planes.str();
		seriesDes.close();
	}
	//Remove the url data
	boost::filesystem::remove_all(urlfilesDir);

}

std::string MovieMaker::getXMLDescriptor() {
	std::ostringstream ss;
	ss << "<FILE name=\"" + prefix + "\">";
	ss << "<POSTER type=\"jpg\">" << prefix << "POSTER.jpg" << "</POSTER>";

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

		std::vector<std::string> filenames(numFrames);
		for (int i = 0; i < numFrames; i++) {
			ss.str("");
			ss << prefix << myId << std::setfill('0') << std::setw(3) << i << ".jpg";
			fn = (dir / ss.str()).string();
			filenames[i] = fn;
			input->saveFrameAsJpeg(fn, i);
			if (saveImages) {
				ss.str("");
				ss << prefix << std::setfill('0') << std::setw(3) << i << ".jpg";
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

			ffmpeg_cmdline << "-r " << (int) (floor(input->getFrameRate())) << " -i " << filePrefix << myId << "%03d.jpg" << " -dframes " << numFrames
					<< " -b:v 1500k -vcodec libx264 -an -g 30 " << ofilePrefix << ".mp4";

			//Check if file with the name already exists, if so ffmpeg will wait for user input
			//Remove the file
			if (boost::filesystem::exists(ofilePrefix + ".mp4"))
				boost::filesystem::remove(ofilePrefix + ".mp4");
			//std::cout<<ffmpeg_cmdline.str().c_str()<<std::endl;
			int returnc = system(ffmpeg_cmdline.str().c_str());
			if (!boost::filesystem::exists(ofilePrefix + ".mp4")) {
				std::cout << "!!!!!!!!!!!!!!!!!!!!!" << ofilePrefix + ".mp4" << "Not found";
				allFound = false;
			}

			//Create ogg and webm files
			ffmpeg_cmdline.str("");

			ffmpeg_cmdline << ffmpegEXEC << " -loglevel panic ";
			ffmpeg_cmdline << "-r " << (int) (floor(input->getFrameRate())) << " -i " << filePrefix << myId << "%03d.jpg" << " -dframes " << numFrames
					<< " -b:v 1500k -vcodec libvpx -an -f webm -g 30 " << ofilePrefix << ".webm";

			//std::cout<<ffmpeg_cmdline.str().c_str()<<std::endl;

			if (boost::filesystem::exists(ofilePrefix + ".webm"))
				boost::filesystem::remove(ofilePrefix + ".webm");

			returnc = system(ffmpeg_cmdline.str().c_str());
			if (!boost::filesystem::exists(ofilePrefix + ".webm")) {
				std::cout << "!!!!!!!!!!!!!!!!!!!!!" << ofilePrefix + ".webm" << "Not found";
				allFound = false;
			}

			ffmpeg_cmdline.str("");

			ffmpeg_cmdline << ffmpegEXEC << " -loglevel panic ";
			ffmpeg_cmdline << "-r " << (int) (floor(input->getFrameRate())) << " -i " << filePrefix << myId << "%03d.jpg" << " -dframes " << numFrames
					<< " -b:v 1500k -vcodec libtheora -an -g 30 " << ofilePrefix << ".ogv";

			//std::cout<<ffmpeg_cmdline.str().c_str()<<std::endl;
			if (boost::filesystem::exists(ofilePrefix + ".ogv"))
				boost::filesystem::remove(ofilePrefix + ".ogv");

			returnc = system(ffmpeg_cmdline.str().c_str());
			if (!boost::filesystem::exists(ofilePrefix + ".ogv")) {
				std::cout << "!!!!!!!!!!!!!!!!!!!!!" << ofilePrefix + ".ogv" << "Not found";
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

				ffmpeg_cmdline << "-r " << (int) (floor(input->getFrameRate())) << " -i " << filePrefix << myId << "%03d.jpg" << " -dframes " << numFrames
						<< " -b:v 1500k -vcodec libx264 -an -g 30 " << ofilePrefix << ".mp4";

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
				ffmpeg_cmdline << "-r " << (int) (floor(input->getFrameRate())) << " -i " << filePrefix << myId << "%03d.jpg" << " -dframes " << numFrames
						<< " -b:v 1500k -vcodec libvpx -an -f webm -g 30 " << ofilePrefix << ".webm";

				//std::cout<<ffmpeg_cmdline.str().c_str()<<std::endl;

				if (!boost::filesystem::exists(ofilePrefix + ".webm")) {
					int returnc = system(ffmpeg_cmdline.str().c_str());
				}
				if (!boost::filesystem::exists(ofilePrefix + ".webm")) {
					allFound = false;
				}
				ffmpeg_cmdline.str("");

				ffmpeg_cmdline << ffmpegEXEC << " -loglevel panic ";
				ffmpeg_cmdline << "-r " << (int) (floor(input->getFrameRate())) << " -i " << filePrefix << myId << "%03d.jpg" << " -dframes " << numFrames
						<< " -b:v 1500k -vcodec libtheora -an -g 30 " << ofilePrefix << ".ogv";

				//std::cout<<ffmpeg_cmdline.str().c_str()<<std::endl;
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
