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
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Map.Entry;
import java.util.Vector;

import javax.annotation.PreDestroy;
import javax.annotation.Resource;
import javax.ejb.LocalBean;
import javax.ejb.Singleton;
import javax.servlet.http.HttpSession;

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
	private Hashtable<Long, HttpSession> resourceSessions;
	private Hashtable<Long, File> resourceDirectories;

	/**
	 * Default constructor.
	 */
	public ResourceManager() {
		resourceCreationTimes = new Vector<Long>();
		resourceSessions = new Hashtable<Long, HttpSession>();
		resourceDirectories = new Hashtable<Long, File>();
	}

	public synchronized File createWorkingDirectory(HttpSession session) {
		// Create working directory
		String scratch = diskScratchSpace;
		String scratchDir = scratch + "/WSSpeckle" + Math.random();
		File sd = new File(scratchDir);
		while (sd.exists()) {
			scratchDir = scratch + "/WSSpeckle" + Math.random();
			sd = new File(scratchDir);
		}
		sd.mkdirs();
		Long timeStamp = new Long(System.currentTimeMillis());
		session.setAttribute("rid", timeStamp); // For comparisons
		resourceCreationTimes.add(timeStamp);
		resourceDirectories.put(timeStamp, sd);
		resourceSessions.put(timeStamp, session);

		return sd;
	}

	// @Schedule(hour = "*/1", persistent = false)
	public void purgeUnusedResources() {
		if (resourceCreationTimes.size() > 0) {
			Long timeStamp = new Long(System.currentTimeMillis() - 1 * 60 * 60
					* 1000);
			Enumeration<Long> times = resourceCreationTimes.elements();
			while (times.hasMoreElements()) {
				Long myTime = times.nextElement();
				if (myTime <= timeStamp) {
					resourceCreationTimes.remove(myTime);
					resourceSessions.remove(myTime);
					File resource = resourceDirectories.get(myTime);
					try {
						if (resource.exists()) // Sometimes if cleanup is called
												// the resource is deleted
							deleteDir(resource);
						resourceDirectories.remove(myTime);
					} catch (Exception exx) {
						Logger log = Logger.getLogger(this.getClass()
								.getSimpleName());
						log.log(Level.INFO,
								"Exception occured while removing resource "
										+ resource.getAbsolutePath() + "\n"
										+ exx.getMessage());
					}
				} else {
					break;
				}
			}
		}
	}

	public void cleanUp(HttpSession session) {
		Long key = null;
		try {
			Object rid = session.getAttribute("rid");
			if (resourceSessions.containsValue(session) == true) {
				ArrayList<Entry<Long, HttpSession>> entries = new ArrayList<Entry<Long, HttpSession>>(
						resourceSessions.entrySet());
				for (Entry<Long, HttpSession> entry : entries) {
					HttpSession rses = entry.getValue();
					if (rses.getAttribute("rid") == rid) {
						key = entry.getKey();
						break;
					}
				}
				if (key != null) {
					resourceCreationTimes.remove(key);
					resourceSessions.remove(key);
					File resource = resourceDirectories.get(key);
					try {
						if (resource.exists()) // Sometimes if cleanup is called
												// the
												// resource is deleted
							deleteDir(resource);
						resourceDirectories.remove(key);
					} catch (Exception exx) {
						Logger log = Logger.getLogger(this.getClass()
								.getSimpleName());
						log.log(Level.INFO,
								"Exception occured while removing resource "
										+ resource.getAbsolutePath() + "\n"
										+ exx.getMessage());
					}
				}
			}
		} catch (Exception exx) {// Sometimes the session is already invalidated
									// in such cases, clean up all invalidated
									// sessions
			ArrayList<Entry<Long, HttpSession>> entries = new ArrayList<Entry<Long, HttpSession>>(
					resourceSessions.entrySet());
			for (Entry<Long, HttpSession> entry : entries) {
				try {
					 entry.getValue();
				} catch (Exception fail) {// invalidated session will fail, so
											// clean up
					Long myTime = entry.getKey();
					resourceCreationTimes.remove(myTime);
					resourceSessions.remove(myTime);
					File resource = resourceDirectories.get(myTime);
					try {
						if (resource.exists()) // Sometimes if cleanup is called
												// the resource is deleted
							deleteDir(resource);
						resourceDirectories.remove(myTime);
					} catch (Exception ext) {
						Logger log = Logger.getLogger(this.getClass()
								.getSimpleName());
						log.log(Level.INFO,
								"Exception occured while removing resource "
										+ resource.getAbsolutePath() + "\n"
										+ ext.getMessage());
					}
				}
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
				resourceSessions.remove(myTime);
				File resource = resourceDirectories.get(myTime);
				try {
					if (resource.exists()) // Sometimes if cleanup is called the
											// resource is deleted
						deleteDir(resource);
					resourceDirectories.remove(myTime);
				} catch (Exception exx) {
					Logger log = Logger.getLogger(this.getClass()
							.getSimpleName());
					log.log(Level.INFO,
							"Exception occured while removing resource "
									+ resource.getAbsolutePath() + "\n"
									+ exx.getMessage());
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
