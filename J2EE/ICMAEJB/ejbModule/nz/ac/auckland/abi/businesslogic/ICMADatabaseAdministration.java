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
package nz.ac.auckland.abi.businesslogic;

import java.io.ByteArrayInputStream;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.List;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.annotation.Resource;
import javax.ejb.Asynchronous;
import javax.ejb.EJB;
import javax.ejb.LocalBean;
import javax.ejb.ObjectNotFoundException;
import javax.ejb.SessionContext;
import javax.ejb.Stateless;
import javax.ejb.TransactionAttribute;
import javax.ejb.TransactionAttributeType;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.persistence.Query;

import nz.ac.auckland.abi.dcm4chee.DCMAccessManager;
import nz.ac.auckland.abi.dcm4chee.PatientRecord;
import nz.ac.auckland.abi.entities.FEMModel;
import nz.ac.auckland.abi.entities.FEMModelModification;
import nz.ac.auckland.abi.entities.Patient;
import nz.ac.auckland.abi.helper.ICMAMetaData;
import nz.ac.auckland.abi.icmaconfiguration.BatchAdditionsManager;
import nz.ac.auckland.abi.icmaconfiguration.CMSContent;
import nz.ac.auckland.abi.icmaconfiguration.ResourceConfigurationManager;
import nz.ac.auckland.abi.icmaconfiguration.ResourceInitializer;

/**
 * Session Bean implementation class ICMADatabaseAdministration
 */
@Stateless
@LocalBean
@TransactionAttribute(TransactionAttributeType.NOT_SUPPORTED)
// Make this bean managed transactions
public class ICMADatabaseAdministration implements ICMADatabaseAdministrationRemote {

	@PersistenceContext(unitName = "ICMADB")
	private EntityManager entityManager;

	@Resource
	private SessionContext ctx;

	@EJB
	private ICMADatabaseAdministrationRemote admin;

	@EJB
	private ResourceConfigurationManager resourceManger;

	@EJB
	private ResourceInitializer rInit;

	@EJB
	PatientsBeanRemote patientsBean;
	@EJB
	StudiesBeanRemote studiesBean;
	@EJB
	USInstancesBeanRemote instanceBean;
	@EJB
	FEMModelsBeanRemote femmodelsBean;
	@EJB
	PatientSourceMapManagerRemote patientMapBean;
	@EJB
	ICMAPatientRecord patientRecord;
	
	@EJB
	BatchAdditionsManager batchManager;

	Logger log = null;

	/**
	 * Default constructor.
	 */
	public ICMADatabaseAdministration() throws Exception {
		log = Logger.getLogger(this.getClass().getSimpleName());
	}

	public void cacheDataBase() throws Exception {
		// Done this way to ensure call is not syncLocked
		if (ctx.getBusinessObject(ICMADatabaseAdministrationRemote.class) != null)
			ctx.getBusinessObject(ICMADatabaseAdministrationRemote.class).synchronizePatients(-1); // All
	}

