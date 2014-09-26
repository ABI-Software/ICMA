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

import java.io.IOException;

import javax.ejb.EJB;
import javax.servlet.ServletException;
import javax.servlet.annotation.HttpConstraint;
import javax.servlet.annotation.ServletSecurity;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.json.simple.JSONObject;

import nz.ac.auckland.abi.businesslogic.DataViewManagerRemote;

/**
 * Servlet implementation class SXPDetails
 */
@WebServlet("/SXPDetails")
public class SXPDetails extends HttpServlet {
	private static final long serialVersionUID = 1L;
	
	@EJB
	private DataViewManagerRemote dvm;       
    /**
     * @see HttpServlet#HttpServlet()
     */
    public SXPDetails() {
        super();
    }

	/**
	 * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse response)
	 */
	protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		String patientID = request.getParameter("patientID");
		if(patientID!=null){
			try{
				String input = dvm.getPatientRecord(patientID);
				response.setContentType("application/json");
				response.getOutputStream().write(input.getBytes());
				//System.out.println(input);
			}catch(Exception exx){
				response.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
				JSONObject json = new JSONObject();
				json.put("err", "Unable to process request: Exception "+exx+" occured");
				response.getOutputStream().write(json.toJSONString().getBytes());
			}
		}else{
			response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
			JSONObject json = new JSONObject();
			json.put("err", "KEY NOT AVAILABLE IN REQUEST");
			response.getOutputStream().write(json.toJSONString().getBytes());
		}
		response.getOutputStream().flush();
	}

	/**
	 * @see HttpServlet#doPost(HttpServletRequest request, HttpServletResponse response)
	 */
	protected void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		String patientID = request.getParameter("patientID");
		if(patientID!=null){
			try{
				String input = dvm.getPatientRecord(patientID);
				response.setContentType("application/json");
				response.getOutputStream().write(input.getBytes());
				//System.out.println(input);
			}catch(Exception exx){
				response.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
				JSONObject json = new JSONObject();
				json.put("err", "Unable to process request: Exception "+exx+" occured");
				response.getOutputStream().write(json.toJSONString().getBytes());
			}
		}else{
			response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
			JSONObject json = new JSONObject();
			json.put("err", "KEY NOT AVAILABLE IN REQUEST");
			response.getOutputStream().write(json.toJSONString().getBytes());
		}
		response.getOutputStream().flush();
	}

}
