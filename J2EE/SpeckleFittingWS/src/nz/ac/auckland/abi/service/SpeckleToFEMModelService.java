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
package nz.ac.auckland.abi.service;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.PrintWriter;
import java.io.StringReader;
import java.io.StringWriter;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;

import javax.annotation.Resource;
import javax.ejb.EJB;
import javax.jws.WebMethod;
import javax.jws.WebService;
import javax.jws.soap.SOAPBinding;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import javax.xml.ws.WebServiceContext;
import javax.xml.ws.WebServiceException;

import nz.ac.auckland.abi.implementations.SpeckleFitting;
import nz.ac.auckland.abi.resource.ResourceManager;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;

@WebService(targetNamespace = "www.abi.auckland.ac.nz/CCF")
@SOAPBinding(style = SOAPBinding.Style.DOCUMENT)
public class SpeckleToFEMModelService {

	@Resource
	private WebServiceContext wsContext;

	@Resource(name = "java:global/ICMAMODELFITTING")
	private String exec;

	@Resource(name = "java:global/ICMAMODELFITTINGDIR")
	private String execDir;
	
	@Resource(name = "java:global/ICMADISKSCRATCHSPACE")
	private String diskScratchSpace;

	@EJB
	private ResourceManager rManager;

	private String modelName = null;

	private String className = null;

	private Logger log = null;

	private static int instanceCounter = 0;

	@WebMethod
	public String submit(String xmlDoc, byte[] dicom) {
		className = this.getClass().getSimpleName();
		log = Logger.getLogger(className);
		String workflowID = "WSWorkFlow" + instanceCounter++;
		// Create working directory
		File sd = rManager.createWorkingDirectory(workflowID);
		String scratchDir = sd.getAbsolutePath();

		log.log(Level.INFO, "Created directory " + scratchDir + " for speckle to model generation");
		try {
			// Set the output directory
			String updatedXML = updateOutputDirectory(xmlDoc, scratchDir);
			// Create file
			String fullFileName = scratchDir + "/S2FEM.xml";

			// dicom is a gzipped data
			ByteArrayOutputStream bos = new ByteArrayOutputStream();
			GZIPInputStream gis = new GZIPInputStream(new ByteArrayInputStream(dicom));
			byte[] buffer = new byte[64536];
			int length = 0;
			while ((length = gis.read(buffer)) > 0)
				bos.write(buffer, 0, length);
			gis.close();

			FileOutputStream fos = new FileOutputStream(scratchDir + "/S2FEM.dcm");
			fos.write(bos.toByteArray());
			fos.close();

			PrintWriter pr = new PrintWriter(fullFileName);
			pr.print(updatedXML);
			pr.close();

			SpeckleFitting st = new SpeckleFitting(fullFileName, scratchDir, exec, execDir);
			st.setErrorLogDirectory(diskScratchSpace+"/FittingError");
			Thread myThread = new Thread(st);
			myThread.start();
			rManager.associateSession(workflowID, st);

		} catch (Exception exx) {
			throw new WebServiceException(exx);
		}
		return workflowID;
	}

	@WebMethod
	public double getCompletion(String workflowID) {
		if (workflowID != null) {
			if (log == null) {
				log = Logger.getLogger(className);
			}
			log.log(Level.INFO, "Getting session of " + workflowID);
			try {
				SpeckleFitting session = rManager.getSession(workflowID);
				return session.getProgress();
			} catch (Exception exx) {
				log.log(Level.INFO, "Exception occured while accessing session of workflow " + workflowID + "\n" + exx);
				return 0.0;
			}
		}
		return 0.0;
	}

	@WebMethod
	public byte[] getDicom(String workflowID) {
		try {
			if (log == null) {
				log = Logger.getLogger(className);
			}
			SpeckleFitting st = rManager.getSession(workflowID);
			while (!st.hasCompleted()) {
				try {
					Thread.sleep(10000);
				} catch (Exception exx) {

				}
			}
			String outDir = rManager.getWorkingDirectory(workflowID).getAbsolutePath();
			log.log(Level.FINE, "Speckle2FEM of " + modelName + "completed");
			log.log(Level.FINE, outDir + "/S2FEM.dcm");
			if (!st.hasFailed()) {
				// Get the dicom file
				String dcmFile = outDir + "/S2FEM.dcm";
				Path path = Paths.get(dcmFile);
				// Compress the dicom for saving memory
				ByteArrayOutputStream bos = new ByteArrayOutputStream();
				GZIPOutputStream gos = new GZIPOutputStream(bos);
				gos.write(Files.readAllBytes(path));
				gos.close();
				return bos.toByteArray();
			} else {
				log.log(Level.INFO, "Error occured while fitting " + st.getOutput());
				throw new WebServiceException(st.getOutput());
			}
		} catch (Exception exx) {
			log.log(Level.INFO, exx.toString());
			exx.printStackTrace();
			throw new WebServiceException(exx);
		}
	}

	@WebMethod
	public void cleanUp(String workflowID) {
		rManager.cleanUp(workflowID);
	}

	private String updateOutputDirectory(String input, String dir) throws Exception {
		DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
		DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
		Document doc = dBuilder.parse(new InputSource(new StringReader(input)));
		doc.getDocumentElement().normalize();
		NodeList list = doc.getElementsByTagName("OUTPUTDIRECTORY");
		Element parent = null;
		if (list != null) {
			for (int i = 0; i < list.getLength(); i++) {
				Element e = (Element) list.item(i);
				parent = (Element) e.getParentNode();
				parent.removeChild(e);
			}
		}
		list = doc.getElementsByTagName("SEGMENTATION");
		parent = (Element) list.item(0);

		Element directory = doc.createElement("OUTPUTDIRECTORY");
		directory.appendChild(doc.createTextNode(dir));
		parent.appendChild(directory);

		// Create element to point to the dicom file
		String dcmFile = dir + "/S2FEM.dcm";
		Element dicomFile = doc.createElement("DICOMFILE");
		dicomFile.appendChild(doc.createTextNode(dcmFile));
		parent.appendChild(dicomFile);

		// Get the modelName
		list = doc.getElementsByTagName("MODELNAME");
		if (list != null) {
			for (int i = 0; i < list.getLength(); i++) {
				if (list.item(i).getNodeType() == Node.ELEMENT_NODE) {
					Element e = (Element) list.item(i);
					modelName = e.getTextContent();
				}
			}
		}

		// write the content
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
}
