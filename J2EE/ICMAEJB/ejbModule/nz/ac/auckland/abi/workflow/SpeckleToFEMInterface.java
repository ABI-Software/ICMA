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
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;

import javax.naming.Context;
import javax.naming.InitialContext;
import javax.xml.namespace.QName;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import javax.xml.ws.Service;

import nz.ac.auckland.abi.speckletofem.wsclient.SpeckleToFEMModelService;

import org.w3c.dom.Document;

public class SpeckleToFEMInterface {

	private String serviceURL; // =
								// "http://localhost:8080/SpeckleFittingWS/SpeckleToFEMModelService?wsdl";//This
								// should be obtained from config
	private String workflowID = null;
	URL wsdlLocation = null;
	QName serviceName = null;
	private Logger log = null;

	public SpeckleToFEMInterface() throws Exception {
		Context context = new InitialContext();
		serviceURL = (String) context.lookup("java:global/ICMASPECKLEFITTINGWS");
		log = Logger.getLogger(this.getClass().getSimpleName());
		log.log(Level.FINE, "Speckle Fitting URL " + serviceURL);
		wsdlLocation = new URL(serviceURL);
		serviceName = new QName("www.abi.auckland.ac.nz/CCF","SpeckleToFEMModelServiceService");
	}

	public void submitJob(String xmlDescription, byte[] dicom) {
		//Stateless invocation of service, create handle for every call
		Service service = Service.create(wsdlLocation, serviceName);
		SpeckleToFEMModelService st = service.getPort(SpeckleToFEMModelService.class);
		workflowID = st.submit(xmlDescription, dicom);
	}

	public void saveDicom(String filename) throws Exception {
		Service service = Service.create(wsdlLocation, serviceName);
		SpeckleToFEMModelService st = service.getPort(SpeckleToFEMModelService.class);
		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		GZIPInputStream gis = new GZIPInputStream(new ByteArrayInputStream(st.getDicom(workflowID)));
		byte[] buffer = new byte[64536];
		int length = 0;
		while ((length = gis.read(buffer)) > 0)
			bos.write(buffer, 0, length);
		gis.close();

		FileOutputStream fos = new FileOutputStream(filename);
		fos.write(bos.toByteArray());
		fos.close();
	}

	public byte[] getDicom() throws Exception {
		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		byte[] dicom = null;
		int ctr = 0;
		while (dicom == null && ctr < 5) {
			try {
				Service service = Service.create(wsdlLocation, serviceName);
				SpeckleToFEMModelService st = service.getPort(SpeckleToFEMModelService.class);
				dicom = st.getDicom(workflowID);
			} catch (Exception exx) {
				Logger log = Logger.getLogger(this.getClass().getSimpleName());
				log.log(Level.INFO,"Exception with SpeckleToFEMModelServiceService Retry count "+ctr+"\t"+exx);
			}
			ctr++;
		}
		GZIPInputStream gis = new GZIPInputStream(new ByteArrayInputStream(dicom));
		byte[] buffer = new byte[64536];
		int length = 0;
		while ((length = gis.read(buffer)) > 0)
			bos.write(buffer, 0, length);
		gis.close();
		return bos.toByteArray();
	}

	public double getProgress() {
		try {
			Service service = Service.create(wsdlLocation, serviceName);
			SpeckleToFEMModelService st = service.getPort(SpeckleToFEMModelService.class);
			if (workflowID != null)
				return st.getCompletion(workflowID);
			else {
				Thread.sleep(1000);
				return st.getCompletion(workflowID);
			}
		} catch (Exception exx) {
			// exx.printStackTrace();
			return 0.0;
		}
	}

	public void cleanUp() {
		Service service = Service.create(wsdlLocation, serviceName);
		SpeckleToFEMModelService st = service.getPort(SpeckleToFEMModelService.class);
		st.cleanUp(workflowID);
	}

	public static void main(String args[]) throws Exception {
		String wDir = "/home/rjag008/clevelandclinic/FittingWithTorsion/build/WS";
		String xmlFile = wDir + "/input.xml";
		String dcmFile = wDir + "/test.dcm";
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

		Path dcmPath = Paths.get(dcmFile);
		// Compress the dicom for saving memory
		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		GZIPOutputStream gos = new GZIPOutputStream(bos);
		gos.write(Files.readAllBytes(dcmPath));
		gos.close();
		byte[] dcmData = bos.toByteArray();

		SpeckleToFEMInterface intf = new SpeckleToFEMInterface();
		intf.submitJob(xml, dcmData);
		SpeckleToFEMInterface intf1 = new SpeckleToFEMInterface();
		intf1.submitJob(xml, dcmData);
		System.out.println("Submit completed");
		double progress = intf.getProgress() + intf1.getProgress();
		while (progress < 2.0) {
			System.out.println("Progress " + progress);
			try {
				Thread.sleep(1000);
			} catch (Exception exx) {

			}
			progress = intf.getProgress() + intf1.getProgress();
		}
		System.out.println("Completed");
		// intf.saveDicom(wDir+"/ws.dcm");
		intf.cleanUp();
	}

}
