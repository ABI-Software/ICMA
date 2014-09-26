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
package nz.ac.auckland.abi.entities;

import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.Serializable;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.zip.Adler32;
import java.util.zip.CheckedInputStream;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;

import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.GeneratedValue;
import javax.persistence.GenerationType;
import javax.persistence.Id;
import javax.persistence.Lob;
import javax.sql.rowset.serial.SerialBlob;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

@Entity(name = "FEMModel")
public class FEMModel implements Serializable{
	private static final long serialVersionUID = 1L;
	
	public FEMModel(){
		super();
	}
	
	public FEMModel(String patientID, String studyID, String seriesID, String instanceID, String author, String annotation, String modelName, String imageUri, String fieldmlUri, byte[] metaData) throws Exception{
		id = instanceID;
		this.patientID = patientID;
		this.studyID = studyID;
		this.seriesID = seriesID;
		this.author = author;
		if(annotation!=null)
			this.annotation = annotation;
		else
			this.annotation = "";
		this.modelName = modelName;
		this.imageUriXML = imageUri.getBytes();
		this.fieldMLUriXML = fieldmlUri.getBytes();
		this.metaData = metaData;
		modelStatus = "SUBMITTED";
	}
	
	@Id
	@GeneratedValue(strategy = GenerationType.IDENTITY)
	@Column(name = "pk")
    private long pk;
	
    @Column(name = "sopid")
    private String id;
    
    @Column(name = "pat_id")
    private String patientID;

    @Column(name = "study_id")
    private String studyID;
    
    @Column(name = "series_id")
    private String seriesID;
    
	@Column(name = "Author")
    private String author;

    @Column(name = "Annotation")
    private String annotation;

    @Column(name = "model_name")
    private String modelName;
	
    @Column(name = "model_status")
    private String modelStatus;
    
    @Column(name = "image_uri_xml")
    @Lob
    private byte[] imageUriXML;
   //private Blob imageUriXML;
    
    @Column(name = "Field_ml_uri_xml")
    @Lob
    private byte[] fieldMLUriXML;
    //private Blob fielMLUriXML;
    
    @Column(name = "metadata")
    @Lob 
    private byte[] metaData;
    //private Blob metaData;
    
 	
 	@Override
 	public String toString() {
 		long msize = 0;
 		try{
 			msize = metaData.length;
 		}catch(Exception exx){
 			
 		}
 		return "FEM Model "+id+"\t"+author+"\t"+modelName+"\t"+annotation+" meta data size is "+msize;
 	}

 	public long getPk(){
 		return pk;
 	}
 	
	public String getId() {
		return id;
	}

	public void setId(String id) {
		this.id = id;
	}

	public String getAuthor() {
		return author;
	}

	public void setAuthor(String author) {
		this.author = author;
	}

	public String getAnnotation() {
		return annotation;
	}

	public void setAnnotation(String annotation) {
		if(annotation!=null)
			this.annotation = annotation;
		else
			this.annotation = "";
	}

	public String getModelName() {
		return modelName;
	}

	public void setModelName(String modelName) {
		this.modelName = modelName;
	}

	public String getModelStatus() {
		return modelStatus;
	}

	public void setModelStatus(String modelStatus) {
		this.modelStatus = modelStatus;
	}

	public byte[] getMetaData() {
		return metaData;
	}

	public void setMetaData(byte[] metaData) {
		this.metaData = metaData;
	}
 	
	public String getPatientID() {
		return patientID;
	}

	public void setPatientID(String patientID) {
		this.patientID = patientID;
	}

	public String getStudyID() {
		return studyID;
	}

	public void setStudyID(String studyID) {
		this.studyID = studyID;
	}

	public String getSeriesID() {
		return seriesID;
	}

