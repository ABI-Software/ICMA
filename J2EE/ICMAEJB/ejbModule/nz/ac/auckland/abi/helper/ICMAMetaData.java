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

import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.FileInputStream;
import java.io.StringWriter;
import java.nio.ByteBuffer;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Properties;
import java.util.Vector;
import java.util.zip.Adler32;
import java.util.zip.CheckedInputStream;
import java.util.zip.GZIPInputStream;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.dcm4che.data.Dataset;
import org.dcm4che.data.DcmObjectFactory;
import org.dcm4che.dict.Tags;
import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public class ICMAMetaData {

	final int BUFFER = 2048;
	private String xmlFile; // Decompressed XML data in a file
	private String transfromationMatrix = null; // Models transformation matrix
	private double duration = 0.0;
	private String modelName = null;
	private String modelAnnotation = null;
	private String status = "non validated";
	private String statusAuthor = null;
	private String author = null;
	private String creationDate = null;
	private String frameSamplingTimes = null;
	private String metaData; // XML data
	private String exRegionFileNamePrefix = null;
	private Vector<String> exRegionFiles;
	private Hashtable<String, String> coordinates; // View, transformation matrix map
	private Hashtable<String, Hashtable<Double, String[]>> strains; // Map of strain type, vector of wall strains
	private Hashtable<String, String> strainGroup; // Map of strainName to strainGroup
	boolean[] selectedStrains;
	
	private double modelPeakGlobalLongitudinalStrain = 0.0;
	private double specklePeakGlobalLongitudinalStrain = 0.0;
	private double ejectionFraction = 0.0;

	public ICMAMetaData(String xmlFile) throws Exception {
		this.xmlFile = xmlFile;
		exRegionFiles = new Vector<String>();
		coordinates = new Hashtable<String, String>();
		strains = new Hashtable<String, Hashtable<Double, String[]>>();
		strainGroup = new Hashtable<String, String>();
		selectedStrains = new boolean[20];
		parseXML();
	}

	public void setAuthor(String author) {
		this.author = author;
	}

	public void setStatusAuthor(String author) {
		this.statusAuthor = author;
	}

	/**
	 * Private method to parse the XML data into CAP model components
	 */
	private void parseXML() throws Exception {
		DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
		DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
		Document doc = null;
		if (xmlFile.endsWith(".xml")) {
			doc = dBuilder.parse(new FileInputStream(xmlFile));
		} else {

			// read in a dicom file
			DataInputStream in = new DataInputStream(new BufferedInputStream(new FileInputStream(xmlFile)));
			Dataset dataSet = DcmObjectFactory.getInstance().newDataset();
			try {
				dataSet.readFile(in, null, -1);
			} catch (Exception e) {
				throw e;
			} finally {
				in.close();
			}
			// Software version Tag value is 1576992 -
			// http://www.dcm4che.org/docs/dcm4che2-apidocs/constant-values.html#org.dcm4che2.data.Tag.SoftwareVersions
			if (dataSet.getString(1576992).trim().toUpperCase().startsWith("ABI ICMA")) {
				ByteBuffer zip = dataSet.getByteBuffer(Tags.OverlayData);
				ByteArrayInputStream mybis = new ByteArrayInputStream(zip.array());
				CheckedInputStream checksum = new CheckedInputStream(mybis, new Adler32());
				GZIPInputStream zis = new GZIPInputStream(new BufferedInputStream(checksum));

				int count = -1;
				byte data[] = new byte[BUFFER];

				ByteArrayOutputStream fos = new ByteArrayOutputStream();
				try {
					while ((count = zis.read(data, 0, BUFFER)) != -1) {
						fos.write(data, 0, count);
					}
				} catch (Exception exx) {
					System.out.println("Exception " + exx + " occured when reading DICOM file");
					exx.printStackTrace();
				}
				fos.close();
				zis.close();
				doc = dBuilder.parse(new ByteArrayInputStream(fos.toByteArray()));
			}
		}
		doc.getDocumentElement().normalize();

		StringWriter writer = new StringWriter();
		StreamResult result = new StreamResult(writer);
		TransformerFactory tf = TransformerFactory.newInstance();
		Transformer transformer = tf.newTransformer();
		transformer.transform(new DOMSource(doc), result);
		metaData = writer.toString();
		
		// Load the meshes (Note there could be more than meshes - but each mesh
		// tag has a heart/organ)
		NodeList nList = doc.getElementsByTagName("MESH");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NamedNodeMap attrs = nNode.getAttributes();
			for (int j = 0; j < attrs.getLength(); j++) {
				Attr attribute = (Attr) attrs.item(j);
				if (attribute.getName().trim().equalsIgnoreCase("prefix")) {
					exRegionFileNamePrefix = attribute.getValue().trim();
				}
				if (attribute.getName().trim().equalsIgnoreCase("count")) {
					int count = Integer.parseInt(attribute.getValue().trim());
					exRegionFiles.setSize(count);
				}
			}
		}

		// We are interested in heart mesh
		// Load the files
		nList = doc.getElementsByTagName("HEART");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			String content = ((Node) text.item(0)).getNodeValue().trim();
			int id = -1;
			NamedNodeMap attrs = nNode.getAttributes();
			for (int j = 0; j < attrs.getLength(); j++) {
				Attr attribute = (Attr) attrs.item(j);
				if (attribute.getName().trim().equalsIgnoreCase("frame")) {
					id = Integer.parseInt(attribute.getValue().trim());
				}
			}
			if (id > -1) {
				exRegionFiles.setElementAt(content, id);
			}
		}

		// Load the mesh transformation matrix
		nList = doc.getElementsByTagName("TRANSFORMATIONMATRIX");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			transfromationMatrix = ((Node) text.item(0)).getNodeValue().trim();
		}
		// Load the model name
		nList = doc.getElementsByTagName("MODELNAME");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			modelName = ((Node) text.item(0)).getNodeValue().trim();
		}
		// Load the modelannotation
		nList = doc.getElementsByTagName("MODELANNOTATION");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			modelAnnotation = ((Node) text.item(0)).getNodeValue().trim();
		}

		// Load the modelstatus
		nList = doc.getElementsByTagName("MODELSTATUS");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			status = ((Node) text.item(0)).getNodeValue().trim();
		}

		// Load the modelstatusAuthor
		nList = doc.getElementsByTagName("MODELSTATUSAUTHOR");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			statusAuthor = ((Node) text.item(0)).getNodeValue().trim();
		}

		// Load the modelAuthor
		nList = doc.getElementsByTagName("MODELAUTHOR");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			author = ((Node) text.item(0)).getNodeValue().trim();
		}

		// Load the modelcreationdate
		nList = doc.getElementsByTagName("MODELCREATIONDATE");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			creationDate = ((Node) text.item(0)).getNodeValue().trim();
		}

		// Load the modelduration
		nList = doc.getElementsByTagName("MODELDURATION");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			duration = Double.parseDouble(((Node) text.item(0)).getNodeValue().trim());
		}

		// Frame vector also needs to be added
		nList = doc.getElementsByTagName("NORMALIZEDFRAMEVECTOR");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			frameSamplingTimes = ((Node) text.item(0)).getNodeValue().trim();
		}
		
		// Load the view transformation matrices
		nList = doc.getElementsByTagName("VIEWPLANETRANSFORMATIONMATRIX");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			String transform = ((Node) text.item(0)).getNodeValue().trim();
			String viewId = null;
			NamedNodeMap attrs = nNode.getAttributes();
			for (int j = 0; j < attrs.getLength(); j++) {
				Attr attribute = (Attr) attrs.item(j);
				// Id is the rotation Angle associated with the view
				if (attribute.getName().trim().equalsIgnoreCase("VIEW")) {
					viewId = attribute.getValue().trim();
				}
			}
			if (viewId != null) {
				addViewCoordinates(viewId, transform);
			}
		}

		//Load strain selections
		nList = doc.getElementsByTagName("STRAINSELECTIONS");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			if (nNode.getNodeType() == Node.ELEMENT_NODE) {
				Element eElement = (Element) nNode;
				NodeList sList = eElement.getElementsByTagName("STRAIN");
				for (int j = 0; j < sList.getLength(); j++) {
					Node sNode = sList.item(j);
					NodeList text = sNode.getChildNodes();
					int id = 0;
					if (text.item(0) != null)
						id = Integer.parseInt(((Node) text.item(0)).getNodeValue().trim());
					selectedStrains[id] = true;
				}
			}
		}
		if(nList.getLength()==0){
			for(int i=1;i<20;i++)
				selectedStrains[i] = true;
		}
		
		// Load strain values
		nList = doc.getElementsByTagName("STRAINGROUP");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			String strainName = null;
			String strainGroup = null;
			int wall_xi = -1;
			int count = 0;
			NamedNodeMap attrs = nNode.getAttributes();

			for (int j = 0; j < attrs.getLength(); j++) {
				Attr attribute = (Attr) attrs.item(j);
				// Id is the rotation Angle associated with the view
				if (attribute.getName().trim().equalsIgnoreCase("type")) {
					strainName = attribute.getValue().trim();
				}
				if (attribute.getName().trim().equalsIgnoreCase("group")) {
					strainGroup = attribute.getValue().trim();
				}
				if (attribute.getName().trim().equalsIgnoreCase("wall")) {
					wall_xi = Integer.parseInt(attribute.getValue().trim());
				}
				if (attribute.getName().trim().equalsIgnoreCase("count")) {
					count = Integer.parseInt(attribute.getValue().trim());
				}
			}

			Hashtable<Double, String[]> wallStrains = null;
			if (strains.containsKey(strainName)) {
				wallStrains = strains.get(strainName);
			} else {
				wallStrains = new Hashtable<Double, String[]>();
			}
			if (nNode.getNodeType() == Node.ELEMENT_NODE) {
				Element eElement = (Element) nNode;
				String myStrains[] = new String[count];
				NodeList sList = eElement.getElementsByTagName("STRAIN");
				for (int j = 0; j < sList.getLength(); j++) {
					Node sNode = sList.item(j);
					NodeList text = sNode.getChildNodes();
					int id = -1;
					NamedNodeMap sattrs = sNode.getAttributes();
					for (int k = 0; k < sattrs.getLength(); k++) {
						Attr attribute = (Attr) sattrs.item(k);
						if (attribute.getName().trim().equalsIgnoreCase("id")) {
							id = Integer.parseInt(attribute.getValue().trim());
						}
					}
					if (text.item(0) != null)
						myStrains[id] = ((Node) text.item(0)).getNodeValue().trim();
					else
						myStrains[id] = null;
				}

				wallStrains.put(new Double(wall_xi), myStrains);
			}
			strains.put(strainName, wallStrains);
			if (strainGroup != null) {
				this.strainGroup.put(strainName, strainGroup);
			}
		}

		// Load the modelduration
		nList = doc.getElementsByTagName("MODELPGS");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			modelPeakGlobalLongitudinalStrain = Double.parseDouble(((Node) text.item(0)).getNodeValue().trim());
		}

		// Load the modelduration
		nList = doc.getElementsByTagName("SPECKLEPGS");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			specklePeakGlobalLongitudinalStrain = Double.parseDouble(((Node) text.item(0)).getNodeValue().trim());
		}

		// Load the modelduration
		nList = doc.getElementsByTagName("MODELEGF");
		for (int i = 0; i < nList.getLength(); i++) {
			Node nNode = nList.item(i);
			NodeList text = nNode.getChildNodes();
			ejectionFraction = Double.parseDouble(((Node) text.item(0)).getNodeValue().trim());
		}

	}

	public String getExRegionFileNamePrefix() {
		return exRegionFileNamePrefix;
	}

	/**
	 * Get the CMGUI -Image to mesh alignment transformation matrix
	 * 
	 * @return transformation matrix components
	 */
	public String getTransfromationMatrix() {
		return transfromationMatrix;
	}

	/**
	 * Get the time ordered list of exregion files associated with the CAP model
	 * 
	 * @return Vector of ordered exregion filenames
	 */
	public Vector<String> getEXRegionFiles() {
		return exRegionFiles;
	}

	public void addViewCoordinates(String viewName, String coordinate) {
		coordinates.put(viewName, coordinate);
	}

	public String getViewTransformationMatrix(String viewName) {
		return coordinates.get(viewName.trim());
	}

	public String[] getStrainNames() {
		Enumeration<String> strainKeys = strains.keys();
		Vector<String> names = new Vector<String>();
		while (strainKeys.hasMoreElements()) {
			names.add(strainKeys.nextElement());
		}
		String[] result = new String[names.size()];
		return names.toArray(result);
	}

	public String getStrainGroup(String strainName) {
		if (strainGroup.containsKey(strainName)) {
			return strainGroup.get(strainName);
		}
		return null;
	}

	public Hashtable<Double, String[]> getStrains(String strainName) {
		return strains.get(strainName);
	}

	public String getModelName() {
		return modelName;
	}

	public String getModelAnnotation() {
		return modelAnnotation;
	}

	public String getAuthor() {
		return author;
	}

	public String getMetaData() {
		return metaData;
	}

	public String getJSON(String instanceID, Properties viewXpaths, Properties exXpaths) {
		JSONObject model = new JSONObject();
		model.put("studyUid", instanceID);
		model.put("name", modelName);
		model.put("startTime", 0);
		model.put("endTime", duration);
		if(frameSamplingTimes!=null)
			model.put("frameVector", frameSamplingTimes);
		else{
			StringBuffer buf = new StringBuffer();
			int nummeshes = exXpaths.size();
			double step = 1.0/(nummeshes-1.0);
			buf.append("0.0");
			for(int i=1;i<nummeshes;i++){
				buf.append(","+i*step);
			}
			model.put("frameVector", buf.toString());
		}
		JSONObject lifeCycle = new JSONObject();
		lifeCycle.put("date", creationDate);
		lifeCycle.put("author", author);
		if (status != null) {
			lifeCycle.put("status", status);
			lifeCycle.put("statusAuthor", statusAuthor);
		} else {
			lifeCycle.put("status", "Validated");
			lifeCycle.put("statusAuthor", author);
		}
		model.put("lifeCycle", lifeCycle);

		JSONObject display = new JSONObject();
		display.put("heart", "lines");
		JSONObject iregions = new JSONObject();

		Enumeration<Object> regions = viewXpaths.keys();
		while (regions.hasMoreElements()) {
			String key = (String) regions.nextElement();
			String regionName = key;
			if(coordinates.get(regionName)!=null){//Only if the region was selected, note the speckle tracking output dcm may have other views too 
				JSONObject region = new JSONObject();
				Properties xpaths = (Properties) viewXpaths.get(key);
				region.put("name", regionName);
				region.put("imageStartIndex", 0);
				region.put("imageEndIndex", xpaths.size());
				region.put("imageType", "jpg");
	
				Enumeration<Object> xpkeys = xpaths.keys();
				key = (String) xpkeys.nextElement();
				String xpath = xpaths.getProperty(key);
				String prefix = xpath.substring(0, xpath.indexOf("_") + 1);
				region.put("imageprefix", prefix);
	
				/*
				 * JSONObject images = new JSONObject(); Enumeration<Object> keys =
				 * xpaths.keys(); while(keys.hasMoreElements()){ String xkey =
				 * (String)keys.nextElement(); String xpath =
				 * xpaths.getProperty(xkey); images.put(xkey, xpath); }
				 * region.put("images", images);
				 */
				iregions.put(regionName, region);
				display.put(regionName, "projection");
			}
		}

		JSONObject mesh = new JSONObject();
		mesh.put("name", "heart");
		mesh.put("regionStartIndex", 0);
		mesh.put("regionEndIndex", exXpaths.size());
		mesh.put("urlSuffix", "jcr:data");
		Enumeration<Object> xpkeys = exXpaths.keys();
		String keyEx = (String) xpkeys.nextElement();
		String xpath = exXpaths.getProperty(keyEx);
		String prefix = xpath.substring(0, xpath.lastIndexOf("/") + 1) + "Heart.";
		mesh.put("meshRegionprefix", prefix);

		iregions.put("MESH", mesh);
		model.put("Regions", iregions);
		model.put("Display", display);
		// Put view transformation matrices
		JSONObject transforms = new JSONObject();
		transforms.put("MESH", transfromationMatrix);
		Enumeration<String> views = coordinates.keys();
		while (views.hasMoreElements()) {
			String viewName = views.nextElement();
			transforms.put(viewName, coordinates.get(viewName));
		}
		model.put("Transforms", transforms);

		//Put the selected Strains data
		JSONArray selectedStrains = new JSONArray();
		for(int i=0;i<19;i++){
			selectedStrains.add(this.selectedStrains[i]);
		}
		model.put("SelectedStrains",selectedStrains);
		// measures
		String[] names = getStrainNames();
		JSONObject measure = new JSONObject();
		measure.put("modelPeakGlobalLongitudinalStrain", "" + modelPeakGlobalLongitudinalStrain);
		measure.put("specklePeakGlobalLongitudinalStrain", "" + specklePeakGlobalLongitudinalStrain);
		measure.put("ejectionFraction", "" + ejectionFraction);
		for (int i = 0; i < names.length; i++) {
			JSONObject strain = new JSONObject();
			strain.put("name", names[i]);
			String strainGroup = getStrainGroup(names[i]);
			if (strainGroup == null)
				strainGroup = "OTHER";
			strain.put("group", strainGroup);
			Hashtable<Double, String[]> table = getStrains(names[i]);

			Enumeration<Double> keys = table.keys();
			int minKey = 18; // As strains have 17 segments and an average
			int minTimeSteps = 100;
			JSONObject timeseries = new JSONObject();
			while (keys.hasMoreElements()) {
				Double key = keys.nextElement();
				String[] strains = table.get(key);
				if (strains.length < minKey)
					minKey = strains.length;
				JSONObject wall = new JSONObject();
				if (!names[i].startsWith("SPECKLE") && !names[i].startsWith("DIFF")) {
					for (int j = 0; j < strains.length; j++) {
						if(strains[j]!=null){
							String toks[] = strains[j].split(",");
							int tlength = toks.length;
							if (tlength < minTimeSteps)
								minTimeSteps = tlength;
							wall.put("" + (j + 1), strains[j]);
						}
					}
					/*
					 * if (strains.length > 1) { wall.put("average",
					 * strains[strains.length - 1]); } else {// Handle linear
					 * distortion String toks[] = strains[0].split(","); int
					 * tlength = toks.length; if (tlength < minTimeSteps)
					 * minTimeSteps = tlength; wall.put("1", strains[0]); }
					 */
				} else { // Handle speckle strains appropriately
					minKey = strains.length;
					for (int j = 0; j < strains.length; j++) {
						// Data contains segmentid; strains
						if (strains[j] != null) {
							try {
								String record[] = strains[j].split(";");
								String toks[] = record[1].split(",");
								int tlength = toks.length;
								if (tlength < minTimeSteps)
									minTimeSteps = tlength;
								wall.put(record[0].trim(), record[1].trim());
							} catch (Exception exx) {
								System.out.println(exx);
								System.out.println("Strain "+key+"\t"+strains[j]);
							}
						}
					}
				}
				wall.put("xi", (int) key.doubleValue());
				timeseries.put("wall" + key.doubleValue(), wall);
			}
			strain.put("timePoints", minTimeSteps);
			strain.put("numwallsegments", table.size());
			strain.put("numsegments", minKey);
			// strain.put("average", 1);
			strain.put("timeseries", timeseries);
			measure.put(names[i], strain);
		}
		model.put("Measures", measure);

		return model.toJSONString();
	}

	
	
	
	public static String getFittingXML(ByteArrayInputStream dicomFile) throws Exception {
		final int BUFFER = 2048;
		DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
		DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
		Document doc = null;

		// read in a dicom file
		DataInputStream in = new DataInputStream(new BufferedInputStream(dicomFile));
		Dataset dataSet = DcmObjectFactory.getInstance().newDataset();
		try {
			dataSet.readFile(in, null, -1);
		} catch (Exception e) {
			throw e;
		} finally {
			in.close();
		}
		// Software version Tag value is 1576992 -
		// http://www.dcm4che.org/docs/dcm4che2-apidocs/constant-values.html#org.dcm4che2.data.Tag.SoftwareVersions
		if (dataSet.getString(1576992).trim().toUpperCase().startsWith("ABI ICMA")) {
			ByteBuffer zip = dataSet.getByteBuffer(Tags.OverlayData);
			ByteArrayInputStream mybis = new ByteArrayInputStream(zip.array());
			CheckedInputStream checksum = new CheckedInputStream(mybis, new Adler32());
			GZIPInputStream zis = new GZIPInputStream(new BufferedInputStream(checksum));

			int count = -1;
			byte data[] = new byte[BUFFER];

			ByteArrayOutputStream fos = new ByteArrayOutputStream();
			try {
				while ((count = zis.read(data, 0, BUFFER)) != -1) {
					fos.write(data, 0, count);
				}
			} catch (Exception exx) {
				System.out.println("Exception " + exx + " occured when reading DICOM file");
				exx.printStackTrace();
			}
			fos.close();
			zis.close();
			doc = dBuilder.parse(new ByteArrayInputStream(fos.toByteArray()));
		}

		doc.getDocumentElement().normalize();

		// Load the meshes (Note there could be more than meshes - but each mesh
		// tag has a heart/organ)
		NodeList nList = doc.getElementsByTagName("SPECKLETRACKINGOUTPUT");
		if(nList.item(0)==null){//MRI Model
			nList = doc.getElementsByTagName("CMISSCAPIO");
		}
		if (nList.item(0) != null) {
			Node nNode = nList.item(0);

			StringWriter writer = new StringWriter();
			StreamResult result = new StreamResult(writer);
			TransformerFactory tf = TransformerFactory.newInstance();
			Transformer transformer = tf.newTransformer();
			transformer.transform(new DOMSource(nNode), result);
			return writer.toString();
		} else {
			return null;
		}
	}

