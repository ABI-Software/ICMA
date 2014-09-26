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


#ifndef DICOMIMAGE_H_
#define DICOMIMAGE_H_
#include <string>
#include <vector>
#include "Point3D.h"

class DICOMImage
{
public:
	explicit DICOMImage(const std::string& filename);
	~DICOMImage()
	{
	}

	std::string const& GetFilename()
	{
		return filename_;
	}
	
	std::pair<Vector3D,Vector3D> GetImageOrientation() const
	{
		return std::make_pair(orientation1_, orientation2_);
	}
	
	Point3D const& GetImagePosition() const
	{
		return position3D_;
	}
	
	std::string const& GetSeriesDescription() const
	{
		return seriesDescription_;
	}
	
	std::string const& GetSequenceName() const
	{
		return sequenceName_;
	}
	
	double GetTriggerTime() const
	{
		return triggerTime_;
	}
	
	std::string const& GetPatientName() const
	{
		return patientName_;
	}
	
	std::string const& GetPatientID() const
	{
		return patientId_;
	}
	
	std::string const& GetScanDate() const
	{
		return scanDate_;
	}
	
	std::string const& GetDateOfBirth() const
	{
		return dateOfBirth_;
	}
	
	std::string const& GetGender() const
	{
		return gender_;
	}
	
	std::string const& GetAge() const
	{
		return age_;
	}
	
	int GetSeriesNumber() const
	{
		return seriesNumber_;
	}
	
	size_t GetImageWidth() const
	{
		return width_;
	}
	
	size_t GetImageHeight() const
	{
		return height_;
	}
	
	std::string const& GetStudyInstanceUID() const
	{
		return studyInstanceUID_;
	}
	
	std::string const& GetSopInstanceUID() const
	{
		return sopInstanceUID_;
	}
	
	std::string const& GetSeriesInstanceUID() const
	{
		return seriesInstanceUID_;
	}
	
	int GetInstanceNumber() const
	{
		return instanceNumber_;
	}
	
	std::vector<Point3D> getImagePlaneCoordinates();

private:
	void ReadDICOMFile();
	
	std::string filename_;
	unsigned int width_;
	unsigned int height_;
	double pixelSizeX_, pixelSizeY_;
	
	
	std::string studyInstanceUID_;
	std::string sopInstanceUID_;
	std::string seriesInstanceUID_;
	std::string seriesDescription_;
	std::string sequenceName_;
	double triggerTime_;
	int seriesNumber_;
	Point3D position3D_;
	Vector3D orientation1_, orientation2_;
	
	Point3D shiftedPosition_;
	Vector3D shiftedOrientation1_, shiftedOrientation2_;

	int instanceNumber_;
	
	std::string patientName_;
	std::string patientId_;
	std::string scanDate_;
	std::string dateOfBirth_;
	std::string gender_;
	std::string age_;
	
	bool isShifted_;
	bool isRotated_;
};

#endif /* DICOMIMAGE_H_ */
