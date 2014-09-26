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

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.logging.Logger;

import javax.ejb.LocalBean;
import javax.ejb.Stateless;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.persistence.Query;

import nz.ac.auckland.abi.entities.ModelView;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;

/**
 * Session Bean implementation class ModelViewBean
 */
@Stateless
@LocalBean
public class ModelViewBean implements ModelViewBeanRemote {

	@PersistenceContext(unitName = "ICMADB")
	private EntityManager entityManager;

	Logger log = null;

	public ModelViewBean() {
		log = Logger.getLogger(this.getClass().getSimpleName());
	}

	public List<JSONObject> getListOfModels(String queryString, int start, int limit)
			throws Exception {
		String q = "SELECT * FROM ModelView M";
		if (queryString != null)
			q = q + " " + queryString;
		//log.log(Level.INFO, q);

		Query query = entityManager.createNativeQuery(q,ModelView.class);
		List<ModelView> models = query.getResultList();
		//log.log(Level.INFO,"Number of models "+models.size());
		//log.log(Level.INFO,"Start "+start+" Limit "+limit);
				// Process them into JSONObjects
		HashMap<String, JSONObject> patients = new HashMap<String, JSONObject>();
		JSONParser parser = new JSONParser();
		int mCTR = 0;
		int end = limit+start;
		for (ModelView view : models) {
			//log.log(Level.INFO,mCTR+"\t"+view.getPatientId()+"\t"+view.getModelName());
			if(mCTR<start){
				
				patients.put(view.getPatientId(), null);
				mCTR = patients.size();
				continue;
			}
			if(mCTR>end)
				break;
			mCTR = patients.size();
			try {
				if(!view.getModelStatus().equalsIgnoreCase("SUBMITTED")){ //Skip models that were just submitted and do not contain annotation data
					JSONObject patient = patients.get(view.getPatientId());
					JSONArray modelArray = (JSONArray) patient.get("models");
					JSONObject viewModel = new JSONObject();
					viewModel.put("pk", "" + view.getPk());
					viewModel.put("studydate", view.getStudyDate());
					viewModel.put("name", view.getModelName());
					viewModel.put("status", view.getModelStatus());
					viewModel.put("annotation",
							parser.parse(view.getModelAnnotation()));
					modelArray.add(viewModel);
				}
			} catch (NullPointerException ex) {
				JSONObject patient = new JSONObject();
				patient.put("id", view.getPatientId());
				patient.put("name", view.getPatientName());
				patient.put("gender", view.getPatientGender());
				patient.put("birthdate", view.getPatientBirthDate());
				JSONArray modelArray = new JSONArray();
				JSONObject viewModel = new JSONObject();
				viewModel.put("pk", "" + view.getPk());
				viewModel.put("studydate", view.getStudyDate());
				viewModel.put("name", view.getModelName());
				viewModel.put("status", view.getModelStatus());
				viewModel.put("annotation",
						parser.parse(view.getModelAnnotation()));
				modelArray.add(viewModel);
				patient.put("models", modelArray);
				patients.put(view.getPatientId(), patient);
			}
		}
		List<JSONObject> result = new ArrayList<JSONObject>(patients.values());
		//Remove the null values
		result.removeAll(Collections.singleton(null));
		//log.log(Level.FINE,"Number of results "+result.size());
		return result;
	}
	
	public List<JSONObject> getListOfModelsWithMetaData(String queryString,int start, int limit) throws Exception{
		String q = "SELECT * FROM ModelView M";
		if (queryString != null)
			q = q + " " + queryString;
		//log.log(Level.FINE, q);

		Query query = entityManager.createNativeQuery(q,ModelView.class);
		List<ModelView> models = query.getResultList();

		// Process them into JSONObjects
		HashMap<String, JSONObject> patients = new HashMap<String, JSONObject>();
		JSONParser parser = new JSONParser();
		int mCTR = 0;
		int end = limit+start;
		for (ModelView view : models) {
			if(mCTR<start){
				patients.put(view.getPatientId(), null);
				mCTR = patients.size();
				continue;
			}
			if(mCTR>end)
				break;
			mCTR = patients.size();
			try {
				JSONObject patient = patients.get(view.getPatientId());
				JSONArray modelArray = (JSONArray) patient.get("models");
				JSONObject viewModel = new JSONObject();
				viewModel.put("pk", "" + view.getPk());
				viewModel.put("studydate", view.getStudyDate());
				viewModel.put("id", view.getModelID());
				viewModel.put("name", view.getModelName());
				viewModel.put("status", view.getModelStatus());
				viewModel.put("annotation",
						parser.parse(view.getModelAnnotation()));
				viewModel.put("metadata", view.getMetaData());
				modelArray.add(viewModel);
			} catch (NullPointerException ex) {
				JSONObject patient = new JSONObject();
				patient.put("id", view.getPatientId());
				patient.put("name", view.getPatientName());
				patient.put("gender", view.getPatientGender());
				patient.put("birthdate", view.getPatientBirthDate());
				JSONArray modelArray = new JSONArray();
				JSONObject viewModel = new JSONObject();
				viewModel.put("pk", "" + view.getPk());
				viewModel.put("studydate", view.getStudyDate());
				viewModel.put("id", view.getModelID());
				viewModel.put("name", view.getModelName());
				viewModel.put("status", view.getModelStatus());
				viewModel.put("annotation",
						parser.parse(view.getModelAnnotation()));
				viewModel.put("metadata", view.getMetaData());
				modelArray.add(viewModel);
				patient.put("models", modelArray);
				patients.put(view.getPatientId(), patient);
			}
		}
		List<JSONObject> result = new ArrayList<JSONObject>(patients.values());
		//Remove the null values
		result.removeAll(Collections.singleton(null));
		return result;
	}

	public long getTotalNumberOfPatients() {
		String q = "SELECT COUNT(DISTINCT patientId) FROM " + ModelView.class.getName() + " M";
		Query query = entityManager.createQuery(q);
		List<Long> models = query.getResultList();
		return models.get(0).longValue();
	}

}
