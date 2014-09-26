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

#ifndef VNLFACTORY_H_
#define VNLFACTORY_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "vnl/vnl_vector.h"
#include "vnl/vnl_sparse_matrix.h"
#include "vnl/algo/vnl_conjugate_gradient.h"

#ifdef _MSC_VER
#define __func__ __FUNCTION__
#endif

#include "solverlibraryfactory.h"

namespace cap {

class VNLVector: public Vector {
public:
	VNLVector(int dim, double value = 0) :
			impl_(new vnl_vector<double>(dim, value)) {
	}

	explicit VNLVector(vnl_vector<double>& vec) :
			impl_(&vec) {
	}

	~VNLVector() {
		delete impl_;
	}

	const vnl_vector<double>& GetImpl() const {
		return *impl_;
	}

	vnl_vector<double>& GetImpl() {
		return *impl_;
	}

	std::string ToString() const {
		std::stringstream ss;
		ss << *impl_;

		return ss.str();
	}

	Vector& operator+=(const Vector& other) {
		const VNLVector& otherConc = static_cast<const VNLVector&>(other);
		assert(impl_->size() == otherConc.impl_->size());

		std::transform(impl_->begin(), impl_->end(), otherConc.impl_->begin(),
				impl_->begin(), std::plus<double>());

		return *this;
	}

	Vector& operator-=(const Vector& other) {
		const VNLVector& otherConc = static_cast<const VNLVector&>(other);
		assert(impl_->size() == otherConc.impl_->size());

		std::transform(impl_->begin(), impl_->end(), otherConc.impl_->begin(),
				impl_->begin(), std::minus<double>());

		return *this;
	}

	double& operator[](int index) {
		return (*impl_)[index];
	}

	const double& operator[](int index) const {
		return (*impl_)[index];
	}

private:
	vnl_vector<double> *impl_;
};

class VNLMatrix: public SparseMatrix {
public:
	explicit VNLMatrix(vnl_sparse_matrix<double>& m) :
			impl_(&m) {
	}

	VNLMatrix(int m, int n) :
			impl_(new vnl_sparse_matrix<double>(m, n)) {
	}

	~VNLMatrix() {
		delete impl_;
	}

	vnl_sparse_matrix<double>& GetImpl() {
		return *impl_;
	}

	const vnl_sparse_matrix<double>& GetImpl() const {
		return *impl_;
	}

	Vector* mult(const Vector& v) const {
		VNLVector* ret = new VNLVector(impl_->rows());
		const vnl_vector<double>& vImpl =
				static_cast<const VNLVector&>(v).GetImpl();
		impl_->mult(vImpl, ret->GetImpl());

		return ret;
	}

	Vector* trans_mult(const Vector& v) const {
		VNLVector* ret = new VNLVector(impl_->columns());
		const vnl_vector<double>& vImpl =
				static_cast<const VNLVector&>(v).GetImpl();
		impl_->pre_mult(vImpl, ret->GetImpl());
		return ret;
	}

private:
	vnl_sparse_matrix<double>* impl_;
};

class VNLGSmoothAMatrix: public GSmoothAMatrix //, public vnl_sparse_matrix<double>
{
	// This class only need to conform to the interface GMM++ cg requires 

public:
	const vnl_sparse_matrix<double>* S, *B;
	const vnl_sparse_matrix<double>* P;

	VNLGSmoothAMatrix(const vnl_sparse_matrix<double>& Sref,
			const vnl_sparse_matrix<double>& Bref) {
		S = &Sref;
		B = &Bref;
	}

	~VNLGSmoothAMatrix() {
	}

	void UpdateData(const SparseMatrix& M) {
		P = &(static_cast<const VNLMatrix*>(&M)->GetImpl());
	}

	Vector* mult(const Vector& v) const {
		return 0;
	}

	Vector* trans_mult(const Vector& v) const {
		return 0;
	}

	unsigned int rows() const {
		return S->rows();
	}

	unsigned int columns() const {
		return B->columns();
	}