	public void setSeriesID(String seriesID) {
		this.seriesID = seriesID;
	}

	
	public String[] getImageUris(){
		try{
			String imgXML = new String(imageUriXML);
			Document doc = DocumentBuilderFactory.newInstance().newDocumentBuilder().parse(imgXML);
			NodeList nList = doc.getElementsByTagName("IMAGES");
			String[] uris = new String[200];
			int counter = 0;
			for (int i = 0; i < nList.getLength(); i++) {
				Node nNode = nList.item(i);
				NamedNodeMap attrs = nNode.getAttributes();
				int id = 0;
				for (int j = 0; j < attrs.getLength(); j++) {
					Attr attribute = (Attr) attrs.item(j);
					if (attribute.getName().trim().equalsIgnoreCase("id")) {
						id = Integer.parseInt(attribute.getValue().trim());
						break;
					}
				}
				NodeList text = nNode.getChildNodes();
				String uri = ((Node) text.item(0)).getNodeValue().trim();
				try{
					uris[id] = uri;
					counter++;
				}catch(Exception exx){
					Logger log = Logger.getLogger(this.getClass().getSimpleName());
					log.log(Level.SEVERE,"Error Traversing image uris "+uri+"\t is associated with id "+id);
				}
			}
			String[] result = new String[counter];
			int nctr = 0;
			for(int i=0;i<uris.length;i++){
				if(uris[i]!=null){
					result[nctr++] = uris[i];
					if(nctr==counter)
						break;
				}
			}
		}catch(Exception exx){
			Logger log = Logger.getLogger(this.getClass().getSimpleName());
			log.log(Level.SEVERE,"Error Traversing image uris "+exx);
		}
		return null;
	}
	
	public String[] getFieldMLUris(){
		try{
			String fmlXML = new String(fieldMLUriXML);
			Document doc = DocumentBuilderFactory.newInstance().newDocumentBuilder().parse(fmlXML);
			NodeList nList = doc.getElementsByTagName("FIELDML");
			String[] uris = new String[200];
			int counter = 0;
			for (int i = 0; i < nList.getLength(); i++) {
				Node nNode = nList.item(i);
				NamedNodeMap attrs = nNode.getAttributes();
				int id = 0;
				for (int j = 0; j < attrs.getLength(); j++) {
					Attr attribute = (Attr) attrs.item(j);
					if (attribute.getName().trim().equalsIgnoreCase("id")) {
						id = Integer.parseInt(attribute.getValue().trim());
						break;
					}
				}
				NodeList text = nNode.getChildNodes();
				String uri = ((Node) text.item(0)).getNodeValue().trim();
				try{
					uris[id] = uri;
					counter++;
				}catch(Exception exx){
					Logger log = Logger.getLogger(this.getClass().getSimpleName());
					log.log(Level.SEVERE,"Error Traversing fieldML uris "+uri+"\t is associated with id "+id);
				}
			}
			String[] result = new String[counter];
			int nctr = 0;
			for(int i=0;i<uris.length;i++){
				if(uris[i]!=null){
					result[nctr++] = uris[i];
					if(nctr==counter)
						break;
				}
			}
		}catch(Exception exx){
			Logger log = Logger.getLogger(this.getClass().getSimpleName());
			log.log(Level.SEVERE,"Error Traversing fieldML uris "+exx);
		}
		return null;
	}
	
	public String getBlobXML() throws Exception{
		final int BUFFER = 2048;
		
		//The blob is the zipped overlay data
		// Create a new input stream as the input stream is closed by downstream
		// processing
		SerialBlob msBlob = new SerialBlob(metaData);
		
		CheckedInputStream checksum = new CheckedInputStream(msBlob.getBinaryStream(),
				new Adler32());
		GZIPInputStream zis = new GZIPInputStream(new BufferedInputStream(
				checksum));

		int count = -1;
		byte data[] = new byte[BUFFER];

		ByteArrayOutputStream fos = new ByteArrayOutputStream();
		try{
			while ((count = zis.read(data, 0, BUFFER)) != -1) {
				fos.write(data, 0, count);
			}
		}catch(Exception exx){
			exx.printStackTrace();
		}
		fos.close();
		String xmlString = fos.toString();
		zis.close();
		
		return xmlString;
	}
	
	public void setXMLAsBlob(String xmlString) throws Exception{
		ByteArrayOutputStream compressed = new ByteArrayOutputStream(xmlString.length());
		GZIPOutputStream zos = new GZIPOutputStream(compressed);
		zos.write(xmlString.getBytes());
		zos.close();
		metaData = compressed.toByteArray();
	}
	
	
	public String getJSON() throws Exception{
		Properties bundle = new Properties();
		bundle.loadFromXML(new ByteArrayInputStream(getBlobXML().getBytes()));
		String modelJSON = bundle.getProperty("MODELJSON");
		return modelJSON;
	}
	
}
