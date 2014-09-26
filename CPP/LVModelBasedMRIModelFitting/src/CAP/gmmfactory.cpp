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
#include "gmmfactory.h"

void gmm::mult(const gmm::GMMGSmoothAMatrix &m, const std::vector<double> &x, std::vector<double> &y) {
	std::vector<double> temp1(m.B->nrows()), temp2(m.P->nrows());

	gmm::mult(*m.B, x, temp1); //temp1 = G * x;


	gmm::mult(*m.P, temp1, temp2); //temp2 = P * temp1;
	gmm::mult(gmm::transposed(*m.P), temp2, temp1); //temp1 = P.trans_mult(temp2);
	gmm::mult(gmm::transposed(*m.B), temp1, y); //temp2 = G.trans_mult(temp1);

	gmm::mult(*m.S, x, y, y);

}

void gmm::mult(const gmm::GMMGSmoothAMatrix &m, const gmm::VectorRef &ref, const std::vector<double> &v, std::vector<double>& y) {

	std::vector<double> x(m.B->ncols()), temp1(m.B->nrows()), temp2(m.P->nrows());

	x.assign(ref.begin_, ref.end_);

	mult(*m.B, x, temp1); //temp1 = G * x;
	gmm::mult(*m.P, temp1, temp2); //temp2 = P * temp1;
	gmm::mult(gmm::transposed(*m.P), temp2, temp1); //temp1 = P.trans_mult(temp2);
	gmm::mult(gmm::transposed(*m.B), temp1, y); //temp2 = G.trans_mult(temp1);

	gmm::mult(*m.S, x, y, y);
	gmm::add(v, y, y);
}
