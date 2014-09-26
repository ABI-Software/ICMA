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
#ifndef LIBARCHIVEWRAPPER_H_
#define LIBARCHIVEWRAPPER_H_

extern "C"{
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "archive.h"
#include "archive_entry.h"
}

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/iterator/transform_iterator.hpp>


extern "C" int copy_data(struct archive *ar, struct archive *aw);
extern "C" int extractArchive(const char* filename, const char* directory);
extern "C" int createArchiveofFiles(char** files,unsigned long *size,unsigned int fileCount,const char* filename);
extern "C" int createArchiveofFilesPC(char** files,unsigned long *size,unsigned int fileCount,const char* filename, const char* tarHostDir);

class LibArchiveWrapper {
public:
	/**
	 * Extract the tar.gz file "filename" into the directory "directory", the directory(s) is created
	 * if it doesnt exist
	 * @param filename a std::string - fully qualified path to the archive file
	 * @param directory a std::string - fully qualified (or relative to current working directory) of the directory where the contents should be extracted
	 * @return integer 0 - success, +ve warning, -ve failed
	 */
	static int extract(std::string filename, std::string directory);
	/**
	 * Archive the input directory into a tar.gz file with "filename"
	 * @param is a std::string - fully qualified (or relative to current working directory) of the directory to be archived
	 * @param is a std::string - fully qualified (or relative to current working directory) of the archive name
	 * @return integer 0 - success, +ve warning, -ve failed
	 */
	static int archive(std::string directory, std::string filename);
	/**
	 * Archive the input filelist into a tar.gz file with "filename"
	 * @param is a std::vector<std::string> - fully qualified (or relative to current working directory) of the files to be archived
	 * @param is a std::string - fully qualified (or relative to current working directory) of the archive name
	 * @return integer 0 - success, +ve warning, -ve failed
	 */
	static int archive(std::vector<std::string> files, std::string filename);
	/**
	 * Archive the input directory into a tar.gz file with "filename"
	 * @param is a std::string - fully qualified (or relative to current working directory) of the directory to be archived
	 * @param is a std::string - fully qualified (or relative to current working directory) of the archive name
	 * @param is a std::string - the archive entries are stored in the tarHostDirectory, their prior directory branching is removed
	 * @return integer 0 - success, +ve warning, -ve failed
	 */
	static int archive(std::string directory, std::string filename, std::string tarHostDirectory);
	/**
	 * Archive the input filelist into a tar.gz file with "filename"
	 * @param is a std::vector<std::string> - fully qualified (or relative to current working directory) of the files to be archived
	 * @param is a std::string - fully qualified (or relative to current working directory) of the archive name
	 * @param is a std::string - the archive entries are stored in the tarHostDirectory, their prior directory branching is removed
	 * @return integer 0 - success, +ve warning, -ve failed
	 */
	static int archive(std::vector<std::string> files, std::string filename, std::string tarHostDirectory);
};

#endif /* LIBARCHIVEWRAPPER_H_ */
