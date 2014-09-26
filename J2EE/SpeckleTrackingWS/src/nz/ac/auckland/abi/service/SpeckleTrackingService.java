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

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FilenameFilter;
import java.io.PrintWriter;
import java.io.StringReader;
import java.io.StringWriter;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.zip.GZIPOutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

import javax.annotation.Resource;
import javax.ejb.EJB;
import javax.jws.WebMethod;
import javax.jws.WebService;
import javax.jws.soap.SOAPBinding;
import javax.servlet.http.HttpSession;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import javax.xml.ws.WebServiceContext;
import javax.xml.ws.WebServiceException;
import javax.xml.ws.handler.MessageContext;
import javax.xml.ws.soap.MTOM;

import nz.ac.auckland.abi.implementations.SpeckleTracking;
import nz.ac.auckland.abi.resource.ResourceManager;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;

@MTOM(enabled = true, threshold=10240)
@WebService(targetNamespace = "www.abi.auckland.ac.nz/CCF")
@SOAPBinding(style = SOAPBinding.Style.DOCUMENT)
public class SpeckleTrackingService {

	@Resource
	private WebServiceContext wsContext;

	@Resource(name = "java:global/ICMASPECKLETRACKING")
	private String exec;

	@Resource(name = "java:global/ICMASPECKLETRACKINGDIR")
	private String execDir;

	@EJB
	private ResourceManager rManager;

	private String modelName = null;

	private String className = null;

	private Logger log = null;

	@WebMethod
	public void submitWithDisplacements(String xml, ArrayList<String> views, ArrayList<byte[]> files) {
/*		Hashtable<String, byte[]> filenames = new Hashtable<String, byte[]>();
		for(int i=0;i<views.size();i++){
			filenames.put(views.get(i), files.get(i));
		}
		className = this.getClass().getSimpleName();
		log = Logger.getLogger(className);
		MessageContext mc = wsContext.getMessageContext();
		HttpSession session = ((javax.servlet.http.HttpServletRequest) mc.get(MessageContext.SERVLET_REQUEST)).getSession();
		if (session == null)
			throw new WebServiceException("HTTP session not found");
		// Create working directory
		File sd = rManager.createWorkingDirectory(session);
		String scratchDir = sd.getAbsolutePath();

		log.log(Level.FINE, "Created directory " + scratchDir + " for speckle tracking");
		try {
			String speckleExec = dispExec;
			// Set the output directory
			String dirUpdatedXML = updateOutputDirectory(xml, scratchDir);
			String updatedXML = updateDisplacementData(dirUpdatedXML, filenames, scratchDir);
			// Create file
			String fullFileName = scratchDir + "/ST.xml";

			PrintWriter pr = new PrintWriter(fullFileName);
			pr.print(updatedXML);
			pr.close();

			SpeckleTracking st = new SpeckleTracking(fullFileName, speckleExec, execDir);
			Thread myThread = new Thread(st);
			myThread.start();
			// Add the speckle tracking object to context
			session.setAttribute(className + "SpeckleTracking", st);
			session.setAttribute(className + "OutputDirectory", scratchDir);
			session.setAttribute(className + "ModelName", modelName);// Set in
			// updateOutputDirectory
			// call
		} catch (Exception exx) {
			throw new WebServiceException(exx);
		}*/
		
		throw new WebServiceException("Service unavailable");
	}
	
	
	@WebMethod
	public void submit(String xmlDoc) {
		className = this.getClass().getSimpleName();
		log = Logger.getLogger(className);
		MessageContext mc = wsContext.getMessageContext();
		HttpSession session = ((javax.servlet.http.HttpServletRequest) mc.get(MessageContext.SERVLET_REQUEST)).getSession();
		if (session == null)
			throw new WebServiceException("HTTP session not found");
		// Create working directory
		File sd = rManager.createWorkingDirectory(session);
		String scratchDir = sd.getAbsolutePath();

		log.log(Level.FINE, "Created directory " + scratchDir + " for speckle tracking");
		try {
			String speckleExec = exec;
			// Set the output directory
			String updatedXML = updateOutputDirectory(xmlDoc, scratchDir);
			// Create file
			String fullFileName = scratchDir + "/ST.xml";

			PrintWriter pr = new PrintWriter(fullFileName);
			pr.print(updatedXML);
			pr.close();

			SpeckleTracking st = new SpeckleTracking(fullFileName, speckleExec, execDir);
			Thread myThread = new Thread(st);
			myThread.start();
			// Add the speckle tracking object to context
			session.setAttribute(className + "SpeckleTracking", st);
			session.setAttribute(className + "OutputDirectory", scratchDir);
			session.setAttribute(className + "ModelName", modelName);// Set in
			// updateOutputDirectory
			// call
		} catch (Exception exx) {
			throw new WebServiceException(exx);
		}
	}

