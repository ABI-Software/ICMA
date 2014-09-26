/*
 * ImageFileReader.cpp
 *
 *  Created on: 15/05/2014
 *      Author: rjag008
 */

#include "ImageFileReader.h"

template<typename ImageType>
ImageFileReader<ImageType>::ImageFileReader(std::string filename, double theight, double twidth) {
	this->filename = filename;
	targetHeight = (unsigned int) round(theight);
	targetWidth = (unsigned int) round(twidth);
}

template<typename ImageType>
ImageFileReader<ImageType>::~ImageFileReader() {

}

template<typename ImageType>
void ImageFileReader<ImageType>::Update() {
	std::string ext = boost::to_upper_copy(filename);
	if (boost::algorithm::ends_with(ext, "JPG")) {
		typedef itk::ImageFileReader<ImageType> ReaderType;
		typename ReaderType::Pointer reader = ReaderType::New();
		reader->SetFileName(filename);
		reader->Update();
		image = reader->GetOutput();
		image->DisconnectPipeline();
	} else {
		typedef itk::ImageIOBase::IOComponentType ScalarPixelType;

		itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(filename.c_str(), itk::ImageIOFactory::ReadMode);
		imageIO->SetFileName(filename);
		imageIO->ReadImageInformation();
		const ScalarPixelType pixelType = imageIO->GetComponentType();

		if (imageIO->GetComponentTypeAsString(pixelType) == "double" || boost::algorithm::ends_with(ext, "DCM") || boost::algorithm::ends_with(ext, "DICOM")) {
			typedef itk::ImageFileReader<InternalImageType> ReaderType;
			typedef itk::MetaDataDictionary DictionaryType;
			ImageIOType::Pointer gdcmIO = ImageIOType::New();
			ReaderType::Pointer reader = ReaderType::New();
			reader->SetImageIO(gdcmIO);
			reader->SetFileName(filename);
			reader->Update();
			typedef itk::MetaDataDictionary DictionaryType;
			const DictionaryType & dictionary = gdcmIO->GetMetaDataDictionary();
			typedef itk::MetaDataObject<std::string> MetaDataStringType;
			DictionaryType::ConstIterator end = dictionary.End();
			int windowCenter = 0;
			int windowWidth = 0;
			{
				std::string entryId = "0028|1051";
				DictionaryType::ConstIterator tagItr = dictionary.Find(entryId);
				if (tagItr != end) {
					MetaDataStringType::ConstPointer entryvalue = dynamic_cast<const MetaDataStringType *>(tagItr->second.GetPointer());
					if (entryvalue) {
						std::string tagvalue = boost::algorithm::trim_copy(entryvalue->GetMetaDataObjectValue());
						windowWidth = atoi(tagvalue.c_str());
					}
				}
			}
			{
				std::string entryId = "0028|1050";
				DictionaryType::ConstIterator tagItr = dictionary.Find(entryId);
				if (tagItr != end) {
					MetaDataStringType::ConstPointer entryvalue = dynamic_cast<const MetaDataStringType *>(tagItr->second.GetPointer());
					if (entryvalue) {
						std::string tagvalue = boost::algorithm::trim_copy(entryvalue->GetMetaDataObjectValue());
						windowCenter = atoi(tagvalue.c_str());
					}
				}
			}
			typedef typename itk::IntensityWindowingImageFilter<InternalImageType, ImageType> IntensityWindowingImageFilterType;
			typename IntensityWindowingImageFilterType::Pointer filter = IntensityWindowingImageFilterType::New();
			filter->SetInput(reader->GetOutput());
			int min = 2 * windowCenter - windowWidth;
			if (min < 1)
				min = 1;
			filter->SetWindowMinimum(min);
			filter->SetWindowMaximum(windowWidth);
			filter->SetOutputMinimum(0);
			filter->SetOutputMaximum(255);
			filter->Update();
			image = filter->GetOutput();
			image->DisconnectPipeline();
		} else {
			typedef itk::ImageFileReader<ImageType> ReaderType;
			typename ReaderType::Pointer reader = ReaderType::New();
			reader->SetFileName(filename);
			reader->Update();
			image = reader->GetOutput();
			image->DisconnectPipeline();
		}
	}
	//Resample Image to meet target height and width
	typedef typename itk::ResampleImageFilter<ImageType, ImageType> ResampleImageFilterType;
	typename ResampleImageFilterType::Pointer resample = ResampleImageFilterType::New();
	typedef itk::AffineTransform<double, 2> TransformType;
	TransformType::Pointer transform = TransformType::New();
	resample->SetTransform(transform);
	typedef typename itk::NearestNeighborInterpolateImageFunction<ImageType, double> InterpolatorType;
	typename InterpolatorType::Pointer interpolator = InterpolatorType::New();
	resample->SetInterpolator(interpolator);
	resample->SetDefaultPixelValue(0);
	typename ImageType::SpacingType spacing, outputSpacing;
	spacing = image->GetSpacing();
	typename ImageType::SizeType size, outputSize;
	outputSize[0] = targetWidth;
	outputSize[1] = targetHeight;
	size = image->GetLargestPossibleRegion().GetSize();
	for (unsigned int d = 0; d < 2; d++) {
		outputSpacing[d] = spacing[d] * ((double) size[d] / (double) outputSize[d]);
	}
	typename ImageType::PointType origin;
	origin[0] = 0;
	origin[1] = 0;
	resample->SetOutputSpacing(outputSpacing);
	resample->SetOutputOrigin(origin);
	typename ImageType::DirectionType direction;
	direction.SetIdentity();
	resample->SetOutputDirection(direction);
	size[0] = targetWidth;  // number of pixels along X
	size[1] = targetWidth;  // number of pixels along Y
	resample->SetSize(size);
	resample->SetInput(image);
	resample->Update();
	image = resample->GetOutput();
	image->DisconnectPipeline();
	//Set spacing to 1.0 as expected by ActiveContour filter
	spacing[0] = 1.0;
	spacing[1] = 1.0;
	image->SetSpacing(spacing);
}

template<typename ImageType>
typename ImageType::Pointer ImageFileReader<ImageType>::GetOutput() {
	return image;
}
