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

template<typename ImageType>
RegionOfInterest<ImageType>::RegionOfInterest(std::vector<typename ImageType::Pointer> img) :
		images(img), maxrad(40) {
	unsigned int N = images.size();
	typedef typename itk::CastImageFilter<ImageType, VImageType> CastFilterType;
	typedef itk::AddImageFilter<VImageType> AddImageFilterType;
	typedef itk::SubtractImageFilter<VImageType> SubtractImageFilterType;
	typedef itk::DivideImageFilter<VImageType, VImageType, VImageType> DivideImageFilterType;
	//Cast the images to float type
	std::vector<VImageType::Pointer> vimages(N);
	typename CastFilterType::Pointer caster = CastFilterType::New();
	for (int i = 0; i < N; i++) {
		caster->SetInput(images[i]);
		caster->Update();
		vimages[i] = caster->GetOutput();
		vimages[i]->DisconnectPipeline();
	}
	if (N > 1) {
		VImageType::Pointer summedImage;
		VImageType::Pointer meanImage;
		//Find mean image
		AddImageFilterType::Pointer adder = AddImageFilterType::New();
		adder->SetInput1(vimages[0]);
		for (int i = 1; i < N - 1; i++) {
			adder->SetInput2(vimages[i]);
			adder->Update();
			VImageType::Pointer temp = adder->GetOutput();
			temp->DisconnectPipeline();
			adder->SetInput1(temp);
		}
		adder->SetInput2(vimages[N - 1]);
		adder->Update();
		summedImage = adder->GetOutput();
		summedImage->DisconnectPipeline();

		DivideImageFilterType::Pointer divider = DivideImageFilterType::New();
		divider->SetInput(summedImage);
		divider->SetConstant((float) N);
		divider->Update();
		meanImage = divider->GetOutput();
		meanImage->DisconnectPipeline();

		//Compute variation
		std::vector<VImageType::Pointer> dimages(N);
		SubtractImageFilterType::Pointer subtractor = SubtractImageFilterType::New();

		for (int i = 0; i < N; i++) {
			subtractor->SetInput1(vimages[i]);
			subtractor->SetInput2(meanImage);
			subtractor->Update();
			dimages[i] = subtractor->GetOutput();
			dimages[i]->DisconnectPipeline();
		}
		//Sum dimages
		adder->SetInput1(dimages[0]);
		for (int i = 1; i < N - 1; i++) {
			adder->SetInput2(dimages[i]);
			adder->Update();
			VImageType::Pointer temp = adder->GetOutput();
			temp->DisconnectPipeline();
			adder->SetInput1(temp);
		}
		adder->SetInput2(dimages[N - 1]);

		divider->SetInput(adder->GetOutput());
		divider->SetConstant((float) N);
		divider->Update();

		variance = divider->GetOutput();
		variance->DisconnectPipeline();
		typedef itk::StatisticsImageFilter<VImageType> StatisticsImageFilterType;
		typename StatisticsImageFilterType::Pointer statisticsImageFilter = StatisticsImageFilterType::New();
		statisticsImageFilter->SetInput(variance);
		statisticsImageFilter->Update();

		itk::ImageRegionIterator<VImageType> imageIterator(variance, variance->GetLargestPossibleRegion());

		float mean = statisticsImageFilter->GetMean();
		while (!imageIterator.IsAtEnd()) {
			float val = imageIterator.Get();
			if (fabs(val - mean) < 1.0e-6) {
				imageIterator.Set(0.0);
			} else {
				imageIterator.Set(1.0);
			}
			++imageIterator;
		}
		//Mask non central regions
		VImageType::SizeType vsize = variance->GetLargestPossibleRegion().GetSize();
		typedef typename itk::MaskImageFilter<VImageType, VImageType> MDaskImageFilterType;
		typedef itk::PolygonSpatialObject<2> PolygonType;
		PolygonType::Pointer polygon = PolygonType::New();
		PolygonType::PointType point;
		Point3D centroid(vsize[0] / 2, vsize[1] / 2, 0);

		unsigned int xt = round(maxrad);//round(vsize[0] / 4);
		unsigned int yt = round(maxrad);//round(vsize[1] / 4);

		polygon->ComputeObjectToWorldTransform();
		point[0] = centroid.x - xt;
		point[1] = centroid.y - yt;
		polygon->AddPoint(point);
		point[0] = centroid.x + xt;
		point[1] = centroid.y - yt;
		polygon->AddPoint(point);
		point[0] = centroid.x + xt;
		point[1] = centroid.x + yt;
		polygon->AddPoint(point);
		point[0] = centroid.x - xt;
		point[1] = centroid.x + yt;
		polygon->AddPoint(point);

		typename MDaskImageFilterType::Pointer mdmasker = MDaskImageFilterType::New();
		// create the filter and the output mask image
		typedef typename itk::SpatialObjectToImageFilter<PolygonType, VImageType> XSpatialObjectToImageFilterType;
		typename XSpatialObjectToImageFilterType::Pointer imageFilter = XSpatialObjectToImageFilterType::New();
		imageFilter->SetInput(polygon);
		imageFilter->SetInsideValue(1);
		imageFilter->SetOutsideValue(0);
		polygon->SetThickness(1.0);
		imageFilter->SetSize(vsize);
		imageFilter->SetOrigin(variance->GetOrigin());
		imageFilter->SetSpacing(variance->GetSpacing());
		imageFilter->Update();
		mdmasker->SetInput(variance);
		mdmasker->SetMaskImage(imageFilter->GetOutput());
		mdmasker->Update();
		variance = mdmasker->GetOutput();
		variance->DisconnectPipeline();
	} else {
		variance = vimages[0];
	}
}

