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

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.StringWriter;
import java.net.URL;
import java.net.URLConnection;
import java.nio.channels.Channels;
import java.nio.channels.ReadableByteChannel;
import java.util.Enumeration;
import java.util.Hashtable;

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

import com.sun.org.apache.xml.internal.security.utils.Base64;

public class WSMRIModelFittingXMLInputEncoder {

	Hashtable<String, String[]> frames;
	Hashtable<String, String[]> edlm;
	Hashtable<String, String[]> eslm;
	Hashtable<String, String> edTime;
	Hashtable<String, String> esTime;
	Hashtable<String, String> endCycle;
	String server;
	Hashtable<String, String[]> tlc;
	Hashtable<String, String[]> trc;
	Hashtable<String, String[]> blc;
	Hashtable<String, String[]> brc;
	Hashtable<String, String> seriesid;
	Hashtable<String, String> type;
	Hashtable<String, String> height;
	Hashtable<String, String> width;

	String modelName;

	public WSMRIModelFittingXMLInputEncoder() {
		modelName = null;
		frames = new Hashtable<String, String[]>();
		edlm = new Hashtable<String, String[]>();
		eslm = new Hashtable<String, String[]>();
		edTime = new Hashtable<String, String>();
		esTime = new Hashtable<String, String>();
		endCycle = new Hashtable<String, String>();
		server = null;
		tlc = new Hashtable<String, String[]>();
		blc = new Hashtable<String, String[]>();
		trc = new Hashtable<String, String[]>();
		brc = new Hashtable<String, String[]>();
		seriesid = new Hashtable<String, String>();
		type = new Hashtable<String, String>();
		width = new Hashtable<String, String>();
		height = new Hashtable<String, String>();
	}

	public void addPlane(String planeid, String[] frames, String[] tlc, String[] trc, String[] blc, String[] brc, String seriesid) {
		this.frames.put(planeid, frames);
		this.tlc.put(planeid, tlc);
		this.trc.put(planeid, trc);
		this.blc.put(planeid, blc);
		this.brc.put(planeid, brc);
		this.seriesid.put(planeid, seriesid);
	}

	public void addPlaneType(String planeid, String type) {
		this.type.put(planeid, type);
	}

	public void addPlaneEDTime(String planeid, String edTime) {
		this.edTime.put(planeid, edTime);
	}

	public void addPlaneESTime(String planeid, String esTime) {
		this.esTime.put(planeid, esTime);
	}

	public void addPlaneENDCycleTime(String planeid, String endTime) {
		this.endCycle.put(planeid, endTime);
	}

	public void addPlaneEDLM(String planeid, String[] edlm) {
		this.edlm.put(planeid, edlm);
	}

	public void addPlaneESLM(String planeid, String[] eslm) {
		this.eslm.put(planeid, eslm);
	}

	public void addServer(String server) {
		this.server= server;
	}
	
	public void addPlaneHeight(String planeid, String height){
		this.height.put(planeid,height);
	}
	
	public void addPlaneWidth(String planeid, String width){
		this.width.put(planeid,width);
	}
	
	public void setModelName(String modelName) {
		this.modelName = modelName;
	}

