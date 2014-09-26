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


#ifndef CMISSDICOMENCAPSULATOR_H_
#define CMISSDICOMENCAPSULATOR_H_
#include <vector>
#include "itkMatrix.h"
#include "itkRGBPixel.h"
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkExtractImageFilter.h"
#include "itkJoinSeriesImageFilter.h"
#include "itkResampleImageFilter.h"
#include "itkIdentityTransform.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkGDCMImageIO.h"
#include "itkJPEGImageIO.h"
#include "gdcmUIDGenerator.h"
#include "itkImageSeriesWriter.h"
#include "DCMTKUtils.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>

#include "MarkerPlotter.h"

#include "DICOMInputManager.h"
/** \class CMISSDICOMEncapsulator
 * \brief A service class that bundles different View frames and cmiss data into a single dicom
 *
 * This class provides the facilities for bundling frames from different views, associated metadata
 * This file needs to be updated with exnode and exelem files when the fitting process is completed
 */

class CMISSDICOMEncapsulator
{
  //Private types
  typedef itk::Image<PixelType, 2> SliceImageType;
  typedef itk::JoinSeriesImageFilter<SliceImageType, OutputImageType> JoinSeriesImageFilterType;
  typedef itk::ImageSeriesWriter<InputImageType, OutputImageType> DICOM3DWriterType;
  typedef itk::ImageSeriesReader<SliceImageType> ReaderType;
  typedef itk::ImageSeriesReader<InputImageType> DReaderType;
  typedef itk::ImageFileWriter<SliceImageType> ImageWriterType;
  typedef itk::GDCMImageIO ImageIOType;
  typedef itk::JPEGImageIO JpegIOType;
  typedef itk::ExtractImageFilter<InputImageType, SliceImageType> ExtractImageFilterType;
  typedef itk::ResampleImageFilter<SliceImageType, SliceImageType> ResampleImageFilterType;
  typedef itk::IdentityTransform<double, 2> TransformType;
  typedef itk::NearestNeighborInterpolateImageFunction<SliceImageType> InterpolatorType;

  std::string tagApplicationName; // Application Name tag 0018,9524 - CMISS
  std::string tagTransducerData; //TransducerData 0018,5010  - Details of view rotation angle, frame count, apex, basel,baser
  InputImageType::Pointer image; //Pointer to the 3D dicom image composed for the input views
  std::string filename; //Name of the output dicom file
  std::string workingDirectory; //The directory where the dicom file would be stored
  std::vector<DICOMInputManager*> views; //The ultrasound views to be bundled
  std::vector<std::vector<std::vector<double*>*>*>* imageMap;
  std::vector<std::string> viewNames; //Names ultrasound views to be bundled
  std::string processingFunction; //Brief descriptor for annotating the model file

  double m_TargetWidth; //The target width of the frame image - defaults to 636 pixels
  double m_TargetHeight; //The target height of the frame image - defaults to 434 pixels

  unsigned int resolution; //The number of frames the use wants

  /**
   * Extract a 2D slice from 3D Image
   * Member has been defined in the header as SliceImageType is private and not accessible in cpp file
   * @param a Input3DImageType::Pointer to source image from which the slice needs to extracted
   * @param an integer describing the plane
   * @param an integer describing the frame to be extracted
   * @return a smart pointer to the extracted slice
   */
  SliceImageType::Pointer
  extract2DImageSlice(InputImageType::Pointer dicomImage, int plane, int slice);

  /**
   * Retrieves the frames from each view and creates a 3D image by joining them
   * in the time/z direction. The resuling image is stored in the member image
   * @return is an integer, the number of frames that were joined
   */
  int
  composeImages();

public:
  /**
   * Simple constructor that takes the target DICOM filename as input
   * @param is std:string
   */
  CMISSDICOMEncapsulator(std::string outputDICOM, unsigned int resol);

  /**
   * Set the processing function description to enable to user to
   * get an idea regarding the model
   * @param a std::string
   */
  void
  setProcessingFunction(std::string desc);
  /**
   * Get the current processing function description
   */
  std::string
  getProcessingFunction();
  /**
   * Set the Views that need to be bundled into a single DICOM
   * @param is a std::vector of Pointers to UltrasoundViewManager
   */
  inline void
  setViews(std::vector<std::string>& myViewNames, std::vector<DICOMInputManager*>& myViews)
  {
    views = myViews;
    viewNames = myViewNames;
  }

  inline void
  setImageMap(std::vector<std::vector<std::vector<double*>*>*>* iMap)
  {
    imageMap = iMap;
  }
  /**
   * Set the directory where the exnode, exelem model files are stored. These would be
   * bundled into the DICOM
   * @param is a std::string
   * @return is a std::string, the expected output dcm filename
   */
  inline std::string
  setWorkingDirectory(std::string wDir)
  {
    boost::filesystem::path dir(wDir);
    workingDirectory = dir.string();
    boost::filesystem::path qfilename = dir / filename;

    return qfilename.string();
  }
  /**
   * Get the directory where the bundled DICOM file will be stored
   * @return is a std::string
   */
  inline std::string
  getWorkingDirectory()
  {
    return workingDirectory;
  }

  /**
   * Saves the images in the views to files. Target directory is the output directory
   */
  void
  saveViewFrames();

  /**
   * Builds and saves a DICOM file with the header information present in the parameter
   * The input vector is expected to be a vector of tagkey,tagvalue pairs. Unique sopiuid
   * and other private tags are added to the composed file.
   * @param is a std::vector of std::pair of std::string
   * @return is a std::string
   */
  std::string
  buildOutput(std::vector<std::pair<std::string, std::string> > dict);
  virtual
  ~CMISSDICOMEncapsulator();
};

#endif /* CMISSDICOMENCAPSULATOR_H_ */
