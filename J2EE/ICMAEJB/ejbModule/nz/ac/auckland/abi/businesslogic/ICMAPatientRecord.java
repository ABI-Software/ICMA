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

import java.util.List;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.ejb.EJB;
import javax.ejb.LocalBean;
import javax.ejb.Stateless;
import javax.ejb.TransactionAttribute;
import javax.ejb.TransactionAttributeType;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;

import nz.ac.auckland.abi.dcm4chee.StudyRecord;
import nz.ac.auckland.abi.entities.FEMModel;
import nz.ac.auckland.abi.entities.MRStudyInstances;
import nz.ac.auckland.abi.entities.PACSStudy;
import nz.ac.auckland.abi.entities.USStudyInstances;
import nz.ac.auckland.abi.entities.Patient;
import nz.ac.auckland.abi.entities.PatientSource;
import nz.ac.auckland.abi.icmaconfiguration.ResourceConfigurationManager;

/**
 * Session Bean implementation class ICMAPatientRecord This ensures that all
 * transaction managed operations are performed through it methods. As a
 * consequence, all methods are expected to complete within the transaction
 * timeout period
 */
@Stateless
@LocalBean
public class ICMAPatientRecord {

	@PersistenceContext(unitName = "ICMADB")
	private EntityManager entityManager;

	@EJB
	private ResourceConfigurationManager resourceManger;

	@EJB
	PatientsBeanRemote patientsBean;
	@EJB
	StudiesBeanRemote studiesBean;
	@EJB
	USInstancesBeanRemote usInstanceBean;
	@EJB
	MRInstancesBeanRemote mrInstanceBean;
	@EJB
	FEMModelsBeanRemote femmodelsBean;
	@EJB
	PatientSourceMapManagerRemote patientMapBean;

	Logger log = null;

	public ICMAPatientRecord() {
		log = Logger.getLogger(this.getClass().getSimpleName());
	}

	@TransactionAttribute(TransactionAttributeType.REQUIRES_NEW)
	public void batchAdd(Patient entity, Vector<PACSStudy> studyInstances, Vector<USStudyInstances> usInstances, Vector<MRStudyInstances> mrInstances, Vector<FEMModel> femInstances)
			throws Exception {
		// Update patient source, if patient is non null
		PatientSource psrc = null;
		String status = null;
		try {
			if (entity != null) {
				String patientID = entity.getId();
				psrc = new PatientSource();
				psrc.setId(patientID);
				psrc = patientMapBean.findMap(psrc);
				status = psrc.getStatus();
				psrc.setStatus("operating");
				psrc = patientMapBean.updateMap(psrc);
				patientsBean.addPatient(entity);
				status = "In Sync";
			}
			if (studyInstances != null) {
				for (PACSStudy study : studyInstances) {
					studiesBean.addStudy(study);
				}
			}
			// Add the instances
			if (usInstances != null) {
				for (USStudyInstances inst : usInstances) {
					log.log(Level.FINE, "Adding instance " + inst.getInstanceID());
					usInstanceBean.addInstance(inst);
				}
			}
			if (mrInstances != null) {
				for (MRStudyInstances inst : mrInstances) {
					log.log(Level.FINE, "Adding instance " + inst.getInstanceID());
					mrInstanceBean.addInstance(inst);
				}
			}
			if (femInstances != null) {
				for (FEMModel model : femInstances) {
					log.log(Level.FINE, "Adding instance " + model);
					// Since batchAdd is called when patient record is being
					// synced from PACS
					// DICOM file is null
					femmodelsBean.addFEMModel(model, null, this.getClass().getSimpleName());
				}
			}
		} catch (Exception exx) {
			throw exx;
		}
		if (psrc != null) {// Update the status
			psrc.setStatus(status);
			patientMapBean.updateMap(psrc);
		}
	}

	@TransactionAttribute(TransactionAttributeType.REQUIRES_NEW)
	public boolean removePatient(String patientID) throws Exception {
		Patient myPatient = patientsBean.getPatientByID(patientID);
		// Check if the patient has a PACS record
		if (myPatient != null) {
			String status = null;
			// Update patient source
			PatientSource psrc = new PatientSource();
			psrc.setId(patientID);
			psrc = patientMapBean.findMap(psrc);
			status = psrc.getStatus();
			psrc.setStatus("operating");
			psrc = patientMapBean.updateMap(psrc);
			boolean deleteSourceRecord = false;
			if (status.equalsIgnoreCase("ICMA Only")) {
				deleteSourceRecord = true;
			}
			try {
				// Remove patient and related records
				List<PACSStudy> studies = studiesBean.getStudiesForPatient(myPatient);
				for (PACSStudy study : studies) {
					// Remove the CMS records\
					try {
						resourceManger.removeStudy(study.getStudyID());
					} catch (Exception exx) {// Ignore

					}
					List<USStudyInstances> inst = usInstanceBean.getInstancesForStudy(study);
					for (USStudyInstances pacsi : inst) {
						usInstanceBean.removeInstance(pacsi);
					}
					List<MRStudyInstances> mrinst = mrInstanceBean.getInstancesForStudy(study);
					for (MRStudyInstances pacsi : mrinst) {
						mrInstanceBean.removeInstance(pacsi);
					}
					List<FEMModel> models = femmodelsBean.getStudyModels(study);
					for (FEMModel model : models) {
						femmodelsBean.removeFEMModel(model, this.getClass().getSimpleName());
					}
				}

				patientsBean.removePatient(myPatient);
				status = "PACS Only";
			} catch (Exception exx) {
				exx.printStackTrace();
				deleteSourceRecord = false; // Ensures that the record remains
				log.log(Level.INFO, "Exception occured while removing patient record with ID " + patientID);
			}
			{
				if (!deleteSourceRecord) {
					log.log(Level.INFO, "Setting status of patient " + patientID + " to " + status);
					psrc.setStatus(status);
					patientMapBean.updateMap(psrc);
				} else {
					patientMapBean.deleteMap(psrc);
				}
			}
			return true;
		}
		return false;
	}

	public void addPatientStudies(String patientID, Vector<StudyRecord> pstudies, Vector<Vector<USStudyInstances>> usinstances, Vector<Vector<MRStudyInstances>> mrinstances,
			Vector<Vector<FEMModel>> femModels) throws Exception {
		for (int i = 0; i < pstudies.size(); i++) {
			StudyRecord rec = pstudies.elementAt(i);
			PACSStudy newStudy = new PACSStudy(patientID, rec.getStudyInstanceUID(), rec.getStudyDate(), rec.getStudyDescription());
			try {
				Vector<PACSStudy> studyInstances = new Vector<PACSStudy>();
				studyInstances.add(newStudy);
				// Add study related data
				batchAdd(null, studyInstances, usinstances.elementAt(i), mrinstances.elementAt(i), femModels.elementAt(i));
			} catch (Exception ejbEx) {
				log.log(Level.INFO, "Exception occured while adding study record with ID " + rec.getStudyInstanceUID() + "\n" + ejbEx);
				ejbEx.printStackTrace();
			}
		}
	}
}
