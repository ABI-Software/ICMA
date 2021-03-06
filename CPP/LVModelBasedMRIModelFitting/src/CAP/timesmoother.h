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


#ifndef CAPTIMESMOOTHER_H_
#define CAPTIMESMOOTHER_H_

#include <vector>
#include <gmm/gmm.h>

namespace cap {

//struct TimeSmootherImpl;

struct TimeSmootherImpl {
	TimeSmootherImpl() :
			S(11, 11), Priors(11, 134) {
	}

	gmm::csc_matrix<double> S;
	gmm::dense_matrix<double> Priors;
};

class TimeSmoother {

public:
	TimeSmoother();

	~TimeSmoother();

	std::vector<double> FitModel(int parameterIndex,
			const std::vector<double>& dataPoints,
			const std::vector<int>& framesWithDataPoints) const;

	double ComputeLambda(double xi, const std::vector<double>& params) const;

	std::vector<double> GetPrior(int paramNumber) const;

private:
	double MapToXi(double time) const;
	TimeSmootherImpl* pImpl;

	static const int NUMBER_OF_PARAMETERS = 11;
	static const int CAP_WEIGHT_GP = 10;
};

} // end namespace cap
#endif /* CAPTIMESMOOTHER_H_ */
