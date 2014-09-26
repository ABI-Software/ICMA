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
package nz.ac.auckland.abi.resource;

import java.io.File;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

import javax.annotation.PreDestroy;
import javax.annotation.Resource;
import javax.ejb.LocalBean;
import javax.ejb.Schedule;
import javax.ejb.Singleton;

import nz.ac.auckland.abi.implementations.SpeckleFitting;

import org.jboss.logging.Logger;
import org.jboss.logging.Logger.Level;

/**
 * Session Bean implementation class ResourceManager
 */
@Singleton
@LocalBean
public class ResourceManager {

	@Resource(name = "java:global/ICMADISKSCRATCHSPACE")
	private String diskScratchSpace;

	private Vector<Long> resourceCreationTimes;
	private Hashtable<Long, String> resourceSessions;
	private Hashtable<String, SpeckleFitting> workflowSessions;
	private Hashtable<Long, File> resourceDirectories;
	private Hashtable<String, Long> workflowTimeMap;

	/**
	 * Default constructor.
	 */
	public ResourceManager() {
		resourceCreationTimes = new Vector<Long>();
		resourceSessions = new Hashtable<Long, String>();
		resourceDirectories = new Hashtable<Long, File>();
		workflowSessions = new Hashtable<String, SpeckleFitting>();
		workflowTimeMap = new Hashtable<String, Long>();
	}

	public synchronized File createWorkingDirectory(String workflowID) {
		// Create working directory
		String scratch = diskScratchSpace;
		String scratchDir = scratch + "/WSS2FEM" + Math.random();
		File sd = new File(scratchDir);
		while (sd.exists()) {
			scratchDir = scratch + "/WSS2FEM" + Math.random();
			sd = new File(scratchDir);
		}
		sd.mkdirs();
		Long timeStamp = new Long(System.currentTimeMillis());
		resourceCreationTimes.add(timeStamp);
		resourceDirectories.put(timeStamp, sd);
		resourceSessions.put(timeStamp, workflowID);
		workflowTimeMap.put(workflowID, timeStamp);
		return sd;
	}

	public void associateSession(String workflowID, SpeckleFitting session) {
		workflowSessions.put(workflowID, session);
	}

	public SpeckleFitting getSession(String workflowID) {
		try {
			return workflowSessions.get(workflowID);
		} catch (NullPointerException exx) {
			return null;
		}
	}

	public File getWorkingDirectory(String workflowID) {
		Long key = null;
		/*
		 * ArrayList<Entry<Long, String>> entries = new ArrayList<Entry<Long,
		 * String>>( resourceSessions.entrySet()); for (Entry<Long, String>
		 * entry : entries) { String rses = entry.getValue(); if
		 * (rses.equalsIgnoreCase(workflowID)) { key = entry.getKey(); break; }
		 * }
		 */
		key = workflowTimeMap.get(workflowID);
		if (key != null) {
			return resourceDirectories.get(key);
		}
		return null;
	}

	@Schedule(hour = "*/1", persistent = false)
	public void purgeUnusedResources() {
		if (resourceCreationTimes.size() > 0) {
			Long timeStamp = new Long(System.currentTimeMillis() - 1 * 60 * 60 * 1000);
			Enumeration<Long> times = resourceCreationTimes.elements();
			while (times.hasMoreElements()) {
				Long myTime = times.nextElement();
				if (myTime <= timeStamp) {
					resourceCreationTimes.remove(myTime);
					String session = resourceSessions.get(myTime);
					String workflowID = resourceSessions.get(myTime);
					resourceSessions.remove(myTime);
					workflowTimeMap.remove(workflowID);
					workflowSessions.remove(session);
					File resource = resourceDirectories.get(myTime);
					try {
						if (resource.exists()) // Sometimes if cleanup is called
												// the resource is deleted
							deleteDir(resource);
						resourceDirectories.remove(myTime);
					} catch (Exception exx) {
						Logger log = Logger.getLogger(this.getClass().getSimpleName());
						log.log(Level.INFO, "Exception occured while removing resource " + resource.getAbsolutePath() + "\n" + exx.getMessage());
					}
				} else {
					break;
				}
			}
		}
	}

	public void cleanUp(String workflowID) {
		File resource = null;
		try {
			Long myTime = workflowTimeMap.get(workflowID);
			resourceCreationTimes.remove(myTime);
			String session = resourceSessions.get(myTime);
			resourceSessions.remove(myTime);
			workflowTimeMap.remove(workflowID);
			workflowSessions.remove(session);
			resource = resourceDirectories.get(myTime);

			if (resource.exists()) // Sometimes if cleanup is called
									// the resource is deleted
				deleteDir(resource);
			resourceDirectories.remove(myTime);
		} catch (Exception exx) {
			Logger log = Logger.getLogger(this.getClass().getSimpleName());
			if(resource!=null){
				log.log(Level.INFO, "Exception occured while removing resource " + resource.getAbsolutePath() + "\n" + exx.getMessage());
			}else{
				log.log(Level.INFO, "Exception occured while cleaning workflow" + workflowID + "\n" + exx.getMessage());
			}
		}
	}

	@PreDestroy
	public void removeAllResources() {
		if (resourceCreationTimes.size() > 0) {
			Enumeration<Long> times = resourceCreationTimes.elements();
			while (times.hasMoreElements()) {
				Long myTime = times.nextElement();
				resourceCreationTimes.remove(myTime);
				String key = resourceSessions.get(myTime);
				resourceSessions.remove(myTime);
				workflowSessions.remove(key);
				File resource = resourceDirectories.get(myTime);
				try {
					if (resource.exists()) // Sometimes if cleanup is called the
											// resource is deleted
						deleteDir(resource);
				} catch (Exception exx) {
					Logger log = Logger.getLogger(this.getClass().getSimpleName());
					log.log(Level.INFO, "Exception occured while removing resource " + resource.getAbsolutePath() + "\n" + exx.getMessage());
				}
			}
		}
	}

	private boolean deleteDir(File dir) {
		if (dir.isDirectory()) {
			String[] children = dir.list();
			for (int i = 0; i < children.length; i++) {
				boolean success = deleteDir(new File(dir, children[i]));
				if (!success) {
					return false;
				}
			}
		}
		// The directory is now empty so delete it
		return dir.delete();
	}
}
