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
/*
 * ImageFileReader.h
 *
 *  Created on: 15/05/2014
 *      Author: rjag008
 */

#ifndef IMAGEFILEREADER_H_
#define IMAGEFILEREADER_H_
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include "itkImageFileReader.h"
#include "itkGDCMImageIO.h"
#include "itkJPEGImageIO.h"
#include "itkMetaDataObject.h"
#include "itkIntensityWindowingImageFilter.h"

#include "itkResampleImageFilter.h"
#include "itkAffineTransform.h"
#include "itkNearestNeighborInterpolateImageFunction.h"


template<typename ImageType>
class ImageFileReader {
	typedef itk::Image<double,2> InternalImageType;
	typedef itk::GDCMImageIO ImageIOType;
	typename ImageType::Pointer image;
	unsigned int targetHeight;
	unsigned int targetWidth;
	std::string filename;
public:
	ImageFileReader(std::string filename,double theight,double twidth);
	virtual ~ImageFileReader();
	void Update();
	typename ImageType::Pointer GetOutput();
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "ImageFileReader.hxx"
#endif

#endif /* IMAGEFILEREADER_H_ */
