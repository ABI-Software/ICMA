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

#include "LongAxisRegulariser.h"

LongAxisRegulariser::LongAxisRegulariser(std::vector<std::vector<Point3D> >& markers) :
	frameMarkers(markers) {
	//In place regulariser, so markers is modified
	int num_views = frameMarkers.size();
	if(num_views>1){
		MatrixType radii(num_views+2,RBASEINDEX+1);
		radii.fill(0);
#ifdef debug
		MatrixType changes(num_views,RBASEINDEX+1);
		changes.fill(0);
#endif

		for(int i=0;i<num_views;i++){
			std::vector<Point3D>& marks(frameMarkers[i]);
			Point3D apex = marks[APEXINDEX];
			Point3D base = (marks[LBASEINDEX]+marks[RBASEINDEX])*0.5;
			Vector3D abVec = (apex -base);
			for(int k=0;k<APEXINDEX;k++){
				double lambda = (marks[k] - base)*abVec/(abVec*abVec);
				Point3D s = base + abVec*lambda; //The perpendicular projection of marker on apex-base vector
				Vector3D leftV = marks[k] - s;
				radii[i][k] = leftV.Length();
			}
			//Do for apex
			radii[i][APEXINDEX] = abVec.Length();

			for(int k=APEXINDEX+1;k<=RBASEINDEX;k++){
				double lambda = (marks[k] - base)*abVec/(abVec*abVec);
				Point3D s = base + abVec*lambda; //The perpendicular projection of marker on apex-base vector
				Vector3D rightV = marks[k] - s;
				radii[i][k] = rightV.Length();
			}
		}
		//Compute Mean
		//Compute SD
		int meanIdx = num_views ;
		int sdIdx   = num_views + 1;
		for(int i=0;i<num_views;i++){
			for(int j=LBASEINDEX;j<=RBASEINDEX;j++){
				radii[meanIdx][j]+=radii[i][j];
			}
		}
		for(int j=LBASEINDEX;j<=RBASEINDEX;j++){
			radii[meanIdx][j]/=num_views;
		}
		for(int j=LBASEINDEX;j<=RBASEINDEX;j++){
			double temp = 0.0;
			for(int i=0;i<num_views;i++){
				temp += (radii[i][j] - radii[meanIdx][j])*(radii[i][j] - radii[meanIdx][j]);
			}
			radii[sdIdx][j] = sqrt(temp/num_views);
		}
		//Compute z-score
		//Penalize all entries beyond +/- 1 SD
#ifdef debug
		for(int i=0;i<num_views;i++){
			for(int j=LBASEINDEX;j<=RBASEINDEX;j++){
				changes[i][j]= (radii[i][j] - radii[meanIdx][j])/radii[sdIdx][j];
			}
		}


		std::cout<<"\t********************"<<std::endl;
		for(int i=0;i<num_views;i++){
			for(int j=LBASEINDEX;j<=RBASEINDEX;j++){
				std::cout<<changes[i][j]<<" ";
			}
			std::cout<<std::endl;
		}
#endif
		//Compute new radius
		for(int i=0;i<num_views;i++){
			for(int j=LBASEINDEX;j<=RBASEINDEX;j++){
				double zscore = (radii[i][j] - radii[meanIdx][j])/radii[sdIdx][j];
				if(fabs(zscore)>1.0){
					//radii[i][j] = radii[sdIdx][j] + radii[meanIdx][j];
					radii[i][j] = radii[meanIdx][j];
				}else{
					radii[i][j] = 0.0;
				}
			}
		}
		//Commit changes
		for(int i=0;i<num_views;i++){
			std::vector<Point3D>& marks(frameMarkers[i]);
			Point3D apex = marks[APEXINDEX];
			Point3D base = (marks[LBASEINDEX]+marks[RBASEINDEX])*0.5;
			Vector3D abVec = (apex -base);
			for(int k=0;k<APEXINDEX;k++){
				if(radii[i][k]>0.0){
					double lambda = (marks[k] - base)*abVec/(abVec*abVec);
					Point3D s = base + abVec*lambda; //The perpendicular projection of marker on apex-base vector
					Vector3D leftV = marks[k] - s;
					leftV.Normalise();
#ifdef debug
					std::cout<<marks[k]<<"\t";
#endif
					marks[k] = s + leftV*radii[i][k];
#ifdef debug
					std::cout<<marks[k]<<std::endl;
#endif
				}
			}
			//Skip apex
			for(int k=APEXINDEX+1;k<=RBASEINDEX;k++){
				if(radii[i][k]>0.0){
					double lambda = (marks[k] - base)*abVec/(abVec*abVec);
					Point3D s = base + abVec*lambda; //The perpendicular projection of marker on apex-base vector
					Vector3D rightV = marks[k] - s;
					rightV.Normalise();
#ifdef debug
					std::cout<<marks[k]<<"\t";
#endif
					marks[k] = s + rightV*radii[i][k];
#ifdef debug
					std::cout<<marks[k]<<std::endl;
#endif
				}
			}
		}


	}
}

std::vector<std::vector<Point3D> > LongAxisRegulariser::getMarkers() {
	return frameMarkers;
}

LongAxisRegulariser::~LongAxisRegulariser() {

}

