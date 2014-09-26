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
 *  Contributor(s): J Chung
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


#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <string>
#include <vector>

/**
 * Gets all the names of the files in the given directory.  This list excludes any file names
 * starting with a '.'.  @see GetAllFileNamesRecursive
 *
 * @param	dirname	Pathname of the directory.
 *
 * @return	all file names.
 */
const std::vector<std::string> GetAllFileNames(const std::string& dirname);

/**
 * Gets all file names recursively.  This list excludes directories, only files that are marked
 * as a regular file. @see GetAllFileNames
 *
 * @param	dirname	Pathname of the directory to start from.
 *
 * @return	all file names found in a recursive search.
 */
const std::vector<std::string> GetAllFileNamesRecursive(
		const std::string& dirname);

/**
 * Creates a directory.
 *
 * @param	dirname	Pathname of the directory.
 *
 * @return	true if it succeeds, false if it fails.
 */
bool MakeDirectory(const std::string& dirname);

/**
 * Removes the file described by filename from the system.
 *
 * @param	filename	Filename of the file.
 *
 * @return	true if it succeeds, false if it fails.
 */
bool RemoveFile(const std::string& filename);

/**
 * Queries if a given file exists.
 *
 * @param	filename	Filename of the file.
 *
 * @return	true if it succeeds, false if it fails.
 */
bool IsFile(const std::string& filename);

/**
 * Queries if a given directory exists.
 *
 * @param	dirname	Name of the directory.
 *
 * @return	true if it succeeds, false if it fails.
 */
bool IsDirectory(const std::string& dirname);

/**
 * Writes a character buffer to file.
 *
 * @param	filename	Filename of the file.
 * @param	data		The data.
 * @param	len			The length.
 *
 * @return	true if it succeeds, false if it fails.
 */
bool WriteCharBufferToFile(const std::string& filename, unsigned char data[],
		unsigned int len);

/**
 * Writes a character buffer to string.
 *
 * @param	data	The data.
 * @param	len 	The length.
 *
 * @return	A std::string containing the char buffer.
 */
std::string WriteCharBufferToString(unsigned char data[], unsigned int len);

/**
 * Removes the directory described by dirname.
 *
 * @param	dirname	Directory name of the directory.
 *
 * @return	true if it succeeds, false if it fails.
 */
bool DeleteDirectory(const std::string& dirname);

/**
 * Creates a temporary empty file and returns the filename in the returned string.
 *
 * @param	directory	Pathname of the directory to create file in, "" by default.
 *
 * @return	The name of the empty file.
 */
std::string CreateTemporaryEmptyFile(const std::string& directory = "");

/**
 * Given a string name this function will return the filename for the current platform.  The
 * filename includes the extension and starts from the last system directory marker.
 *
 * \param name a string to get the file name from. \returns a string starting from the last
 * system directory marker.
 *
 * @param	name	The name.
 *
 * @return	The file name.
 */
std::string GetFileName(const std::string& name);

/**
 * Given a string name this function will return the
 * filename without extension for the current platform.
 * The file name without extension starts from the last
 * directory marker to the last extension marker.
 *
 * \param name a string to get the file name without
 * extension from.
 * \returns a string starting from the last system
 * directory marker to the last extension marker.
 */
std::string GetFileNameWOE(const std::string& name);

/**
 * Gets a path.
 *
 * @param	path	Full pathname of the file.
 *
 * @return	The path.
 */
std::string GetPath(const std::string& path);

#endif /* FILESYSTEM_H_ */