/*	public static JSONObject getSpeckleXML(ByteArrayInputStream dicomFile) throws Exception {
		final int BUFFER = 2048;
		DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
		DocumentBuilder dBuilder = dbFactory.newDocumentBuilder();
		Document doc = null;

		// read in a dicom file
		DataInputStream in = new DataInputStream(new BufferedInputStream(dicomFile));
		Dataset dataSet = DcmObjectFactory.getInstance().newDataset();
		try {
			dataSet.readFile(in, null, -1);
		} catch (Exception e) {
			throw e;
		} finally {
			in.close();
		}
		// Software version Tag value is 1576992 -
		// http://www.dcm4che.org/docs/dcm4che2-apidocs/constant-values.html#org.dcm4che2.data.Tag.SoftwareVersions
		if (dataSet.getString(1576992).trim().toUpperCase().startsWith("ABI ICMA")) {
			ByteBuffer zip = dataSet.getByteBuffer(Tags.OverlayData);
			ByteArrayInputStream mybis = new ByteArrayInputStream(zip.array());
			CheckedInputStream checksum = new CheckedInputStream(mybis, new Adler32());
			GZIPInputStream zis = new GZIPInputStream(new BufferedInputStream(checksum));

			int count = -1;
			byte data[] = new byte[BUFFER];

			ByteArrayOutputStream fos = new ByteArrayOutputStream();
			try {
				while ((count = zis.read(data, 0, BUFFER)) != -1) {
					fos.write(data, 0, count);
				}
			} catch (Exception exx) {
				System.out.println("Exception " + exx + " occured when reading DICOM file");
				exx.printStackTrace();
			}
			fos.close();
			zis.close();
			doc = dBuilder.parse(new ByteArrayInputStream(fos.toByteArray()));
		}

		doc.getDocumentElement().normalize();
		JSONObject obj = new JSONObject();
		// Load the meshes (Note there could be more than meshes - but each mesh
		// tag has a heart/organ)
		NodeList nList = doc.getElementsByTagName("SPECKLETRACKINGOUTPUT");
		if (nList.item(0) != null) {
			Node nNode = nList.item(0);

			StringWriter writer = new StringWriter();
			StreamResult result = new StreamResult(writer);
			TransformerFactory tf = TransformerFactory.newInstance();
			Transformer transformer = tf.newTransformer();
			transformer.transform(new DOMSource(nNode), result);
			obj.put("SPECKLETRACKINGOUTPUT", writer.toString());
			//The speckle tracking input needs to be constructed from speckle tracking output data
			return obj;
		} else {
			return null;
		}
	}
*/	
	
	public static void main(String args[]) throws Exception {

/*		ICMAMetaData metaTest = new ICMAMetaData("/home/rjag008/clevelandclinic/ICMASCRATCH/WSS2FEM0.03405995938467099/S2FEM.dcm");
		Properties viewXPath = new Properties();
		Properties testXPath = new Properties();
		Properties exXPath = new Properties();
		for (int i = 0; i < 19; i++) {
			testXPath.put("TCH_" + i, "/12345/37376/TCH_" + i);
			exXPath.put("Heart." + i, "/12345/37376/Heart.");
		}
		viewXPath.put("APLAX", testXPath);
		viewXPath.put("FCH", testXPath);
		viewXPath.put("TCH", testXPath);
		String jSON = metaTest.getJSON("testx", viewXPath, exXPath);*/
		
		ICMAMetaData metaData = new ICMAMetaData("/home/rjag008/ClevelandClinic/CPP/MRIModelStrainMeasures/Release/strain.xml");
		
	}

	/*
	 * Properties viewXPath = new Properties(); Properties testXPath = new
	 * Properties(); Properties exXPath = new Properties(); for(int
	 * i=0;i<19;i++){ testXPath.put("APLAX_"+i, "/12345/37376/APLAX_"+i);
	 * exXPath.put("Heart."+i, "/12345/37376/Heart."); } viewXPath.put("APLAX",
	 * testXPath); viewXPath.put("FCH", testXPath); viewXPath.put("TCH",
	 * testXPath);
	 * 
	 * String jSON = metaTest.getJSON("testx", viewXPath, exXPath);
	 * 
	 * java.io.PrintWriter pr = new
	 * java.io.PrintWriter("/people/rjag008/Desktop/Test.json"); pr.print(jSON);
	 * pr.close();
	 */

}
