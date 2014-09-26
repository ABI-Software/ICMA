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

#ifndef CMISSDICOMENCAPSULATOR_CPP_
#define CMISSDICOMENCAPSULATOR_CPP_
#include "CMISSDICOMEncapsulator.h"

int
CMISSDICOMEncapsulator::composeImages()
{
  MarkerPlotter plotter(5, 5);
  if (views.size() > 0)
    {
      if (imageMap == NULL)
        {
          JoinSeriesImageFilterType::Pointer joiner = JoinSeriesImageFilterType::New();
          double origin = 0.0;
          double spacing = 1.0;
          joiner->SetOrigin(origin);
          joiner->SetSpacing(spacing);
          for (int i = 0; i < views.size(); i++)
            {
              DICOMInputManager* curview = views[i];
              InputImageType::Pointer image = curview->getTargetDicom();
              float* fids = curview->getFrames();
              //Add the first frame - this logic should match the logic in MotionEstimatorThread::speckleTrack()
              joiner->PushBackInput(extract2DImageSlice(image, 2, round(fids[0])));
              unsigned int numFrames = curview->getNumberOfFrames();
              double step = ((double) numFrames - 2) / (resolution - 2);
              int frameCtr = 1;
              for (double val = step; frameCtr < (resolution - 1); val += step)
                {
                  unsigned int ix = round(val);
                  joiner->PushBackInput(extract2DImageSlice(image, 2, round(fids[ix])));
                  frameCtr++;
                }
              //Add the last frame
              joiner->PushBackInput(extract2DImageSlice(image, 2, round(fids[numFrames - 1])));
              delete[] fids;
            }
          joiner->Update();
          image = joiner->GetOutput();
          image->DisconnectPipeline();
        }
      else
        {
          JoinSeriesImageFilterType::Pointer joiner = JoinSeriesImageFilterType::New();
          double origin = 0.0;
          double spacing = 1.0;
          joiner->SetOrigin(origin);
          joiner->SetSpacing(spacing);
          for (int i = 0; i < views.size(); i++)
            {
              DICOMInputManager* curview = views[i];
              InputImageType::Pointer image = curview->getTargetDicom();
              float* fids = curview->getFrames();
              //Add the first frame - this logic should match the logic in MotionEstimatorThread::speckleTrack()
              SliceImageType::Pointer slice = extract2DImageSlice(image, 2, round(fids[0]));
              std::vector<double*>* pts = imageMap->at(i)->at(0);

              SliceImageType::Pointer output = plotter.plot(slice, *(pts));
              joiner->PushBackInput(output);

              unsigned int numFrames = curview->getNumberOfFrames();

              double step = ((double) numFrames - 2) / (resolution - 2);
              int frameCtr = 1;
              for (double val = step; frameCtr < (resolution - 1); val += step)
                {
                  unsigned int ix = round(val);
                  slice = extract2DImageSlice(image, 2, round(fids[ix]));
                  pts = imageMap->at(i)->at(frameCtr);
                  output = plotter.plot(slice, *(pts));
                  joiner->PushBackInput(output);
                  frameCtr++;
                }

              //Add the last frame
              slice = extract2DImageSlice(image, 2, round(fids[numFrames - 1]));
              pts = imageMap->at(i)->at(0);
              output = plotter.plot(slice, *(pts));
              joiner->PushBackInput(output);
              delete[] fids;
            }

          joiner->Update();
          image = joiner->GetOutput();
          image->DisconnectPipeline();
        }
    }
  return resolution;
}

CMISSDICOMEncapsulator::CMISSDICOMEncapsulator(std::string outputDICOM, unsigned int resol) :
    tagApplicationName("0018|9524"), tagTransducerData("0018|5010")
{
  imageMap = NULL;
  m_TargetWidth = 800.0;
  m_TargetHeight = 600.0;
  filename = outputDICOM;
  processingFunction = filename;
  workingDirectory = itksys::SystemTools::GetCurrentWorkingDirectory();
  resolution = resol;
}

void
CMISSDICOMEncapsulator::setProcessingFunction(std::string desc)
{
  processingFunction = desc;
}

std::string
CMISSDICOMEncapsulator::getProcessingFunction()
{
  return processingFunction;
}

