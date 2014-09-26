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
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import javax.ejb.EJB;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import nz.ac.auckland.abi.businesslogic.PatientSourceMapManagerRemote;
import nz.ac.auckland.abi.entities.PatientSource;
import nz.ac.auckland.abi.icmaconfiguration.BatchAdditionsManager;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

/**
 * Servlet implementation class PatientDBInterface
 */
@WebServlet("/PatientDBInterface")
public class PatientDBInterface extends HttpServlet {
	private static final long serialVersionUID = 1L;
      
	@EJB
	PatientSourceMapManagerRemote sourceManger;
	
	@EJB
	BatchAdditionsManager batchJobManager;
	
    /**
     * @see HttpServlet#HttpServlet()
     */
    public PatientDBInterface() {
        super();
        // TODO Auto-generated constructor stub
    }

	/**
	 * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse response)
	 */
	protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		processRequest(request, response);
	}

	/**
	 * @see HttpServlet#doPost(HttpServletRequest request, HttpServletResponse response)
	 */
	protected void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		processRequest(request, response);
	}

	private void processRequest(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException{
		String[] cols = { "id", "name", "status", "id", "id"};
	     
	    JSONObject result = new JSONObject();
	    JSONArray array = new JSONArray();
	    int amount = 10;
	    int start = 0;
	    int echo = 0;
	    int col = 0;
	     
	    String id = "";
	    String name = "";
	    String icma = "";
	    String dir = "asc";
	    String sStart = request.getParameter("iDisplayStart");
	    String sAmount = request.getParameter("iDisplayLength");
	    String sEcho = request.getParameter("sEcho");
	    String sCol = request.getParameter("iSortCol_0");
	    String sdir = request.getParameter("sSortDir_0");
	     
	    id = request.getParameter("sSearch_0");
	    name = request.getParameter("sSearch_1");
	    icma = request.getParameter("sSearch_2");
	    if(id==null){
	    	id="";
	    }
	    if(name==null){
	    	name = "";
	    }
	    if(icma==null){
	    	icma="";
	    }
	    List<String> sArray = new ArrayList<String>();
	    if (!id.equals("")) {
	        String sEngine = " id like '%" + id + "%'";
	        sArray.add(sEngine);
	        //or combine the above two steps as:
	        //sArray.add(" engine like '%" + engine + "%'");
	        //the same as followings
	    }
	    if (!name.equals("")) {
	        String sBrowser = " name like '%" + name + "%'";
	        sArray.add(sBrowser);
	    }
	    if (!icma.equals("")) {
	        String sBrowser = " status like '%" + icma + "%'";
	        sArray.add(sBrowser);
	    }
	     
	    String individualSearch = "";
	    if(sArray.size()==1){
	        individualSearch = sArray.get(0);
	    }else if(sArray.size()>1){
	        for(int i=0;i<sArray.size()-1;i++){
	            individualSearch += sArray.get(i)+ " and ";
	        }
	        individualSearch += sArray.get(sArray.size()-1);
	    }
	     
	    if (sStart != null) {
	        start = Integer.parseInt(sStart);
	        if (start < 0)
	            start = 0;
	    }
	    if (sAmount != null) {
	        amount = Integer.parseInt(sAmount);
	        if (amount < 10 || amount > 100)
	            amount = 10;
	    }
	    if (sEcho != null) {
	        echo = Integer.parseInt(sEcho);
	    }
	    if (sCol != null) {
	        col = Integer.parseInt(sCol);
	        if (col < 0 || col > 2)
	            col = 0;
	    }
	    if (sdir != null) {
	        if (!sdir.equals("asc"))
	            dir = "desc";
	    }
	    String colName = cols[col];
	    try {
		    int total = sourceManger.getMapCount();
		    //Determine the list of patients, let this be total
		    
		    int totalAfterFilter = total;
		    //result.put("sEcho",echo);
		 
		    //Now load the array with records that math the search key words

	        String searchSQL = "";
	        String searchTerm = request.getParameter("sSearch");
	        if(searchTerm!=null){
		        String globeSearch =  " where (p.id like '%"+searchTerm+"%'"
		        						+ " or p.name like '%"+searchTerm+"%'"
		                                + " or p.status like '%"+searchTerm+"%')";
		        if(searchTerm!=""&&individualSearch!=""){
		            searchSQL = globeSearch + " and " + individualSearch;
		        }
		        else if(individualSearch!=""){
		            searchSQL = " where " + individualSearch;
		        }else if(searchTerm!=""){
		            searchSQL=globeSearch;
		        }
	        }
	        if(colName!= null)
	        	searchSQL += " order by p." + colName + " " + dir;
	        searchSQL += " limit " + start + ", " + amount;

	        List<PatientSource> patients = sourceManger.getAllPatients(searchSQL);
	        for(PatientSource pat : patients){
	            JSONArray ja = new JSONArray();
	            ja.add(pat.getId());
	            ja.add(pat.getName());
	            if(!batchJobManager.hasOngoingProcess(pat.getId()))
	            	ja.add(pat.getStatus());
	            else
	            	ja.add("Operation in progress");
	            ja.add(pat.getId());
	            ja.add(pat.getId());
	            array.add(ja);
	        }
            totalAfterFilter = patients.size();
	        result.put("iTotalRecords", total);
	        result.put("iTotalDisplayRecords", totalAfterFilter);
	        result.put("aaData", array);
	        response.setContentType("application/json");
	        response.setHeader("Cache-Control", "no-store");
	        PrintWriter pr = new PrintWriter(response.getOutputStream());
	        pr.print(result);
	        pr.close();
	    }catch(Exception exx){
	    	
	    }
	}
}
