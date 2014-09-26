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


#include "DCMTKUtils.h"



void DCMTKUtils::compressDICOM(std::string input, std::string output)
{
	//DJEncoderRegistration::registerCodecs(); // register JPEG codecs
	DcmFileFormat fileformat;
	E_TransferSyntax opt_oxfer = EXS_JPEGProcess14SV1;

	if (fileformat.loadFile(input.c_str()).good())
	{
	  DcmDataset *dataset = fileformat.getDataset();
	  DcmItem *metaInfo = fileformat.getMetaInfo();
	  DJ_RPLossless params; // codec parameters, we use the defaults

	  // this causes the lossless JPEG version of the dataset to be created
	  dataset->chooseRepresentation(opt_oxfer, &params);

	  // check if everything went well
	  if (dataset->canWriteXfer(opt_oxfer))
	  {
	    // force the meta-header UIDs to be re-generated when storing the file
	    // since the UIDs in the data set may have changed
	    delete metaInfo->remove(DCM_MediaStorageSOPClassUID);
	    delete metaInfo->remove(DCM_MediaStorageSOPInstanceUID);

	    // store in lossless JPEG format
	    fileformat.saveFile(output.c_str(), opt_oxfer);
	  }else{
			throw new SegmentationAndFittingException("Error compressing input DICOM file");
	  }
	}
	//DJEncoderRegistration::cleanup(); // deregister JPEG codecs
}



void DCMTKUtils::decompressDICOM(std::string input, std::string output)
{
	//DJDecoderRegistration::registerCodecs(); // register JPEG codecs
	DcmFileFormat fileformat;
	if (fileformat.loadFile(input.c_str()).good())
	{
	  DcmDataset *dataset = fileformat.getDataset();

	  // decompress data set if compressed
	  dataset->chooseRepresentation(EXS_LittleEndianExplicit, NULL);

	  // check if everything went well
	  if (dataset->canWriteXfer(EXS_LittleEndianExplicit))
	  {
	    fileformat.saveFile(output.c_str(), EXS_LittleEndianExplicit);
	  }
	}else{
		std::cout<<"Error opening input DICOM file "<<input<<std::endl;
		throw new SegmentationAndFittingException("Error opening input DICOM file");
	}
	//DJDecoderRegistration::cleanup(); // deregister JPEG codecs
}

std::string  DCMTKUtils::getHeaderAsXML(std::string input){
	//DJDecoderRegistration::registerCodecs(); // register JPEG codecs
	DcmFileFormat fileformat;
	std::ostringstream ss;
	if (fileformat.loadFile(input.c_str()).good())
	{
	  DcmDataset *dataset = fileformat.getDataset();
	  dataset->writeXML(ss);
	}else{
		std::cout<<"Error opening input DICOM file "<<input<<std::endl;
		throw new SegmentationAndFittingException("Error opening input DICOM file");
	}
	//DJDecoderRegistration::cleanup(); // deregister JPEG codecs
	return ss.str();
}

DCMTKUtils::DCMTKUtils() {
	DJDecoderRegistration::registerCodecs(); // register JPEG codecs
}

DCMTKUtils::~DCMTKUtils() {
	//DJDecoderRegistration::cleanup(); // deregister JPEG codecs
}
