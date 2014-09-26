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

import java.io.StringWriter;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.traversal.DocumentTraversal;
import org.w3c.dom.traversal.NodeFilter;
import org.w3c.dom.traversal.NodeIterator;

public class SpeckleToFEMXMLInputGenerator {

	String xml = null;

	public SpeckleToFEMXMLInputGenerator(JSONObject obj) throws Exception {
		DocumentBuilderFactory docFactory = DocumentBuilderFactory
				.newInstance();
		DocumentBuilder docBuilder = docFactory.newDocumentBuilder();
		
		String modelName = (String) obj.get("MODELNAME");
		// root elements
		Document doc = docBuilder.newDocument();
		Element rootElement = doc.createElement("SEGMENTATION");
		doc.appendChild(rootElement);

		Attr attr = doc.createAttribute("chamber");
		attr.setValue("LV");
		rootElement.setAttributeNode(attr);
		attr = doc.createAttribute("name");
		attr.setValue(modelName);
		rootElement.setAttributeNode(attr);

		Element docElem = doc.createElement("MODELNAME");
		docElem.appendChild(doc.createTextNode(modelName));
		rootElement.appendChild(docElem);
		docElem = doc.createElement("NUMBEROFFRAMES");
		docElem.appendChild(doc.createTextNode((String) obj.get("NUMBEROFFRAMES")));
		rootElement.appendChild(docElem);
		docElem = doc.createElement("DURATION");
		docElem.appendChild(doc.createTextNode((String) obj.get("DURATION")));
		rootElement.appendChild(docElem);
		String[] landMarks = { "BASE", "APEX", "GELEM_CENTROID" };
		for (int i = 0; i < landMarks.length; i++) {
			docElem = doc.createElement(landMarks[i]);
			addCoordinate(doc, docElem, (JSONArray) obj.get(landMarks[i]));
			rootElement.appendChild(docElem);
		}
		docElem = doc.createElement("RVINSERTS");
		JSONArray rvs = (JSONArray) obj.get("RVINSERTS");
		for (int i = 0; i < rvs.size(); i++) {
			Element rvins = doc.createElement("INSERT");
			addCoordinate(doc, rvins, (JSONArray) rvs.get(i));
			docElem.appendChild(rvins);
		}
		rootElement.appendChild(docElem);

		JSONArray views = (JSONArray) obj.get("views");
		JSONArray types = (JSONArray) obj.get("types");
		if (views.size() < 1)
			throw new Exception("Speckle Fitting input has no views!!");
		for (int i = 0; i < views.size(); i++) {
			String viewName = (String) views.get(i);
			for (int j = 0; j < types.size(); j++) {
				String type = (String) types.get(j);
				try {
					JSONObject view = (JSONObject) obj.get(viewName);
					JSONObject marks = (JSONObject) view.get(type);
					boolean target = false;
					if (marks.get("TARGET") != null) {
						if (((String) marks.get("TARGET"))
								.equalsIgnoreCase("YES")) {
							target = true;
						}
					}
					// Add markers to the root
					addMarkers(doc, rootElement, viewName, type, target,(JSONArray) marks.get("MARKERS"));

					// Create an element to contain view data
					docElem = doc.createElement("VIEWMETADATA");

					attr = doc.createAttribute("VIEW");
					attr.setValue(viewName);
					docElem.setAttributeNode(attr);

					attr = doc.createAttribute("TYPE");
					attr.setValue(type);
					docElem.setAttributeNode(attr);

					attr = doc.createAttribute("BPM");
					attr.setValue((String) marks.get("BPM"));
					docElem.setAttributeNode(attr);

					attr = doc.createAttribute("EC");
					attr.setValue((String) marks.get("EC"));
					docElem.setAttributeNode(attr);

					attr = doc.createAttribute("ED");
					attr.setValue((String) marks.get("ED"));
					docElem.setAttributeNode(attr);

					Element uri = doc.createElement("URI");
					uri.appendChild(doc.createTextNode((String) marks
							.get("URI")));
					docElem.appendChild(uri);

					rootElement.appendChild(docElem);
				} catch (Exception exx) {
					exx.printStackTrace();
				}
			}
		}
		
		//If strains were selected pass that data
		JSONObject sselection = (JSONObject) obj.get("selectedStrains");
		if(sselection!=null){
			String[] strains = new String[18];
			String[] skeys = (String [])sselection.keySet().toArray(strains);
			if(skeys.length>0){
				docElem = doc.createElement("SELECTEDSTRAINS");
				for(int ski=0;ski<skeys.length;ski++){
					if(skeys[ski]==null)//As there can be less than 16 strains
						break;
					Boolean bol = (Boolean)sselection.get(skeys[ski]);
					if(bol){
						Element strain = doc.createElement("STRAIN");
						strain.appendChild(doc.createTextNode(skeys[ski]));
						docElem.appendChild(strain);
					}
				}
				rootElement.appendChild(docElem);
			}
		}
		try{
			TransformerFactory transformerFactory = TransformerFactory
					.newInstance();
			Transformer transformer = transformerFactory.newTransformer();
			DOMSource source = new DOMSource(doc);
			StringWriter stringWriter = new StringWriter();
			StreamResult result = new StreamResult(stringWriter);
			transformer.transform(source, result);
	
			xml = stringWriter.getBuffer().toString();
		}catch(Exception exx){
			//Check for null elements
			
			Logger log = Logger.getLogger(this.getClass().getSimpleName());
			log.log(Level.SEVERE,"Error occured while traversing xml record: ");
			DocumentTraversal traversal = (DocumentTraversal) doc;

	        NodeIterator iterator = traversal.createNodeIterator(
	          doc.getDocumentElement(), NodeFilter.SHOW_ELEMENT, null, true);

	        for (Node n = iterator.nextNode(); n != null; n = iterator.nextNode()) {
	            //System.out.println("Element: " + ((Element) n).getTagName());
	            String tagname = ((Element) n).getTagName();
	            NamedNodeMap map = ((Element)n).getAttributes();
	            if(map.getLength() > 0) {
	                for(int i=0; i<map.getLength(); i++) {
	                    Node node = map.item(i);
	                    log.log(Level.SEVERE,node.getNodeName() + "=" + node.getNodeValue());
	                }
	                log.log(Level.SEVERE,tagname + "=" + ((Element)n).getTextContent());
	            }
	            else {
	            	log.log(Level.SEVERE,tagname + "=" + ((Element)n).getTextContent());
	            }
	        }
	        throw exx;
		}
	}