std::string
CMISSDICOMEncapsulator::buildOutput(std::vector<std::pair<std::string, std::string> > dict)
{
  int count = composeImages();

  ReaderType::DictionaryRawPointer outputDictionary = new ReaderType::DictionaryType;

  typedef itk::MetaDataObject<std::string> MetaDataStringType;

  for (int i = 0; i < dict.size(); i++)
    {
      std::pair<std::string, std::string>& value = dict[i];
      std::string tagkey = value.first;
      std::string tagvalue = value.second;
      itk::EncapsulateMetaData<std::string>(*outputDictionary, tagkey, tagvalue);
    }


  //Set software version 0018,1020
  itk::EncapsulateMetaData<std::string>(*outputDictionary, "0018|1020", "ABI ICMA 0.1");

  //Set processing function 0018,5020
  itk::EncapsulateMetaData<std::string>(*outputDictionary, "0018|5020", processingFunction);

  //Set ApplicationName
  itk::EncapsulateMetaData<std::string>(*outputDictionary, tagApplicationName, "ABI ICMA 0.1");

  //Create a sop uid for the image, while keeping the series and study ids same
  gdcm::UIDGenerator sopUid;
  std::string sopInstanceUID = sopUid.Generate();

  itk::EncapsulateMetaData<std::string>(*outputDictionary, "0008|0018", sopInstanceUID);
  itk::EncapsulateMetaData<std::string>(*outputDictionary, "0002|0003", sopInstanceUID);

  std::ostringstream ss;
  ss << count;
  //Set number of frames to be image count
  itk::EncapsulateMetaData<std::string>(*outputDictionary, "0028|0008", ss.str());
  //Set stop trim to be the image count 0008,2143
  itk::EncapsulateMetaData<std::string>(*outputDictionary, "0008|2143", ss.str());

  InputImageType::SizeType ninputSize = image->GetLargestPossibleRegion().GetSize();

  std::ostringstream in1;
  in1 << ninputSize[0];
  //Set rows 0028,0010
  itk::EncapsulateMetaData<std::string>(*outputDictionary, "0028,0010", in1.str());
  std::ostringstream in2;
  in2 << ninputSize[1];
  //Set columns 0028,0011
  itk::EncapsulateMetaData<std::string>(*outputDictionary, "0028,0011", in2.str());

  //Set pixel spacing 0028,0030
  InputImageType::SpacingType ninputSpacing = image->GetSpacing();
  std::ostringstream ims;
  ims << ninputSpacing[0] << "\\" << ninputSpacing[1] << "\\" << ninputSpacing[2];
  itk::EncapsulateMetaData<std::string>(*outputDictionary, "0028|0030", ims.str());

  //Image position patient
  std::ostringstream impp;
  impp << 0.0 << "\\" << 0.0 << "\\" << 0.0;
  itk::EncapsulateMetaData<std::string>(*outputDictionary, "0020|0032", impp.str());

  //Image Orientation patient
  std::ostringstream iop;
  iop << 1.0 << "\\" << 0.0 << "\\" << 0.0 << "\\" << 0.0 << "\\" << 1.0 << "\\" << 0.0;
  itk::EncapsulateMetaData<std::string>(*outputDictionary, "0020|0037", iop.str());

  //Store the view names and corresponding image count in the order they are stored in the 3D sequence
  ss.str("");
  for (int i = 0; i < viewNames.size(); i++)
    {
      ss << viewNames[i] << "|" << resolution << "#";
    }

  itk::EncapsulateMetaData<std::string>(*outputDictionary, tagTransducerData, ss.str());

  ReaderType::DictionaryArrayType output;
  output.push_back(outputDictionary);

  ImageIOType::Pointer gdcmIO = ImageIOType::New();
  // Save as JPEG 2000 Lossless
    // Explicitely specify which compression type to use
  gdcmIO->SetCompressionType(itk::GDCMImageIO::JPEG2000);

  DICOM3DWriterType::Pointer seriesWriter = DICOM3DWriterType::New();
  // Request compression of the ImageIO
  seriesWriter->UseCompressionOn();

  boost::filesystem::path dir(workingDirectory);
  std::string fn = (dir / filename).string();
  seriesWriter->SetInput(image);
  seriesWriter->SetImageIO(gdcmIO);
  seriesWriter->SetFileName(fn);
  seriesWriter->SetMetaDataDictionaryArray(&output);
  seriesWriter->Update();

  delete outputDictionary;
  return sopInstanceUID;
}

