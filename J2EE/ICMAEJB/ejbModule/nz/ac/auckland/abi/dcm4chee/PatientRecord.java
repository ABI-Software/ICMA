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
import java.util.Date;

import org.dcm4che.data.Dataset;
import org.dcm4che.dict.Tags;

public class PatientRecord implements Serializable{
	private static final long serialVersionUID = 1L;
	private String patientID;
	private String patientName;
	private String patientSex;
	private String patientBirthDate;
	private String physicianName;

	public PatientRecord(Dataset dataSet) {
		try {
			// dataSet.dumpDataset(System.out, null);
		} catch (Exception exx) {

		}
		patientID = dataSet.getString(Tags.PatientID);
		// The patient name is recorded in reverse i.e. surname, first name,
		// initials, salutation
		// Reverse this
		String name = dataSet.getString(Tags.PatientName).replace("^", ", ")
				.replaceAll(", ,", ",");
		String toks[] = name.split(",");
		StringBuffer nbuf = new StringBuffer();
		for (int i = 0; i < toks.length; i++) {
			nbuf.append(toks[toks.length - 1 - i] + " ");
		}
		patientName = nbuf.toString();
		patientSex = parsePatientSex(dataSet);
		patientBirthDate = parsePatientBirthDate(dataSet);
		physicianName = parseAssociatedPhysician(dataSet);
	}

	public String getPatientID() {
		return patientID;
	}

	public String getPatientName() {
		return patientName;
	}

	public String getPatientSex() {
		return patientSex;
	}

	public String getPatientBirthDate() {
		return patientBirthDate;
	}

	public String getPhysicianName() {
		return physicianName;
	}


	public String toString() {
		StringBuffer buf = new StringBuffer();
		buf.append(patientID + "\t");
		buf.append(patientName + "\t");
		buf.append(patientSex + "\t");
		buf.append(patientBirthDate + "\t");
		buf.append(physicianName + "\t");
		return buf.toString();
	}

	private String parsePatientSex(Dataset dataSet) {
		String pSex = "";
		try {// Some datasets do not have sex setup
			pSex = dataSet.getString(1048640).trim();
			if (!pSex.equalsIgnoreCase("M") && !pSex.equalsIgnoreCase("F")) {
				pSex = "UNKNOWN";
			}
		} catch (Exception exx) {

		}
		return pSex;
	}

	private String parsePatientBirthDate(Dataset dataSet) {
		String pBirthDate = dataSet.getString(Tags.PatientBirthDate);
		if (pBirthDate != null) {
			try {
				DateFormat formatter = new SimpleDateFormat("yyyyMMdd");
				Date date = (Date) formatter.parse(pBirthDate);
				SimpleDateFormat formatDate = new SimpleDateFormat("dd-MM-yyyy");
				pBirthDate = formatDate.format(date).toString();
			} catch (Exception exx) {
				exx.printStackTrace();
			}
		} else {
			pBirthDate = "UNKNOWN!";
		}
		return pBirthDate;
	}

	private String parseAssociatedPhysician(Dataset dataSet) {
		String pName = dataSet.getString(Tags.ReferringPhysicianName);
		if (pName == null) { // Try Physician reading the study
			pName = dataSet.getString(Tags.NameOfPhysicianReadingStudy);
		}
		if (pName != null) {
			if (pName.indexOf("\"") > 0) {
				pName = pName.replace("\"", "`~");
			}
			if (pName.indexOf("'") > 0)
				pName = pName.replace("'", "~`");
			String name = pName.replace("^", ", ").replaceAll(", ,", ",");
			String toks[] = name.split(",");
			StringBuffer nbuf = new StringBuffer();
			for (int i = 0; i < toks.length; i++) {
				nbuf.append(toks[toks.length - 1 - i] + " ");
			}
			pName = nbuf.toString();
		} else {
			pName = "UNKNOWN";
		}
		return pName;
	}
}
