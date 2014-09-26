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
package nz.ac.auckland.abi.workflow;

import java.io.StringWriter;
import java.util.Enumeration;
import java.util.Hashtable;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class WSSpeckleTrackingXMLInputEncoder {

	Hashtable<String, String[]> views;
	Hashtable<String, String[]> esframes;
	String modelName;
	int numberOfFrames;

	double[] apex;
	double[] base;
	double[] rvins;
	
	public WSSpeckleTrackingXMLInputEncoder(){
		views = new Hashtable<String, String[]>();
		esframes = new Hashtable<String, String[]>();
		modelName = null;
		apex = new double[2];
		base = new double[2];
		rvins = new double[2];
		numberOfFrames = 19;
	}
	
	public void addView(String viewName, String patientID, String studyID, String seriesID, String objectID, String markers, String edframe, String ecframe, String bpm, String wadoHost, String wadoPort, boolean target){

		String imageURL = "http://" + wadoHost + ":"
				+ wadoPort + "/wado?requestType=WADO&studyUID="
				+ studyID + "&seriesUID="
				+ seriesID + "&objectUID=" + objectID +"&contentType=application/dicom";
		String[] mark = markers.split(","); //Markers are expected to be comma separated basel->apex->baser + apex diff total of 10
		String[] rec = null;
		if(bpm==null)
			rec = new String[8];
		else{
			rec = new String[9];
			rec[8] = bpm;
		}
		
		rec[0] = imageURL;
		
		if(!viewName.startsWith("SAX")){
			double targApex_x ;
			double targApex_y;
			double targBase_x;
			double targBase_y; 

			if(mark.length==18){
				targApex_x = Double.parseDouble(mark[8]);
				targApex_y = Double.parseDouble(mark[9]);
				targBase_x = 0.5*(Double.parseDouble(mark[0]) + Double.parseDouble(mark[16]));
				targBase_y = 0.5*(Double.parseDouble(mark[1]) + Double.parseDouble(mark[17])); 
			}else{
				targApex_x = Double.parseDouble(mark[2]);
				targApex_y = Double.parseDouble(mark[3]);
				targBase_x = 0.5*(Double.parseDouble(mark[0]) + Double.parseDouble(mark[4]));
				targBase_y = 0.5*(Double.parseDouble(mark[1]) + Double.parseDouble(mark[5])); 	
			}
			double rvInsety_x = Double.parseDouble(mark[0]);
			double rvInsety_y = Double.parseDouble(mark[1]);
			
			if(target || (apex[0]==0 && base[0] == 0)){
				apex[0] = targApex_x; apex[1] = targApex_y;
				base[0] = targBase_x; base[1] = targBase_y;
				rvins[0] = rvInsety_x; rvins[1] = rvInsety_y;
			}
			
			rec[1] = targApex_x+","+targApex_y;
			rec[2] = targBase_x+","+targBase_y;
			rec[3] = rvInsety_x+","+rvInsety_y;
		
		}
		rec[4] = markers;
		if (target) {
			rec[5] = "YES";
		} else {
			rec[5] = "NO";
		}
		rec[6] = edframe;
		rec[7] = ecframe;
		views.put(viewName, rec);
	}
	
	public void addESFrameData(String viewName,String esframe,String markers){
		String[] rec = new String[2];
		rec[0] = esframe;
		rec[1] = markers;
		esframes.put(viewName, rec);
	}
	
	
	public void setModelName(String modelName) {
		this.modelName = modelName;
	}

	public void addCoordinate(Document doc, Element parent, String x, String y, String z){
		Element xe = doc.createElement("x");
		Element ye = doc.createElement("y");
		Element ze = doc.createElement("z");
		xe.appendChild(doc.createTextNode(x));
		ye.appendChild(doc.createTextNode(y));
		ze.appendChild(doc.createTextNode(z));
		parent.appendChild(xe);
		parent.appendChild(ye);
		parent.appendChild(ze);
	}
	
	public String getXML() throws Exception {
		DocumentBuilderFactory docFactory = DocumentBuilderFactory
				.newInstance();
		DocumentBuilder docBuilder = docFactory.newDocumentBuilder();

		// root element
		Document doc = docBuilder.newDocument();
		Element rootElement = doc.createElement("SEGMENTATION");
		doc.appendChild(rootElement);

		Element apE = doc.createElement("APEX");
		addCoordinate(doc, apE, ""+apex[0] , ""+apex[1], "0");
		rootElement.appendChild(apE);
		
		Element bsE = doc.createElement("BASE");
		addCoordinate(doc, bsE, ""+base[0] , ""+base[1], "0");
		rootElement.appendChild(bsE);
		
		Element rvI = doc.createElement("RVINSERTS");
		Element ins = doc.createElement("INSERT");
		addCoordinate(doc, ins, ""+rvins[0] , ""+rvins[1], "0");
		rvI.appendChild(ins);
		rootElement.appendChild(rvI);
		
		if(modelName!=null){
			Element model = doc.createElement("MODELNAME");
			model.appendChild(doc.createTextNode(modelName));
			rootElement.appendChild(model);
		}
		
		//Set target height and width; Note the WebUI assumes a 800x600 canvas
		
		Element targetHeight = doc.createElement("TARGETHEIGHT");
		targetHeight.appendChild(doc.createTextNode("600"));
		rootElement.appendChild(targetHeight);

		Element targetWidth = doc.createElement("TARGETWIDTH");
		targetWidth.appendChild(doc.createTextNode("800"));
		rootElement.appendChild(targetWidth);

		
		// View elements
		Enumeration<String> viewKeys = views.keys();
		while (viewKeys.hasMoreElements()) {
			String viewName = viewKeys.nextElement();
			String[] values = views.get(viewName);
			Element view = doc.createElement("MARKERS");
			Element url = doc.createElement("URI");
			//Store the url
			url.appendChild(doc.createTextNode(values[0]));
			view.appendChild(url);
			
			Attr vname = doc.createAttribute("VIEW");
			vname.setValue(viewName);
			view.setAttributeNode(vname);


			if(values.length>5){
				if(values[5]!=null && values[5].equalsIgnoreCase("YES")){
					Attr targ = doc.createAttribute("TARGET");
					targ.setValue("YES");
					view.setAttributeNode(targ);
				}
				if(values.length>6){
					Attr targ = doc.createAttribute("ED");
					targ.setValue(values[6]);
					view.setAttributeNode(targ);
					Attr targ1 = doc.createAttribute("EC");
					targ1.setValue(values[7]);
					view.setAttributeNode(targ1);
				}
				if(values.length>=8){
					Attr targ1 = doc.createAttribute("BPM");
					targ1.setValue(values[8]);
					view.setAttributeNode(targ1);
				}
			}
			
			String[] marks = values[4].split(",");
			int maxIndex = marks.length;
			int ctr = 1;
			for(int i=0;i<maxIndex;i+=2){ 
				Element mark = doc.createElement("MARKS");
				Attr targ = doc.createAttribute("ID");
				targ.setValue(""+ctr);
				mark.setAttributeNode(targ);
				addCoordinate(doc, mark, marks[i], marks[i+1], "0");
				//System.out.println(ctr+"\t"+ marks[i]+", "+ marks[i+1]);
				view.appendChild(mark);
				ctr++;
			}
			//Check if view has esdata
			if(esframes.containsKey(viewName)){
				String[] rec = esframes.get(viewName);
				Element es = doc.createElement("ESFRAME");
				Attr frameno = doc.createAttribute("ESFRAMENO");
				frameno.setValue(rec[0]);
				es.setAttributeNode(frameno);
				String[] mks = rec[1].split(",");
				int mIndex = mks.length;
				//Only the first and last pairs, ignore apex
				{ 
					Element mark = doc.createElement("MARKS");
					Attr targ = doc.createAttribute("ID");
					targ.setValue("1");
					mark.setAttributeNode(targ);
					addCoordinate(doc, mark, mks[0], mks[1], "0");
					es.appendChild(mark);
				}
				{ 
					Element mark = doc.createElement("MARKS");
					Attr targ = doc.createAttribute("ID");
					targ.setValue("2");
					mark.setAttributeNode(targ);
					addCoordinate(doc, mark, mks[mIndex-2], mks[mIndex-1], "0");
					es.appendChild(mark);
				}
				view.appendChild(es);
			}
			rootElement.appendChild(view);
		}

		// write the content into xml file
		TransformerFactory transformerFactory = TransformerFactory
				.newInstance();
		Transformer transformer = transformerFactory.newTransformer();
		DOMSource source = new DOMSource(doc);
		StringWriter stringWriter = new StringWriter();
		StreamResult result = new StreamResult(stringWriter);

		// Output to console for testing
		// StreamResult result = new StreamResult(System.out);

		transformer.transform(source, result);

		return stringWriter.getBuffer().toString();
	}

	public String getModelName() {
		return modelName;
	}
	
	
	public int getNumberOfFrames() {
		return numberOfFrames;
	}

	public void setNumberOfFrames(int numberOfFrames) {
		this.numberOfFrames = numberOfFrames;
	}
	
}
