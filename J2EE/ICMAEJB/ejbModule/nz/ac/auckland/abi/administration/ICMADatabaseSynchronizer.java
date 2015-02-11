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
package nz.ac.auckland.abi.administration;

import java.io.File;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.List;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.annotation.PostConstruct;
import javax.annotation.Resource;
import javax.ejb.AccessTimeout;
import javax.ejb.Asynchronous;
import javax.ejb.EJB;
import javax.ejb.Schedule;
import javax.ejb.SessionContext;
import javax.ejb.Singleton;
import javax.ejb.Startup;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.persistence.Query;

import nz.ac.auckland.abi.businesslogic.DataViewManagerRemote;
import nz.ac.auckland.abi.businesslogic.FEMModelsBeanRemote;
import nz.ac.auckland.abi.businesslogic.ICMADatabaseAdministration;
import nz.ac.auckland.abi.businesslogic.ICMADatabaseAdministrationRemote;
import nz.ac.auckland.abi.businesslogic.PatientSourceMapManagerRemote;
import nz.ac.auckland.abi.businesslogic.PatientsBeanRemote;
import nz.ac.auckland.abi.dcm4chee.DCMAccessManager;
import nz.ac.auckland.abi.dcm4chee.PatientRecord;
import nz.ac.auckland.abi.entities.FEMModelModification;
import nz.ac.auckland.abi.entities.Patient;
import nz.ac.auckland.abi.entities.PatientSource;
import nz.ac.auckland.abi.icmaconfiguration.ResourceConfigurationManager;

import org.json.simple.JSONObject;

@Singleton
@Startup
public class ICMADatabaseSynchronizer {

	@Resource(name = "java:global/ICMADISKSCRATCHSPACE")
	private String diskScratchSpace;

	@Resource(name = "java:global/ICMAALLOWLOCALDICOMLOADING")
	private boolean allowLocalLoad;
	
	@PersistenceContext(unitName = "ICMADB")
	private EntityManager entityManager;

	@Resource
    	private SessionContext ctx;
	
	private boolean initializingICMA = true;
	
	private boolean synchronizingWithPACS = false;

	@EJB
	private ICMADatabaseAdministration admin;
	
	@EJB
	private DataViewManagerRemote dataViewManager;
	
	@EJB
	private PatientsBeanRemote patientsBean;
	
	@EJB
	private PatientSourceMapManagerRemote mapManager;
	
	@EJB
	private ResourceConfigurationManager rManager;
	
	@EJB
	private FEMModelsBeanRemote modelBean;
	
	Logger log = null;
	
	/*
	 * Initialize the ICMA DB for the application instance Since the singleton
	 * class is constructed once for the app, this method is invoked only once
	 * Each user session should ensure the creation of an instance of this class
	 */
	@PostConstruct
	public void initICMADB() throws Exception {
		log = Logger.getLogger(this.getClass().getSimpleName());
		
		// Ensure that the icma db is up to date with the PACS data
		// Ensure patient id's are consistent
		if(initializingICMA){
			try {
				//updatePatientSourceMap(-1);
				//purge any dangling fem model records i.e status= create
				//String q = "SELECT fem_model_pk FROM FEMModelModification p WHERE p.Action = "+FEMModelModification.submit;
				String q = "SELECT distinct m.sopid FROM FEMModelModification p, FEMModel m WHERE m.pk=p.fem_model_pk AND m.model_status = 'SUBMITTED' AND p.Action = "+FEMModelModification.create;
				Query nQuery = entityManager.createNativeQuery(q);
				List<String> pks = nQuery.getResultList();
				for(String modelID : pks){
					//admin.removeModel(modelID, "purge");
					//Check if the 
					modelBean.removeModel(modelID, "purge");
				}
				if(pks.size()>0)
					log.log(Level.INFO, "Purged "+pks.size()+" unsaved models");
				initializingICMA = false;
			} catch (Exception exx) {
				exx.printStackTrace();
				log.log(Level.SEVERE,exx.toString());
			}

		}
	}


