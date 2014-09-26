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

import javax.ejb.EJB;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import nz.ac.auckland.abi.businesslogic.FEMModelsBeanRemote;
import nz.ac.auckland.abi.businesslogic.ICMADatabaseAdministrationRemote;
import nz.ac.auckland.abi.icmaconfiguration.ResourceConfigurationManager;
import nz.ac.auckland.abi.workflow.WSWorkflowManager;

import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;

/**
 * Servlet implementation class ModelManager
 */
@WebServlet("/SXMUpdate")
public class ModelManager extends HttpServlet {
	private static final long serialVersionUID = 1L;

	@EJB
	private ICMADatabaseAdministrationRemote admin;

	@EJB
	private WSWorkflowManager workflowManager;
	
	@EJB
	private ResourceConfigurationManager rManager;
	
	@EJB
	FEMModelsBeanRemote model;
	/**
	 * @see HttpServlet#HttpServlet()
	 */
	public ModelManager() {
		super();
		// TODO Auto-generated constructor stub
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
			HttpServletResponse hresponse) throws ServletException, IOException {
		StringBuffer buf = new StringBuffer();
		try {
			BufferedReader br = request.getReader();
			String input = null;
			while ((input = br.readLine()) != null) {
				buf.append(input);
			}
			br.close();

			JSONObject response = new JSONObject();
			JSONParser parser = new JSONParser();
			JSONObject req = (JSONObject) parser.parse(buf.toString());
			//System.out.println(buf.toString());
			try {
				if (((String) req.get("action")).equalsIgnoreCase("update")) {
					//System.out.println("Updating");
					if (model.updateModel((String) req.get("modelid"),
							(String) req.get("annotation"),
							(String) req.get("status"),
							(String) req.get("userid"))) {
						response.put("modelid", req.get("modelid"));
						response.put("msg", "success");
					} else {
						hresponse.setStatus(HttpServletResponse.SC_BAD_REQUEST);
					}
				}
				if (((String) req.get("action")).startsWith("create")) {
					//System.out.println("Creating");
					String temp = ((String) req.get("action"));
					String workFlowID = temp.substring("create".length());
					//The create field does not have instance ID
					String modelID = rManager.getWsWorkFlowInfo(workFlowID).getInstanceID();
					if (model.updateModel(modelID,
							(String) req.get("annotation"),
							(String) req.get("status"),
							(String) req.get("userid"))) {
						response.put("modelid", req.get("modelid"));
						response.put("msg", "success");
						workflowManager.saveModel(workFlowID);
					} else {
						hresponse.setStatus(HttpServletResponse.SC_BAD_REQUEST);
					}
				}
				// also handle delete
				if (((String) req.get("action")).startsWith("delete")) {
					//System.out.println("Deleting");
					String modelID = null;
					try {
						if(req.get("modelid")==null){
							String temp = ((String) req.get("action"));
							String workFlowID = temp.substring("create".length());
							//The create field does not have instance ID
							modelID = rManager.getWsWorkFlowInfo(workFlowID).getInstanceID();
						}else{
							modelID = (String) req.get("modelid");
						}
						
						if (model.removeModel(modelID,"this")) {
							response.put("modelid", req.get("modelid"));
							response.put("msg", "success");
						} else {
							hresponse
									.setStatus(HttpServletResponse.SC_BAD_REQUEST);
						}
					} catch (Exception exx) {
						exx.printStackTrace();
						if(modelID!=null)
							hresponse.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
						else
							hresponse.setStatus(HttpServletResponse.SC_BAD_REQUEST);
					}
				}
			} catch (Exception exx) {
				hresponse
						.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
				exx.printStackTrace();
			}
			String out = response.toJSONString();
			hresponse.getOutputStream().write(out.getBytes());
		} catch (Exception exx) {
			System.out.println("Error reading input " + exx);
		}
	}

}