	@Asynchronous
	public void synchronizePatients(int lookBack) throws Exception {

		if (lookBack > -1) {
			Hashtable<String, PatientRecord> pacstable = new Hashtable<String, PatientRecord>();
			Vector<PatientRecord> pacsPatients = DCMAccessManager.getPatients(lookBack);
			for (PatientRecord pat : pacsPatients) {
				pacstable.put(pat.getPatientID(), pat);
			}
			Hashtable<String, Patient> icmatable = new Hashtable<String, Patient>();
			List<Patient> icmaPatients = patientsBean.getAllPatients(null);
			for (Patient pat : icmaPatients) {
				icmatable.put(pat.getId(), pat);
			}

			ArrayList<String> newPatients = new ArrayList<String>(pacstable.keySet());
			ArrayList<String> curPatients = new ArrayList<String>(icmatable.keySet());

			ArrayList<String> temp = new ArrayList<String>(newPatients);
			temp.removeAll(curPatients);

			// Add in the new Patients
			for (String patId : temp) {
				try {
					PatientRecord pat = pacstable.get(patId);
					// Synchronously
					Patient myPatient = patientsBean.getPatientByID(pat.getPatientID());
					if (myPatient == null) {
						batchManager.addPatientFromPacs(pat, "ICMA",lookBack);
					} else {
						log.log(Level.INFO, "Patient with ID " + pat.getPatientID() + " already exists");
					}
				} catch (Exception exx) {
					log.log(Level.INFO, "Exception occured while adding patient " + patId + " ICMA may not be in sync with the PACS");
				}
			}

			temp = new ArrayList<String>(curPatients);
			temp.removeAll(newPatients);

			// Remove all the deleted (in PACS) patients
			for (String patId : temp) {
				try {
					removePatient(patId);
				} catch (Exception exx) {
					log.log(Level.INFO, "Exception occured while purging patient " + patId + " ICMA may not be in sync with the PACS");
				}
			}
		} else {

			// resourceManger.dropCMS();
			// addPatientFromPACS("55437475");
			// addPatientFromPACS("12885539");
			// removePatient("12885539");
			// addPatientFromPACS("50184730");
			/*
			 * Vector<PatientRecord> pacsPatients = DCMAccessManager
			 * .getPatients(lookBack); // Add in the new Patients -
			 * synchronously String skip[] =
			 * {"10/06/24:131043","26644232","008","007","10/08/18:083732",
			 * "54545290", "41295775", "10/07/12:145429", "10/07/20:124502",
			 * "52591546", "55023646", "55036063", "40726152", "17593650",
			 * "55144877", "10/06/30:102545", "20595809", "NASA1029004",
			 * "NASA1029004", "10/08/02:151615", "31188601", "10/07/20:144153",
			 * "55347239", "NASA005", "35784705", "10/08/26:074610",
			 * "10/06/30:150117" };
			 * 
			 * for (int ctr=0;ctr<pacsPatients.size();ctr++) { if(ctr>10){
			 * break; } PatientRecord pat = pacsPatients.elementAt(ctr); String
			 * pid = pat.getPatientID(); boolean skipRec = false; for(String
			 * skipr : skip){ if(skipr.equalsIgnoreCase(pid)){ skipRec = true; }
			 * } if(skipRec) continue;
			 * 
			 * try { Patient myPatient =
			 * patientsBean.getPatientByID(pat.getPatientID()); if (myPatient ==
			 * null) { if (pat != null) { batchAdd(pat,false); } } else {
			 * System.out.println("Skipping Patient with ID " +
			 * pat.getPatientID() + " record already exists"); } } catch
			 * (Exception exx) { System.out
			 * .println("Exception occured while adding patient " +
			 * pat.getPatientID() + " ICMA may not be in sync with the PACS");
			 * System.out.println("Stopping the process"); break; } }
			 */
			log.log(Level.INFO, "Added patient records");
		}

	}

	public void addPatientFromPACS(String patientID) throws Exception {
		try {
			if (rInit.getBatchProcessSubmissions().get(patientID) == null) {
				log.log(Level.INFO, "Adding Patient id " + patientID);
				rInit.getBatchProcessSubmissions().put(patientID, new Long(System.currentTimeMillis()));
				PatientRecord pat = DCMAccessManager.getPatient(patientID);
				addPatientFromPACS(pat);
			} else {
				log.log(Level.INFO, "Patient id " + patientID + " resubmitted");
			}
		} catch (Exception exx) {
			exx.printStackTrace();
			throw exx;
		}
	}

	public void addPatientFromPACS(PatientRecord pat) throws Exception {
		try {
			log.log(Level.INFO, pat.toString());
			Patient myPatient = patientsBean.getPatientByID(pat.getPatientID());
			if (myPatient == null) {
				batchManager.addPatientFromPacs(pat, "ICMA",-1);
			} else {
				log.log(Level.INFO, "Patient with ID " + pat.getPatientID() + " already exists");
			}
		} catch (Exception exx) {
			exx.printStackTrace();
			throw exx;
		}
	}

	public void refreshPatientFromPACS(String patientID) throws Exception {
		Patient myPatient = patientsBean.getPatientByID(patientID);
		PatientRecord pat = DCMAccessManager.getPatient(patientID);
		if (myPatient == null) {
			if (pat != null) {
				batchManager.addPatientFromPacs(pat, "ICMA",-1);
			} else {
				throw new ObjectNotFoundException("Patient with ID " + patientID + " not found in PACS");
			}
		} else {
			if (pat != null) {
				// Remove patient and related records
				patientRecord.removePatient(patientID);
				// Add the patient related studies
				batchManager.addPatientFromPacs(pat, "ICMA",-1);
			} else {
				throw new ObjectNotFoundException("Patient with ID " + patientID + " not found in PACS");
			}
		}
	}

