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
package nz.ac.auckland.abi.helper;

import java.io.StringReader;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.logging.Level;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;


public class ProcessSpeckleTrackingOutput {
	private String xml = null;
	private JSONObject result = null;
	private Hashtable<String, String> viewNames = null;
	private Hashtable<String, String> typeNames = null;

	public ProcessSpeckleTrackingOutput(String targetXML) throws Exception {
		xml = targetXML;
		result = new JSONObject();
		viewNames = new Hashtable<String, String>();
		typeNames = new Hashtable<String, String>();
		DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
		DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
		Document doc = dBuilder.parse(new InputSource(new StringReader(xml)));
		doc.getDocumentElement().normalize();

		Element modelName = (Element) doc.getElementsByTagName("MODELNAME").item(0);
		result.put("MODELNAME", modelName.getTextContent());

		String[] landMarks = { "BASE", "APEX", "GELEM_CENTROID" };
		for (int i = 0; i < landMarks.length; i++) {
			Element objx = (Element) doc.getElementsByTagName(landMarks[i]).item(0);
			{
				Element x = (Element) objx.getElementsByTagName("x").item(0);
				Element y = (Element) objx.getElementsByTagName("y").item(0);
				/*
				 * JSONObject obj = new JSONObject(); obj.put("x",
				 * x.getTextContent()); obj.put("y", y.getTextContent());
				 */
				JSONArray obj = new JSONArray();
				obj.add(x.getTextContent());
				obj.add(y.getTextContent());
				result.put(landMarks[i], obj);
			}
		}
		// Process frame time vectors
		JSONObject framevectors = new JSONObject();
		Element frametimevectors = (Element) doc.getElementsByTagName("FRAMEVECTORS").item(0);
		NodeList normalizedframevectors = frametimevectors.getChildNodes();
		for (int i = 0; i < normalizedframevectors.getLength(); i++) {
			Node node = normalizedframevectors.item(i);
			if (node.getNodeType() == 1) {
				Element fm = (Element) node;
				framevectors.put(fm.getAttribute("VIEW"), fm.getTextContent());
			}
		}
		result.put("FRAMEVECTORS", framevectors);

		Element rvInsert = (Element) doc.getElementsByTagName("RVINSERTS").item(0);
		NodeList inserts = rvInsert.getChildNodes();
		JSONArray rvs = new JSONArray();
		for (int i = 0; i < inserts.getLength(); i++) {
			Node node = inserts.item(i);
			if (node.getNodeType() == 1) {
				Element ins = (Element) node;
				Element x = (Element) ins.getElementsByTagName("x").item(0);
				Element y = (Element) ins.getElementsByTagName("y").item(0);
				// rvs.add(x.getTextContent()+", "+y.getTextContent());
				JSONArray obj = new JSONArray();
				obj.add(x.getTextContent());
				obj.add(y.getTextContent());
				rvs.add(obj);
			}
		}
		result.put("RVINSERTS", rvs);
		Element duration = (Element) doc.getElementsByTagName("DURATION").item(0);
		result.put("DURATION", duration.getTextContent());
		Element nof = (Element) doc.getElementsByTagName("NUMBEROFFRAMES").item(0);
		result.put("NUMBEROFFRAMES", nof.getTextContent());
		int numberOfFrames = Integer.parseInt(nof.getTextContent());
		NodeList markers = doc.getElementsByTagName("MARKERS");
		final int numNodes = markers.getLength();
		for (int i = 0; i < numNodes; i++) {
			Element marker = (Element) markers.item(i);
			String view = marker.getAttribute("VIEW").toUpperCase();
			String type = marker.getAttribute("TYPE").toUpperCase();
			String frame = marker.getAttribute("FRAME");
			int nom = 0;
			int fid = 0;
			try {
				nom = Integer.parseInt(marker.getAttribute("NOOFMARKERS"));
				fid = Integer.parseInt(frame);
			} catch (NumberFormatException nux) {
				// Record from speckletracking input, get ed ec and bpm
				String ed = marker.getAttribute("ED");
				String ec = marker.getAttribute("EC");
				String bpm = marker.getAttribute("BPM");
				String target = marker.getAttribute("TARGET");

				JSONObject vt = (JSONObject) result.get(view);
				if (vt == null) {
					vt = new JSONObject();
					result.put(view, vt);
					viewNames.put(view, view);
				}
				type = "ENDO"; // Default type
				JSONObject vtype = (JSONObject) vt.get(type);
				if (vtype == null) {
					vtype = new JSONObject();
					vt.put(type, vtype);
					typeNames.put(type, type);
				}
				vtype.put("ED", ed);
				vtype.put("EC", ec);
				vtype.put("BPM", bpm);
				if (target != null) {
					if (target.trim().length() > 0)
						vtype.put("TARGET", target);
				}
				// Get image dcm information
				Element uri = (Element) marker.getElementsByTagName("URI").item(0);
				vtype.put("URI", uri.getTextContent());
				continue;
			}
			JSONObject vt = (JSONObject) result.get(view);
			if (vt == null) {
				vt = new JSONObject();
				result.put(view, vt);
				viewNames.put(view, view);
			}
			JSONObject vtype = (JSONObject) vt.get(type);
			if (vtype == null) {
				vtype = new JSONObject();
				vt.put(type, vtype);
				typeNames.put(type, type);
				JSONArray frames = new JSONArray();
				for (int nc = 0; nc < numberOfFrames; nc++)
					frames.add(null);
				vtype.put("MARKERS", frames);
			}
			JSONArray frames = (JSONArray) vtype.get("MARKERS");
			JSONArray coords = new JSONArray();
			for (int c = 0; c < nom; c++) {
				coords.add(null);
			}
			NodeList marks = marker.getElementsByTagName("MARK");
			final int numMarks = marks.getLength();
			for (int j = 0; j < numMarks; j++) {
				Element mk = (Element) marks.item(j);
				int id = Integer.parseInt(mk.getAttribute("ID"));
				Element x = (Element) mk.getElementsByTagName("x").item(0);
				Element y = (Element) mk.getElementsByTagName("y").item(0);
				JSONArray coord = new JSONArray();
				coord.add(x.getTextContent());
				coord.add(y.getTextContent());
				coords.set(id, coord);
			}
			/*
			 * for(int j=0;j<numMarks;j++){ if(coords.get(j)==null){
			 * coords.remove(j); } }
			 */
			frames.set(fid, coords);
		}
		JSONArray viewName = new JSONArray();
		ArrayList<String> keys = new ArrayList<String>(viewNames.keySet());
		for (String key : keys) {
			viewName.add(key);
		}
		result.put("views", viewName);
		java.util.logging.Logger.getLogger(this.getClass().getSimpleName()).log(Level.INFO,"Adding views " + viewName.toJSONString());
		viewName = new JSONArray();
		keys = new ArrayList<String>(typeNames.keySet());
		for (String key : keys) {
			viewName.add(key);
		}
		result.put("types", viewName);
	}

	public JSONObject getJSON() {
		return result;
	}

	public static void main(String args[]) throws Exception {
		String filename = "/home/rjag008/clevelandclinic/LVModelBasedSpeckleTracking/Release/AllView/SaxViews.xml";
		DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
		DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
		Document doc = dBuilder.parse(filename);
		doc.getDocumentElement().normalize();
		TransformerFactory transformerFactory = TransformerFactory.newInstance();
		Transformer transformer = transformerFactory.newTransformer();
		DOMSource source = new DOMSource(doc);
		StringWriter stringWriter = new StringWriter();
		StreamResult result = new StreamResult(stringWriter);
		transformer.transform(source, result);

		ProcessSpeckleTrackingOutput ps = new ProcessSpeckleTrackingOutput(stringWriter.getBuffer().toString());

		System.out.println(ps.result.toJSONString());
	}

}
