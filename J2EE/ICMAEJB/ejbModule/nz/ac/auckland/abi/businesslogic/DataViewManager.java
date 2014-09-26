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

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.ejb.EJB;
import javax.ejb.LocalBean;
import javax.ejb.Stateless;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.persistence.Query;

import nz.ac.auckland.abi.entities.FEMModel;
import nz.ac.auckland.abi.entities.FEMModelModification;
import nz.ac.auckland.abi.entities.MRStudyInstances;
import nz.ac.auckland.abi.entities.PACSStudy;
import nz.ac.auckland.abi.entities.USStudyInstances;
import nz.ac.auckland.abi.entities.Patient;
import nz.ac.auckland.abi.icmaconfiguration.ResourceConfigurationManager;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;

/**
 * Session Bean implementation class DataViewManager
 */
@Stateless
@LocalBean
public class DataViewManager implements DataViewManagerRemote {

	@PersistenceContext(unitName = "ICMADB")
    private EntityManager entityManager;
	
	@EJB
	private PatientsBeanRemote patientsBean;
	@EJB
	private StudiesBeanRemote studiesBean;
	@EJB
	private USInstancesBeanRemote usInstanceBean;
	@EJB
	private MRInstancesBeanRemote mrInstanceBean;
	@EJB
	private FEMModelsBeanRemote femmodelsBean;
	
	@EJB
	private ResourceConfigurationManager resourceManager;
	
	@EJB
	private ICMADatabaseAdministration admin;
    /**
     * Default constructor. 
     */
    public DataViewManager() {
    
    }

