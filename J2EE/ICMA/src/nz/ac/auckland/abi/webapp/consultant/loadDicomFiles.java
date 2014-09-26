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

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.annotation.Resource;
import javax.ejb.EJB;
import javax.servlet.ServletException;
import javax.servlet.annotation.HttpConstraint;
import javax.servlet.annotation.MultipartConfig;
import javax.servlet.annotation.ServletSecurity;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.Part;

import org.json.simple.JSONObject;

import nz.ac.auckland.abi.administration.ICMADatabaseSynchronizer;
import nz.ac.auckland.abi.dcm4chee.DCMAccessManager;

/**
 * Servlet implementation class loadDicomFiles
 */
@ServletSecurity(@HttpConstraint(rolesAllowed = "ICMAUSER"))
@MultipartConfig(fileSizeThreshold = 1024 * 1024)
@WebServlet(description = "Servlet interface to load dicom files into local PACS", urlPatterns = { "/loadDicomFiles" })
public class loadDicomFiles extends HttpServlet {
	private static final long serialVersionUID = 1L;

	@Resource(name = "java:global/ICMADISKSCRATCHSPACE")
	private String diskScratchSpace;

	@EJB
	private ICMADatabaseSynchronizer icmadbmanager;
	Logger log;

	/**
	 * @see HttpServlet#HttpServlet()
	 */
	public loadDicomFiles() {
		super();
		log = Logger.getLogger(this.getClass().getSimpleName());
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

	protected void processRequest(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		// constructs path of the directory to save uploaded file
		if (request.getParameter("refreshPatientMap") == null) {
			String uploadFilePath = diskScratchSpace + "/uploads" + Math.random();
			try {
				// creates the save directory if it does not exists
				File fileSaveDir = new File(uploadFilePath);
				while (fileSaveDir.exists()) {
					uploadFilePath = diskScratchSpace + "/uploads" + Math.random();
					fileSaveDir = new File(uploadFilePath);
				}
				fileSaveDir.mkdirs();

				log.log(Level.FINE, "Upload File Directory=" + fileSaveDir.getAbsolutePath());

				String fileName = null;
				// Get all the parts from request and write it to the file on
				// server
				ArrayList<File> dicomFiles = new ArrayList<File>();
				for (Part part : request.getParts()) {
					fileName = getFileName(part);
					File dicomFile = new File(uploadFilePath, fileName);
					part.write(dicomFile.getAbsolutePath());
					dicomFiles.add(dicomFile);
				}

				JSONObject jresponse =  DCMAccessManager.storeDataSets(dicomFiles);
				jresponse.put("success", "upload");
				response.getWriter().println(jresponse.toJSONString());
				response.setStatus(HttpServletResponse.SC_OK);
				// Remove the files
				try {
					for (File dc : dicomFiles)
						dc.delete();
					fileSaveDir.delete();
				} catch (Exception ex1) {

				}

			} catch (Exception exx) {
				StringWriter sr = new StringWriter();
				PrintWriter pr = new PrintWriter(sr);
				exx.printStackTrace(pr);
				exx.printStackTrace();
				JSONObject json = new JSONObject();
				json.put("error", "upload");
				json.put("msg", sr.toString());
				response.getWriter().println(json.toJSONString());
				response.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
			}
		}else{
			icmadbmanager.updatePatientMap();
		}
	}

	/**
	 * Utility method to get file name from HTTP header content-disposition
	 */
	private String getFileName(Part part) {
		String contentDisp = part.getHeader("content-disposition");
		log.log(Level.FINE, "content-disposition header= " + contentDisp);
		String[] tokens = contentDisp.split(";");
		for (String token : tokens) {
			if (token.trim().startsWith("filename")) {
				return token.substring(token.indexOf("=") + 2, token.length() - 1);
			}
		}
		return "";
	}

}
