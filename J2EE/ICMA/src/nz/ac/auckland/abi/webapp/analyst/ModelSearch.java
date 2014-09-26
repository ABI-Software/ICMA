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

import javax.ejb.EJB;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import nz.ac.auckland.abi.businesslogic.ModelViewBeanRemote;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

/**
 * Servlet implementation class ModelSearch
 */
@WebServlet("/ModelSearch")
public class ModelSearch extends HttpServlet {
	private static final long serialVersionUID = 1L;

	@EJB
	ModelViewBeanRemote modelViewer;

	/**
	 * @see HttpServlet#HttpServlet()
	 */
	public ModelSearch() {
		super();
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

	protected void processRequest(HttpServletRequest request,
			HttpServletResponse response) throws ServletException, IOException {

		String[] cols = { "pat_id", "pat_name", "pat_sex", "pat_birthdate", "models" };

		JSONObject result = new JSONObject();
		JSONArray array = new JSONArray();
		int amount = 10;
		int start = 0;
		int echo = 0;
		int col = 0;

		String dir = "asc";
		String sStart = request.getParameter("iDisplayStart");
		String sAmount = request.getParameter("iDisplayLength");
		String sEcho = request.getParameter("sEcho");
		String sCol = request.getParameter("iSortCol_0");
		String sdir = request.getParameter("sSortDir_0");

		String id = request.getParameter("PatientID");
		String gender = request.getParameter("Gender");
		String status = request.getParameter("Status");
		String studyType = request.getParameter("StudyType");
		String classification = request.getParameter("SpecialClassification");
		String gravity = request.getParameter("GravitationalConditioning");
		String hashTags = request.getParameter("HashTags");

		String fid = request.getParameter("selPatientID");
		String fgender = request.getParameter("selGender");
		String fstatus = request.getParameter("selStatus");
		String fstudyType = request.getParameter("selStudyType");
		String fclassification = request
				.getParameter("selSpecialClassification");
		String fgravity = request.getParameter("selGravitationalConditioning");
		//String fhashTags = request.getParameter("selHashTags");

		StringBuffer searchString = new StringBuffer();
		String all = request.getParameter("search");
		if (all == null) {

			String previousConjunction = "";
			if (id != null) {
				searchString.append("M.pat_id like '%" + id.toUpperCase()
						+ "%'");
				if (fid.equalsIgnoreCase("AND")) {
					previousConjunction = "AND";
				} else {
					previousConjunction = "OR";
				}
			}
			if (gender != null) {
				searchString.append("  " + previousConjunction + " ");
				searchString.append("M.pat_sex like '%"
						+ gender.toUpperCase() + "%'");
				if (fgender.equalsIgnoreCase("AND")) {
					previousConjunction = "AND";
				} else {
					previousConjunction = "OR";
				}
			}
			if (status != null) {
				searchString.append("  " + previousConjunction + " ");
				searchString.append("M.model_status like '%"
						+ status.toUpperCase() + "%'");
				if (fstatus.equalsIgnoreCase("AND")) {
					previousConjunction = "AND";
				} else {
					previousConjunction = "OR";
				}
			}
			if (studyType != null) {
				searchString.append("  " + previousConjunction + " ");
				searchString
						.append("M.Annotation REGEXP \'\"studyType\":\"([^\"]*)"
								+ studyType.toUpperCase() + "([^\"]*)\"\'");
				if (fstudyType.equalsIgnoreCase("AND")) {
					previousConjunction = "AND";
				} else {
					previousConjunction = "OR";
				}
			}
			if (classification != null) {
				searchString.append("  " + previousConjunction + " ");
				searchString
						.append("M.Annotation REGEXP \'\"classification\":\"([^\"]*)"
								+ classification.toUpperCase() + "([^\"]*)\"\'");
				if (fclassification.equalsIgnoreCase("AND")) {
					previousConjunction = "AND";
				} else {
					previousConjunction = "OR";
				}
			}
			if (gravity != null) {
				searchString.append("  " + previousConjunction + " ");
				searchString
						.append("M.Annotation REGEXP \'\"gravityType\":\"([^\"]*)"
								+ gravity.toUpperCase() + "([^\"]*)\"\'");
				if (fgravity.equalsIgnoreCase("AND")) {
					previousConjunction = "AND";
				} else {
					previousConjunction = "OR";
				}
			}
			if (hashTags != null) {
				String tags[] = hashTags.split(",");
				previousConjunction = "";
				searchString.append(" (");
				for (String tag : tags) {
					searchString.append("  " + previousConjunction + " ");
					searchString
							.append("M.Annotation REGEXP \'\"annotation\":\"([^\"]*)"
									+ tag.toUpperCase().trim() + "([^\"]*)\"\'");
					previousConjunction = "OR";
				}
				searchString.append(" ) ");
			}
			if(searchString.toString().length()>0){
				String out = " WHERE "+searchString.toString();
				searchString = new StringBuffer();
				searchString.append(out);
			}
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
			if (col < 0 || col > 3)
				col = 0;
		}
		if (sdir != null) {
			if (!sdir.equals("asc"))
				dir = "desc";
		}
		String colName = cols[col];

		if (colName != null)
			searchString.append(" order by M." + colName + " " + dir);
		//searchString.append(" limit " + start + ", " + amount);
		
		try {
			List<JSONObject> mResult = modelViewer.getListOfModels(searchString.toString(),start,amount);
			int total = (int) modelViewer.getTotalNumberOfPatients();
			// Determine the list of patients, let this be total

			// Datatables needs the object as a JSON Array
			for (JSONObject obj : mResult) {
				JSONArray arr = (JSONArray) obj.get("models");
				obj.put("counter", arr.size());
				array.add(obj);
			}

			int totalAfterFilter = mResult.size();
			//System.out.println(totalAfterFilter+" ("+total+")");
			result.put("iTotalRecords", total);
			result.put("iTotalDisplayRecords", totalAfterFilter);
			result.put("aaData", array);
			response.setContentType("application/json");
			response.setHeader("Cache-Control", "no-store");
			PrintWriter pr = new PrintWriter(response.getOutputStream());
			pr.print(result);
			pr.close();
		} catch (Exception exx) {

		}

	}

}
