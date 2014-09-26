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

#ifndef SOLVERLIBRARYFACTORY_H_
#define SOLVERLIBRARYFACTORY_H_

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

namespace cap {

class Vector {
public:
//	virtual Vector& operator=(double value) = 0;
	virtual ~Vector() {
	}
	;

	virtual std::string ToString() const = 0;

	virtual Vector& operator-=(const Vector&) = 0;

	virtual Vector& operator+=(const Vector&) = 0;

	virtual double& operator[](int index) = 0;

	virtual const double& operator[](int index) const = 0;

	//virtual std::vector<double> ToSTDVector();
};

inline std::ostream& operator<<(std::ostream& out, const Vector& v) {
	out << v.ToString();

	return out;
}

class SparseMatrix {
public:
	virtual ~SparseMatrix() {
	}
	;

	virtual Vector* mult(const Vector& v) const = 0; // Should be namespace level function?

	virtual Vector* trans_mult(const Vector& v) const = 0;
};

class GSmoothAMatrix: public SparseMatrix {
	// Note that this class (and its subclasses) works as a proxy to the actual Matrices G,S and P
	// and hence does not own them and not responsible for deleting them
public:
	virtual void UpdateData(const SparseMatrix& P) = 0;
	virtual ~GSmoothAMatrix() {
	}
	;
};

class Preconditioner {
};

struct Entry {
	double value;
	int rowIndex;
	int colIndex;
};

class SolverLibraryFactory {
public:
	SolverLibraryFactory(const std::string& name) :
			name_(name) {
	}

	virtual ~SolverLibraryFactory() {
	}
	;

	virtual Vector* CreateVector(int dimension = 0,
			double value = 0.0) const = 0;
	virtual Vector* CreateVectorFromFile(const std::string& filename) const = 0;

	virtual SparseMatrix* CreateSparseMatrixFromFile(
			const std::string& filename) const = 0;
	virtual SparseMatrix* CreateSparseMatrix(int m, int n,
			const std::vector<Entry>& entries) const = 0;

	virtual GSmoothAMatrix* CreateGSmoothAMatrix(const SparseMatrix& S,
			const SparseMatrix& G) const = 0;

	virtual Preconditioner* CreateDiagonalPreconditioner(
			const SparseMatrix& m) const = 0;

	virtual void CG(const SparseMatrix& A, Vector& x, const Vector& rhs,
			const Preconditioner& pre, int maximumIteration,
			double tolerance) const = 0;

	virtual const std::string& GetName() const {
		return name_;
	}
protected:
	std::string name_;
};

} // end namespace cap
#endif /* SOLVERLIBRARYFACTORY_H_ */
