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
 *  Copyright (C) 2011-2014 by the University of Auckland.
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
 *
 *******************************************************************************/
package nz.ac.auckland.abi.dicomprocessing;

public class Moviemaker {
	private Dicom2Movie dcm2movie;
	private DicomSeries2Movie dcmseries2movie;
	private boolean series = false;
	
	public Moviemaker(boolean isSeriesType) throws Exception{
		series = isSeriesType;
	}
	
	public boolean isSeriesType(){
		return series;
	}
	
	public Dicom2Movie getDicom2Movie(){
		return dcm2movie;
	}
	
	public void setDicom2Movie(Dicom2Movie maker){
		dcm2movie = maker;
	}
	
	public DicomSeries2Movie getDicomSeries2Movie(){
		return dcmseries2movie;
	}
	
	public void setDicomSeries2Movie(DicomSeries2Movie maker){
		dcmseries2movie = maker;
	}
	
	public void cleanUp(){
		if(series){
			dcmseries2movie.cleanUp();
		}else{
			dcm2movie.cleanUp();
		}
	}
}