	// Commits and cache update should be performed using timers/schedules
	// As PreDestroy may not be invoked
	// The following method will be invoked every five minutes in a hour
	@AccessTimeout(-1)
	@Schedule(minute = "*/5", hour = "*", persistent = false)
	//@Schedule(hour = "*/12", persistent = false)
	public void synchronizeWithPACS() {
		int lookBack = 5;
		// Deleted studies (in PACS) are removed, while new studies are added
		if (!initializingICMA&&!synchronizingWithPACS) {
			synchronizingWithPACS = true;
			log.log(Level.INFO,"Synchronization with PACS called");
			List<Patient> activePatients = patientsBean.getActivePatients();
			if(activePatients!=null){
				for(Patient patient : activePatients){
					try{
						ctx.getBusinessObject(ICMADatabaseAdministrationRemote.class).synchronizePatientStudies(patient.getId(), lookBack);
					}catch(Exception exx){
						log.log(Level.SEVERE,"Exception "+exx+" occured, while synchronizing "+patient.getId()+" with PACS");
					}
				}
			}
			synchronizingWithPACS = false;
		}
	}
	
	//Removes patients from active status
	@AccessTimeout(-1)
	//@Schedule(minute = "*/30", hour = "*", persistent = false)
	public void updatePatientStatus(){
		if (!initializingICMA) {
			log.log(Level.INFO,"Updating patient active status");
			List<Patient> activePatients = patientsBean.getActivePatients();
			if(activePatients!=null){
				for(Patient patient : activePatients){
					patient.setActive(0);
				}
			}
		}
	}
	
	//Updates patient map, essentially check if new patients were added to the pacs
	@AccessTimeout(-1)
	@Schedule(minute = "*/30", hour = "*", persistent = false)
	public void updatePatientMap(){
		if (!initializingICMA) {
			log.log(Level.INFO,"Updating patient maps");
			updatePatientSourceMap(-1);
		}
	}

	
	@AccessTimeout(-1)
	@Schedule(minute = "*/30", hour = "*", persistent = false)
	public void purgeUnsavedWorkflows(){
		rManager.purgeUnsavedWSWorkflowModels(admin);
	}
	
	@Asynchronous
	private void updatePatientSourceMap(int lookBack){
		String[] status = {"In Sync","PACS Only","ICMA Only"};
		//Delete the existing map and create afresh
		mapManager.purgeMap();
		Hashtable<String, PatientSource> pacstable = new Hashtable<String, PatientSource>();
		try{
			Vector<PatientRecord> pacsPatients = DCMAccessManager.getPatients(lookBack);
			for (PatientRecord pat : pacsPatients) {
				try{
					//Just to ensure that the patient has some studies
					//PatientRecord prec = DCMAccessManager.getPatient(pat.getPatientID());
					PatientSource ps = new PatientSource(pat.getPatientID(),pat.getPatientName(),status[1]);
					pacstable.put(pat.getPatientID(), ps);
				}catch(Exception exx){
					
				}
			}
			List<Patient> icmaPatients = patientsBean.getAllPatients(null);
			for (Patient pat : icmaPatients) {
				if(pacstable.containsKey(pat.getId())){
					pacstable.get(pat.getId()).setStatus(status[0]);
				}else{
					PatientSource ps = new PatientSource(pat.getId(),pat.getName(),status[2]);
					pacstable.put(pat.getId(), ps);
				}
			}
			Enumeration<PatientSource> pats = pacstable.elements();
			while(pats.hasMoreElements()){
				PatientSource src = pats.nextElement();
				entityManager.persist(src);
				entityManager.flush();
				/*if(mapManager.findMap(src)==null){
					try{
						entityManager.persist(src);
					}catch(Exception exx){
						
					}
				}else{
					PatientSource srx = mapManager.findMap(src);
					srx.setStatus(src.getStatus());
					mapManager.updateMap(srx);
				}*/
			}
		}catch(Exception exx){
			
		}

	}
	
}
