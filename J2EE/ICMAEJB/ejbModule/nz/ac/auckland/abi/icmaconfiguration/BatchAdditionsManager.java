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
package nz.ac.auckland.abi.icmaconfiguration;

import java.util.ArrayList;
import java.util.Hashtable;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.annotation.Resource;
import javax.ejb.AccessTimeout;
import javax.ejb.EJB;
import javax.ejb.Schedule;
import javax.ejb.Singleton;
import javax.ejb.Startup;
import javax.management.openmbean.KeyAlreadyExistsException;

import nz.ac.auckland.abi.businesslogic.ICMAPatientRecord;
import nz.ac.auckland.abi.dcm4chee.DCMAccessManager;
import nz.ac.auckland.abi.dcm4chee.PatientRecord;
import nz.ac.auckland.abi.dicomprocessing.Moviemaker;
import nz.ac.auckland.abi.helper.PatientJobRecord;

/**
 * Session Bean implementation class BatchAdditionsManager
 */
@Singleton
@Startup
public class BatchAdditionsManager {

	@Resource(name = "java:global/ICMAPARALLELTASKLIMIT")
	private int taskLimit;

	private boolean checkingInProgress = false;
	private int currentTaskCount = 0;
	@EJB
	ResourceConfigurationManager rManager;
	@EJB
	ICMAPatientRecord patientRecord;

	private Hashtable<String, PatientJobRecord> patientJobRecords;
	private Hashtable<String, Moviemaker> patientJobs;
	private Vector<String> jobsubmissionorder;
	private Hashtable<String, Thread> activeList;
	private Logger log;

	/**
	 * Default constructor.
	 */
	public BatchAdditionsManager() {
		patientJobs = new Hashtable<String, Moviemaker>();
		activeList = new Hashtable<String, Thread>();
		patientJobRecords = new Hashtable<String, PatientJobRecord>();
		jobsubmissionorder = new Vector<String>();
		log = Logger.getLogger(this.getClass().getSimpleName());
	}

	public void addPatientFromPacs(String patientID, String author,int lookback) throws Exception {
		PatientRecord pat = DCMAccessManager.getPatient(patientID);
		addPatientFromPacs(pat, author,lookback);
	}

	public void addPatientFromPacs(PatientRecord pat, String author,int lookback) throws Exception {
		if (!patientJobRecords.containsKey(pat.getPatientID())) {
			PatientJobRecord record = new PatientJobRecord(pat, author, rManager, patientRecord,lookback);
			patientJobRecords.put(pat.getPatientID(), record);
			log.log(Level.FINE,"Created job record for patient "+pat);
			Hashtable<String, Moviemaker> newJobs = record.getProcessRecord();
			ArrayList<String> keys = new ArrayList<String>(newJobs.keySet());
			for (String key : keys) {
				jobsubmissionorder.add(key);
				patientJobs.put(key, newJobs.get(key));
			}
			if (currentTaskCount < taskLimit) {
				checkForCompletedTasks();
			}
		} else {
			throw new KeyAlreadyExistsException("A request for " + pat.getPatientID() + " is already ongoing, please wait for it to complete");
		}
	}
	

	@AccessTimeout(-1)
	@Schedule(minute = "*/1", hour = "*", persistent = false)
	public void checkForCompletedTasks() {
		if (!checkingInProgress) {
			if (patientJobs.size() > 0 || currentTaskCount > 0) {
				checkingInProgress = true;
				if (currentTaskCount > 0) {
					ArrayList<String> keys = new ArrayList<String>(activeList.keySet());
					for (String thx : keys) {
						Thread thread = activeList.get(thx);
						log.log(Level.FINE, "Task " + thx + " is on active list");
						if (!thread.isAlive()) {// Completed
							log.log(Level.FINE, "Task " + thx + " has completed");
							try {
								String[] toks = thx.split("#");
								PatientJobRecord jrec = patientJobRecords.get(toks[0]);
								if (jrec != null) {
									jrec.setJobCompletionStatus(thx);
									if (jrec.getCompletionStatus()) {//
										// remove job records of the patient
										Hashtable<String, Moviemaker> newJobs = jrec.getProcessRecord();
										ArrayList<String> jkeys = new ArrayList<String>(newJobs.keySet());
										for (String key : jkeys) {
											patientJobs.remove(key);
										}
										patientJobRecords.remove(toks[0]);
									}
								}
							} catch (Exception exx) {
								log.log(Level.FINE, "Exception occured while accessing record " + thx);
								ArrayList<String> pjk = new ArrayList<String>(patientJobRecords.keySet());
								log.log(Level.FINE, "Available records are ");
								for (String key : pjk) {
									log.log(Level.FINE, key);
								}
							}
							// Remove task from active list
							int jid = -1;
							for (int jo = 0; jo < jobsubmissionorder.size(); jo++) {
								String id = jobsubmissionorder.elementAt(jo);
								if (id.equalsIgnoreCase(thx)) {
									jid = jo;
									break;
								}
							}
							if (jid > -1) {
								log.log(Level.FINE, "Removed job " + jobsubmissionorder.elementAt(jid) + " post completion " + (jobsubmissionorder.size() - 1) + " tasks left");
								jobsubmissionorder.removeElementAt(jid);
							}
							activeList.remove(thx);
						}
					}
					currentTaskCount = activeList.size();
				}
				if (currentTaskCount < taskLimit) {
					for (int jo = 0; jo < jobsubmissionorder.size() && currentTaskCount < taskLimit; jo++) {
						String id = jobsubmissionorder.elementAt(jo);
						if (!activeList.containsKey(id)) {
							Moviemaker maker = patientJobs.get(id);
							Thread taskThread;
							if(maker.isSeriesType()){
								taskThread = new Thread(maker.getDicomSeries2Movie());
							}else{
								taskThread = new Thread(maker.getDicom2Movie());
							}
							taskThread.start();
							activeList.put(id, taskThread);
							log.log(Level.FINE, "Created new job " + id);
							currentTaskCount++;
						}
					}
				}
				checkingInProgress = false;
			}
		}
	}
	
	public boolean hasOngoingProcess(String patientID){
		return patientJobRecords.containsKey(patientID);
	}
}
