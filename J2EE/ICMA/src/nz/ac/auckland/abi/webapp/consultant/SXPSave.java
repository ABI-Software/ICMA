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
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import javax.ejb.EJB;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import nz.ac.auckland.abi.businesslogic.DataViewManagerRemote;
import nz.ac.auckland.abi.icmaconfiguration.CMSContent;
import nz.ac.auckland.abi.icmaconfiguration.ResourceConfigurationManager;

import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;

/**
 * Servlet implementation class SXPDetails
 */
@WebServlet("/SXPSave")
public class SXPSave extends HttpServlet {
	private static final long serialVersionUID = 1L;
	
	@EJB
	private ResourceConfigurationManager rManager;
	
	@EJB
	private DataViewManagerRemote dvm;       
    /**
     * @see HttpServlet#HttpServlet()
     */
    public SXPSave() {
        super();
    }

	/**
	 * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse response)
	 */
	protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		processRequest(request,response);
	}

	/**
	 * @see HttpServlet#doPost(HttpServletRequest request, HttpServletResponse response)
	 */
	protected void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		processRequest(request,response);
	}

	private void processRequest(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		String patientID = request.getParameter("patientID");
		if(patientID!=null){
			try{
				String input = dvm.getPatientRecord(patientID);
				File scratch = new File(rManager.getDiskScratchSpace()+"/SXPSAVE/"+patientID);
				if(!scratch.exists()){
					scratch.mkdirs();
				}
				PrintWriter pr = new PrintWriter(new File(scratch,"json.txt"));
				pr.println(input);
				pr.close();
				JSONParser parser = new JSONParser();
				JSONObject object = (JSONObject)parser.parse(input);
				//Get the studies and output the files
				JSONObject studies = (JSONObject)object.get("studies");
				if(studies!=null){
					List<String> keys = new ArrayList<String>(studies.keySet());
					for(String key : keys){
						JSONObject study = (JSONObject) studies.get(key);
						//Handle different views
						if(study!=null){
							JSONObject views = (JSONObject) study.get("views");
							List<String> vkeys = new ArrayList<String>(views.keySet());
							for(String vKey : vkeys){
								if(vKey.startsWith("view")){
									JSONObject view = (JSONObject) views.get(vKey);
									if(view!=null){
										String image_uri = (String) view.get("image");
										JSONObject video = (JSONObject) view.get("video");
										String mp4Uri = (String) video.get("mp4");
										String oggUri = (String) video.get("ogg");
										String webmUri = (String) video.get("webm");
										saveURIToFile(scratch, image_uri);
										saveURIToFile(scratch, mp4Uri);
										saveURIToFile(scratch, oggUri);
										saveURIToFile(scratch, webmUri);
									}
								}
							}
						}
					}
				}
				JSONObject Models = (JSONObject)object.get("Models");
				if(Models!=null){
					List<String> keys = new ArrayList<String>(Models.keySet());
					for(String key : keys){
						if(key.startsWith("model")){
							JSONObject model = (JSONObject) Models.get(key);
							JSONObject region = (JSONObject) model.get("Regions");
							List<String> mkeys = new ArrayList<String>(region.keySet());
							for(String mkey : mkeys){
								if(!mkey.equalsIgnoreCase("MESH")){
									JSONObject view = (JSONObject) region.get(mkey);
									try{
										Long startIndex = (Long) view.get("imageStartIndex");
										Long endIndex = (Long) view.get("imageEndIndex");
										String prefix = (String) view.get("imageprefix");
										String type = (String) view.get("imageType");	
										for(int ctr=startIndex.intValue();ctr<endIndex.intValue();ctr++){
											saveURIToFile(scratch, prefix+ctr+"."+type);
										}
									}catch(Exception exx){
										
									}
								}else if(mkey.equalsIgnoreCase("MESH")){
									JSONObject view = (JSONObject) region.get(mkey);
									if(view!=null){
										Long startIndex = (Long) view.get("regionStartIndex");
										Long endIndex = (Long) view.get("regionEndIndex");
										String prefix = (String) view.get("meshRegionprefix");
										for(int ctr=startIndex.intValue();ctr<endIndex.intValue();ctr++){
											saveURIToFile(scratch, prefix+ctr+".exregion");
										}
									}
								}
							}
						}
					}
				}
				String output = "Successfully saved data to "+scratch.getAbsolutePath();
				response.getOutputStream().write(output.getBytes());
				//System.out.println(input);
			}catch(Exception exx){
				exx.printStackTrace();
				response.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
				response.getOutputStream().write(("Unable to process request: Exception "+exx+" occured").getBytes());
			}
		}else{
			response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
			response.getOutputStream().write("KEY NOT AVAILABLE IN REQUEST".getBytes());
		}
		response.getOutputStream().flush();
	}
	
	private void saveURIToFile(File scratch, String uri) throws Exception{
		String dir = uri.substring(0,uri.lastIndexOf('/'));
		File targetDir = new File(scratch,dir);
		if(!targetDir.exists()){
			targetDir.mkdirs();
		}
		File targetFile = new File(scratch,uri);
		FileOutputStream output = new FileOutputStream(targetFile);
		CMSContent content = rManager.getCMSNodeAt(uri);
		content.output.writeTo(output);
	}
	
}
