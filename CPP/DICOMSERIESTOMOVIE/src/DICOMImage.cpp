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


#include "DICOMImage.h"
#include "gdcmReader.h"
#include "gdcmAttribute.h"

#include "math.h"

#include <string>
#include <ostream>
#include <boost/algorithm/string.hpp>


DICOMImage::DICOMImage(const std::string& filename)
	: filename_(filename),
	  isShifted_(false),
	  isRotated_(false)
{
	ReadDICOMFile();
}

std::vector<Point3D> DICOMImage::getImagePlaneCoordinates() {
	Point3D tlc,trc,blc,brc;
	Point3D pos = position3D_;

	tlc = pos - 0.5*pixelSizeX_*orientation1_ - 0.5*pixelSizeY_*orientation2_;

	double fieldOfViewX = width_*pixelSizeX_;

	trc = tlc + fieldOfViewX*orientation1_;
	double fieldOfViewY = height_*pixelSizeY_;

	blc = tlc + fieldOfViewY*orientation2_;

	brc = blc + (trc - tlc);

	std::vector<Point3D> result;
	result.push_back(tlc);
	result.push_back(trc);
	result.push_back(blc);
	result.push_back(brc);
	return result;
}

void DICOMImage::ReadDICOMFile()
{
	// study instance uid (0020,000d)
	// sop instance uid (0008,0018) 
	// rows (0028,0010)
	// columns (0028,0011)
	// slice thickness_ (0018,0050)
	// image position (0020,0032) - (0020,0030)(old)
	// image orientation (0020,0037) - (0020,0035)(old)
	// pixel spacing (0028,0030)
	// series description (0008,103e) 
	// trigger time (0018,1060) 
	// (0018,1090) IS [28]         # 2,1 Cardiac Number of Images

	gdcm::Reader r;
	r.SetFileName( filename_.c_str() );
	if( !r.Read() )
	{
		std::cout << "Can't read file: " << filename_ << std::endl;
		throw std::exception();
	}
	
	gdcm::DataSet const& ds = r.GetFile().GetDataSet();
	
	{
		// SOP instance UID (0008,0018) 
		const gdcm::DataElement& sopiuid = ds.GetDataElement(gdcm::Tag(0x0008,0x0018));
		gdcm::Attribute<0x0008,0x0018> at_sopiuid;
		at_sopiuid.SetFromDataElement(sopiuid);
		sopInstanceUID_ = at_sopiuid.GetValue();
		// gdcm leaves some non alpha numeric characters at the back
		// get rid of them here
		boost::trim_right_if(sopInstanceUID_, !boost::is_digit());

	}
	
	{
		// Study Instance UID (0020,000d)
		const gdcm::DataElement& studyiuid = ds.GetDataElement(gdcm::Tag(0x0020,0x000d));
		gdcm::Attribute<0x0020,0x000d> at_studyiuid;
		at_studyiuid.SetFromDataElement(studyiuid);
		studyInstanceUID_ = at_studyiuid.GetValue();
		boost::trim_right_if(studyInstanceUID_, !boost::is_digit());
	}
	
	{
		// Series Instance UID (0020,000E)
		const gdcm::DataElement& seriesiuid = ds.GetDataElement(gdcm::Tag(0x0020,0x000e));
		gdcm::Attribute<0x0020,0x000e> at_seriesiuid;
		at_seriesiuid.SetFromDataElement(seriesiuid);
		seriesInstanceUID_ = at_seriesiuid.GetValue();
		boost::trim_right_if(seriesInstanceUID_, !boost::is_digit());
	}
	
	// series number (0020,0011)
	if (ds.FindDataElement(gdcm::Tag(0x0020,0x0011)))
	{
		const gdcm::DataElement& seriesNum = ds.GetDataElement(gdcm::Tag(0x0020,0x0011));
		gdcm::Attribute<0x0020,0x0011> at_sn;
		at_sn.SetFromDataElement(seriesNum);
		seriesNumber_ = at_sn.GetValue();
	}
	else
	{
		std::cout << "Series number not found\n";
		throw std::exception();
	}
	
	// series description (0008,103e)
	if (ds.FindDataElement(gdcm::Tag(0x0008,0x103E)))
	{
		const gdcm::DataElement& seriesDesc = ds.GetDataElement(gdcm::Tag(0x0008,0x103E));
		gdcm::Attribute<0x0008,0x103E> at_sd;
		at_sd.SetFromDataElement(seriesDesc);
		seriesDescription_ = at_sd.GetValue();
	}
	else
	{
		seriesDescription_ = "";
	}
	
	// sequence name (0018,0024)
	if (ds.FindDataElement(gdcm::Tag(0x0018,0x0024)))
	{
		const gdcm::DataElement& seqName = ds.GetDataElement(gdcm::Tag(0x0018,0x0024));
		gdcm::Attribute<0x0018,0x0024> at_seqName;
		at_seqName.SetFromDataElement(seqName);
		sequenceName_ = at_seqName.GetValue();
	}
	else
	{
		sequenceName_ = "";
	}
	
	// trigger time trigger time (0018,1060)
	if (ds.FindDataElement(gdcm::Tag(0x0018,0x1060)))
	{
		const gdcm::DataElement& triggerTime = ds.GetDataElement(gdcm::Tag(0x0018,0x1060));
		gdcm::Attribute<0x0018,0x1060> at_tt;
		at_tt.SetFromDataElement(triggerTime);
		triggerTime_ = at_tt.GetValue();
	}
	else
	{
		triggerTime_ = -1;
	}

	if (ds.FindDataElement(gdcm::Tag(0x0028,0x0010)))
	{
		const gdcm::DataElement& rows = ds.GetDataElement(gdcm::Tag(0x0028,0x0010));
		gdcm::Attribute<0x0028,0x0010> at_rows;
		at_rows.SetFromDataElement(rows);
		height_ = at_rows.GetValue();
	}
	else
	{
		throw std::exception();
	}

	{
		const gdcm::DataElement& cols = ds.GetDataElement(gdcm::Tag(0x0028,0x0011));
		gdcm::Attribute<0x0028,0x0011> at_cols;
		at_cols.SetFromDataElement(cols);
		width_ = at_cols.GetValue();
	}


	if (ds.FindDataElement(gdcm::Tag(0x0020,0x0032)))
	{
		const gdcm::DataElement& position = ds.GetDataElement(gdcm::Tag(0x0020,0x0032));
		gdcm::Attribute<0x0020,0x0032> at;
		at.SetFromDataElement(position);
		position3D_ = Point3D(at[0],at[1],at[2]);
	}
	else
	{
		std::cout << "Image Position not found.\n";
		throw std::exception();
	}

	if (ds.FindDataElement(gdcm::Tag(0x0020,0x0037)))
	{
		const gdcm::DataElement& orientation = ds.GetDataElement(gdcm::Tag(0x0020,0x0037));
		gdcm::Attribute<0x0020,0x0037> at_ori;
		at_ori.SetFromDataElement(orientation);
		orientation1_ = Vector3D(at_ori[0],at_ori[1],at_ori[2]);
		orientation2_ = Vector3D(at_ori[3],at_ori[4],at_ori[5]);
	}
	else
	{
		std::cout << "(0x20,0x37) Image Orientation - Patient is missing. We will compute the orientation from (0x20,0x35) instead.\n";
		const gdcm::DataElement& orientation = ds.GetDataElement(gdcm::Tag(0x0020,0x0035));
		gdcm::Attribute<0x0020,0x0035> at_ori; //test
		at_ori.SetFromDataElement(orientation);
		orientation1_ = Vector3D(-at_ori[0],at_ori[1],-at_ori[2]);
		orientation2_ = Vector3D(-at_ori[3],at_ori[4],-at_ori[5]);
	}
	
	if (ds.FindDataElement(gdcm::Tag(0x0028,0x0030)))
	{
		const gdcm::DataElement& spacing = ds.GetDataElement(gdcm::Tag(0x0028,0x0030));
		gdcm::Attribute<0x0028,0x0030> at_spc;
		at_spc.SetFromDataElement(spacing);
		pixelSizeX_ = at_spc[0];
		pixelSizeY_ = at_spc[1];
	}
	else
	{
		std::cout << "Pixel Spacing not found\n";
		throw std::exception();
	}
	
	//patient name (0010,0010) 
	if (ds.FindDataElement(gdcm::Tag(0x0010,0x0010)))
	{
		const gdcm::DataElement& de = ds.GetDataElement(gdcm::Tag(0x0010,0x0010));
		gdcm::Attribute<0x0010,0x0010> at;
		at.SetFromDataElement(de);
		patientName_ = at.GetValue();
	}
	else
	{
		patientName_ = "N/A";
	}
	
	//patient id (0010,0020)
	if (ds.FindDataElement(gdcm::Tag(0x0010,0x0020)))
	{
		const gdcm::DataElement& de = ds.GetDataElement(gdcm::Tag(0x0010,0x0020));
		gdcm::Attribute<0x0010,0x0020> at;
		at.SetFromDataElement(de);
		patientId_ = at.GetValue();
	}
	else
	{
		patientId_ = "N/A";
	}
	
	//acquisition date (0008,0022) 
	if (ds.FindDataElement(gdcm::Tag(0x0008,0x0022)))
	{
		const gdcm::DataElement& de = ds.GetDataElement(gdcm::Tag(0x0008,0x0022));
		gdcm::Attribute<0x0008,0x0022> at;
		at.SetFromDataElement(de);
		scanDate_ = at.GetValue();
	}
	else
	{
		scanDate_ = "N/A";
	}
	
	
	//date of birth (0010,0030)
	if (ds.FindDataElement(gdcm::Tag(0x0010,0x0030)))
	{
		const gdcm::DataElement& de = ds.GetDataElement(gdcm::Tag(0x0010,0x0030));
		gdcm::Attribute<0x0010,0x0030> at;
		at.SetFromDataElement(de);
		dateOfBirth_ = at.GetValue();
	}
	else
	{
		dateOfBirth_ = "N/A";
	}
	
	//gender (0010,0040)
	if (ds.FindDataElement(gdcm::Tag(0x0010,0x0040)))
	{
		const gdcm::DataElement& de = ds.GetDataElement(gdcm::Tag(0x0010,0x0040));
		gdcm::Attribute<0x0010,0x0040> at;
		at.SetFromDataElement(de);
		gender_ = at.GetValue();
	}
	else
	{
		gender_ = "N/A";
	}
	
	//age (0010,1010)
	if (ds.FindDataElement(gdcm::Tag(0x0010,0x1010)))
	{
		const gdcm::DataElement& de = ds.GetDataElement(gdcm::Tag(0x0010,0x1010));
		gdcm::Attribute<0x0010,0x1010> at;
		at.SetFromDataElement(de);
		age_ = at.GetValue();
	}
	else
	{
		age_ = "N/A";
	}
	
	//instanceNumber_ (0020,0013)
	if (ds.FindDataElement(gdcm::Tag(0x0020,0x0013)))
	{
		const gdcm::DataElement& de = ds.GetDataElement(gdcm::Tag(0x0020,0x0013));
		gdcm::Attribute<0x0020,0x0013> at;
		at.SetFromDataElement(de);
		instanceNumber_ = at.GetValue();
	}
	else
	{
		instanceNumber_ = -1;
	}
	
}