	@WebMethod
	public double getCompletion() {
		MessageContext mc = wsContext.getMessageContext();
		HttpSession session = ((javax.servlet.http.HttpServletRequest) mc.get(MessageContext.SERVLET_REQUEST)).getSession();
		if (session == null)
			throw new WebServiceException("HTTP session not found");
		SpeckleTracking st = (SpeckleTracking) session.getAttribute(className + "SpeckleTracking");
		String modelName = (String) session.getAttribute(className + "ModelName");
		log.log(Level.FINE, "Progress of " + modelName + " is " + st.getProgress());
		return st.getProgress();
	}

	@WebMethod
	public byte[] getDicom() {
		try {
			MessageContext mc = wsContext.getMessageContext();
			HttpSession session = ((javax.servlet.http.HttpServletRequest) mc.get(MessageContext.SERVLET_REQUEST)).getSession();
			if (session == null)
				throw new WebServiceException("HTTP session not found");
			SpeckleTracking st = (SpeckleTracking) session.getAttribute(className + "SpeckleTracking");
			while (!st.hasCompleted()) {
				try {
					Thread.sleep(10000);
				} catch (Exception exx) {

				}
			}

			String outDir = (String) session.getAttribute(className + "OutputDirectory");
			String modelName = (String) session.getAttribute(className + "ModelName");
			log.log(Level.FINE, "SpeckleTracking of " + modelName + "completed");
			//log.log(Level.FINE, outDir + "/" + modelName + ".dcm");
			log.log(Level.FINE, outDir + "/LVBST.dcm");
			if (!st.hasFailed()) {
				// Get the dicom file
				//String dcmFile = outDir + "/" + modelName + ".dcm";
				String dcmFile = outDir + "/LVBST.dcm";
				Path path = Paths.get(dcmFile);
				// Compress the dicom for saving memory
				ByteArrayOutputStream bos = new ByteArrayOutputStream();
				GZIPOutputStream gos = new GZIPOutputStream(bos);
				gos.write(Files.readAllBytes(path));
				gos.close();
				return bos.toByteArray();
			} else {
				throw new WebServiceException(st.getOutput());
			}
		} catch (Exception exx) {
			throw new WebServiceException(exx);
		}
	}

	@WebMethod
	public String getSpeckleTrackingXML() {
		try {
			MessageContext mc = wsContext.getMessageContext();
			HttpSession session = ((javax.servlet.http.HttpServletRequest) mc.get(MessageContext.SERVLET_REQUEST)).getSession();
			if (session == null)
				throw new WebServiceException("HTTP session not found");
			SpeckleTracking st = (SpeckleTracking) session.getAttribute(className + "SpeckleTracking");
			while (!st.hasCompleted()) {
				try {
					Thread.sleep(10000);
				} catch (Exception exx) {

				}
			}

			String outDir = (String) session.getAttribute(className + "OutputDirectory");
			//String modelName = (String) session.getAttribute(className + "ModelName");

			if (!st.hasFailed()) {
				// Get the dicom file
				//String xmlFile = outDir + "/" + modelName + ".xml";
				String xmlFile = outDir + "/LVBST.xml";
				DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
				DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
				Document doc = dBuilder.parse(xmlFile);
				doc.getDocumentElement().normalize();

				// Remove Output Directory
				{
					NodeList list = doc.getElementsByTagName("OUTPUTDIRECTORY");
					Element parent = null;
					if (list != null) {
						for (int i = 0; i < list.getLength(); i++) {
							Element e = (Element) list.item(i);
							parent = (Element) e.getParentNode();
							parent.removeChild(e);
						}
					}
				}
				//Remove displacement values
				{
					NodeList list = doc.getElementsByTagName("DISP");
					Element parent = null;
					if (list != null) {
						for (int i = 0; i < list.getLength(); i++) {
							Element e = (Element) list.item(i);
							parent = (Element) e.getParentNode();
							parent.removeChild(e);
						}
					}
				}
				TransformerFactory transformerFactory = TransformerFactory.newInstance();
				Transformer transformer = transformerFactory.newTransformer();
				DOMSource source = new DOMSource(doc);
				StringWriter stringWriter = new StringWriter();
				StreamResult result = new StreamResult(stringWriter);
				transformer.transform(source, result);

				return stringWriter.getBuffer().toString();
			} else {
				throw new WebServiceException(st.getOutput());
			}
		} catch (Exception exx) {
			throw new WebServiceException(exx);
		}
	}

