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

#ifndef WALLSEGMENT_H_
#define WALLSEGMENT_H_

class WallSegment {
public:
	int elementidb;
	int elementida;
	double* xia;
	double* xib;

	WallSegment(int ea, int eb) {
		elementida = ea;
		elementidb = eb;
		xia = new double[3];
		xib = new double[3];
	}

	WallSegment(const WallSegment& seg) {
		elementida = seg.elementida;
		elementidb = seg.elementidb;
		xia = new double[3];
		xib = new double[3];
		xia[0] = seg.xia[0];
		xia[1] = seg.xia[1];
		xia[2] = seg.xia[2];
		xib[0] = seg.xib[0];
		xib[1] = seg.xib[1];
		xib[2] = seg.xib[2];
	}

	~WallSegment() {
		delete[] xia;
		delete[] xib;
	}

	void setXIA(double xia1, double xia2, double xia3) {
		xia[0] = xia1;
		xia[1] = xia2;
		xia[2] = xia3;
	}

	void setXIB(double xib1, double xib2, double xib3) {
		xib[0] = xib1;
		xib[1] = xib2;
		xib[2] = xib3;
	}
};

#endif /* WALLSEGMENT_H_ */