	private Element addCoordinate(Document doc, Element parent, JSONArray coord) {
		Element x = doc.createElement("x");

		Object obj = coord.get(0);
		x.appendChild(doc.createTextNode("" + obj));
		obj = coord.get(1);
		Element y = doc.createElement("y");
		y.appendChild(doc.createTextNode("" + obj));
		Element z = doc.createElement("z");
		z.appendChild(doc.createTextNode("0"));

		parent.appendChild(x);
		parent.appendChild(y);
		parent.appendChild(z);

		return parent;
	}

	private Element addMarkers(Document doc, Element parent, String view,
			String type, boolean target, JSONArray marks) {
		for (int i = 0; i < marks.size(); i++) {
			Element mark = doc.createElement("MARKERS");
			Attr attr = doc.createAttribute("FRAME");
			attr.setValue("" + i);
			mark.setAttributeNode(attr);
			attr = doc.createAttribute("TYPE");
			attr.setValue(type);
			mark.setAttributeNode(attr);
			attr = doc.createAttribute("VIEW");
			attr.setValue(view);
			mark.setAttributeNode(attr);
			if (target) {
				attr = doc.createAttribute("TARGET");
				attr.setValue("YES");
				mark.setAttributeNode(attr);
			}
			addFrame(doc, mark, (JSONArray) marks.get(i));
			parent.appendChild(mark);
		}
		return parent;
	}

	private Element addFrame(Document doc, Element parent, JSONArray marks) {
		for (int i = 0; i < marks.size(); i++) {
			Element mark = doc.createElement("MARK");
			Attr attr = doc.createAttribute("ID");
			attr.setValue("" + i);
			mark.setAttributeNode(attr);
			JSONArray coord = (JSONArray) marks.get(i);
			if (coord.size() > 2) {// If there is a third element in the
									// coordinate it is unselected
				attr = doc.createAttribute("SELECT");
				attr.setValue("false");
				mark.setAttributeNode(attr);
			}
			addCoordinate(doc, mark, (JSONArray) marks.get(i));
			parent.appendChild(mark);
		}
		Attr attr = doc.createAttribute("NOOFMARKERS");
		attr.setValue("" + marks.size());
		parent.setAttributeNode(attr);
		return parent;
	}

	public String getXML() {
		return xml;
	}

	public static void main(String args[]) throws Exception {
		Path path = Paths.get("/home/data/ICMASCRATCH/speckle.json");
		String json = new String(Files.readAllBytes(path), "UTF-8");
		JSONParser parser = new JSONParser();
		JSONObject obj = (JSONObject) parser.parse(json);
		SpeckleToFEMXMLInputGenerator spec = new SpeckleToFEMXMLInputGenerator(
				obj);
		System.out.println(spec.getXML());
	}

}