template<typename ImageType>
itk::Image<float, 2>::Pointer RegionOfInterest<ImageType>::getVarianceImage() {
	return variance;
}

template<typename ImageType>
std::vector<double> RegionOfInterest<ImageType>::getROIBoundingBox(double cutoffRadius) {
	const unsigned int numcirles = 2;
	const unsigned int minrad = std::max(maxrad/10,cutoffRadius);
	typedef itk::MedianImageFilter<VImageType, VImageType> MedianImageFilterType;
	MedianImageFilterType::Pointer medianFilter = MedianImageFilterType::New();
	MedianImageFilterType::InputSizeType radius;
	radius.Fill(7);

	medianFilter->SetRadius(radius);
	medianFilter->SetInput(variance);

	typedef itk::HoughTransform2DCirclesImageFilter<VImageType::PixelType, VImageType::PixelType> HoughTransformFilterType;
	typedef HoughTransformFilterType::CirclesListType CirclesListType;
	HoughTransformFilterType::Pointer houghFilter = HoughTransformFilterType::New();
	houghFilter->SetInput(medianFilter->GetOutput());
	houghFilter->SetNumberOfCircles(numcirles);
	houghFilter->SetMinimumRadius(minrad);
	houghFilter->SetMaximumRadius(maxrad);
	houghFilter->Update();
	HoughTransformFilterType::CirclesListType circles;
	circles = houghFilter->GetCircles(numcirles);

	VImageType::SizeType size = variance->GetLargestPossibleRegion().GetSize();
	VImageType::IndexType center;
	center[0] = size[0] / 2;
	center[1] = size[1] / 2;

	float ccrad = 0;
	VImageType::IndexType coff;
	double distance = size[1];
	CirclesListType::const_iterator itCircles = circles.begin();
	while (itCircles != circles.end()) {
		VImageType::IndexType off;
		off[0] = (*itCircles)->GetObjectToParentTransform()->GetOffset()[0];
		off[1] = (*itCircles)->GetObjectToParentTransform()->GetOffset()[1];
		double d = sqrt((off[0] - center[0]) * (off[0] - center[0]) + (off[1] - center[1]) * (off[1] - center[1]));
		if (d < distance) {
			coff = off;
			distance = d;
			ccrad = (*itCircles)->GetRadius()[0];
		}
		itCircles++;
	}
//#define debug
#ifdef debug

	typedef typename itk::ImageDuplicator<ImageType> ImageDuplicatorType;
	typename ImageDuplicatorType::Pointer duplicator = ImageDuplicatorType::New();
	duplicator->SetInputImage(images[0]);
	duplicator->Update();
	//typename ImageType::Pointer localOutputImage = ImageType::New();
	typename ImageType::Pointer localOutputImage = duplicator->GetOutput();
	localOutputImage->DisconnectPipeline();
	typename ImageType::RegionType region;
	typename ImageType::IndexType localIndex;
	region.SetSize(variance->GetLargestPossibleRegion().GetSize());
	region.SetIndex(variance->GetLargestPossibleRegion().GetIndex());
	localOutputImage->SetRegions(region);
	localOutputImage->SetOrigin(variance->GetOrigin());
	localOutputImage->SetSpacing(variance->GetSpacing());
	localOutputImage->Allocate();

	itCircles = circles.begin();
	while (itCircles != circles.end()) {
		std::cout << "Center: ";
		std::cout << (*itCircles)->GetObjectToParentTransform()->GetOffset() << std::endl;
		std::cout << "Radius: " << (*itCircles)->GetRadius()[0] << std::endl;
		// Software Guide : EndCodeSnippet
		//  Software Guide : BeginLatex
		//
		//  We draw white pixels in the output image to represent each circle.
		//
		//  Software Guide : EndLatex
		// Software Guide : BeginCodeSnippet
		for (double angle = 0; angle <= 2 * vnl_math::pi; angle += vnl_math::pi / 60.0) {
			localIndex[0] = (long int) ((*itCircles)->GetObjectToParentTransform()->GetOffset()[0] + (*itCircles)->GetRadius()[0] * std::cos(angle));
			localIndex[1] = (long int) ((*itCircles)->GetObjectToParentTransform()->GetOffset()[1] + (*itCircles)->GetRadius()[0] * std::sin(angle));
			typename ImageType::RegionType outputRegion = localOutputImage->GetLargestPossibleRegion();
			if (outputRegion.IsInside(localIndex)) {
				localOutputImage->SetPixel(localIndex, 255);
			}
		}
		itCircles++;
	}
	// Software Guide : EndCodeSnippet
	//  Software Guide : BeginLatex
	//
	//  We setup a writer to write out the binary image created.
	//
	//  Software Guide : EndLatex
	// Software Guide : BeginCodeSnippet
	static int roifile = 0;
	std::ostringstream ss;
	ss << "Roifile" << roifile << ".jpg";
	typedef typename itk::ImageFileWriter<ImageType> WriterType;
	typename WriterType::Pointer writer = WriterType::New();
	writer->SetFileName(ss.str());
	writer->SetInput(localOutputImage);
	writer->Update();

	typedef typename itk::CastImageFilter<VImageType, ImageType> ICastFilterType;
	typename ICastFilterType::Pointer caster = ICastFilterType::New();
	caster->SetInput(medianFilter->GetOutput());
	ss.str("");
	ss << "MFile" << roifile << ".jpg";
	writer->SetFileName(ss.str());
	writer->SetInput(caster->GetOutput());
	writer->Update();
	roifile++;
#endif

	std::vector<double> result;
	result.push_back(coff[0]);
	result.push_back(coff[1]);
	result.push_back(ccrad);
	return result;
}

