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
import javax.servlet.AsyncContext;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletResponse;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import nz.ac.auckland.abi.icmaconfiguration.ResourceConfigurationManager;
import nz.ac.auckland.abi.workflow.WSWorkflowManager;

import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

/**
 * Servlet implementation class SXModelFit
 */
@WebServlet(value = "/SXModelFit", asyncSupported = true)
public class SXModelFit extends HttpServlet {
	private static final long serialVersionUID = 1L;
	private final long CALLBACK_TIMEOUT = 600000; // ms ~ 10 min
	private String workflowID = null;
	private JSONObject jObj = null;
	private ServletResponse respObj = null;
	private AsyncContext ctx = null;

	@EJB
	ResourceConfigurationManager rManager;

	@EJB
	WSWorkflowManager workflowManager;

	@Override
	public void init(ServletConfig config) throws ServletException {
		super.init(config);
	}

	/**
	 * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse
	 *      response)
	 */
	protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		processRequest(request, response);
	}

	/**
	 * @see HttpServlet#doPost(HttpServletRequest request, HttpServletResponse
	 *      response)
	 */
	protected void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		processRequest(request, response);
	}

	private void processRequest(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		StringBuffer jb = new StringBuffer();
		String line = null;
		try {
			BufferedReader reader = request.getReader();
			while ((line = reader.readLine()) != null)
				jb.append(line);
		} catch (Exception e) {
			throw new IOException("Invalid request");
		}

		try {
			final Logger log = Logger.getLogger(this.getClass().getSimpleName());
			String wfID = request.getParameter("workflowID");
			String resultRequest = request.getParameter("result");
			// Handle progress and result check requests (these send query
			// parameters)
			if (wfID != null) {
				if (resultRequest == null) {
					try {
						JSONObject obj = workflowManager.getProgress(wfID);
						response.getWriter().write(obj.toJSONString());
					} catch (Exception exx) {
						response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
						response.getWriter().write(exx.toString());
					}
				} else {
					try {
						JSONObject obj = workflowManager.getModelFittingOutput(wfID);
						response.getWriter().write(obj.toJSONString());
					} catch (Exception exx) {
						response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
						response.getWriter().write(exx.toString());
					}
				}
				return;
			}
			// Do this after checking for progress and result request checks
			// As the json is not available for other requests
			JSONParser parser = new JSONParser();
			JSONObject jsonObject = (JSONObject) parser.parse(jb.toString());
			jsonObject.put("username", request.getRemoteUser());
			log.log(Level.FINE, jsonObject.toJSONString());
			jObj = jsonObject;

			if (jsonObject.get("workflowID") == null) {
				ctx = request.startAsync();
				ctx.setTimeout(CALLBACK_TIMEOUT);
				respObj = ctx.getResponse();
				Thread speckleTrack = new Thread(new Runnable() {
					public void run() {
						try {
							workflowID = workflowManager.trackSpeckles(jObj);
							JSONObject wfObj = new JSONObject();

							double progress = 0.0;
							while (progress < 1.0) {
								try {
									Thread.sleep(10000);
								} catch (Exception exx) {

								}
								JSONObject obj = workflowManager.getProgress(workflowID);
								String prog = (String) obj.get("tracking");
								progress = Double.parseDouble(prog);
								log.log(Level.FINE, "Speckle tracking progress " + progress);
							}
							log.log(Level.FINE, "Speckle tracking completed");
							// Now start the fitting
							JSONObject fitResult = workflowManager.getSpeckleTrackingOutput(workflowID);
							log.log(Level.FINE, fitResult.toJSONString());
							String fworkflowID = workflowManager.fitModelToSpeckles(fitResult, workflowID);
							wfObj.put("workflowID", fworkflowID);
							respObj.getWriter().write(wfObj.toJSONString());
							ctx.complete();
							log.log(Level.FINE, "Speckle fitting started");
							progress = 0.0;
							while (progress < 1.0) {
								try {
									Thread.sleep(10000);
								} catch (Exception exx) {

								}
								JSONObject obj = workflowManager.getProgress(fworkflowID);
								String prog = (String) obj.get("fitting");
								log.log(Level.INFO, "Speckle fitting progress of " + fworkflowID + "\t" + prog);
								try{
									progress = Double.parseDouble(prog);
								}catch(Exception exx){ //Sometimes the workflow on the other end is not ready and will return null for progress
									log.log(Level.INFO, "Speckle fitting progress returned null object!! "+exx);
									progress = 0.0;
								}
							}
							log.log(Level.FINE, "Speckle fitting completed");
						} catch (Exception exx) {
							HttpServletResponse response = (HttpServletResponse) ctx.getResponse();
							response.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
							exx.printStackTrace();
							ctx.complete();
						}
					}
				});
				speckleTrack.start();
			} else {
				String oldWorkflowID = (String) jObj.get("workflowID"); 
				// log.log(Level.FINE, "Fitting model to "+jObj.toJSONString());
				workflowID  = workflowManager.fitModelToSpeckles(jObj, oldWorkflowID);
				//log.log(Level.INFO,"Switching workflow id from "+oldWorkflowID+" to "+workflowID);
				JSONObject wfObj = new JSONObject();
				wfObj.put("workflowID", workflowID);
				response.getWriter().write(wfObj.toJSONString());
			}
		} catch (ParseException e) {
			// crash and burn
			throw new IOException("Error parsing JSON request string " + jb.toString());
		} catch (Exception ex) {
			throw new ServletException(ex);
		}
	}

}
