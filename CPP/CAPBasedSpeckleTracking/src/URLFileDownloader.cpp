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

#ifndef URLFILEDOWNLOADER_CPP_
#define URLFILEDOWNLOADER_CPP_

#include "URLFileDownloader.h"
#include "SegmentationAndFittingException.h"
int URLFileDownloader::curl_init = 0;

static int HeaderTransferProgress(void *clientp, double dltotal, double dlnow,
		double ultotal, double ulnow) {
	unsigned long int* headersize = (unsigned long int*) clientp;
	// Download everything if headersize is 0
	if (headersize[0] == 0) {
		return 0;
	}
	if (dlnow > headersize[0]) {
		return 1;
	}
	return 0;
}

URLFileDownloader::URLFileDownloader(std::string dir) {
	userid = "";
	password= "";
	//This is not thread safe, so check it using a static handle
	if (curl_init == 0) {
		curl_global_init(CURL_GLOBAL_ALL);
	}
	//Count the number of instances
	//so that when the instances are deleted, clean up can be called when curl_init = 0
	curl_init++;
	workingDir = dir;
}

URLFileDownloader::~URLFileDownloader() {
	//Delete the files
	for (int i = 0; i < filenames.size(); i++) {

		std::string dirname = workingDir;
		if (workingDir[workingDir.size() - 1] == '/'
				|| workingDir[workingDir.size() - 1] == '\\') {
			dirname = workingDir.substr(0, workingDir.size() - 1);
		}

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
		std::string fn = dirname+std::string("\\")+filenames[i];
#else
		std::string fn = dirname + std::string("/") + filenames[i];
#endif
		remove(fn.c_str());
	}
	curl_init--;
	if (curl_init == 0) {
		curl_global_cleanup();
	}
}


int URLFileDownloader::loadURLToFile(std::string uri, std::string filename) {
	std::vector<std::string>::iterator it;
	for (it = filenames.begin(); it != filenames.end(); it++) {
		if ((*it).compare(filename) == 0) {
			filenames.erase(it);
			break;
		}
	}
	unsigned int MaxSize = 0; //zero implies read everything
	//Get Curl started
	CURL* curlHandle = curl_easy_init();
	if (curlHandle == NULL) {
		SAFTHROW("Failed to initialise CURL")
	}
	curl_easy_reset(curlHandle);
	// Start reading 1 meg at most
	curl_easy_setopt( curlHandle, CURLOPT_PROGRESSFUNCTION,
			HeaderTransferProgress);
	curl_easy_setopt( curlHandle, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt( curlHandle, CURLOPT_PROGRESSDATA, &MaxSize);
	curl_easy_setopt( curlHandle, CURLOPT_WRITEFUNCTION, NULL);
	curl_easy_setopt( curlHandle, CURLOPT_FOLLOWLOCATION, true);
	curl_easy_setopt( curlHandle, CURLOPT_URL, uri.c_str());
	curl_easy_setopt( curlHandle, CURLOPT_HTTPGET, 1);
	curl_easy_setopt( curlHandle, CURLOPT_WRITEFUNCTION, NULL);

	if(userid.length()==0){
		curl_easy_setopt( curlHandle, CURLOPT_USERNAME, userid.c_str());
		curl_easy_setopt( curlHandle, CURLOPT_PASSWORD, password.c_str());
	}

	if(proxy.length()==0){
		curl_easy_setopt( curlHandle, CURLOPT_PROXY, proxy.c_str());
	}
	// Open the filename
	std::string dirname = workingDir;
	if (workingDir[workingDir.size() - 1] == '/'
			|| workingDir[workingDir.size() - 1] == '\\') {
		dirname = workingDir.substr(0, workingDir.size() - 1);
	}

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
	std::string fn = dirname+std::string("\\")+filename;
#else
	std::string fn = dirname + std::string("/") + filename;
#endif

	FILE* f = fopen(fn.c_str(), "wb");
	curl_easy_setopt( curlHandle, CURLOPT_WRITEDATA, f);
	CURLcode retval = curl_easy_perform(curlHandle);
	fclose(f);
	//Clean up curl
	curl_easy_cleanup(curlHandle);
	// Data sucessfully read
	if (retval == CURLE_OK || retval == CURLE_ABORTED_BY_CALLBACK) {
		filenames.push_back(filename);
		return 0;
	} else {
		return (int) retval;
	}

}

#endif
