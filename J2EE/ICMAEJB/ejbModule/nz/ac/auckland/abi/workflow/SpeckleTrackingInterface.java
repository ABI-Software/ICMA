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

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.FileOutputStream;
import java.io.StringWriter;
import java.net.URL;
import java.util.Hashtable;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.zip.GZIPInputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import javax.naming.Context;
import javax.naming.InitialContext;
import javax.xml.namespace.QName;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import javax.xml.ws.BindingProvider;
import javax.xml.ws.Service;

import nz.ac.auckland.abi.speckletracking.wsclient.SpeckleTrackingService;

import org.w3c.dom.Document;

public class SpeckleTrackingInterface {

	
	private String serviceURL; // = "http://localhost:8080/SpeckleTrackingWS/SpeckleTrackingService?wsdl";//This should be obtained from config

	private SpeckleTrackingService st = null;
	private Logger log = null;
	
	public SpeckleTrackingInterface() throws Exception{
		Context context = new InitialContext();
		serviceURL = (String)context.lookup("java:global/ICMASPECKLETRACKINGWS");
		log = Logger.getLogger(this.getClass().getSimpleName());
		log.log(Level.FINE,"Speckle Fitting URL "+serviceURL);
		URL wsdlLocation = new URL(serviceURL);
		QName serviceName = new QName("www.abi.auckland.ac.nz/CCF", "SpeckleTrackingServiceService");
		Service service = Service.create(wsdlLocation, serviceName);
		st = service.getPort(SpeckleTrackingService.class);
		//Enable cookie, to make the web service stateful
		((BindingProvider)st).getRequestContext().put(BindingProvider.SESSION_MAINTAIN_PROPERTY, true);
	}
	
/*	public void submitJob(String xmlDescription,Hashtable<String,byte[]> viewData) throws Exception{
		//Store the displacement data using viewName
		Enumeration<String> views = viewData.keys();
		ArrayList<String> viewNames = new ArrayList<String>();
		ArrayList<byte[]> files = new ArrayList<byte[]>();
		while(views.hasMoreElements()){
			String viewName = (String) views.nextElement();
			viewNames.add(viewName);
			files.add(viewData.get(viewName));
		}
		
        
        //Enable MTOM
        BindingProvider bindingProvider = ((BindingProvider)st);
        SOAPBinding sopadBinding = (SOAPBinding) bindingProvider.getBinding();
        sopadBinding.setMTOMEnabled(true);
		st.submitWithDisplacements(xmlDescription,viewNames,files);
	}*/
	
	public void submitJob(String xmlDescription) throws Exception{
		st.submit(xmlDescription);
	}
	
	public void saveDicom(String filename) throws Exception{
		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		GZIPInputStream gis = new GZIPInputStream(new ByteArrayInputStream(st.getDicom()));
		byte[] buffer = new byte[64536];
		int length = 0;
		while((length=gis.read(buffer))>0)
			bos.write(buffer, 0, length);
		gis.close();
		
		FileOutputStream fos = new FileOutputStream(filename);
		fos.write(bos.toByteArray());
		fos.close();
	}
	
	public byte[] getDicom() throws Exception{
		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		GZIPInputStream gis = new GZIPInputStream(new ByteArrayInputStream(st.getDicom()));
		byte[] buffer = new byte[64536];
		int length = 0;
		while((length=gis.read(buffer))>0)
			bos.write(buffer, 0, length);
		gis.close();
		return bos.toByteArray();
	}
	
	public Hashtable<String,byte[]> getImages() throws Exception{
		Hashtable<String, byte[]> images = new Hashtable<String, byte[]>();
		ByteArrayInputStream in = new ByteArrayInputStream(st.getImageArchive());
		ZipInputStream zis = new ZipInputStream(in);
		ZipEntry ze = zis.getNextEntry();
		byte[] buffer = new byte[84534];
		int len = 0;
		while(ze!=null){
			String filename = ze.getName();
			ByteArrayOutputStream bos = new ByteArrayOutputStream();
			while((len=zis.read(buffer))>0){
				bos.write(buffer, 0, len);
			}
			bos.close();
			images.put(filename, bos.toByteArray());
			zis.closeEntry();
			ze=zis.getNextEntry();
		}
		zis.close();
		return images;
	}
	
	
	public double getProgress(){
		return st.getCompletion();
	}
	
	public String getXML(){
		return st.getSpeckleTrackingXML();
	}

	public void cleanUp(){
		st.cleanUp();
	}
	
	public static void main(String args[]) throws Exception{
		String wDir = "/home/rjag008/clevelandclinic/SpeckleTracking/build/Testing";
		String xmlFile = wDir+"/1View_ST.xml";
		DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
		DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
		Document doc = dBuilder.parse(xmlFile);
		doc.getDocumentElement().normalize();
		
		TransformerFactory transformerFactory = TransformerFactory
				.newInstance();
		Transformer transformer = transformerFactory.newTransformer();
		DOMSource source = new DOMSource(doc);
		StringWriter stringWriter = new StringWriter();
		StreamResult result = new StreamResult(stringWriter);
		transformer.transform(source, result);

		String xml = stringWriter.getBuffer().toString();
		
		SpeckleTrackingInterface intf = new SpeckleTrackingInterface();
		intf.submitJob(xml);
		System.out.println("Submit completed");
		double progress = intf.getProgress();
		while(progress<1.0){
			System.out.println("Progress "+progress);
			try{
				Thread.sleep(10000);
			}catch(Exception exx){
				
			}
			progress = intf.getProgress();
		}
		intf.saveDicom(wDir+"/ws.dcm");
		System.out.println(intf.getXML());
		//intf.cleanUp();
	}
	
}
