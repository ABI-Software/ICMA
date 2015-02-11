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
package nz.ac.auckland.abi.webapp.consultant;

import java.util.List;

import javax.ejb.EJB;
import javax.management.openmbean.KeyAlreadyExistsException;
import javax.ws.rs.Consumes;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

import nz.ac.auckland.abi.businesslogic.FEMModelsBeanRemote;
import nz.ac.auckland.abi.businesslogic.ICMADatabaseAdministrationRemote;
import nz.ac.auckland.abi.icmaconfiguration.BatchAdditionsManager;
import nz.ac.auckland.abi.json.entities.AssociateRequest;
import nz.ac.auckland.abi.json.entities.BatchAssociateRequest;
import nz.ac.auckland.abi.json.entities.RemoveModelRequest;
import nz.ac.auckland.abi.json.entities.UpdateModelRequest;
import nz.ac.auckland.abi.workflow.WSWorkflowManagerRemote;

import org.json.simple.JSONObject;

//Obtains user details from http session

@Path("/admin")
public class PatientManager {

	@EJB
	ICMADatabaseAdministrationRemote admin;
	
	
	@EJB
	BatchAdditionsManager batchProcessor;
	
	@EJB
	FEMModelsBeanRemote model;
	
	@EJB
	WSWorkflowManagerRemote workflowManager;
	
	@POST @Path("associate")
	@Consumes(MediaType.APPLICATION_JSON)
	@Produces("application/json")
	public Response associate(AssociateRequest req){
		boolean result = true;
		JSONObject response = new JSONObject();
		try{
			if(req.action.equalsIgnoreCase("sync")){
				//add the patient
				//admin.addPatientFromPACS(req.getPatientid());
				batchProcessor.addPatientFromPacs(req.getPatientid(), "ICMADatabaseAdministration",-1);
				response.put("msg", "submitted");
			}else if(req.action.equalsIgnoreCase("delete")){
				//Remove the patient
				result = admin.removePatient(req.getPatientid());
				if(result)
					response.put("msg", "success");
				else
					response.put("msg", "failed");
			}else if(req.action.equalsIgnoreCase("resync")){
				//add the patient
				admin.refreshPatientFromPACS(req.getPatientid());
				response.put("msg", "success");
			}
		}catch(Exception exx){
			return Response.status(Response.Status.INTERNAL_SERVER_ERROR).entity(exx.toString()).build();
		}

		
		String json = response.toJSONString(); //Should be a parsable json string
		
		//return Response.serverError().entity("UUID cannot be blank").build();
		//return Response.status(Response.Status.NOT_FOUND).entity("Entity not found for UUID: " + uuid).build();
		return Response.ok(json, MediaType.APPLICATION_JSON).build();
	}
	
	@POST @Path("batchAssociate")
	@Consumes(MediaType.APPLICATION_JSON)
	@Produces("application/json")
	public Response batchAssociate(BatchAssociateRequest req){
		List<String> patients = req.getPatientid();
		StringBuffer buf = new StringBuffer();
		if(req.action.equalsIgnoreCase("sync")){
			for(String patient: patients){
				try{
					buf.append(patient);
					//add the patient
					//admin.addPatientFromPACS(patient);
					batchProcessor.addPatientFromPacs(patient, "ICMADatabaseAdministration",-1);
					buf.append("#success#");
				}catch(KeyAlreadyExistsException kex){
					buf.append("#exists#");
				}catch(Exception exx){
					buf.append("#failed#");
				}
			}
		}else{
			for(String patient: patients){
				try{
					buf.append(patient);
					admin.removePatient(patient);
					buf.append("#success#");
				}catch(Exception exx){
					buf.append("#failed#");
				}
			}
		}
		
		JSONObject response = new JSONObject();
		response.put("msg", buf.toString()); 
		String json = response.toJSONString(); //Should be a parsable json string

		return Response.ok(json, MediaType.APPLICATION_JSON).build();
	}
	
