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

template<typename SliceImageType>
inline MarkerPlotter<SliceImageType>::MarkerPlotter(unsigned int width, unsigned int height) : xw(width), yw(height) {
}

template<typename SliceImageType>
inline MarkerPlotter<SliceImageType>::~MarkerPlotter() {
}

template<typename SliceImageType>
inline void MarkerPlotter<SliceImageType>::plotToFile(std::string filename, typename SliceImageType::Pointer image, std::vector<Point3D> pts) {
	typedef typename itk::ImageFileWriter<SliceImageType> WriterType;
	typename WriterType::Pointer write = WriterType::New();
	typename SliceImageType::Pointer result;
	typename SliceImageType::SizeType size = image->GetLargestPossibleRegion().GetSize();
	typedef typename itk::ImageDuplicator<SliceImageType> DuplicatorType;
	try {
		typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
		duplicator->SetInputImage(image);
		duplicator->Update();

		result = duplicator->GetOutput();
		result->DisconnectPipeline();
		//	result->FillBuffer(0);
		for (unsigned int i = 0; i < pts.size(); i++) {
			typename SliceImageType::IndexType speckleIndex;
			speckleIndex[0] = pts[i].x;
			speckleIndex[1] = pts[i].y;
			try {
				int xv = -xw;
				while (xv <= (int) xw) {
					int yv = -yw;
					while (yv <= (int) yw) {
						typename SliceImageType::IndexType pixel = speckleIndex;
						pixel[0] += xv;
						pixel[1] += yv;
						if (pixel[0] < size[0] && pixel[0] > -1 && pixel[1] < size[1] && pixel[1] > -1)
							result->SetPixel(pixel, 255);
						yv++;
					}
					xv++;
				}
			} catch (itk::ExceptionObject& obj) {
				std::cout << "Speckle value was " << pts[i] << std::endl;
				std::cout << " Exception was " << obj.what() << std::endl;
			}
		}

	} catch (itk::ExceptionObject& obj) {
		std::cout << obj.what() << std::endl;
		std::cout << "Offending image returned" << std::endl;
		result = image;
	}
	write->SetFileName(filename);
	write->SetInput(result);
	write->Update();
}