void
CMISSDICOMEncapsulator::saveViewFrames()
{
  MarkerPlotter plotter(5, 5);
  ImageWriterType::Pointer writer = ImageWriterType::New();
  JpegIOType::Pointer jpegType = JpegIOType::New();
  jpegType->SetQuality(50); //Just for internet based rendering at reduced size
  writer->SetImageIO(jpegType);
  writer->UseCompressionOn();
  if (views.size() > 0)
    {
      if (imageMap == NULL)
        {
          JoinSeriesImageFilterType::Pointer joiner = JoinSeriesImageFilterType::New();
          double origin = 0.0;
          double spacing = 1.0;
          joiner->SetOrigin(origin);
          joiner->SetSpacing(spacing);
          for (int i = 0; i < views.size(); i++)
            {
              DICOMInputManager* curview = views[i];
              std::string curViewName = viewNames[i];
              InputImageType::Pointer image = curview->getTargetDicom();
              float* fids = curview->getFrames();
              //Add the first frame - this logic should match the logic in MotionEstimatorThread::speckleTrack()
              writer->SetInput(extract2DImageSlice(image, 2, round(fids[0])));
              std::ostringstream filename;
              filename.str("");
              filename << workingDirectory << "/" << curViewName << "0.jpg";
              writer->SetFileName(filename.str().c_str());
              writer->Update();
              unsigned int numFrames = curview->getNumberOfFrames();
              double step = ((double) numFrames - 2) / (resolution - 2);
              int frameCtr = 1;
              for (double val = step; frameCtr < (resolution - 1); val += step)
                {
                  unsigned int ix = round(val);
                  writer->SetInput(extract2DImageSlice(image, 2, round(fids[ix])));
                  filename.str("");
                  filename << workingDirectory << "/" << curViewName << frameCtr << ".jpg";
                  writer->SetFileName(filename.str().c_str());
                  writer->Update();
                  frameCtr++;
                }
              //Add the last frame
              writer->SetInput(extract2DImageSlice(image, 2, round(fids[numFrames - 1])));
              filename.str("");
              filename << workingDirectory << "/" << curViewName << frameCtr << ".jpg";
              writer->SetFileName(filename.str().c_str());
              writer->Update();

              delete[] fids;
            }
        }
      else
        {
          JoinSeriesImageFilterType::Pointer joiner = JoinSeriesImageFilterType::New();
          double origin = 0.0;
          double spacing = 1.0;
          joiner->SetOrigin(origin);
          joiner->SetSpacing(spacing);
          for (int i = 0; i < views.size(); i++)
            {
              DICOMInputManager* curview = views[i];
              InputImageType::Pointer image = curview->getTargetDicom();
              std::string curViewName = viewNames[i];
              float* fids = curview->getFrames();
              //Add the first frame - this logic should match the logic in MotionEstimatorThread::speckleTrack()
              SliceImageType::Pointer slice = extract2DImageSlice(image, 2, round(fids[0]));
              std::vector<double*>* pts = imageMap->at(i)->at(0);

              SliceImageType::Pointer output = plotter.plot(slice, *(pts));
              writer->SetInput(output);
              std::ostringstream filename;
              filename.str("");
              filename << workingDirectory << "/" << curViewName << "0.jpg";
              writer->SetFileName(filename.str().c_str());
              writer->Update();

              unsigned int numFrames = curview->getNumberOfFrames();

              double step = ((double) numFrames - 2) / (resolution - 2);
              int frameCtr = 1;
              for (double val = step; frameCtr < (resolution - 1); val += step)
                {
                  unsigned int ix = round(val);
                  slice = extract2DImageSlice(image, 2, round(fids[ix]));
                  pts = imageMap->at(i)->at(frameCtr);
                  output = plotter.plot(slice, *(pts));
                  writer->SetInput(output);
                  filename.str("");
                  filename << workingDirectory << "/" << curViewName << frameCtr << ".jpg";
                  writer->SetFileName(filename.str().c_str());
                  writer->Update();
                  frameCtr++;
                }

              //Add the last frame
              slice = extract2DImageSlice(image, 2, round(fids[numFrames - 1]));
              pts = imageMap->at(i)->at(0);
              output = plotter.plot(slice, *(pts));
              writer->SetInput(output);
              filename.str("");
              filename << workingDirectory << "/" << curViewName << frameCtr << ".jpg";
              writer->SetFileName(filename.str().c_str());
              writer->Update();
              delete[] fids;
            }
        }
    }

}

CMISSDICOMEncapsulator::~CMISSDICOMEncapsulator()
{

}

itk::Image<PixelType, 2>::Pointer
CMISSDICOMEncapsulator::extract2DImageSlice(InputImageType::Pointer dicomImage, int plane, int slice)
{
  InputImageType::RegionType inputRegion = dicomImage->GetLargestPossibleRegion();

  InputImageType::SizeType mySize = inputRegion.GetSize();
  mySize[plane] = 0;

  InputImageType::IndexType myStart = inputRegion.GetIndex();
  const unsigned int sliceNumber = slice;
  myStart[plane] = sliceNumber;

  InputImageType::RegionType desiredRegion;
  desiredRegion.SetSize(mySize);
  desiredRegion.SetIndex(myStart);

  ExtractImageFilterType::Pointer filter = ExtractImageFilterType::New();

  filter->SetExtractionRegion(desiredRegion);
  filter->SetDirectionCollapseToIdentity();
  filter->SetInput(dicomImage);
  filter->Update();

  SliceImageType::Pointer img = filter->GetOutput();

  SliceImageType::SizeType imgSize = img->GetBufferedRegion().GetSize();
  SliceImageType::SpacingType imgSpacing = img->GetSpacing();

  SliceImageType::SizeType noutputSize;
  noutputSize[0] = m_TargetWidth;
  noutputSize[1] = m_TargetHeight;

  SliceImageType::SpacingType outputSpacing;

  for (unsigned int d = 0; d < 2; d++)
    {
      outputSpacing[d] = imgSpacing[d] * ((double) imgSize[d] / (double) noutputSize[d]);
    }

  ResampleImageFilterType::Pointer resample = ResampleImageFilterType::New();
  resample->SetInput(img);
  resample->SetSize(noutputSize);
  resample->SetOutputSpacing(outputSpacing);
  resample->SetTransform(TransformType::New());
  resample->SetInterpolator(InterpolatorType::New());
  resample->UpdateLargestPossibleRegion();

  SliceImageType::Pointer output = resample->GetOutput();

  outputSpacing[0] = 1.0;
  outputSpacing[1] = 1.0;

  output->SetSpacing(outputSpacing);

  output->DisconnectPipeline();
  return output;
}

#endif