	@POST @Path("removeModel")
	@Consumes(MediaType.APPLICATION_JSON)
	@Produces("application/json")
	public Response removeModel(RemoveModelRequest req){
		JSONObject response = new JSONObject();

		try{
			if(model.removeModel(req.getModelid(), req.getActor())){
				response.put("modelid",req.getModelid());
				response.put("msg", "success"); 				
			}else{
				return Response.status(Response.Status.BAD_REQUEST).entity("Model with id "+req.getModelid()+" does not exist").build();
			}
		}catch(Exception exx){
			return Response.status(Response.Status.INTERNAL_SERVER_ERROR).entity(exx.toString()).build();
		}

		String json = response.toJSONString(); //Should be a parsable json string
		return Response.ok(json, MediaType.APPLICATION_JSON).build();
	}
	
	@POST @Path("updateModel")
	@Consumes(MediaType.APPLICATION_JSON)
	@Produces("application/json")
	public Response updateModel(AssociateRequest req){
		
		JSONObject response = new JSONObject();

/*		try{
			if(req.modelid==null){
				return Response.status(Response.Status.BAD_REQUEST).entity("Model id is null").build();
			}
			if(req.userid==null){
				return Response.status(Response.Status.BAD_REQUEST).entity("User id is null").build();
			}
			if(req.getAction().equalsIgnoreCase("update")){
				if(admin.updateModel(req.modelid, req.annotation, req.status, req.userid)){
					response.put("modelid",req.getModelid());
					response.put("msg", "success"); 				
				}else{
					return Response.status(Response.Status.BAD_REQUEST).entity("Model with id "+req.getModelid()+" does not exist").build();
				}
			}
			if(req.getAction().startsWith("create")){
				String temp = req.getAction();
				String workFlowID = temp.substring("create".length());
				if(admin.updateModel(req.modelid, req.annotation, req.status, req.userid)){
					response.put("modelid",req.getModelid());
					response.put("msg", "success"); 
					workflowManager.saveModel(workFlowID);
				}else{
					return Response.status(Response.Status.BAD_REQUEST).entity("Model with id "+req.getModelid()+" does not exist").build();
				}
			}
			//also handle delete
			if(req.getAction().startsWith("delete")){
				try{
					if(admin.removeModel(req.getModelid(), "this")){
						response.put("modelid",req.getModelid());
						response.put("msg", "success"); 				
					}else{
						return Response.status(Response.Status.BAD_REQUEST).entity("Model with id "+req.getModelid()+" does not exist").build();
					}
				}catch(Exception exx){
					return Response.status(Response.Status.INTERNAL_SERVER_ERROR).entity(exx.toString()).build();
				}
			}
		}catch(Exception exx){
			return Response.status(Response.Status.INTERNAL_SERVER_ERROR).entity(exx.toString()).build();
		}*/

		
		String json = response.toJSONString(); //Should be a parsable json string
		return Response.ok(json, MediaType.APPLICATION_JSON).build();
	}
	
	@POST @Path("exportModel")
	@Consumes(MediaType.APPLICATION_JSON)
	@Produces("application/json")
	public Response exportModel(UpdateModelRequest req){
		JSONObject response = new JSONObject();

		try{
			if(req.modelid==null){
				return Response.status(Response.Status.BAD_REQUEST).entity("Model id is null").build();
			}
			response.put("modelid",req.getModelid());
			response.put("msg", admin.exportModel(req.getModelid())); 				
		}catch(Exception exx){
			return Response.status(Response.Status.INTERNAL_SERVER_ERROR).entity(exx.toString()).build();
		}

		String json = response.toJSONString(); //Should be a parsable json string
		return Response.ok(json, MediaType.APPLICATION_JSON).build();
	}
	
/*	@POST @Path("batchCompute")
	@Consumes(MediaType.APPLICATION_JSON)
	@Produces("application/json")
	public Response batchCompute(BatchAssociateRequest req){
		List<String> patients = req.getPatientid();
		StringBuffer buf = new StringBuffer();
		if(req.action.equalsIgnoreCase("computedisplacementfield")){
			for(String patient: patients){
				try{
					buf.append(patient);
					//add the patient
					//admin.addPatientFromPACS(patient);
					
					buf.append("#success#");
				}catch(KeyAlreadyExistsException kex){
					buf.append("#exists#");
				}catch(Exception exx){
					buf.append("#failed#");
				}
			}
		}
		JSONObject response = new JSONObject();
		response.put("msg", buf.toString()); 
		String json = response.toJSONString(); //Should be a parsable json string

		return Response.ok(json, MediaType.APPLICATION_JSON).build();
	}*/
	
}
