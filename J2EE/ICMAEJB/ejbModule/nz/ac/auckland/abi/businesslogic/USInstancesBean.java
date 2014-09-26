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

import javax.ejb.Stateless;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.persistence.Query;

import nz.ac.auckland.abi.entities.PACSStudy;
import nz.ac.auckland.abi.entities.USStudyInstances;

/**
 * Session Bean implementation class InstancesBean
 */
@Stateless
public class USInstancesBean implements USInstancesBeanRemote {

	@PersistenceContext(unitName = "ICMADB")
    private EntityManager entityManager;
	
    /**
     * Default constructor. 
     */
    public USInstancesBean() {
       
    }

    
	public void addInstance(USStudyInstances instance){
		entityManager.persist(instance);
	}
	
	public void removeInstance(USStudyInstances instance){
		entityManager.remove(entityManager.merge(instance)); //To handle detached instances, merge them with em
	}
	
	public List<USStudyInstances> getInstancesForStudy(PACSStudy study){
	   	String q = "SELECT p from " + USStudyInstances.class.getName() + " p where p.studyID = :study_id";
        Query query = entityManager.createQuery(q).setParameter("study_id",study.getStudyID());
        List<USStudyInstances> instances = query.getResultList();
        return instances;
	}
}