	friend class VNLGSmoothLinearSystem;
};

//class GMMPreconditioner : public Preconditioner
//{
//public:
//	GMMPreconditioner(const gmm::csc_matrix<double>& m)
//	: impl_(m)
//	{}
//	
//	const gmm::diagonal_precond<gmm::csc_matrix<double> >& GetImpl() const
//	{
//		return impl_;
//	}
//private:
//	gmm::diagonal_precond<gmm::csc_matrix<double> > impl_;
//};

template<typename T>
int readHBMatrix(const std::string& filename, T& A) {
	std::ifstream input(filename.c_str());

	std::string line;

	if (!input.is_open()) {
		std::cout << "Can't open Harwell Boeing file: " << filename
				<< std::endl;
		return -1;
	}

	for (int i = 0; i < 2; i++) {
		std::getline(input, line);
//    cout << line << endl;
	}

	input >> line; //RUA

	int numCol, numRow, numNonZero;
	input >> numRow;
	input >> numCol;
	input >> numNonZero;

	std::cout << "NumRow = " << numRow << ", NumCol = " << numCol
			<< ", NumNonZero = " << numNonZero << endl;
	A.set_size(numRow, numCol);

	int* columnIndex = new int[numCol + 1];
	int* rowIndex = new int[numNonZero];

	std::getline(input, line); // not needed
//  cout << line << endl;
	std::getline(input, line); // not needed
//  cout << line << endl;

	for (int i = 0; i <= numCol; ++i) {
		input >> columnIndex[i];
//    cout << columnIndex[i] << endl;
	}

	for (int i = 0; i < numNonZero; ++i) {
		input >> rowIndex[i];
//    cout << rowIndex[i] << endl;
	}

	for (int i = 0; i < numCol; ++i) {
		int endIndex = columnIndex[i + 1] - 1;
		int indexintoRowIndex = columnIndex[i] - 1;
		while (indexintoRowIndex < endIndex) {
			int row = rowIndex[indexintoRowIndex] - 1;
			double value;
			//input >> map[i][row] ;
			input >> value;
//      cout << i << " " << " " << row << " = " << value << " " << endl;;
			A(row, i) = value;
			indexintoRowIndex++;
		}
		//cout << endl;
	}

	delete[] columnIndex;
	delete[] rowIndex;

	return 0;
}

#include "vnl/vnl_sparse_matrix_linear_system.h"
#include "vnl/algo/vnl_lsqr.h"

#include "vnl/vnl_linear_system.h"
class VNLGSmoothLinearSystem: public vnl_linear_system {
public:
	//::Constructor from vnl_sparse_matrix<double> for system Ax = b
	// Keeps a reference to the original sparse matrix A and vector b so DO NOT DELETE THEM!!
	VNLGSmoothLinearSystem(VNLGSmoothAMatrix const& A,
			vnl_vector<double> const& b) :
			vnl_linear_system(A.columns(), A.rows()), A_(A), b_(b), jacobi_precond_(
					A.columns()) {
		vnl_vector<double> tmp(get_number_of_unknowns());
		A_.S->diag_AtA(tmp);
		jacobi_precond_.set_size(tmp.size());
		for (unsigned int i = 0; i < tmp.size(); ++i)
			jacobi_precond_[i] = 1.0 / double(tmp[i]);
	}

	//:  Implementations of the vnl_linear_system virtuals.
	void multiply(vnl_vector<double> const& x, vnl_vector<double> & b) const {
		vnl_vector<double> temp1(A_.B->rows()), temp2(A_.P->rows());

		A_.B->mult(x, temp1);	//temp1 = G * x;

		//	cout << *m.P << endl;
		A_.P->mult(temp1, temp2);	//temp2 = P * temp1;
		A_.P->pre_mult(temp2, temp1);
		A_.B->pre_mult(temp1, b);	//temp2 = G.trans_mult(temp1);

		//gmm::mult(*m.S,x,y,y);
		A_.S->mult(x, temp1);
		b = b + temp1;
	}

	//:  Implementations of the vnl_linear_system virtuals.
	void transpose_multiply(vnl_vector<double> const& x,
			vnl_vector<double> & b) const {
		vnl_vector<double> temp1(A_.B->rows()), temp2(A_.P->rows());

		A_.B->mult(x, temp1);	//temp1 = G * x;

		//	cout << *m.P << endl;
		A_.P->mult(temp1, temp2);	//temp2 = P * temp1;
		A_.P->pre_mult(temp2, temp1);
		A_.B->pre_mult(temp1, b);	//temp2 = G.trans_mult(temp1);

		//gmm::mult(*m.S,x,y,y);
		A_.S->mult(x, temp1);
		b = b + temp1;
	}

