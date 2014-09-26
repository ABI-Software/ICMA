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
import java.util.logging.Logger;

import javax.ejb.Stateless;
import javax.jcr.nodetype.ConstraintViolationException;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.persistence.Query;

import nz.ac.auckland.abi.dcm4chee.DCMAccessManager;
import nz.ac.auckland.abi.dcm4chee.StudyRecord;
import nz.ac.auckland.abi.entities.PACSStudy;
import nz.ac.auckland.abi.entities.PACSStudyPK;
import nz.ac.auckland.abi.entities.Patient;

/**
 * Session Bean implementation class StudiesBean
 */
@Stateless
public class StudiesBean implements StudiesBeanRemote {

	@PersistenceContext(unitName = "ICMADB")
    private EntityManager entityManager;
	
    /**
     * Default constructor. 
     */
    public StudiesBean() {
    	
    }

	public void addStudy(PACSStudy study){
		PACSStudyPK key = new PACSStudyPK();
		key.setPatientID(study.getPatientID());
		key.setStudyID(study.getStudyID());
		PACSStudy dbo = entityManager.find(PACSStudy.class, key);
		if(dbo!=null){
			if (!dbo.getStudyDescription().equalsIgnoreCase(study.getStudyDescription())){
				study.setStudyDescription("MIXED");
			}
			entityManager.merge(study);
		}else{
			entityManager.persist(study);
		}
	}
	
	public void removeStudy(PACSStudy study){
		entityManager.remove(entityManager.merge(study));
	}
	
	public PACSStudy getStudy(String studyID) throws Exception{
		String q = "SELECT p from " + PACSStudy.class.getName() + " p where p.studyID = :studyID";
        Query query = entityManager.createQuery(q).setParameter("studyID", studyID);
        List<PACSStudy> studies = query.getResultList();
        if(studies.size()==1){//Check the study size, if it is zero the records from PACS may not be loaded
        	return studies.get(0);
        }else if(studies.size()==0){
        	return null;
        }else{
        	throw new ConstraintViolationException("Multiple records for study "+studyID+" found!!!");
        }
	}
	
	public List<PACSStudy> getStudiesForPatient(Patient patient){
	   	String q = "SELECT p from " + PACSStudy.class.getName() + " p where p.patientID = :patientID";
        Query query = entityManager.createQuery(q).setParameter("patientID", patient.getId());
        List<PACSStudy> studies = query.getResultList();
        if(studies.size()==0){//Check the study size, if it is zero the records from PACS may not be loaded
        	try{
        		Vector<StudyRecord> pstudies = DCMAccessManager.getPatientStudies(patient.getId(), -1);
        		for(StudyRecord rec: pstudies){
        			PACSStudy newStudy = new PACSStudy(patient.getId(),rec.getStudyInstanceUID(),rec.getStudyDate(), rec.getStudyDescription());
        			addStudy(newStudy);
        		}
        		
        	}catch(Exception pex){
        		Logger log = Logger.getLogger(this.getClass().getSimpleName());
        		log.info("Error occured when accessing studies from pacs for "+patient+ "\t"+pex);
        	}
        	query = entityManager.createQuery(q);
            studies = query.getResultList();
        }
        return studies;
	}

    
}
