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

import javax.ejb.EJB;
import javax.ws.rs.Consumes;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

import nz.ac.auckland.abi.administration.ICMAAccessManager;
import nz.ac.auckland.abi.json.entities.ChangePasswordRequest;
import nz.ac.auckland.abi.json.entities.UserAssociateRequest;
import nz.ac.auckland.abi.json.entities.UserLevelRequest;

import org.json.simple.JSONObject;

//Obtains user details from http session

@Path("/Admin")
public class UserManager {

	@EJB
	ICMAAccessManager admin;
	
	
	@POST @Path("addAdmin")
	@Consumes(MediaType.APPLICATION_JSON)
	@Produces("application/json")
	public Response addAdmin(UserLevelRequest req){
		JSONObject response = new JSONObject();
		try{
			String username = req.getUserName();
			admin.assignAdmin(username);
			response.put("msg", "success");
		}catch(Exception exx){
			return Response.status(Response.Status.BAD_REQUEST).entity(exx.toString()).build();
		}
		String json = response.toJSONString(); //Should be a parsable json string
		
		//return Response.serverError().entity("UUID cannot be blank").build();
		//return Response.status(Response.Status.NOT_FOUND).entity("Entity not found for UUID: " + uuid).build();
		return Response.ok(json, MediaType.APPLICATION_JSON).build();
	}
	
	@POST @Path("removeAdmin")
	@Consumes(MediaType.APPLICATION_JSON)
	@Produces("application/json")
	public Response removeAdmin(UserLevelRequest req){
		JSONObject response = new JSONObject();
		try{
			String username = req.getUserName();
			admin.removeAdmin(username);
			response.put("msg", "success");
		}catch(Exception exx){
			return Response.status(Response.Status.BAD_REQUEST).entity(exx.toString()).build();
		}
		String json = response.toJSONString(); //Should be a parsable json string
		
		//return Response.serverError().entity("UUID cannot be blank").build();
		//return Response.status(Response.Status.NOT_FOUND).entity("Entity not found for UUID: " + uuid).build();
		return Response.ok(json, MediaType.APPLICATION_JSON).build();
	}
	
	@POST @Path("removeUser")
	@Consumes(MediaType.APPLICATION_JSON)
	@Produces("application/json")
	public Response removeUser(UserLevelRequest req){
		JSONObject response = new JSONObject();
		try{
			String username = req.getUserName();
			admin.removeUser(username);
			response.put("msg", "success");
		}catch(Exception exx){
			return Response.status(Response.Status.BAD_REQUEST).entity(exx.toString()).build();
		}
		String json = response.toJSONString(); //Should be a parsable json string
		
		//return Response.serverError().entity("UUID cannot be blank").build();
		//return Response.status(Response.Status.NOT_FOUND).entity("Entity not found for UUID: " + uuid).build();
		return Response.ok(json, MediaType.APPLICATION_JSON).build();
	}
	
	
	@POST @Path("addUser")
	@Consumes(MediaType.APPLICATION_JSON)
	@Produces("application/json")
	public Response addUser(UserAssociateRequest req){
		JSONObject response = new JSONObject();
		try{
			admin.addNewUser(req.getUsername(), req.getPassword());
			response.put("msg", "success");
		}catch(Exception exx){
			return Response.status(Response.Status.BAD_REQUEST).entity(exx.toString()).build();
		}
		String json = response.toJSONString(); //Should be a parsable json string
		
		//return Response.serverError().entity("UUID cannot be blank").build();
		//return Response.status(Response.Status.NOT_FOUND).entity("Entity not found for UUID: " + uuid).build();
		return Response.ok(json, MediaType.APPLICATION_JSON).build();
	}
	
	
	
	@POST @Path("changePassword")
	@Consumes(MediaType.APPLICATION_JSON)
	@Produces("application/json")
	public Response changePassword(ChangePasswordRequest req){
		JSONObject response = new JSONObject();
		try{
			admin.changeUserPassword(req.getUsername(), req.getOldpassword(), req.getNewpassword());
			response.put("msg", "success");
		}catch(Exception exx){
			return Response.status(Response.Status.BAD_REQUEST).entity(exx.toString()).build();
		}
		String json = response.toJSONString(); //Should be a parsable json string
		
		//return Response.serverError().entity("UUID cannot be blank").build();
		//return Response.status(Response.Status.NOT_FOUND).entity("Entity not found for UUID: " + uuid).build();
		return Response.ok(json, MediaType.APPLICATION_JSON).build();
	}
		
}