	@Asynchronous
	public void synchronizePatientStudies(String patientID, int lookBack) throws Exception {
		Patient myPatient = patientsBean.getPatientByID(patientID);
		PatientRecord pat = DCMAccessManager.getPatient(patientID);
		if (myPatient == null) {
			if (pat != null) {
				batchManager.addPatientFromPacs(pat, "ICMA",-1);
			} else {
				throw new ObjectNotFoundException("Patient with ID " + patientID + " not found in PACS");
			}
		} else {
			if (pat != null) {
				// Add the patient related studies
				batchManager.addPatientFromPacs(pat, "ICMA",lookBack);
			} else {
				throw new ObjectNotFoundException("Patient with ID " + patientID + " not found in PACS");
			}
		}
	}

	/*	@Asynchronous
	 	public void synchronizeStudyInstances(String patientID, String studyID, int lookBack) throws Exception {

		if (rInit.canStartTask()) {
			Vector<InstanceRecord> instances = DCMAccessManager.getStudyInstances(studyID, lookBack);

			// Check if the study and fem instances exist
			PACSStudy study = studiesBean.getStudy(studyID);
			List<USStudyInstances> dbinst = instanceBean.getInstancesForStudy(study);
			List<FEMModel> dbmodels = femmodelsBean.getStudyModels(study);
			for (USStudyInstances dbi : dbinst) {
				for (InstanceRecord pinst : instances) {
					if (dbi.getInstanceID() == pinst.getSopIuid()) {
						instances.remove(pinst);
						break;
					}
				}
			}

			for (FEMModel dbi : dbmodels) {
				for (InstanceRecord pinst : instances) {
					if (dbi.getId() == pinst.getSopIuid()) {
						instances.remove(pinst);
						break;
					}
				}
			}

			PACSInstanceprocessor processInstances = new PACSInstanceprocessor(this.getClass().getSimpleName(), resourceManger, patientID, studyID, instances);

			Vector<USStudyInstances> dinst = processInstances.getPACSInstances();
			Vector<FEMModel> feminst = processInstances.getFEMModels();

			// Atomic add to DB
			// batchAdd(null, null, dinst, feminst);
			patientRecord.batchAdd(null, null, dinst, feminst);
			processInstances.cleanup();
			rInit.taskCompleted();
		}
	}*/