	public void addCoordinate(Document doc, Element parent, String x, String y, String z) {
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
		DocumentBuilderFactory docFactory = DocumentBuilderFactory.newInstance();
		DocumentBuilder docBuilder = docFactory.newDocumentBuilder();

		// root element
		Document doc = docBuilder.newDocument();
		Element rootElement = doc.createElement("SEGMENTATION");
		doc.appendChild(rootElement);

		if (modelName != null) {
			Element model = doc.createElement("MODELNAME");
			model.appendChild(doc.createTextNode(modelName));
			rootElement.appendChild(model);
		}
		if (server != null) {
			Element se = doc.createElement("SERVER");
			se.appendChild(doc.createTextNode(server));
			rootElement.appendChild(se);
		}
		// View elements
		Enumeration<String> viewKeys = frames.keys();
		while (viewKeys.hasMoreElements()) {
			String viewName = viewKeys.nextElement();
			String[] values = frames.get(viewName);
			Element planes = doc.createElement("PLANES");
			rootElement.appendChild(planes);
			Element planeid = doc.createElement("PLANEID");
			planeid.appendChild(doc.createTextNode(viewName));
			planes.appendChild(planeid);
			Element seriesid = doc.createElement("SERIESID");
			seriesid.appendChild(doc.createTextNode(this.seriesid.get(viewName)));
			planes.appendChild(seriesid);

			Element framesE = doc.createElement("FRAMES");
			for (int fc = 0; fc < values.length; fc++) {
				Element frameE = doc.createElement("FRAME");
				Attr ID = doc.createAttribute("ID");
				ID.setValue("" + fc);
				frameE.setAttributeNode(ID);
				frameE.appendChild(doc.createTextNode(values[fc]));
				framesE.appendChild(frameE);
			}
			{
				Element trc = doc.createElement("TRC");
				String[] coord = this.trc.get(viewName);
				addCoordinate(doc, trc, coord[0], coord[1], coord[2]);
				framesE.appendChild(trc);
			}
			{
				Element blc = doc.createElement("BLC");
				String[] coord = this.blc.get(viewName);
				addCoordinate(doc, blc, coord[0], coord[1], coord[2]);
				framesE.appendChild(blc);
			}
			{
				Element tlc = doc.createElement("TLC");
				String[] coord = this.tlc.get(viewName);
				addCoordinate(doc, tlc, coord[0], coord[1], coord[2]);
				framesE.appendChild(tlc);
			}
			{
				Element brc = doc.createElement("BRC");
				String[] coord = this.brc.get(viewName);
				addCoordinate(doc, brc, coord[0], coord[1], coord[2]);
				framesE.appendChild(brc);
			}
			{
				// Set target height and width; Note the WebUI assumes a 256x256 canvas

				Element targetHeight = doc.createElement("TARGETHEIGHT");
				if(height.containsKey(viewName))
					targetHeight.appendChild(doc.createTextNode(height.get(viewName)));
				else
					targetHeight.appendChild(doc.createTextNode("256"));
				framesE.appendChild(targetHeight);

				Element targetWidth = doc.createElement("TARGETWIDTH");
				if(width.containsKey(viewName))
					targetWidth.appendChild(doc.createTextNode(width.get(viewName)));
				else
					targetWidth.appendChild(doc.createTextNode("256"));
				framesE.appendChild(targetWidth);
			}
			
			Element numframes = doc.createElement("NUMFRAMES");
			numframes.appendChild(doc.createTextNode("" + values.length));
			framesE.appendChild(numframes);

			if (this.edlm.containsKey(viewName)) {
				Element markers = doc.createElement("MARKERS");
				Element EDLM = doc.createElement("EDLM");
				String[] edlm = this.edlm.get(viewName);
				for (int i = 0; i < edlm.length; i++) {
					Element mark = doc.createElement("MARK");
					String[] toks = edlm[i].split(",");
					addCoordinate(doc, mark, toks[0], toks[1],toks[2]);
					EDLM.appendChild(mark);
				}
				markers.appendChild(EDLM);
				if (this.eslm.containsKey(viewName)) {
					String[] eslm = this.eslm.get(viewName);
					Element ESLM = doc.createElement("ESLM");
					for (int i = 0; i < eslm.length; i++) {
						Element mark = doc.createElement("MARK");
						String[] toks = eslm[i].split(",");
						addCoordinate(doc, mark, toks[0], toks[1],toks[2]);
						ESLM.appendChild(mark);
					}
					markers.appendChild(ESLM);
				}
				framesE.appendChild(markers);
			}
			planes.appendChild(framesE);

			if (edTime.containsKey(viewName)) {
				String val = edTime.get(viewName);
				Element edtime = doc.createElement("EDTIME");
				edtime.appendChild(doc.createTextNode(val));
				planes.appendChild(edtime);
			}

			if (esTime.containsKey(viewName)) {
				String val = esTime.get(viewName);
				Element estime = doc.createElement("ESTIME");
				estime.appendChild(doc.createTextNode(val));
				planes.appendChild(estime);
			}

			if (endCycle.containsKey(viewName)) {
				String val = endCycle.get(viewName);
				Element endcycle = doc.createElement("ENDCYCLE");
				endcycle.appendChild(doc.createTextNode(val));
				planes.appendChild(endcycle);
			}

			if (type.containsKey(viewName)) {
				String val = type.get(viewName);
				Element etype = doc.createElement("TYPE");
				etype.appendChild(doc.createTextNode(val));
				planes.appendChild(etype);
			}

			rootElement.appendChild(planes);
		}

		// write the content into xml file
		TransformerFactory transformerFactory = TransformerFactory.newInstance();
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

	public static void main(String args[]) throws Exception {
		String json = null;
		BufferedReader br = new BufferedReader(new FileReader(new File(args[0])));
		StringBuffer buf = new StringBuffer();
		while ((json = br.readLine()) != null) {
			buf.append(json);
		}
		br.close();
		String workDir = "/home/rjag008/ClevelandClinic/CPP/LVModelBasedMRIModelFitting/Debug/mriimages";
		File wDir = new File(workDir,"MRIImages0");
		int fctr =1;
		while(wDir.exists()){
			wDir = new File(workDir,"MRIImages"+fctr++);
		}
		wDir.mkdirs();
		fctr = 0;
/*		Authenticator.setDefault (new Authenticator() {
		    protected PasswordAuthentication getPasswordAuthentication() {
		        return new PasswordAuthentication ("jdt1955", "icma".toCharArray());
		    }
		});*/
		
		json = buf.toString();
		JSONParser parser = new JSONParser();
		JSONObject jsonObject = (JSONObject) parser.parse(json);
		WSMRIModelFittingXMLInputEncoder encoder = new WSMRIModelFittingXMLInputEncoder();
		encoder.setModelName((String) jsonObject.get("MODELNAME"));
		String studyid = (String) jsonObject.get("studyUID");
		String patients = (String) jsonObject.get("patientID");

		JSONArray jsonArray = (JSONArray) jsonObject.get("planes");
		if (jsonArray != null) {
			int len = jsonArray.size();
			for (int i = 0; i < len; i++) {
				JSONObject planes = (JSONObject) jsonArray.get(i);
				String edtime = ""+((Double) planes.get("edtime"));
				String estime = ""+((Double) planes.get("estime"));
				String endcycle = ""+((Double) planes.get("endcycle"));
				String server = (String) planes.get("server");
				String seriesid = (String) planes.get("seriesid");
				String type = (String) planes.get("type");
				String planeid = (String) planes.get("planeid");
				JSONObject frames = (JSONObject) planes.get("frames");
				int numframes = Integer.parseInt((String)frames.get("NUMFRAMES"));
				String[] urls = new String[numframes];
				for(int j=0;j<numframes;j++){
					URL imgurl = new URL(server+frames.get(""+j));
					URLConnection uc = imgurl.openConnection();
					String authorizationString = "Basic" + Base64.encode("jdt1955:icma".getBytes());
					uc.setRequestProperty ("Authorization", authorizationString);
					File imgFile = new File(wDir,"Image"+(fctr++));
					ReadableByteChannel rbc = Channels.newChannel(imgurl.openStream());
			        FileOutputStream fos = new FileOutputStream(imgFile);
			        fos.getChannel().transferFrom(rbc, 0, Long.MAX_VALUE);
			        fos.close();
			        rbc.close();
					urls[j] = imgFile.getName();
				}
				String fwidth = (String) planes.get("WIDTH");
				String fheight = (String) planes.get("HEIGHT");
				String[] tlc, blc, trc, brc;
				trc = ((String)frames.get("TRC")).split(",");
				blc = ((String)frames.get("BLC")).split(",");
				tlc = ((String)frames.get("TLC")).split(",");
				brc = ((String)frames.get("BRC")).split(",");
				String[] edlm=null, eslm=null;
				JSONArray edlmA = (JSONArray) planes.get("EDLM");
				if (edlmA != null) {
					int size = edlmA.size();
					edlm = new String[size];
					for(int ei=0;ei<size;ei++){
						JSONArray coord = (JSONArray)edlmA.get(ei);
						edlm[ei] = coord.get(0)+","+coord.get(1)+","+coord.get(2);
					}
				}
				JSONArray eslmA = (JSONArray) planes.get("ESLM");
				if (eslmA != null) {
					int size = eslmA.size();
					eslm = new String[size];
					for(int ei=0;ei<size;ei++){
						JSONArray coord = (JSONArray)eslmA.get(ei);
						eslm[ei] = coord.get(0)+","+coord.get(1)+","+coord.get(2);
					}
				}
				encoder.addPlane(planeid, urls, tlc, trc, blc, brc, seriesid);
				if(edlm!=null)
					encoder.addPlaneEDLM(planeid, edlm);
				if(eslm!=null)
					encoder.addPlaneESLM(planeid, eslm);
				if(!endcycle.equalsIgnoreCase("null"))
					encoder.addPlaneENDCycleTime(planeid, endcycle);
				if(!edtime.equalsIgnoreCase("null"))
					encoder.addPlaneEDTime(planeid, edtime);
				if(!estime.equalsIgnoreCase("null"))
					encoder.addPlaneESTime(planeid, estime);
				if(type!=null)
					encoder.addPlaneType(planeid, type);
				if(fwidth!=null)
					encoder.addPlaneWidth(planeid, fwidth);
				if(fheight!=null)
					encoder.addPlaneHeight(planeid, fheight);
			}
			encoder.addServer(wDir.getAbsolutePath());
		}
		System.out.println(encoder.getXML());

	}

}
