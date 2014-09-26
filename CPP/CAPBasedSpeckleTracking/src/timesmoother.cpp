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
 *  Contributor(s): J Chung
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


#include <iostream>
#include <sstream>
#include "basis.h"


#include "debug.h"
#include "timesmoother.h"
#include "filesystem.h"


#include "timevaryingprior.dat.h"
#include "globalsmoothtvmatrix.dat.h"

namespace cap {

TimeSmoother::TimeSmoother() :
		pImpl(new TimeSmootherImpl) {
	//read in S
	std::string tmpFileName = CreateTemporaryEmptyFile();
	WriteCharBufferToFile(tmpFileName, globalsmoothtvmatrix_dat,
			globalsmoothtvmatrix_dat_len);
	Harwell_Boeing_load(tmpFileName.c_str(), pImpl->S);
	RemoveFile(tmpFileName);

	std::string prior = WriteCharBufferToString(timevaryingprior_dat,
			timevaryingprior_dat_len);
	std::stringstream ss(std::stringstream::in | std::stringstream::out);
	ss << prior;

	for (int row = 0; row < 11; row++) {
		for (int col = 0; col < 134; col++) {
			ss >> pImpl->Priors(row, col);
		}
		for (int col = 0; col < 80; col++) {
			double temp;
			ss >> temp; // mu and theta?
		}
	}

}

TimeSmoother::~TimeSmoother() {
	delete pImpl;
}

double TimeSmoother::MapToXi(double time) const {
	return time;
}

std::vector<double> TimeSmoother::FitModel(int parameterIndex,
		const std::vector<double>& dataPoints,
		const std::vector<int>& framesWithDataPoints) const {
	// 1. Project data points (from each frame) to model to get corresponding xi
	// Here the data points are the nodal parameters at each frame and linearly map to xi
	// 2. Construct P

	FourierBasis basis;
	int numRows = dataPoints.size();
	gmm::dense_matrix<double> P(numRows, NUMBER_OF_PARAMETERS);


	for (int i = 0; i < numRows; i++) {
		double xiDouble[1];
		xiDouble[0] = MapToXi(static_cast<double>(i) / numRows); //REVISE design
		double psi[NUMBER_OF_PARAMETERS];
		basis.Evaluate(psi, xiDouble);
		for (int columnIndex = 0; columnIndex < NUMBER_OF_PARAMETERS;
				columnIndex++) {
			P(i, columnIndex) = psi[columnIndex];
			if (framesWithDataPoints[i]) {
				P(i, columnIndex) *= CAP_WEIGHT_GP; //TEST
			}
		}
	}


	// 3. Construct A
	// Note that G is the identity matrix. StS is read in from file.
	gmm::dense_matrix<double> A(NUMBER_OF_PARAMETERS, NUMBER_OF_PARAMETERS),
			temp(NUMBER_OF_PARAMETERS, NUMBER_OF_PARAMETERS);
	gmm::mult(gmm::transposed(P), P, temp);
	gmm::add(pImpl->S, temp, A);

	// 4. Construct rhs
	std::vector<double> prior(11), p(numRows), rhs(11);
	for (int i = 0; i < 11; i++) {
		prior[i] = pImpl->Priors(i, parameterIndex);
	}

	gmm::mult(P, prior, p);

	std::vector<double> dataLambda = dataPoints;
	for (unsigned int i = 0; i < dataLambda.size(); i++) {
		if (framesWithDataPoints[i]) {
			dataLambda[i] *= CAP_WEIGHT_GP;
		}
	}
	std::transform(dataLambda.begin(), dataLambda.end(), p.begin(), p.begin(),
			std::minus<double>());

	gmm::mult(transposed(P), p, rhs);

	// 5. Solve normal equation (direct solver) 
	std::vector<double> x(gmm::mat_nrows(A));
	gmm::lu_solve(A, x, rhs);

	std::transform(x.begin(), x.end(), prior.begin(), x.begin(),
			std::plus<double>());
	return x;
}

std::vector<double> TimeSmoother::GetPrior(int paramNumber) const {
	std::vector<double> prior(11);
	for (int i = 0; i < 11; i++) {
		prior[i] = pImpl->Priors(i, paramNumber);
	}

	return prior;
}

double TimeSmoother::ComputeLambda(double xi,
		const std::vector<double>& params) const {
	double psi[NUMBER_OF_PARAMETERS];
	FourierBasis basis;
	double xiDouble[1];
	xiDouble[0] = xi;
	basis.Evaluate(psi, xiDouble);

	double lambda(0);
	for (int i = 0; i < NUMBER_OF_PARAMETERS; i++) {
		lambda += params[i] * psi[i];
	}

	return lambda;
}

} // end namespace cap