	public boolean removePatient(String patientID) throws Exception {
		if (!rInit.getBatchProcessSubmissions().containsKey(patientID))
			return patientRecord.removePatient(patientID);
		else
			throw new Exception("Data related to " + patientID + " is under process. Cannot remove record.");
	}

/*	@Asynchronous
	public void batchAdd(PatientRecord pat, boolean refresh) throws Exception {
		final int lookBack = -1;
		boolean createdAllRecords = true;

		while (rInit.canStartTask()) {
			try {
				Thread.sleep(10000);
			} catch (Exception exx) {

			}
			log.log(Level.FINE, "Batch add waiting for tasks to complete");
		}

		String patientID = pat.getPatientID();
		Vector<StudyRecord> pstudies = DCMAccessManager.getPatientStudies(patientID, lookBack);
		Vector<PACSStudy> studyInstances = new Vector<PACSStudy>();
		Vector<USStudyInstances> dicomInstances = new Vector<USStudyInstances>();
		Vector<FEMModel> femInstances = new Vector<FEMModel>();
		Vector<String> createdCMSNodes = new Vector<String>();

		for (StudyRecord rec : pstudies) {
			PACSStudy newStudy = new PACSStudy(patientID, rec.getStudyInstanceUID(), rec.getStudyDate(), rec.getStudyDescription());
			studyInstances.add(newStudy);
			try {

				Vector<InstanceRecord> instances = DCMAccessManager.getStudyInstances(rec.getStudyInstanceUID(), lookBack);

				PACSInstanceprocessor processInstances = new PACSInstanceprocessor(this.getClass().getSimpleName(), resourceManger, patientID, rec.getStudyInstanceUID(), instances);

				Vector<USStudyInstances> dinst = processInstances.getPACSInstances();
				Vector<FEMModel> feminst = processInstances.getFEMModels();
				dicomInstances.addAll(dinst);
				femInstances.addAll(feminst);

				log.log(Level.FINE, "Study " + rec.getStudyInstanceUID() + " has " + dinst.size() + " instances and " + feminst.size() + " models");
				processInstances.cleanup();
				createdCMSNodes.addAll(processInstances.getCreatedNodes());
			} catch (Exception ejbEx) {
				createdAllRecords = false;
				log.log(Level.INFO, "Exception occured while adding study record with ID " + rec.getStudyInstanceUID() + "\n" + ejbEx);
				ejbEx.printStackTrace();
			}
		}
		rInit.taskCompleted();
		// Perform atomic operations
		if (createdAllRecords) {
			if ((dicomInstances.size() + femInstances.size()) > 0) {
				// Add the patient to ensure foreign key consistency
				Patient entity = null;
				if (!refresh) { // new entry
					entity = new Patient(pat.getPatientID(), pat.getPatientName(), pat.getPatientBirthDate(), pat.getPatientSex());
				}
				patientRecord.batchAdd(entity, studyInstances, dicomInstances, femInstances);
				// Hit the database
				// entityManager.flush();
				log.log(Level.INFO, "Successfully added patient " + pat.getPatientID() + " record ");
				rInit.getBatchProcessSubmissions().remove(pat.getPatientID());
			} else {
				log.log(Level.INFO, "Did not add patient " + pat.getPatientID() + " record as no instance information has been uploaded ");
			}
		} else {
			// Remove all cms nodes
			try {
				resourceManger.removeCMSNodes(createdCMSNodes);
			} catch (Exception exx) {
				log.log(Level.INFO, "" + exx);
			}
			log.log(Level.INFO, "****************************************");
			log.log(Level.INFO, "Failed to add patient " + patientID);
			log.log(Level.INFO, "****************************************");
		}
	}
*/
	public String exportModel(String instanceID) throws Exception {
		FEMModel model = femmodelsBean.getModel(instanceID);
		if (model != null) {
			String modelPk = "" + model.getPk();
			String q = "SELECT p from " + FEMModelModification.class.getName() + " p where p.id = :modelid and p.metadata is not null";
			Query query = entityManager.createQuery(q).setParameter("modelid", modelPk);
			List<FEMModelModification> models = query.getResultList();
			String metaData = models.get(0).getMetadata();
			CMSContent dicom = resourceManger.getCMSNodeAt(metaData);

			String exportData = ICMAMetaData.getFittingXML(new ByteArrayInputStream(dicom.output.toByteArray()));
			return exportData;
		}
		return null;

	}

/*	public JSONObject getModelSpeckleMetaData(String instanceID) throws Exception {
		FEMModel model = femmodelsBean.getModel(instanceID);
		if (model != null) {
			String modelPk = "" + model.getPk();
			String q = "SELECT p from " + FEMModelModification.class.getName() + " p where p.id = :modelid and p.metadata is not null";
			Query query = entityManager.createQuery(q).setParameter("modelid", modelPk);
			List<FEMModelModification> models = query.getResultList();
			String metaData = models.get(0).getMetadata();
			CMSContent dicom = resourceManger.getCMSNodeAt(metaData);

			JSONObject speckleData = ICMAMetaData.getSpeckleXML(new ByteArrayInputStream(dicom.output.toByteArray()));
			JSONObject obj = new JSONObject();
			if(speckleData.containsKey("SPECKLETRACKINGOUTPUT"))
				obj.put("speckleoutput", speckleData.get("SPECKLETRACKINGOUTPUT"));
			if(speckleData.containsKey("SPECKLETRACKINGINPUT"))
				obj.put("speckleinput", speckleData.get("SPECKLETRACKINGINPUT"));
			obj.put("name",model.getModelName());
			obj.put("patientID", model.getPatientID());
			obj.put("studyid", model.getStudyID());
			obj.put("seriesid", model.getSeriesID());
			obj.put("annotation", model.getAnnotation());
			obj.put("author", model.getAuthor());
			return obj;
		}
		return null;
	}*/

}
