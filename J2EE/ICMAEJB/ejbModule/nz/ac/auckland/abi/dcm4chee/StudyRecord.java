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
package nz.ac.auckland.abi.dcm4chee;


import java.io.Serializable;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;

import org.dcm4che.data.Dataset;


/**
 * class StudyRecord \brief A data class that manages all PACS information
 * related to a patient's study
 * 
 * @author rjag008
 * @version 0.1
 */
public class StudyRecord implements Serializable{
	private static final long serialVersionUID = 1L;
	final boolean debug = false;
	String studyInstanceUID; // The study instance UID associated with this
								// instance
	String studyDescription = ""; // Brief description of the study
	String studyDate = "UNKNOWN";
	String modalitiesInStudy = "";

	/**
	 * Constructor to load a study from a query
	 * 
	 * @param dataSet
	 */

	public StudyRecord(Dataset dataSet) {
		studyInstanceUID = dataSet.getString(2097165);
		studyDescription = ((dataSet.getString(528432) != null) ? dataSet
				.getString(528432) : "UNKNOWN!");
		if ((studyDate = (dataSet.getString(524320))) != null) {
			DateFormat formatter = new SimpleDateFormat("yyyyMMdd");
			try {
				Date date = (Date) formatter.parse(studyDate);
				SimpleDateFormat formatDate = new SimpleDateFormat("dd-MM-yyyy");
				studyDate = formatDate.format(date).toString();
				String studyTime = null;
				if((studyTime = (dataSet.getString(524336))) != null){
					SimpleDateFormat formatTime = new SimpleDateFormat("kkmmss");
					Date time = (Date) formatTime.parse(studyTime);
					SimpleDateFormat formatedTime = new SimpleDateFormat("kk:mm:ss.SSSS");
					studyDate = studyDate+" "+formatedTime.format(time).toString();
				}else{
					//Suffix a 0 time to avoid downstream errors
					SimpleDateFormat formatedTime = new SimpleDateFormat("kk:mm:ss.SSSS");
					studyDate = studyDate+" "+formatedTime.format(0).toString();
				}
			} catch (Exception ex) {
				ex.printStackTrace();
			}
		} else {
			Calendar cal = new GregorianCalendar();
			studyDate = cal.getTime().toString();
		}

		modalitiesInStudy = getModalities(dataSet, 524385);
	}


	public String getStudyInstanceUID(){
		return studyInstanceUID;
	}
	
	public String getStudyDescription(){
		return studyDescription;
	}
	
	public String getStudyDate(){
		return studyDate;
	}
	
	public String getStudyModalities(){
		return modalitiesInStudy;
	}
		
	private String getModalities(Dataset dataSet, int modalityConstant) {
		String[] temp = null;
		String modality = "";
		if (dataSet.getStrings(modalityConstant) != null)
			temp = dataSet.getStrings(modalityConstant);
		else {
			modality = "unknown";
		}
		for (int i = 0; i < temp.length; ++i) {
			if (i == temp.length - 1)
				modality = modality + temp[i];
			else
				modality = modality + temp[i] + "\\";
		}
		return modality;
	}
	
	public String toString(){
		return "Study "+studyInstanceUID+" dated "+studyDate+" with description "+studyDescription;
	}
	
}
