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
package nz.ac.auckland.abi.workflow;

public class WSWorkflowInfo {

	private static long workflowid = 0;
	private long myid;
	private SpeckleTrackingInterface sti;
	private SpeckleToFEMInterface smi;
	private MRIModelFittingInterface mrifit;
	private boolean stiUpdated = false; // Flag to mark if documents were
										// uploaded to CMS
	private boolean smiUpdated = false; // Flag to mark if documents were
										// uploaded to CMS
	private boolean mriUpdated = false; // Flag to mark if documents were
										// uploaded to CMS

	private long timeStamp;
	private String instanceID;
	private Exception stiException = null;
	private Exception smiException = null;
	private Exception mriException = null;
	private String patientID = null;
	private String studyID = null;
	private String seriesID = null;
	private String modelAuthor = "ICMA";
	private String type = "US";
	private String mrifitworkflowID;

	public WSWorkflowInfo() {
		myid = workflowid++;
		timeStamp = System.currentTimeMillis();
	}

	public void setSpeckleTrackingInterface(SpeckleTrackingInterface st) {
		sti = st;
	}

	public SpeckleTrackingInterface getSpeckleTrackingInterface() {
		return sti;
	}

	public void setSpeckleToFEMInterface(SpeckleToFEMInterface st) {
		smi = st;
	}

	public SpeckleToFEMInterface getSpeckleToFEMInterface() {
		return smi;
	}

	public MRIModelFittingInterface getMRIModelFittingInterface() {
		return mrifit;
	}

	public void setMRIModelFittingInterface(MRIModelFittingInterface mrifit,String workflowID) {
		this.mrifit = mrifit;
		mrifitworkflowID = workflowID;
		type = "MRI";
	}
	
	public String getMRIFitWorkFlowId(){
		return mrifitworkflowID;
	}

	public String getWorkFlowID() {
		return "workflow" + myid;
	}

	public long getTimeStamp() {
		return timeStamp;
	}

	public void setInstanceID(String id) {
		instanceID = id;
	}

	public String getInstanceID() {
		return instanceID;
	}

	public boolean getStiStatus() {
		return stiUpdated;
	}

	public void setStiStatus(boolean status) {
		stiUpdated = status;
	}

	public boolean getSMiStatus() {
		return smiUpdated;
	}

	public void setSMiStatus(boolean status) {
		smiUpdated = status;
	}

	public boolean getMriUpdated() {
		return mriUpdated;
	}

	public void setMriUpdated(boolean mriUpdated) {
		this.mriUpdated = mriUpdated;
	}

	public boolean isStiUpdated() {
		return stiUpdated;
	}

	public void setStiUpdated(boolean stiUpdated) {
		this.stiUpdated = stiUpdated;
	}

	public Exception getMRIException() {
		return mriException;
	}

	public void setMRIException(Exception mriException) {
		this.mriException = mriException;
	}

	public Exception getSMiException() {
		return smiException;
	}

	public void setSMiException(Exception smiException) {
		this.smiException = smiException;
	}

	public Exception getStiException() {
		return stiException;
	}

	public void setStiException(Exception stiException) {
		this.stiException = stiException;
	}

	public String getPatientID() {
		return patientID;
	}

	public void setPatientID(String patientID) {
		this.patientID = patientID;
	}

	public String getStudyID() {
		return studyID;
	}

	public void setStudyID(String studyID) {
		this.studyID = studyID;
	}

	public String getSeriesID() {
		return seriesID;
	}

	public void setSeriesID(String seriesID) {
		this.seriesID = seriesID;
	}

	public String getModelAuthor() {
		return modelAuthor;
	}

	public void setModelAuthor(String modelAuthor) {
		this.modelAuthor = modelAuthor;
	}

	public String getType(){
		return type;
	}
	
	
	public void cleanup() {
		try {
			if (sti != null)
				sti.cleanUp();
		} catch (Exception exx) {

		}
		try {
			if (smi != null)
				smi.cleanUp();
		} catch (Exception ex1) {

		}
		try {
			if (mrifit != null)
				mrifit.cleanUp(mrifitworkflowID);
		} catch (Exception ex1) {

		}
	}
}
