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

import javax.ejb.LocalBean;
import javax.ejb.Stateless;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.persistence.Query;

import nz.ac.auckland.abi.entities.PatientSource;

/**
 * Session Bean implementation class PatientSourceMapManager
 */
@Stateless
@LocalBean
public class PatientSourceMapManager implements PatientSourceMapManagerRemote {

	@PersistenceContext(unitName = "ICMADB")
    private EntityManager entityManager;

    
	public void addMap(PatientSource map) throws Exception{
		entityManager.persist(map);
	}
	
	public PatientSource findMap(PatientSource map) throws Exception{
		String q = "SELECT p from " + PatientSource.class.getName() + " p where p.id = :patid";
		Query query = entityManager.createQuery(q).setParameter("patid", map.getId());
		List<PatientSource> patients = query.getResultList();
        if(patients.size()>0){
        	return patients.get(0);
        }
        return null;
	}
	
	public PatientSource updateMap(PatientSource map) throws Exception{
		return entityManager.merge(map);
	}
	
	public List<PatientSource> getAllPatients(String filter) throws Exception{
		//Using native query as HQL does not support limit
		String q = "SELECT * from  PatientSource p";
		if(filter!=null)
			q = q+filter;
		//System.out.println(q);
        Query query = entityManager.createNativeQuery(q, PatientSource.class);
        List<PatientSource> patients = query.getResultList();
        return patients;
		
	}

	public int getMapCount() throws Exception{
		String q = "SELECT count(*) from  PatientSource p";
		Query query = entityManager.createNativeQuery(q);
        List<java.math.BigInteger> patients = query.getResultList();
        return patients.get(0).intValue();
	}
	
	public void deleteMap(PatientSource map) throws Exception{
		PatientSource merged = null;
		try{
			merged = entityManager.merge(map);
		}catch(Exception exx){
			
		}
		entityManager.remove(merged);
	}
	
	public void purgeMap(){
		String q = "DELETE FROM PatientSource";
		entityManager.createQuery(q).executeUpdate();
		entityManager.flush();
	}
	
}
