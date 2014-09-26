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

#include "LibArchiveWrapper.h"

int LibArchiveWrapper::extract(std::string filename, std::string directory) {
	const char* file = filename.c_str();
	const char* dir = directory.c_str();
	//If directory structure doesnt exist create one
	boost::filesystem::path Path(directory);
	if(!boost::filesystem::is_directory(Path)){
		if(boost::filesystem::is_regular_file(Path)){
			return -1;
		}else{
			boost::filesystem::create_directories(Path);
		}
	}
	return extractArchive(file, dir);
}

int LibArchiveWrapper::archive(std::string directory, std::string filename) {
	const char* file = filename.c_str();
	const char* dir = directory.c_str();
	std::vector<std::string> filenames;
	std::vector<unsigned long> sizes;
	boost::filesystem::path Path(directory);
	boost::filesystem::recursive_directory_iterator iter =
			boost::filesystem::recursive_directory_iterator(Path);
	boost::filesystem::recursive_directory_iterator endIter =
			boost::filesystem::recursive_directory_iterator();
	while (iter != endIter) {
		if (!boost::filesystem::is_regular_file(iter->status())){
			++iter;
			continue;
		}
		std::stringstream ss;
		ss<<iter->path();
		filenames.push_back(ss.str());
		sizes.push_back((unsigned long)boost::filesystem::file_size(*iter));
		++iter;
	}
	char** filen = new char*[filenames.size()];
	unsigned long * fsize = new unsigned long[filenames.size()];
	for (int i = 0; i < filenames.size(); i++) {
		filen[i] = const_cast<char*>(filenames[i].c_str());
		fsize[i] = sizes[i];
	}
	int result = createArchiveofFiles(filen, fsize, filenames.size(),filename.c_str());
	delete[] filen;
	delete[] fsize;
	return result;
}


int LibArchiveWrapper::archive(std::string directory, std::string filename,  std::string tarHostDirectory) {
	const char* file = filename.c_str();
	const char* dir = directory.c_str();
	std::vector<std::string> filenames;
	std::vector<unsigned long> sizes;
	boost::filesystem::path Path(directory);
	boost::filesystem::recursive_directory_iterator iter =
			boost::filesystem::recursive_directory_iterator(Path);
	boost::filesystem::recursive_directory_iterator endIter =
			boost::filesystem::recursive_directory_iterator();
	while (iter != endIter) {
		if (!boost::filesystem::is_regular_file(iter->status())){
			++iter;
			continue;
		}
		std::stringstream ss;
		ss<<iter->path();
		filenames.push_back(ss.str());
		sizes.push_back((unsigned long)boost::filesystem::file_size(*iter));
		++iter;
	}
	char** filen = new char*[filenames.size()];
	unsigned long * fsize = new unsigned long[filenames.size()];
	for (int i = 0; i < filenames.size(); i++) {
		filen[i] = const_cast<char*>(filenames[i].c_str());
		fsize[i] = sizes[i];
	}
	int result = createArchiveofFilesPC(filen, fsize, filenames.size(),filename.c_str(), tarHostDirectory.c_str());
	delete[] filen;
	delete[] fsize;
	return result;
}


int LibArchiveWrapper::archive(std::vector<std::string> files,
		std::string filename, std::string tarHostDirectory) {
	char** filenames = new char*[files.size()];
	unsigned long * fsize = new unsigned long[files.size()];
	for (int i = 0; i < files.size(); i++) {
		filenames[i] = const_cast<char*>(files[i].c_str());
		fsize[i] = boost::filesystem::file_size(boost::filesystem::path(files[i]));
	}
	int result = createArchiveofFilesPC(filenames, fsize,files.size(),
			filename.c_str(), tarHostDirectory.c_str());
	delete[] filenames;
	delete[] fsize;
	return result;
}



int LibArchiveWrapper::archive(std::vector<std::string> files,
		std::string filename) {
	char** filenames = new char*[files.size()];
	unsigned long * fsize = new unsigned long[files.size()];
	for (int i = 0; i < files.size(); i++) {
		filenames[i] = const_cast<char*>(files[i].c_str());
		fsize[i] = boost::filesystem::file_size(boost::filesystem::path(files[i]));
	}
	int result = createArchiveofFiles(filenames, fsize,files.size(),
			filename.c_str());
	delete[] filenames;
	delete[] fsize;
	return result;
}

extern "C" {

int extractArchive(const char* filename, const char* directory) {
	struct archive *a;
	struct archive *ext;
	struct archive_entry *entry;
	int r;
	int flags = ARCHIVE_EXTRACT_TIME;
	int warnCount = 0;
	int dirlen = strlen(directory);
	a = archive_read_new();
	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);
	/*
	 * Note: archive_write_disk_set_standard_lookup() is useful
	 * here, but it requires library routines that can add 500k or
	 * more to a static executable.
	 */
	archive_read_support_compression_gzip(a);
	archive_read_support_format_tar(a);
	/*
	 * On my system, enabling other archive formats adds 20k-30k
	 * each.  Enabling gzip decompression adds about 20k.
	 * Enabling bzip2 is more expensive because the libbz2 library
	 * isn't very well factored.
	 */
	if (filename != NULL && strcmp(filename, "-") == 0)
		filename = NULL;
	if ((r = archive_read_open_file(a, filename, 10240))) {
		printf("%s\n", archive_error_string(a));
		return -1;
	}
	while (true) {
		r = archive_read_next_header(a, &entry);
		if (r == ARCHIVE_EOF)
			break;
		if (r != ARCHIVE_OK) {
			printf("%s\n", archive_error_string(a));
			return -1;
		}
		//Set entry to be extracted under the given directory
		const char* path = archive_entry_pathname(entry);
		int pathlength = dirlen + strlen(path) + 2; //One for / and the other for '\0'
		char newPath[pathlength];
		snprintf(newPath, pathlength, "%s/%s", directory, path);
		archive_entry_set_pathname(entry, newPath);

		r = archive_write_header(ext, entry);
		if (r != ARCHIVE_OK) {
			printf("%s\n", archive_error_string(ext));
			warnCount++;
		} else {
			copy_data(a, ext);
			r = archive_write_finish_entry(ext);
			if (r != ARCHIVE_OK) {
				printf("%s\n", archive_error_string(ext));
				warnCount++;
			}
		}
	}
	archive_read_close(a);
	//archive_read_free(a);
	return warnCount;
}

