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
package nz.ac.auckland.abi.dcm4chee;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.StringWriter;
import java.util.Properties;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.dcm4che.dict.Tags;
import org.dcm4che2.tool.dcm2xml.Dcm2Xml;
import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public class DicomFileHelper {

	static final String seriesUIDTag = "0020000E";
	static final String studyUIDTag  = "0020000D";
	static final String instanceUIDTag = "00080018";
	static final String patientIDTag   = "00100020";
	static final String imagePositionPatientTag = "00200032";
	static final String imageOrientationPatientTag   = "00200037";
	
	
	
	public static String getHeader(String filename) throws Exception{
		File infile = new File(filename);
		File outfile = new File(infile.getParent(),"headerInfo.xml");
		Dcm2Xml reader = new Dcm2Xml();
		reader.setExclude(new int[] {Tags.EncapsulatedDocument,Tags.PixelData, Tags.OverlayData});
		reader.setComments(true);
		reader.convert(infile, outfile);
		
		DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
		DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
		Document doc = dBuilder.parse(new FileInputStream(outfile));
		doc.getDocumentElement().normalize();
		
		StringWriter writer = new StringWriter();
	    StreamResult result = new StreamResult(writer);
	    TransformerFactory tf = TransformerFactory.newInstance();
	    Transformer transformer = tf.newTransformer();
	    transformer.transform(new DOMSource(doc), result);
		
	    outfile.delete();
	    
		return writer.toString();
	}
	
	public static Properties getIds(String xml) throws Exception{
		Properties prop = new Properties();
		DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
		DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
		Document doc = dBuilder.parse(new ByteArrayInputStream(xml.getBytes()));

		NodeList nList = doc.getElementsByTagName("attr");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			try{
				String content = ((Node) text.item(0)).getNodeValue().trim();
				NamedNodeMap attrs = nNode.getAttributes();
				for (int j = 0; j < attrs.getLength(); j++) {
					Attr attribute = (Attr) attrs.item(j);
					if (attribute.getName().trim().equalsIgnoreCase("tag")) {
						if(attribute.getValue().trim().equalsIgnoreCase(seriesUIDTag)){
							prop.put("SERIES", content);
						}else if(attribute.getValue().trim().equalsIgnoreCase(studyUIDTag)){
							prop.put("STUDY", content);
						}else if(attribute.getValue().trim().equalsIgnoreCase(instanceUIDTag)){
							prop.put("INSTANCE", content);
						}else if(attribute.getValue().trim().equalsIgnoreCase(patientIDTag)){
							prop.put("PATIENT", content);
						}else{
							break;
						}
					}
	
				}
			}catch(Exception exx){
				
			}
		}
		return prop;
	}
	
	public static Properties getImagePositionAndOrientation(String xml) throws Exception{
		Properties prop = new Properties();
		DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
		DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
		Document doc = dBuilder.parse(new ByteArrayInputStream(xml.getBytes()));

		NodeList nList = doc.getElementsByTagName("attr");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			try{
				String content = ((Node) text.item(0)).getNodeValue().trim();
				NamedNodeMap attrs = nNode.getAttributes();
				for (int j = 0; j < attrs.getLength(); j++) {
					Attr attribute = (Attr) attrs.item(j);
					if (attribute.getName().trim().equalsIgnoreCase("tag")) {
						if(attribute.getValue().trim().equalsIgnoreCase(imagePositionPatientTag)){
							prop.put("POSITION", content);
						}else if(attribute.getValue().trim().equalsIgnoreCase(imageOrientationPatientTag)){
							prop.put("ORIENTATION", content);
						}else{
							break;
						}
					}
	
				}
			}catch(Exception exx){
				
			}
		}
		return prop;
	}
	
	
	public static void main(String args[]) throws Exception{
		
		//getHeader("/people/rjag008/Desktop/test.dcm");
		getImagePositionAndOrientation(getHeader("/home/data/ICMASCRATCH/pip/082919590/MR0.dcm")).list(System.out);
	}
	
}
