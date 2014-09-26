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

import java.io.IOException;
import java.io.PrintWriter;
import java.util.List;
import java.util.Vector;

import javax.ejb.EJB;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

import nz.ac.auckland.abi.icmaconfiguration.ResourceConfigurationManager;

/**
 * Servlet implementation class UserDOCSList
 */
@WebServlet("/UserDOCSList")
public class UserDOCSList extends HttpServlet {
	private static final long serialVersionUID = 1L;
       
	@EJB
	private ResourceConfigurationManager rManager;
	
	
    /**
     * @see HttpServlet#HttpServlet()
     */
    public UserDOCSList() {
        super();
        // TODO Auto-generated constructor stub
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

	
	protected void processRequest(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		String filter = request.getParameter("filter");
		String deleteRequest = request.getParameter("delete");
		if(deleteRequest==null){
			try{
				List<String> paths = rManager.getUserDocs(request.getRemoteUser(),filter);
				JSONArray array = new JSONArray();
				for(String path : paths){
					String name = path.substring(path.lastIndexOf("/")+1);
					JSONObject obj = new JSONObject();
					obj.put("name", name);
					obj.put("path", path);
					array.add(obj);
				}
	
				int totalAfterFilter = paths.size();
				JSONObject result = new JSONObject();
				result.put("iTotalRecords", totalAfterFilter);
				result.put("iTotalDisplayRecords", totalAfterFilter);
				result.put("aaData", array);
				response.setContentType("application/json");
				response.setHeader("Cache-Control", "no-store");
				PrintWriter pr = new PrintWriter(response.getOutputStream());
				pr.print(result);
				pr.close();
			}catch(javax.jcr.PathNotFoundException pfn){
				JSONObject result = new JSONObject();
				JSONArray array = new JSONArray();
				result.put("iTotalRecords", 0);
				result.put("iTotalDisplayRecords", 0);
				result.put("aaData", array);
				response.setContentType("application/json");
				response.setHeader("Cache-Control", "no-store");
				PrintWriter pr = new PrintWriter(response.getOutputStream());
				pr.print(result);
				pr.close();
			}catch(Exception exx){
				exx.printStackTrace();
				response.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
				response.getOutputStream().write(exx.toString().getBytes());
			}
		}else{
			try{
				Vector<String> docs = new Vector<String>();
				docs.add(deleteRequest);
				rManager.removeUSERDoc(docs);
			}catch(Exception exx){
				exx.printStackTrace();
				response.setStatus(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
				response.getOutputStream().write(exx.toString().getBytes());
			}
		}
	}
}
