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
#include "MovieMaker.h"
#include "compress/LibArchiveWrapper.h"
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <fstream>
#include <sstream>
#define targetImageHeight 600
#define targetImageWidth 800

//#define pipeline

#ifndef pipeline
#ifdef multithreaded
int MAX_THREAD = 2;
#endif
#endif

int tmain(int argc, char** argv, std::string workingdir) {

	XMLInputReader reader(argv[1]);
	DCMTKUtils DCMtkutil;

	int numFiles = reader.getNumberOfFiles();
	std::ostringstream sstr;
	MovieMaker** movies = new MovieMaker*[numFiles];
	std::string ffmpeg(argv[2]);

	std::string outputdir(reader.getOutputDirectory());
	std::string outputfile(reader.getOutputFile());
	std::string tempDir;

	if (outputdir == "NOT FOUND")
	{
		boost::filesystem::path dir(workingdir);
		std::ostringstream ss;
		ss << "temp" << rand();
		tempDir = (dir / ss.str()).string();
	}
	else
	{
		tempDir = std::string(outputdir);
	}

	//Create the tempDir
	if (!boost::filesystem::exists(tempDir))
	{
		boost::filesystem::create_directory(tempDir);
	}
	//Since multithreading does not handle memory allocations and dynamic data
	//Create storage structures outside and pass the pointer
	unsigned int * frameCounters = new unsigned int[numFiles + 1];
#ifdef pipeline
	//Download and process the images prior to expensive computations
	//Hopefully this should overlap network delays with computational pipelining
	std::vector<boost::thread*> threads;
	for (unsigned int i = 0; i < numFiles; i++)
	{
		std::string uri = reader.getUri(i);
		bool saveImages = reader.saveJpegs();
		std::string prefix = reader.getName(i);

		MovieMaker * movie = new MovieMaker(DCMtkutil, uri, prefix, ffmpeg, saveImages);
		movie->setTargetHeight(targetImageHeight);
		movie->setTargetWidth(targetImageWidth);

		if (outputfile != "NOT FOUND")
		{
			movie->setOutputDir(tempDir);
		}
		else if (outputdir != "NOT FOUND")
		{
			movie->setOutputDir(outputdir);
		}
		else
		{ //When output is stored to db
			movie->setOutputDir(tempDir);
		}
		sstr.str("");
		sstr << workingdir << "/Movie" << i;
		std::string mWdir = sstr.str();
		boost::filesystem::create_directory(mWdir);

		movie->setWorkingDir(mWdir);
		movie->setCounterMemory(frameCounters + i);
		movies[i] = movie;
		boost::thread* thread = new boost::thread(boost::bind(&MovieMaker::Initialize, movie));
		threads.push_back(thread);
	}
	for (unsigned int i = 0; i < numFiles; i++)
	{
		threads[i]->join();
		movies[i]->makeMovie();
		delete threads[i];
	}
#else
#ifdef multithreaded
	const int maxThreads = MAX_THREAD;
	for (unsigned int th = 0; th < numFiles; th+=maxThreads)
	{
		int start = th;
		int end = (th+maxThreads)>numFiles?numFiles:(th+maxThreads);
		boost::thread_group myThreadGroup;
		for (unsigned int i = start; i < end; i++)
		{
#else
			for (unsigned int i = 0; i < numFiles; i++)
			{
#endif
				std::string uri = reader.getUri(i);
				bool saveImages = reader.saveJpegs();
				std::string prefix = reader.getName(i);

				MovieMaker * movie = new MovieMaker(DCMtkutil, uri, prefix, ffmpeg, saveImages);
				movie->setTargetHeight(targetImageHeight);
				movie->setTargetWidth(targetImageWidth);

				if (outputfile != "NOT FOUND")
				{
					movie->setOutputDir(tempDir);
				}
				else if (outputdir != "NOT FOUND")
				{
					movie->setOutputDir(outputdir);
				}
				else
				{ //When output is stored to db
					movie->setOutputDir(tempDir);
				}
				sstr.str("");
				sstr << workingdir << "/Movie" << i;
				std::string mWdir = sstr.str();
				boost::filesystem::create_directory(mWdir);

				movie->setWorkingDir(mWdir);
				movie->setCounterMemory(frameCounters + i);
				movies[i] = movie;
				movie->makeMovie();
#ifdef multithreaded
				myThreadGroup.create_thread(*movie);
			}
			myThreadGroup.join_all();
		}
#endif
	}
#endif
	if (outputfile != "NOT FOUND")
	{
		std::ostringstream ss;
		boost::filesystem::path outDir(tempDir);
		//Create an xml file with details
		std::string oprefix = outputfile;
		if (boost::algorithm::ends_with(outputfile, ".tar.gz"))
			oprefix = outputfile.substr(0, outputfile.find(".tar.gz"));

		std::string xmlFileName;
		if (oprefix.find("/") != std::string::npos)
		{ //If output file is an absolute path, get the name
			std::string tName = oprefix.substr(outputfile.find_last_of("/") + 1, oprefix.length()) + ".xml";
			xmlFileName = (outDir / tName).string();
		}
		else
			xmlFileName = (outDir / (oprefix + ".xml")).string();

		ss << "<ICMA>" << std::endl;
		for (int i = 0; i < numFiles; i++)
			ss << movies[i]->getXMLDescriptor() << std::endl;
		ss << "</ICMA>";

		std::ofstream xmlFile(xmlFileName.c_str());
		xmlFile << ss.str();
		xmlFile.close();

		//Tar and zip the contents of temp dir
		std::vector<std::string> filenames;
		//Add the all the files in the tempDir
		boost::filesystem::directory_iterator end_iter;

		for (boost::filesystem::directory_iterator dir_iter(outDir); dir_iter != end_iter; ++dir_iter)
		{
			if (boost::filesystem::is_regular_file(dir_iter->status()))
			{
				filenames.push_back(dir_iter->path().string());
			}
		}

		//Create file in working directory
		if (LibArchiveWrapper::archive(filenames, oprefix + ".tar.gz", "") < 0)
		{
			std::cout << "Failed to compress files " << outDir.string() << std::endl;
		}

		if (outputdir == "NOT FOUND")
		{ //Delete the tempDir
			boost::filesystem::remove_all(outDir);
		}
	}


	for (int i = 0; i < numFiles; i++)
		delete movies[i];

	delete[] movies;
	delete[] frameCounters;

	return 0;
}

int main(int argc, char** argv) {
	if (argc < 4)
	{
		std::cout << "Usage: movie xml ffmpeg_executable working_directory [max threads]" << std::endl;
		return -1;
	}
#ifdef multithreaded
	if(argc==5)
	{
		MAX_THREAD = atoi(argv[4]);
		if(MAX_THREAD<1)
		{
			MAX_THREAD = 1;
		}
	}
#endif
	int retCode = 0;
	std::ostringstream sstr;
	sstr << argv[3] << "/Work" << random();
	std::string workingdir = sstr.str();

	while (boost::filesystem::exists(workingdir))
	{
		sstr.str("");
		sstr << argv[3] << "/Work" << random();
		workingdir = sstr.str();
	}

	boost::filesystem::create_directory(workingdir);

	tmain(argc, argv, workingdir);
	boost::filesystem::remove_all(workingdir);
	return retCode;
}
