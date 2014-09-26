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

import java.io.File;
import java.io.PrintWriter;
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

public class SegmentationXMLWrapper {
	String filename = null;
	Hashtable<String, String[]> views;
	String outputDirectory;
	String modelName;
	int numberOfFrames;
	String dicomFile = null;
	
	double[] apex;
	double[] base;
	double[] rvins;
	 
	public SegmentationXMLWrapper(String filename) {
		this.filename = filename;
		views = new Hashtable<String, String[]>();
		outputDirectory = null;
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
			//double targApex_x = Double.parseDouble(mark[8]) + Double.parseDouble(mark[18]);
			//double targApex_y = Double.parseDouble(mark[9]) + Double.parseDouble(mark[19]);
			double targApex_x = Double.parseDouble(mark[8]);
			double targApex_y = Double.parseDouble(mark[9]);
			double targBase_x = 0.5*(Double.parseDouble(mark[0]) + Double.parseDouble(mark[16]));
			double targBase_y = 0.5*(Double.parseDouble(mark[1]) + Double.parseDouble(mark[17]));
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
	
	
	

	public void setModelName(String modelName) {
		this.modelName = modelName;
	}

	public void setOutputDirectory(String outDir) {
		outputDirectory = outDir;
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
		
		if(outputDirectory!=null){
			Element directory = doc.createElement("OUTPUTDIRECTORY");
			directory.appendChild(doc.createTextNode(outputDirectory));
			rootElement.appendChild(directory);
		}

		if(modelName!=null){
			Element model = doc.createElement("MODELNAME");
			model.appendChild(doc.createTextNode(modelName));
			rootElement.appendChild(model);
		}
		
		if(dicomFile!=null){
			Element model = doc.createElement("DICOMFILE");
			model.appendChild(doc.createTextNode(dicomFile));
			rootElement.appendChild(model);
		}
		
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
/*			int maxIndex = marks.length - 2; //Ignore the last two as they are diff values
			if(viewName.startsWith("SAX")) //SAX View does not have the bias
				maxIndex += 2;*/
			int maxIndex = marks.length;
			int ctr = 1;
			for(int i=0;i<maxIndex;i+=2){ 
				Element mark = doc.createElement("MARKS");
				Attr targ = doc.createAttribute("ID");
				targ.setValue(""+ctr);
				mark.setAttributeNode(targ);
				addCoordinate(doc, mark, marks[i], marks[i+1], "0");
				view.appendChild(mark);
				ctr++;
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

	public String serialize() throws Exception {
		String xml = getXML();
		PrintWriter pr = new PrintWriter(new File(filename));
		pr.println(xml);
		pr.close();
		return xml;
	}

	public String getOutputDirectory() {
		return outputDirectory;
	}

	public String getModelName() {
		return modelName;
	}
	
	public String getXMLFilename(){
		return filename;
	}
	
	public int getNumberOfFrames() {
		return numberOfFrames;
	}

	public void setNumberOfFrames(int numberOfFrames) {
		this.numberOfFrames = numberOfFrames;
	}
	
	public String getDicomFile() {
		return dicomFile;
	}

	public void setDicomFile(String dicomFile) {
		this.dicomFile = dicomFile;
	}
}