    public JSONObject getPatients(JSONObject query) throws Exception{
    	String start = (String) query.get("start");
    	String count = (String) query.get("count");
    	String sort = (String) query.get("sort");
    	String search = (String) query.get("query");
    	JSONArray records = new JSONArray();
    	StringBuffer filter = new StringBuffer("");
    	if(search!=null){
    		filter.append("	where (p.pat_id like '%"+search+"%'"
					+ " or p.pat_name like '%"+search+"%'"
					+ " or p.pat_birthdate like '%"+search+"%')");
    	}
    	
    	
    	if(sort!=null){
    		String field = sort.substring(sort.indexOf('(')+1,sort.indexOf(')'));
    		String fields[] = field.split(",");
    		filter.append(" order by ");
    		for(int i=0;i<fields.length-1;i++){
    			String key = fields[i];
    			String dbField = "pat_id";
    			if(key.endsWith("id")){
    				dbField = "pat_id";
    			}else if(key.endsWith("patientName")){
    				dbField = "pat_name";
    			}else if(key.endsWith("dateOfBirth")){
    				dbField = "pat_birthdate";
    			}
    			if(dbField!=null){
	    			if(key.startsWith("+")){
	    				filter.append("p."+dbField +" asc, ");
	    			}else{
	    				filter.append("p."+dbField +" desc, ");
	    			}
    			}
    		}
    		{
    			String key = fields[fields.length-1];
    			String dbField = "pat_id";
    			if(key.endsWith("id")){
    				dbField = "pat_id";
    			}else if(key.endsWith("patientName")){
    				dbField = "pat_name";
    			}else if(key.endsWith("dateOfBirth")){
    				dbField = "pat_birthdate";
    			}
    			if(dbField!=null){
	    			if(key.startsWith("+")){
	    				filter.append("p."+dbField +" asc ");
	    			}else{
	    				filter.append("p."+dbField +" desc ");
	    			}
    			}
    		}
    	}
    	
    	if(start!=null && count!=null){
    		filter.append(" limit " + start + ", " + count);
    	}
    	
    	String q = "SELECT * from  Patient p "+filter.toString();
		
		//System.out.println(q);
		
    	Query nQuery = entityManager.createNativeQuery(q, Patient.class);
    	List<Patient> patients = nQuery.getResultList();
        for(Patient pat : patients){
        	JSONObject pr = new JSONObject();
        	pr.put("patientid", pat.getId());
        	pr.put("patientName", pat.getName());
        	pr.put("dateOfBirth", pat.getBirthdate());
        	records.add(pr);
        }
        
        count = ""+patients.size();
        //System.out.println("Num results "+count);
        
    	JSONObject result = new JSONObject();
    	result.put("iTotalRecords", patientsBean.getAllPatientsCount());
    	result.put("start",start);
    	result.put("count",count);
    	result.put("data", records);
    	return result;
    }
    
    
    public String getPatientRecord(String patientID) throws Exception{
    	JSONObject patientRecord = new JSONObject();
    	Patient patient = patientsBean.getPatientByID(patientID);
    	if(patient!=null){
    		//Set the patient to be active
    		patient.setActive(patient.getActive()+1);
    		
    		patientRecord.put("patientid", patient.getId());
    		patientRecord.put("patientName", patient.getName());
    		patientRecord.put("SXSPECKLETRACK", "SXSpeckleTrack");
    		//patientRecord.put("SXSPECKLETRACK", "PollingTest");
    		patientRecord.put("SXMUPDATE", "SXMUpdate");
    		patientRecord.put("SXFIT", "SXModelFit");
    		patientRecord.put("MRIFIT", "SXMRIModelFit");
    		//TODO reflect this data by accessing appropriate data
    		JSONObject rp = new JSONObject();
    		JSONObject metaData = new JSONObject();
    		metaData.put("dateOfBirth", patient.getBirthdate());
    		metaData.put("gender", patient.getSex());
    		metaData.put("contactAddress", "Contact Address Not available");
    		metaData.put("contactPhone", "Contact Phone Not available");
    		
    		JSONParser parser = new JSONParser();
    		List<PACSStudy> studies = studiesBean.getStudiesForPatient(patient);
    		JSONObject usStudiesJSON = new JSONObject();
    		JSONObject mrStudiesJSON = new JSONObject();
    		int usctr = 1;
    		int mrctr = 1;
			SimpleDateFormat dateFormat = new SimpleDateFormat("dd-MM-yyyy kk:mm:ss.SSSS");
			double pSize = 0.0;
			double pWeight = 0.0;
			String referringPhysianName = null;
			java.sql.Timestamp prevDate = null;
    		for(PACSStudy study : studies){
    			List<USStudyInstances> usInstances = usInstanceBean.getInstancesForStudy(study);
    			List<MRStudyInstances> mrInstances = mrInstanceBean.getInstancesForStudy(study);
    			if(usInstances.size()>0){
        			JSONObject studyJSON = new JSONObject();
        			studyJSON.put("studyUID", study.getStudyID());
        			studyJSON.put("name", study.getStudyDescription());
        			studyJSON.put("studyDate", dateFormat.format(study.getStudyDate()));
	    			boolean setValue = false;
	    			if(prevDate==null){
	    				setValue = true;
	    			}
	    			if(prevDate!=null && prevDate.before(study.getStudyDate())){
	    				setValue = true;
	    				prevDate = study.getStudyDate();
	    			}
	    			JSONObject viewJSON = new JSONObject();
	    			for(int i=0;i<usInstances.size();i++){
	    				USStudyInstances instance = usInstances.get(i);
	    				String json = instance.getJSON();
	    				JSONObject sj = (JSONObject) parser.parse(json);
	    				viewJSON.put("view"+(i+1), sj); //This will ensure that height and weight are set if they were not
	    				if(setValue){
	   						pSize = instance.getSize();
	   						pWeight = instance.getWeight();
	   						referringPhysianName = (String)sj.get("ReferringPhysicianName");
	    				}
	    			}
    				studyJSON.put("views", viewJSON);
    				usStudiesJSON.put("study"+usctr++, studyJSON);
    			}
    			if(mrInstances.size()>0){
        			JSONObject studyJSON = new JSONObject();
        			studyJSON.put("studyUID", study.getStudyID());
        			studyJSON.put("name", study.getStudyDescription());
        			studyJSON.put("studyDate", dateFormat.format(study.getStudyDate()));
	    			if(prevDate!=null && prevDate.before(study.getStudyDate())){
	    				prevDate = study.getStudyDate();
	    			}
	    			JSONObject viewJSON = new JSONObject();
	    			for(int i=0;i<mrInstances.size();i++){
	    				MRStudyInstances instance = mrInstances.get(i);
	    				String json = instance.getJSON();
	    				JSONObject sj = (JSONObject) parser.parse(json);
	    				if(referringPhysianName==null)
	    					referringPhysianName = (String)sj.get("ReferringPhysicianName");
	    				viewJSON.put("view"+(i+1), sj); 
	    			}
    				studyJSON.put("views", viewJSON);
    				mrStudiesJSON.put("study"+mrctr++, studyJSON);
    			}
    		}
    		if(referringPhysianName!=null)
    			rp.put("name", referringPhysianName);
    		else
    			rp.put("name", " Not available");
    		rp.put("email", "Not available");
    		rp.put("phone", "Not available");
    		patientRecord.put("ReferringPhysician", rp);

    		
    		//Set the patient height and size from latest instance record
    		metaData.put("size",pSize+"");
    		metaData.put("weight",pWeight+"");
    		if(usctr>1){
    			patientRecord.put("usstudies", usStudiesJSON);
    		}
    		if(mrctr>1){
    			patientRecord.put("mrstudies", mrStudiesJSON);
    		}

    		JSONObject modelsJSON = new JSONObject();
    		usctr = 1;
    		List<FEMModel> models = femmodelsBean.getPatientModels(patient);
    		//System.out.println("Number of models "+models.size());
    		for(FEMModel model : models){
    			try{
    				boolean purged = false;
	    			String json = model.getJSON();
	    			String author = model.getAuthor();
	    			JSONObject modelObj = (JSONObject)parser.parse(json);
	    			JSONObject lcObj = (JSONObject) modelObj.get("lifeCycle");
	    			lcObj.put("author", author); //Done here as ICMA creation process used default
	    			
	    			//Check if this model is yet to be saved and is dangling
	    			int status = femmodelsBean.getModelStatus(model);
	    			if(status==FEMModelModification.create){
	    				String workflowId = resourceManager.findModelWsWorkFlow(model.getId());
	    				if(workflowId!=null){
	    					modelObj.put("workflowId", workflowId);
	    				}else{
	    					//admin.removeModel(model.getId(), "purge");
	    					femmodelsBean.removeModel(model.getId(), "purge");
	    					purged = true;
	    				}
	    			}
	    			if(!purged){
		    			modelObj.put("startTime", 0);
		    			modelObj.put("modelid",model.getId());
		    			modelObj.put("annotation",model.getAnnotation());
		    			modelsJSON.put("model"+usctr++, modelObj);
	    			}else{
	    				Logger log = Logger.getLogger(this.getClass().getSimpleName());
	    				log.log(Level.INFO,"Model "+model.getModelName()+" was purged");
	    			}
    			}catch(Exception exx){
    				Logger log = Logger.getLogger(this.getClass().getSimpleName());
    				log.log(Level.SEVERE,"Exception "+exx+" occured while contructing study details of Patient "+patientID);
    			}
    		}
    		if(usctr>1){
    			patientRecord.put("Models", modelsJSON);
    		}
    		patientRecord.put("patientMetaData", metaData);
    	}
    	
    	return patientRecord.toJSONString();
    }
    
    //If the patient has model records, a list of modelID:modelName is ouput
    public List<String> getPatientModels(String patientID) throws Exception{
    	Patient pat = new Patient(patientID, "", "", "");
    	
    	List<FEMModel> models = femmodelsBean.getPatientModels(pat);
    	if(models.size()>0){
    		List<String> output = new ArrayList<String>();
	    	for(FEMModel model : models){
	    		output.add(model.getId()+":"+model.getModelName());
	    	}
	    	return output;
    	}else{
    		return null;
    	}
    }
    
    //Clients should invoke this method to ensure that the patient is refershed continually
    public void   activatePatientRecord(String patientID) throws Exception{
    	Patient patient = patientsBean.getPatientByID(patientID);
    	patient.setActive(patient.getActive()+1);
    }
}
