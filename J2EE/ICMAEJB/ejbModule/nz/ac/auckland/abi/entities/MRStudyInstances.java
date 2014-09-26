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

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.Serializable;
import java.io.StringWriter;
import java.util.Enumeration;
import java.util.Properties;

import javax.persistence.Basic;
import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.FetchType;
import javax.persistence.Id;
import javax.persistence.Lob;
import javax.sql.rowset.serial.SerialBlob;

import nz.ac.auckland.abi.helper.DICOMMetaDataFilter;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

@Entity(name = "MRStudyInstances")
public class MRStudyInstances implements Serializable{
	private static final long serialVersionUID = 5L;
	
	@Id
    @Column(name = "study_id")
    private String studyID;
	
    @Id
	@Column(name = "instance_id")
    private String instanceID;
    
	@Column(name = "series_id")
    private String seriesID;  

	@Column(name = "position_patient")
    private String positionPatient;
    
	@Column(name = "orientation_patient")
    private String orientationPatient;

    @Column(name = "head_shot_uri")
    private String headShotUri;

    @Column(name = "mp4_uri")
    private String mp4Uri;
 	
    @Column(name = "webm_uri")
    private String webmUri;
    
    @Column(name = "ogv_uri")
    private String ogvUri;

    @Column(name = "preamble") 
    private String preamble;
    
    @Column(name = "metadata")
    @Lob @Basic(fetch=FetchType.LAZY)
    private byte[] metadata;
    
    
	
	public MRStudyInstances(){
		super();
	}
	
	public MRStudyInstances(String sid, String iid, String srid, String position, String orientation, String headuri, String muri, String wuri, String ouri, String preamble, String meta) throws Exception{
		studyID = sid;
		instanceID = iid;
		seriesID = srid;
		positionPatient = position;
		orientationPatient = orientation;
		headShotUri = headuri;
		mp4Uri = muri;
		webmUri = wuri;
		ogvUri = ouri;
		this.preamble = preamble;
		metadata = meta.getBytes();
	}
    
	@Override
 	public String toString(){
 		return "Instance : "+studyID+"\t"+seriesID+"\t"+instanceID+"\t"+preamble+"\t"+headShotUri;
 	}

	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result
				+ ((instanceID == null) ? 0 : instanceID.hashCode());
		result = prime * result
				+ ((seriesID == null) ? 0 : seriesID.hashCode());
		result = prime * result + ((studyID == null) ? 0 : studyID.hashCode());
		return result;
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		MRStudyInstances other = (MRStudyInstances) obj;
		if (instanceID == null) {
			if (other.instanceID != null)
				return false;
		} else if (!instanceID.equals(other.instanceID))
			return false;
		if (seriesID == null) {
			if (other.seriesID != null)
				return false;
		} else if (!seriesID.equals(other.seriesID))
			return false;
		if (studyID == null) {
			if (other.studyID != null)
				return false;
		} else if (!studyID.equals(other.studyID))
			return false;
		return true;
	}



	public String getJSON() throws Exception {
		JSONObject study = new JSONObject();
		study.put("type", "MR");
		study.put("seriesid", seriesID);							
		study.put("planeid", instanceID);
		Properties myproperties = new Properties();
		//Access metadata and get the cms uri for images
		SerialBlob msBlob = new SerialBlob(metadata);
		StringWriter writer = new StringWriter();
		InputStream is = msBlob.getBinaryStream();
		char[] buffer = new char[1024];
        try {
            Reader reader = new BufferedReader(
                    new InputStreamReader(is, "UTF-8"));
            int n;
            while ((n = reader.read(buffer)) != -1) {
                writer.write(buffer, 0, n);
            }
        } finally {
            is.close();
        }
		String theString = writer.toString();
		myproperties.loadFromXML(new ByteArrayInputStream(theString.getBytes()));
		
		//Get the number of images and image plane position data from the preamble
		JSONObject frames = new JSONObject();
		String[] fdata = preamble.split("#");
		int numframes = Integer.parseInt(fdata[0]);
		frames.put("NUMFRAMES", fdata[0]);
		frames.put("TLC", fdata[1]);
		frames.put("TRC", fdata[2]);
		frames.put("BLC", fdata[3]);
		frames.put("BRC", fdata[4]);
		frames.put("HEIGHT", fdata[5]);
		frames.put("WIDTH", fdata[6]);
		for(int fct=0;fct<numframes;fct++){
			frames.put(""+fct, myproperties.getProperty("IMG"+fct+"url"));
		}
		study.put("frames", frames);
		study.put("beatsPerMin", myproperties.getProperty("HeartRate"));
		study.put("image", headShotUri);
		
		
		JSONObject video = new JSONObject();
		video.put("mp4", mp4Uri);
		video.put("ogg", ogvUri);
		video.put("webm", webmUri);
		study.put("video", video);
		
		JSONArray props = new JSONArray();
		Enumeration<Object> pKeys = myproperties.keys();
		while(pKeys.hasMoreElements()){
			String key = (String) pKeys.nextElement();
			String value = myproperties.getProperty(key);
			if(DICOMMetaDataFilter.accept(key)){
				JSONObject pr = new JSONObject();
    			//Convert property names to Sentence case from camelCase
    			if(!key.startsWith("Comments")){
					pr.put(getSentenceCase(key), value);
					props.add(pr);
    			}else{
					pr.put("Comments", value);
					props.add(pr);    				
    			}
			}
		}
		
		study.put("properties", props);
		
		return study.toJSONString();
	}
	
	public String getSentenceCase(String word){
		StringBuffer buf = new StringBuffer("");
		try{
			for(int i=0;i<word.length();i++){
				if(word.charAt(i)!='_'){
					buf.append(word.charAt(i));
				}else{
					buf.append(" ");					
				}
				
			}
			return buf.toString();
		}catch(Exception exx){
			return word;
		}
	}

	public String getStudyID() {
		return studyID;
	}

	public void setStudyID(String studyID) {
		this.studyID = studyID;
	}

	public String getInstanceID() {
		return instanceID;
	}

	public void setInstanceID(String instanceID) {
		this.instanceID = instanceID;
	}

	public String getSeriesID() {
		return seriesID;
	}

	public void setSeriesID(String seriesID) {
		this.seriesID = seriesID;
	}

	public String getPositionPatient() {
		return positionPatient;
	}

	public void setPositionPatient(String positionPatient) {
		this.positionPatient = positionPatient;
	}

	public String getOrientationPatient() {
		return orientationPatient;
	}

	public void setOrientationPatient(String orientationPatient) {
		this.orientationPatient = orientationPatient;
	}

	public String getHeadShotUri() {
		return headShotUri;
	}

	public void setHeadShotUri(String headShotUri) {
		this.headShotUri = headShotUri;
	}

	public String getMp4Uri() {
		return mp4Uri;
	}

	public void setMp4Uri(String mp4Uri) {
		this.mp4Uri = mp4Uri;
	}

	public String getWebmUri() {
		return webmUri;
	}

	public void setWebmUri(String webmUri) {
		this.webmUri = webmUri;
	}

	public String getOgvUri() {
		return ogvUri;
	}

	public void setOgvUri(String ogvUri) {
		this.ogvUri = ogvUri;
	}

	public String getPreamble() {
		return preamble;
	}

	public void setPreamble(String preamble) {
		this.preamble = preamble;
	}

	public byte[] getMetadata() {
		return metadata;
	}

	public void setMetadata(byte[] metadata) {
		this.metadata = metadata;
	}
}

