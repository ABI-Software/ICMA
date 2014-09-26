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

import java.io.BufferedReader;
import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.ejb.EJB;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.annotation.HttpConstraint;
import javax.servlet.annotation.ServletSecurity;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import nz.ac.auckland.abi.workflow.WSWorkflowManager;

import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

/**
 * Servlet implementation class SXMRIModelFit
 */
@WebServlet(value = "/SXMRIModelFit", asyncSupported = true)
public class SXMRIModelFit extends HttpServlet {
	private static final long serialVersionUID = 1L;
	@EJB
	WSWorkflowManager wManager;

	@Override
	public void init(ServletConfig config) throws ServletException {
		super.init(config);
	}

	/**
	 * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse
	 *      response)
	 */
	protected void doGet(HttpServletRequest request,
			HttpServletResponse response) throws ServletException, IOException {
		processRequest(request, response);
	}

	/**
	 * @see HttpServlet#doPost(HttpServletRequest request, HttpServletResponse
	 *      response)
	 */
	protected void doPost(HttpServletRequest request,
			HttpServletResponse response) throws ServletException, IOException {
		processRequest(request, response);
	}

	private void processRequest(HttpServletRequest request,
			HttpServletResponse response) throws ServletException, IOException {
		StringBuffer jb = new StringBuffer();
		String line = null;
		try {
			BufferedReader reader = request.getReader();
			while ((line = reader.readLine()) != null)
				jb.append(line);
		} catch (Exception e) {
			throw new IOException("Invalid request");
		}

		String workflowID = request.getParameter("workflowID");
		String result = request.getParameter("result");
		if(workflowID!=null){
			if(result==null){
				try{
					JSONObject progress = wManager.getProgress(workflowID);
					response.getWriter().write((String)progress.get("mrifitting"));
				}catch(Exception exx){
					exx.printStackTrace();
					response.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
				}
			}else{//Send the result
				try {
					JSONObject obj = wManager.getModelFittingOutput(workflowID);
					response.getWriter().write(obj.toJSONString());
				} catch (Exception exx) {
					response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
					response.getWriter().write(exx.toString());
				}
			}
		}else{
			try {
								
				JSONParser parser = new JSONParser();
				JSONObject jsonObject = (JSONObject) parser.parse(jb.toString());
				jsonObject.put("username", request.getRemoteUser());
				final Logger log = Logger
						.getLogger(this.getClass().getSimpleName());
				log.log(Level.FINE, jsonObject.toJSONString());
				fitMRIModel(response, jsonObject);
	
			} catch (ParseException e) {
				// crash and burn
				throw new IOException("Error parsing JSON request string "
						+ jb.toString());
			} catch (Exception ex) {
				ex.printStackTrace();
				throw new ServletException(ex);
			}
		}
	}
	
	void fitMRIModel(HttpServletResponse response,
			final JSONObject obj) throws Exception {
		try {
			JSONObject json = wManager.fitMRIModel(obj);
			if (response != null) {
				response.getWriter().write(json.toJSONString());
			}
		} catch (Exception exx) {
			exx.printStackTrace();
			try {
				if (response != null) {
					response.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
					response.getWriter().write(exx.toString());
				}
			} catch (Exception exx1) {

			}
		}
	}

	
}
