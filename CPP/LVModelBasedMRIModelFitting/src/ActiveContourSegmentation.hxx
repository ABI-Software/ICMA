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

#include "ActiveContourSegmentation.h"

template<typename ImageType>
ActiveContourSegmentation<ImageType>::ActiveContourSegmentation() {
	// TODO Auto-generated constructor stub

}

template<typename ImageType>
ActiveContourSegmentation<ImageType>::~ActiveContourSegmentation() {
	// TODO Auto-generated destructor stub
}

template<typename ImageType>
typename ImageType::Pointer ActiveContourSegmentation<ImageType>::getActiveContour(typename ImageType::Pointer image, Point3D seed, double propagationScaling) {

	SmoothingFilterType::Pointer smoothing = SmoothingFilterType::New();
	GradientFilterType::Pointer gradientMagnitude = GradientFilterType::New();
	SigmoidFilterType::Pointer sigmoid = SigmoidFilterType::New();

	//  The minimum and maximum values of the SigmoidImageFilter output
	//  are defined with the methods \code{SetOutputMinimum()} and
	//  \code{SetOutputMaximum()}. In our case, we want these two values to be
	//  $0.0$ and $1.0$ respectively in order to get a nice speed image to feed
	//  the \code{FastMarchingImageFilter}. Additional details on the user of the
	//  \doxygen{SigmoidImageFilter} are presented in
	//  section~\ref{sec:IntensityNonLinearMapping}.
	sigmoid->SetOutputMinimum(0.0);
	sigmoid->SetOutputMaximum(1.0);
	FastMarchingFilterType::Pointer fastMarching = FastMarchingFilterType::New();
	GeodesicActiveContourFilterType::Pointer geodesicActiveContour = GeodesicActiveContourFilterType::New();
	//  Software Guide : BeginCodeSnippet
	geodesicActiveContour->SetPropagationScaling(propagationScaling);
	geodesicActiveContour->SetCurvatureScaling(1.0);
	geodesicActiveContour->SetAdvectionScaling(1.0);
	//  Software Guide : EndCodeSnippet
	//  Once activiated the level set evolution will stop if the convergence
	//  criteria or if the maximum number of iterations is reached.  The
	//  convergence criteria is defined in terms of the root mean squared (RMS)
	//  change in the level set function. The evolution is said to have
	//  converged if the RMS change is below a user specified threshold.  In a
	//  real application is desirable to couple the evolution of the zero set
	//  to a visualization module allowing the user to follow the evolution of
	//  the zero set. With this feedback, the user may decide when to stop the
	//  algorithm before the zero set leaks through the regions of low gradient
	//  in the contour of the anatomical structure to be segmented.
	geodesicActiveContour->SetMaximumRMSError(0.02);
	geodesicActiveContour->SetNumberOfIterations(800);

	typename CastImageFilterType::Pointer icaster = CastImageFilterType::New();
	icaster->SetInput(image);
	smoothing->SetInput(icaster->GetOutput());
	gradientMagnitude->SetInput(smoothing->GetOutput());
	sigmoid->SetInput(gradientMagnitude->GetOutput());
	geodesicActiveContour->SetInput(fastMarching->GetOutput());
	geodesicActiveContour->SetFeatureImage(sigmoid->GetOutput());
	typename ThresholdingFilterType::Pointer thresholder = ThresholdingFilterType::New();
	thresholder->SetLowerThreshold(-1000.0);
	thresholder->SetUpperThreshold(0.0);
	thresholder->SetOutsideValue(0);
	thresholder->SetInsideValue(255);
	thresholder->SetInput(geodesicActiveContour->GetOutput());

	smoothing->SetTimeStep(0.125);
	smoothing->SetNumberOfIterations(5);
	smoothing->SetConductanceParameter(9.0);

	const double sigma = 1.0;
	gradientMagnitude->SetSigma(sigma);
	//  The SigmoidImageFilter requires two parameters that define the linear
	//  transformation to be applied to the sigmoid argument. This parameters
	//  have been discussed in Sections~\ref{sec:IntensityNonLinearMapping} and
	//  \ref{sec:FastMarchingImageFilter}.
	const double alpha = -0.5;
	const double beta = 3.0;
	sigmoid->SetAlpha(alpha);
	sigmoid->SetBeta(beta);

	typedef FastMarchingFilterType::NodeContainer NodeContainer;
	typedef FastMarchingFilterType::NodeType NodeType;
	NodeContainer::Pointer seeds = NodeContainer::New();
	InternalImageType::IndexType seedPosition;
	seedPosition[0] = seed.x;
	seedPosition[1] = seed.y;
	//  Nodes are created as stack variables and initialized with a value and an
	//  \doxygen{Index} position. Note that here we assign the value of minus the
	//  user-provided distance to the unique node of the seeds passed to the
	//  FastMarchingImageFilter. In this way, the value will increment
	//  as the front is propagated, until it reaches the zero value corresponding
	//  to the contour. After this, the front will continue propagating until it
	//  fills up the entire image. The rule of thumb for the user is to select this
	//  value as the distance from the seed points at which she want the initial
	//  contour to be.
	const double initialDistance = 5;
	NodeType node;
	const double seedValue = -initialDistance;
	node.SetValue(seedValue);
	node.SetIndex(seedPosition);
	//  The list of nodes is initialized and then every node is inserted using
	//  the \code{InsertElement()}.
	seeds->Initialize();
	seeds->InsertElement(0, node);
	//  The set of seed nodes is passed now to the
	//  FastMarchingImageFilter with the method
	//  \code{SetTrialPoints()}.
	//
	fastMarching->SetTrialPoints(seeds);
	//  Since the FastMarchingImageFilter is used here just as a
	//  Distance Map generator. It does not require a speed image as input.
	//  Instead the constant value $1.0$ is passed using the
	//  \code{SetSpeedConstant()} method.
	//
	fastMarching->SetSpeedConstant(1.0);

	//  The FastMarchingImageFilter requires the user to specify the
	//  size of the image to be produced as output. This is done using the
	//  \code{SetOutputSize()}. Note that the size is obtained here from the
	//  output image of the smoothing filter. The size of this image is valid
	//  only after the \code{Update()} methods of this filter has been called
	//  directly or indirectly.
	//
	fastMarching->SetOutputSize(image->GetBufferedRegion().GetSize());

	thresholder->Update();

	return thresholder->GetOutput();
}
