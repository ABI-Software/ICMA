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

#ifndef LEVELSET_H_
#define LEVELSET_H_
#include <cmath>
#include <string.h> //For memcpy

class LevelSet {
public:
	double **** data;
	double * scratch;
	int elements;
	int xd;
	int yd;
	int maxComponents;
	LevelSet(int num_elements, int x_d, int y_d, int nComp = 3) :
			elements(num_elements), xd(x_d), yd(y_d), maxComponents(nComp) {
		data = new double***[elements];
		for (int i = 0; i < elements; i++) {
			data[i] = new double**[xd];
			for (int j = 0; j < xd; j++) {
				data[i][j] = new double*[yd];
				for (int k = 0; k < yd; k++)
					data[i][j][k] = new double[maxComponents];
			}
		}
		scratch = new double[10*maxComponents];
	}

	~LevelSet() {
		for (int i = 0; i < elements; i++) {
			for (int j = 0; j < xd; j++) {
				for (int k = 0; k < yd; k++)
					delete[] data[i][j][k];
				delete[] data[i][j];
			}
			delete[] data[i];
		}
		delete[] data;
		delete[] scratch;
	}

	double*** operator ()(int eidx) {
		return data[eidx];
	}

	double** operator ()(int eidx, int xid) {
		return data[eidx][xid];
	}

	double* operator ()(int eidx, int xid, int yid) {
		return data[eidx][xid][yid];
	}

	double& operator ()(int eidx, int xid, int yid, int comp) {
		return data[eidx][xid][yid][comp];
	}

	void setData(int eidx, int xid, int yid, double* indata, int n_comp) {
		memcpy(data[eidx][xid][yid],indata,sizeof(double)*n_comp);
	}

	double getArea() {
		double total_area = 0.0;
		for (int eid = 0; eid < elements; eid++) {
			for (int xid = 1; xid < xd; xid++) {
				for (int yid = 0; yid < yd - 1; yid++) {
					total_area = total_area
							+ getSquareArea(data[eid][xid - 1][yid],
									data[eid][xid - 1][yid + 1],
									data[eid][xid][yid],
									data[eid][xid][yid + 1]);
				}
			}
		}
		return total_area;
	}

	double getLPSum(int compID, double power) {
		double total = 0.0;
		for (int eid = 0; eid < elements; eid++) {
			double mt = 0.0;
			for (int xid = 0; xid < xd; xid++) {
				double st = 0.0;
				for (int yid = 0; yid < yd; yid++) {
					st = st + pow(data[eid][xid][yid][compID], power);
				}
				mt += st;
			}
			total +=mt;
		}
		return pow(total, 1.0 / power);
	}

	double getSum(int compID) {
		double total = 0.0;
		for (int eid = 0; eid < elements; eid++) {
			for (int xid = 0; xid < xd; xid++) {
				for (int yid = 0; yid < yd; yid++) {
					total = total + data[eid][xid][yid][compID];
				}
			}
		}
		return total;
	}
private:
	double getLength(double* x1, double* x2) {
		double len = 0.0;
		for (int i = 0; i < maxComponents; i++) {
			len += (x1[i] - x2[i]) * (x1[i] - x2[i]);
		}
		return sqrt(len);
	}

	double getSquareArea(double* ltx, double* rtx, double* lbx, double* rbx) {
		//Done this way to reduce cache misses
		double *lt = scratch;
		double *rt = (scratch+maxComponents);
		double *lb = (scratch+2*maxComponents);
		double *rb = (scratch+3*maxComponents);
		memcpy(lt,ltx,sizeof(double)*maxComponents);
		memcpy(rt,rtx,sizeof(double)*maxComponents);
		memcpy(lb,lbx,sizeof(double)*maxComponents);
		memcpy(rb,rbx,sizeof(double)*maxComponents);

		double a = getLength(lb, lt);
		double b = getLength(lt, rt);
		double c = getLength(rt, rb);
		double d = getLength(rb, lb);
		double e = getLength(lt, rb);

		double s = (a + d + e) / 2.0;
		double left_triangle = sqrt(s * (s - a) * (s - d) * (s - e));

		s = (b + e + c) / 2.0;
		double right_triangle = sqrt(s * (s - b) * (s - e) * (s - c));

		return (left_triangle + right_triangle);
	}

};

#endif /* LEVELSET_H_ */
