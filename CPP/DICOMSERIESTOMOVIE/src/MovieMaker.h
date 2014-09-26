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


#ifndef MOVIEMAKER_H_
#define MOVIEMAKER_H_

#include "DICOMInputManager.h"
#include <boost/filesystem.hpp>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/foreach.hpp>

#include "itkExceptionObject.h"

#include "gdcmReader.h"
#include "gdcmWriter.h"
#include "gdcmFile.h"
#include "gdcmDataSet.h"
#include "DICOMImage.h"


class MovieMaker {
public:

	MovieMaker(DCMTKUtils& DCMtkutil,std::vector<std::string> dcmURI, std::string jprefix, std::string ffmpeg,bool saveJpegs=false);
	virtual ~MovieMaker();

	void setWorkingDir(std::string wdir){
		workingDir = wdir;
	};

	void setOutputDir(std::string odir){
		outputDir = odir;
	};

	void setTargetHeight(double height){
		targetHeight = height;
	};

	void setTargetWidth(double width){
		targetWidth = width;
	};


	void makeMovie();

	std::string getXMLDescriptor();

	void setCounterMemory(unsigned int * mem){
		frameCounter = mem;
	};

protected:
	DCMTKUtils& dcmtkutil;
	DICOMInputManager* input;
	void processInstance();
	unsigned int myId;
	bool stopProcessing;
	unsigned int* frameCounter;
	std::string prefix;
	std::string filetype;
	std::vector<std::string> dicomuri;
	std::string ffmpegEXEC;
	bool saveImages;
	std::string workingDir;
	std::string outputDir;
	double targetHeight;
	double targetWidth;
	bool initialized;
	static int id;
};

#endif /* MOVIEMAKER_H_ */