	@WebMethod
	public byte[] getImageArchive() {
		MessageContext mc = wsContext.getMessageContext();
		HttpSession session = ((javax.servlet.http.HttpServletRequest) mc.get(MessageContext.SERVLET_REQUEST)).getSession();
		if (session == null)
			throw new WebServiceException("HTTP session not found");
		File dir = new File((String) session.getAttribute(className + "OutputDirectory"));

		File[] list = dir.listFiles(new FilenameFilter() {
			public boolean accept(File arg0, String arg1) {
				return arg1.endsWith("jpg") || arg1.endsWith("JPG");
			}
		});

		try {
			ByteArrayOutputStream bos = new ByteArrayOutputStream();
			ZipOutputStream zos = new ZipOutputStream(bos);
			for (File file : list) {
				log.log(Level.FINE, "Adding file " + file.getName() + " to image archive");
				ZipEntry ze = new ZipEntry(file.getName().toUpperCase());
				zos.putNextEntry(ze);
				Path path = Paths.get(file.getAbsolutePath());
				zos.write(Files.readAllBytes(path));
				zos.closeEntry();
			}
			zos.close();
			return bos.toByteArray();
		} catch (Exception exx) {
			throw new WebServiceException(exx);
		}
	}

	@WebMethod
	public void cleanUp() {
		MessageContext mc = wsContext.getMessageContext();
		HttpSession session = ((javax.servlet.http.HttpServletRequest) mc.get(MessageContext.SERVLET_REQUEST)).getSession();
		if (session == null)
			throw new WebServiceException("HTTP session not found");
		rManager.cleanUp(session);
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
		directory.setAttribute("JPEGS", "TRUE"); // To enable the output of
													// frames
		parent.appendChild(directory);

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

/*	private String updateDisplacementData(String xmlString,Hashtable<String, byte[]> viewData, String targetdir) throws Exception{
		//Download the view files into the target dir and up date the xmlfile
		Enumeration<String> views = viewData.keys();
		Hashtable<String, String> filenames = new Hashtable<String, String>();
		while(views.hasMoreElements()){
			String viewName = (String) views.nextElement();
			File target = new File(targetdir,viewName+".gz");
			FileOutputStream fos = new FileOutputStream(target);
			fos.write(viewData.get(viewName));
			fos.close();
			filenames.put(viewName.toUpperCase(), target.getAbsolutePath());
		}
		DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
		DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
		Document doc = dBuilder.parse(new InputSource(new StringReader(xmlString)));
		doc.getDocumentElement().normalize();
		NodeList list = doc.getElementsByTagName("MARKERS");

		if (list != null) {
			for (int i = 0; i < list.getLength(); i++) {
				Element e = (Element) list.item(i);
				String viewName = e.getAttribute("VIEW");
				if(viewName!=null){
					String dispFile = filenames.get(viewName.toUpperCase());
					if(dispFile!=null){
						Element disp = doc.createElement("DISP");
						disp.appendChild(doc.createTextNode(dispFile));
						e.appendChild(disp);
					}
				}
			}
		}
		
		TransformerFactory transformerFactory = TransformerFactory.newInstance();
		Transformer transformer = transformerFactory.newTransformer();
		DOMSource source = new DOMSource(doc);
		StringWriter stringWriter = new StringWriter();
		StreamResult result = new StreamResult(stringWriter);

		transformer.transform(source, result);

		//log.log(Level.FINE,stringWriter.getBuffer().toString());
		
		return stringWriter.getBuffer().toString();
	}
*/
}
