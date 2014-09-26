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
package nz.ac.auckland.abi.webapp.analyst;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.List;

import javax.ejb.EJB;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import nz.ac.auckland.abi.businesslogic.ModelViewBeanRemote;
import nz.ac.auckland.abi.formatting.poi.ModelJSONToExcel;
import nz.ac.auckland.abi.icmaconfiguration.ResourceConfigurationManager;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;

/**
 * Servlet implementation class ReportGenerator
 */
@WebServlet("/ReportGenerator")
public class ReportGenerator extends HttpServlet {
	private static final long serialVersionUID = 1L;
    
	@EJB
	private ModelViewBeanRemote modelViewer; 
	
	@EJB
	private ResourceConfigurationManager rManager;
	
    /**
     * @see HttpServlet#HttpServlet()
     */
    public ReportGenerator() {
        super();
        // TODO Auto-generated constructor stub
    }

		/**
	 * @see HttpServlet#doPost(HttpServletRequest request, HttpServletResponse response)
	 */
	protected void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		processRequest(request,response);
	}

	protected void processRequest(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		StringBuffer bufx = new StringBuffer();
		try {
			BufferedReader br = request.getReader();
			String input = null;
			while ((input = br.readLine()) != null) {
				bufx.append(input);
			}
			br.close();
			System.out.println(bufx.toString());
		}catch(Exception exx){
			response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
			response.getOutputStream().write(("INVALID INPUT PARAMETER(S)\t"+exx).getBytes());
		}
		
		
		String modelList = bufx.toString();
		if(modelList!=null){
			try{
				JSONParser parser = new JSONParser();
				JSONObject object = (JSONObject) parser.parse(modelList);
				JSONArray modelArray = (JSONArray)object.get("pklist");
				JSONArray avs = (JSONArray)object.get("analysisvariables");
				String reportName = (String)object.get("reportname");
				ArrayList<String> uva = new ArrayList<String>();
				for(int i=0;i<avs.size();i++){
					//System.out.println(avs.get(i));
					uva.add((String) avs.get(i));
				}
				ModelJSONToExcel generator = new ModelJSONToExcel(uva);
				//Get the models
				StringBuffer buf = new StringBuffer(" WHERE M.model_pk IN (");
				
				for(int i=0;i<modelArray.size()-1;i++){
					String modelPk = (String) modelArray.get(i);
					buf.append(modelPk+", ");
				}
				buf.append((String) modelArray.get(modelArray.size()-1)+" )");
				
				List<JSONObject> myModelViews = modelViewer.getListOfModelsWithMetaData(buf.toString(),0,modelArray.size()+1);
				//For each model retrive its JSON and create the table
				Hashtable<String, String> modelMap = new Hashtable<String, String>();
				for(JSONObject obj : myModelViews){
					String key = obj.get("id")+"\t"+obj.get("name")+"\t"+obj.get("gender")+"\t"+obj.get("birthdate")+"\t";
					JSONArray lmodelArray = (JSONArray) obj.get("models");
					for(int i=0;i<lmodelArray.size();i++){
						StringBuffer header = new StringBuffer(key);
						JSONObject lm = (JSONObject) lmodelArray.get(i);
						header.append((String)lm.get("studydate")+"\t");
						header.append((String)lm.get("name")+"\t");
						header.append((String)lm.get("status")+"\t");
						JSONObject lo = (JSONObject) lm.get("annotation");
						header.append((String)lo.get("studyType")+"\t");
						header.append((String)lo.get("gravityType")+"\t");
						header.append((String)lo.get("classification"));
						header.append((String)lo.get("notes"));
						JSONObject mjson = (JSONObject)lm.get("metadata");
						JSONObject mea = (JSONObject) mjson.get("Measures");
						header.append("*"+(String)mea.get("modelPeakGlobalLongitudinalStrain"));
						header.append("EF"+(String)mea.get("ejectionFraction"));						
						mjson.put("modelid",(String)lm.get("id"));
						mjson.put("name",(String)lm.get("name"));
						mjson.put("startTime",0);
						modelMap.put(header.toString(), mjson.toJSONString());
					}
				}
				String dir = rManager.getDiskScratchSpace()+"/Analysis";
				File aDir = new File(dir);
				if(!aDir.exists()){
					aDir.mkdirs();
				}
				int fctr = 0;
				File filename = new File(dir,"ModelAnalysis"+fctr+".xls");
				while(filename.exists()){
					fctr++;
					filename = new File(dir,"ModelAnalysis"+fctr+".xls");
				}
				generator.createXLS(modelMap, filename.getAbsolutePath());
				String docuri = rManager.addUSERDoc(request.getRemoteUser(), reportName, "application/vnd.ms-excel", filename);
				response.setStatus(HttpServletResponse.SC_OK);
				response.getOutputStream().write(docuri.getBytes());
			}catch(Exception exx){
				exx.printStackTrace();
				response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
				response.getOutputStream().write(("INVALID INPUT PARAMETER(S)\t"+exx.toString()).getBytes());
			}
		}else{
			response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
			response.getOutputStream().write("INVALID INPUT PARAMETER(S)".getBytes());
		}
	}
	
}
