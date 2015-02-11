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

	public boolean removePatient(String patientID) throws Exception {
		if (!rInit.getBatchProcessSubmissions().containsKey(patientID))
			return patientRecord.removePatient(patientID);
		else
			throw new Exception("Data related to " + patientID + " is under process. Cannot remove record.");
	}


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

}