int copy_data(struct archive* ar, struct archive* aw) {
	int r;
	const void *buff;
	size_t size;
#if ARCHIVE_VERSION >= 3000000
	int64_t offset;
#else
	off_t offset;
#endif

	while (true) {
		r = archive_read_data_block(ar, &buff, &size, &offset);
		if (r == ARCHIVE_EOF)
			return (ARCHIVE_OK);
		if (r != ARCHIVE_OK)
			return (r);
		r = archive_write_data_block(aw, buff, size, offset);
		if (r != ARCHIVE_OK) {
			printf("%s\n", archive_error_string(aw));
			return (r);
		}
	}

}


int createArchiveofFiles(char** files, unsigned long * size,
		unsigned int fileCount, const char* filename) {
	unsigned int ctr = 0;
	struct timespec ts;
	struct archive_entry* entry;
	struct archive* archive = archive_write_new();
	if ((archive_write_set_compression_gzip(archive) != ARCHIVE_OK)
			|| (archive_write_set_format_ustar(archive) != ARCHIVE_OK)
			|| (archive_write_open_filename(archive, filename) != ARCHIVE_OK)) {
		printf("%s\n", archive_error_string(archive));
		return -1;
	}
	for (ctr = 0; ctr < fileCount; ctr++) {
		entry = archive_entry_new();
		clock_gettime(CLOCK_REALTIME, &ts);

		archive_entry_set_pathname(entry, files[ctr]);
		archive_entry_set_size(entry, size[ctr]);
		archive_entry_set_filetype(entry, AE_IFREG);
		archive_entry_set_perm(entry, 0444);
		archive_entry_set_atime(entry, ts.tv_sec, ts.tv_nsec);
		archive_entry_set_birthtime(entry, ts.tv_sec, ts.tv_nsec);
		archive_entry_set_ctime(entry, ts.tv_sec, ts.tv_nsec);
		archive_entry_set_mtime(entry, ts.tv_sec, ts.tv_nsec);

		int rc = archive_write_header(archive, entry);
		char *contents = new char[size[ctr]+1];
		FILE* fp = fopen(files[ctr],"rb");
		fread((void *)contents, size[ctr], 1, fp);
		fclose(fp);
		archive_write_data(archive, contents, size[ctr]);
		archive_entry_free(entry);
		entry = NULL;
		delete[] contents;
		if (ARCHIVE_OK != rc) {
			printf("%s\n", archive_error_string(archive));
			return -1;
		}
	}
	archive_write_finish(archive);
}

int createArchiveofFilesPC(char** files, unsigned long * size,
		unsigned int fileCount, const char* filename, const char* tarHostDir) {
	unsigned int ctr = 0;
	struct timespec ts;
	struct archive_entry* entry;
	struct archive* archive = archive_write_new();
	int dirlen = strlen(tarHostDir);
	if ((archive_write_set_compression_gzip(archive) != ARCHIVE_OK)
			|| (archive_write_set_format_ustar(archive) != ARCHIVE_OK)
			|| (archive_write_open_filename(archive, filename) != ARCHIVE_OK)) {
		printf("%s\n", archive_error_string(archive));
		return -1;
	}
	int tarHostDirLen = strlen(tarHostDir);
	for (ctr = 0; ctr < fileCount; ctr++) {
		entry = archive_entry_new();
		clock_gettime(CLOCK_REALTIME, &ts);

		//Set entry to be stored under the tarHostDir directory
		const char* path = files[ctr];
		int pathlength = dirlen + strlen(path) + 2; //One for / and the other for '\0'
		char newPath[pathlength];
		if(tarHostDirLen>0)
			snprintf(newPath, pathlength, "%s/%s", tarHostDir, boost::filesystem::path(path).filename().c_str());
		else
			snprintf(newPath, pathlength, "%s", boost::filesystem::path(path).filename().c_str());
		archive_entry_set_pathname(entry, newPath);
		archive_entry_set_size(entry, size[ctr]);
		archive_entry_set_filetype(entry, AE_IFREG);
		archive_entry_set_perm(entry, 0444);
		archive_entry_set_atime(entry, ts.tv_sec, ts.tv_nsec);
		archive_entry_set_birthtime(entry, ts.tv_sec, ts.tv_nsec);
		archive_entry_set_ctime(entry, ts.tv_sec, ts.tv_nsec);
		archive_entry_set_mtime(entry, ts.tv_sec, ts.tv_nsec);

		int rc = archive_write_header(archive, entry);
		char *contents = new char[size[ctr]+1];
		FILE* fp = fopen(files[ctr],"rb");
		fread((void *)contents, size[ctr], 1, fp);
		fclose(fp);
		archive_write_data(archive, contents, size[ctr]);
		archive_entry_free(entry);
		entry = NULL;
		delete[] contents;
		if (ARCHIVE_OK != rc) {
			printf("%s\n", archive_error_string(archive));
			return -1;
		}
	}
	archive_write_finish(archive);
}


}