	//:  Implementations of the vnl_linear_system virtuals.
	void get_rhs(vnl_vector<double>& b) const {
		b = b_;
	}
	//:  Implementations of the vnl_linear_system virtuals.
	void apply_preconditioner(vnl_vector<double> const& x,
			vnl_vector<double> & px) const {
		assert(x.size() == px.size());
		px = dot_product(x, jacobi_precond_);
	}

protected:
	VNLGSmoothAMatrix const& A_;
	vnl_vector<double> const& b_;
	vnl_vector<double> jacobi_precond_;
};

class VNLFactory: public SolverLibraryFactory {
public:
	VNLFactory() :
			SolverLibraryFactory("VNL") {
	}

	Vector* CreateVector(int dimension = 0, double value = 0.0) const {
		return new VNLVector(dimension, value);
	}

	Vector* CreateVectorFromFile(const std::string& filename) const {
		vnl_vector<double>* y = new vnl_vector<double>(134);	//FIX
		readVector(filename.c_str(), *y);
//		std::cout << "DEBUG: y = " << *y << std::endl;
		return new VNLVector(*y);
	}

	Preconditioner* CreateDiagonalPreconditioner(const SparseMatrix& m) const {
//		return new GMMPreconditioner(
//				static_cast<const VNLMatrix*>(&m)->GetImpl());
		return 0;
	}

	class VNLSparseMatrixInsert {
	public:
		VNLSparseMatrixInsert(vnl_sparse_matrix<double>& mat) :
				m_(mat) {
		}

		void operator()(const Entry& e) {
			m_(e.rowIndex, e.colIndex) = e.value;
		}

	private:
		vnl_sparse_matrix<double>& m_;
	};

	SparseMatrix* CreateSparseMatrix(int m, int n,
			const std::vector<Entry>& entries) const {
		vnl_sparse_matrix<double>* mat = new vnl_sparse_matrix<double>(m, n);

		VNLSparseMatrixInsert matrixInsert(*mat);
		std::for_each(entries.begin(), entries.end(), matrixInsert);

//		std::cout << "SparseMatrix constructed = " << *mat << endl;
		return new VNLMatrix(*mat);
	}

	SparseMatrix* CreateSparseMatrixFromFile(
			const std::string& filename) const {
		vnl_sparse_matrix<double>* m = new vnl_sparse_matrix<double>;
//		Harwell_Boeing_load(filename, *m);
		std::cout << "calling readHBMatrix\n";
		readHBMatrix(filename, *m);
		std::cout << "done calling readHBMatrix\n";

		return new VNLMatrix(*m);
	}

	GSmoothAMatrix* CreateGSmoothAMatrix(const SparseMatrix& S,
			const SparseMatrix& G) const {
		return new VNLGSmoothAMatrix(
				static_cast<const VNLMatrix*>(&S)->GetImpl(),
				static_cast<const VNLMatrix*>(&G)->GetImpl());
		return 0;
	}

	void CG(const SparseMatrix& A, Vector& x, const Vector& rhs,
			const Preconditioner& pre, int maximumIteration,
			double tolerance) const {
//		std::vector<double>& xImpl = static_cast<GMMVector*>(&x)->GetImpl();
//		const std::vector<double>& rhsImpl = static_cast<const GMMVector*>(&rhs)->GetImpl();
//		const gmm::diagonal_precond<gmm::csc_matrix<double> >& preImpl = static_cast<const GMMPreconditioner*>(&pre)->GetImpl();
//		const gmm::GMMGSmoothAMatrix* AConc = static_cast<const gmm::GMMGSmoothAMatrix*>(&A);
//
//		gmm::iteration iter(tolerance, 0,maximumIteration);
//		gmm::cg(*AConc, xImpl, rhsImpl, preImpl, iter);

		std::cout << __func__ << "\n";
		vnl_vector<double>& xImpl = static_cast<VNLVector&>(x).GetImpl();
		const vnl_vector<double>& rhsImpl =
				static_cast<const VNLVector*>(&rhs)->GetImpl();
		const VNLGSmoothAMatrix& AConc =
				static_cast<const VNLGSmoothAMatrix&>(A);

		VNLGSmoothLinearSystem ls(AConc, rhsImpl);
		vnl_lsqr lsqr(ls);
		lsqr.set_max_iterations(maximumIteration);

		lsqr.minimize(xImpl);
	}

private:

	void readVector(const char* filename, vnl_vector<double>& v) const {
		std::ifstream in(filename);

		int dimension;
		in >> dimension;

		v.set_size(dimension);

		for (int index = 0; index < dimension; index++) {
			// Should use readline?
			in >> v[index];
		}
	}
};

} // end namespace cap
#endif /* VNLFACTORY_H_ */
