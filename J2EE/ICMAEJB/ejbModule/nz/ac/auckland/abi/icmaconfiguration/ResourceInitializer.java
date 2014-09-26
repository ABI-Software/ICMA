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
package nz.ac.auckland.abi.icmaconfiguration;

import java.io.File;
import java.util.Hashtable;

import javax.annotation.PostConstruct;
import javax.annotation.Resource;
import javax.ejb.Singleton;
import javax.ejb.Startup;
import javax.jcr.Node;
import javax.jcr.NodeIterator;
import javax.jcr.Property;
import javax.jcr.PropertyIterator;
import javax.jcr.Repository;
import javax.jcr.RepositoryException;
import javax.jcr.Session;
import javax.jcr.SimpleCredentials;

import nz.ac.auckland.abi.workflow.WSWorkflowInfo;

/**
 * Session Bean implementation class ResourceInitializer
 */
@Singleton
@Startup
public class ResourceInitializer {

	@Resource(name = "java:global/ICMADISKSCRATCHSPACE")
	private String diskScratchSpace;

	@Resource(name = "java:global/ICMACMSUSERID")
	private String icmaCMSUserID;

	@Resource(name = "java:global/ICMACMSPASSWD")
	private String icmaCMSPasswd;

	@Resource(name = "java:global/ICMACMSRPNAME")
	private String icmaCMSRepositoryName;
	
	@Resource(name="java:global/ICMAPARALLELTASKLIMIT")
	private int taskLimit;

	@Resource(mappedName = "java:/jcr/icmaCMS")
	private Repository repository;

	private Hashtable<String, Long>          underBatchProcess;
	
	private Hashtable<String, WSWorkflowInfo> wsWorkflows;

	private Hashtable<String, Object> cache;
	
	private int taskCount = 0;

	/**
	 * Default constructor.
	 */
	public ResourceInitializer() {
		underBatchProcess = new Hashtable<String, Long>();
		wsWorkflows = new Hashtable<String, WSWorkflowInfo>();
		cache = new Hashtable<String, Object>();
	}

	@PostConstruct
	public void init() throws Exception {
		Session session = repository.login(new SimpleCredentials(icmaCMSUserID,
				icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		try {
			session.getRootNode().getNode("ICMADOCS");
			
/*			session.getRootNode().getNode("ICMADOCS").remove();
			session.save();
			session.getRootNode().addNode("ICMADOCS","nt:folder");
			session.save();*/
		} catch (javax.jcr.PathNotFoundException exx) {
			session.getRootNode().addNode("ICMADOCS", "nt:folder");
			session.save();
		}
		try {
			session.getRootNode().getNode("ICMAUSERDOCS");
		} catch (javax.jcr.PathNotFoundException exx) {
			session.getRootNode().addNode("ICMAUSERDOCS", "nt:folder");
			session.save();
		}
		try {
			session.getRootNode().getNode("ICMAWORKFLOWDOCS");
			//traverse(session.getRootNode().getNode("ICMAWORKFLOWDOCS"),0);
			
			//Remove any workflow docs as they are zombies
			session.getRootNode().getNode("ICMAWORKFLOWDOCS").remove();
			session.save();
			session.getRootNode().addNode("ICMAWORKFLOWDOCS","nt:folder");
			session.save();
			 
		} catch (javax.jcr.PathNotFoundException exx) {
			session.getRootNode().addNode("ICMAWORKFLOWDOCS", "nt:folder");
			session.save();
		}
		session.logout();
		// Delete the scratch space
		removeDirectory(diskScratchSpace);
		// Create a new one
		File scratch = new File(diskScratchSpace);
		scratch.mkdirs();
	}


	public Hashtable<String, Long> getBatchProcessSubmissions() {
		return underBatchProcess;
	}
	
	public Hashtable<String, WSWorkflowInfo> getWsWorkFlows() {
		return wsWorkflows;
	}

	public Hashtable<String, Object> getCache() {
		return cache;
	}

	public boolean canStartTask(){
		if(taskCount<taskLimit){
			taskCount++;
			return true;
		}
		return false;
	}
	
	public void taskCompleted(){
		taskCount--;
	}
	
	private boolean removeDirectory(String file) {
		File directory = new File(file);
		if (directory.isFile()) {
			return directory.delete();
		} else {

			String[] list = directory.list();

			// Some JVMs return null for File.list() when the
			// directory is empty.
			if (list != null) {
				for (int i = 0; i < list.length; i++) {
					File entry = new File(directory, list[i]);
					if (entry.isDirectory()) {
						if (!removeDirectory(entry.getPath()))
							return false;
					} else {
						return entry.delete();
					}
				}
			}

			return directory.delete();
		}
	}

	private void traverse(Node n, int level) throws RepositoryException {
		String name = (n.getDepth() == 0) ? "/" : n.getName();
		//System.out.println( name);
		for (PropertyIterator i = n.getProperties(); i.hasNext();) {
			Property p = i.nextProperty();
/*			System.out.println( p.getName() + " = \""
					+ p.getString() + "\"");*/
			//System.out.println( p.getName());
		}
		for (NodeIterator i = n.getNodes(); i.hasNext();) {
			Node nn = i.nextNode();
			traverse(nn, level + 1);
		}
	}
}
