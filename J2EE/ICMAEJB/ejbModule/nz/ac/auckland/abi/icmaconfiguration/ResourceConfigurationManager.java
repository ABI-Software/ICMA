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

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.List;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.annotation.PostConstruct;
import javax.annotation.PreDestroy;
import javax.annotation.Resource;
import javax.ejb.EJB;
import javax.ejb.LocalBean;
import javax.ejb.Stateless;
import javax.ejb.TransactionAttribute;
import javax.ejb.TransactionAttributeType;
import javax.jcr.Binary;
import javax.jcr.Node;
import javax.jcr.NodeIterator;
import javax.jcr.Property;
import javax.jcr.PropertyIterator;
import javax.jcr.Repository;
import javax.jcr.Session;
import javax.jcr.SimpleCredentials;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;

import nz.ac.auckland.abi.businesslogic.FEMModelsBeanRemote;
import nz.ac.auckland.abi.businesslogic.ICMADatabaseAdministrationRemote;
import nz.ac.auckland.abi.workflow.WSWorkflowInfo;

/*
 * Ensure that all calls to this method can be serviced within the 5000 millisecond limit
 * This may become stringent if there are very large number of concurrent users
 */
@Stateless
@LocalBean
public class ResourceConfigurationManager {

	@Resource(name = "java:global/ICMADISKSCRATCHSPACE")
	private String diskScratchSpace;

	@Resource(name = "java:global/ICMACMSUSERID")
	private String icmaCMSUserID;

	@Resource(name = "java:global/ICMACMSPASSWD")
	private String icmaCMSPasswd;

	@Resource(name = "java:global/ICMACMSRPNAME")
	private String icmaCMSRepositoryName;

	@Resource(name = "java:global/ICMASYNCMODELSTOPACS")
	private String icmaSyncModelsToPacs;

	@PersistenceContext(unitName = "ICMADB")
	private EntityManager entityManager;

	@Resource(mappedName = "java:/jcr/icmaCMS")
	private Repository repository;

	@EJB
	private ResourceInitializer resourceInitializer;

	@EJB
	private ProcessManager processManager;

	@EJB
	private FEMModelsBeanRemote femmodelsBean;

	private Logger log = null;

	private static int id = 0;

	private int myId = 0;

	private Hashtable<String, WSWorkflowInfo> wsWorkFlows;

	private Hashtable<String, Object> cache;

	/*
	 * For reasons discussed the following transaction attribute is created
	 * https://community.jboss.org/message/777925
	 */

	@PostConstruct
	@TransactionAttribute(TransactionAttributeType.NEVER)
	public void initCMS() throws Exception {
		log = Logger.getLogger(this.getClass().getSimpleName());
		wsWorkFlows = resourceInitializer.getWsWorkFlows();
		cache = resourceInitializer.getCache();
		myId = id++;
		log.log(Level.INFO, "Resource Manager (" + myId + ") instantiated");
	}

	@PreDestroy
	public void cleanUp() {
		removeDirectory(diskScratchSpace);
	}

	public ProcessManager getProcessManager() {
		return processManager;
	}

	// Schedule a time that checks for workflows thats are older than 3 hours
	// If so such workflows should be deleted

	public void dropCMS() throws Exception {
		Session session = repository.login(new SimpleCredentials(icmaCMSUserID, icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		Node rootNode = session.getRootNode().getNode("ICMADOCS");

		rootNode.remove();
		session.save();
		rootNode = session.getRootNode().addNode("ICMADOCS", "nt:folder");
		session.save();
		session.logout();
	}

	/*
	 * @return the jcr Node to the node where the image is stored
	 */
	public String addImageToCMS(String studyID, String instanceID, String imageID, File imageFile) throws Exception {
		Session session = repository.login(new SimpleCredentials(icmaCMSUserID, icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		Node rootNode = session.getRootNode().getNode("ICMADOCS");
		final String mimeType = "image/jpeg";
		InputStream fileStream = new FileInputStream(imageFile);
		String id = imageID;
		Node content = null;
		try {
			Node study = rootNode.getNode(studyID);
			try {
				Node instance = study.getNode(instanceID);
				try {
					Node object = instance.getNode(id);
					if (object.getNode("jcr:content") == null) {
						content = object.addNode("jcr:content", "nt:resource");
					} else {
						try {
							content = object.getNode("jcr:content");
							content.remove();
							// Without this Constraint violation exception will
							// be thrown
							content = object.addNode("jcr:content", "nt:resource");
						} catch (javax.jcr.PathNotFoundException pfn2) {
						}
					}
				} catch (javax.jcr.PathNotFoundException pfn2) {
					Node object = instance.addNode(id, "nt:file");
					content = object.addNode("jcr:content", "nt:resource");
				}
			} catch (javax.jcr.PathNotFoundException pfn1) {
				Node instance = study.addNode(instanceID, "nt:folder");
				Node object = instance.addNode(id, "nt:file");
				content = object.addNode("jcr:content", "nt:resource");
			}
		} catch (javax.jcr.PathNotFoundException pfn) {
			Node study = rootNode.addNode(studyID, "nt:folder");
			Node instance = study.addNode(instanceID, "nt:folder");
			Node object = instance.addNode(id, "nt:file");
			content = object.addNode("jcr:content", "nt:resource");
		}

		Binary binary = session.getValueFactory().createBinary(fileStream);
		content.setProperty("jcr:data", binary);
		content.setProperty("jcr:mimeType", mimeType);
		fileStream.close();
		session.save();
		String path = content.getParent().getPath();
		session.logout();

		return path;
	}

	/*
	 * @return the jcr Node to the node where the text is stored
	 */

	public String addTextToCMS(String studyID, String instanceID, String txtID, File textFile) throws Exception {
		Session session = repository.login(new SimpleCredentials(icmaCMSUserID, icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		Node rootNode = session.getRootNode().getNode("ICMADOCS");
		final String mimeType = "text/plain";
		InputStream fileStream = new FileInputStream(textFile);

		String id = txtID;
		Node content = null;
		try {
			Node study = rootNode.getNode(studyID);
			try {
				Node instance = study.getNode(instanceID);
				try {
					Node object = instance.getNode(id);
					if (object.getNode("jcr:content") == null) {
						content = object.addNode("jcr:content", "nt:resource");
					} else {
						try {
							content = object.getNode("jcr:content");
							content.remove();
							// Without this Constraint violation exception will
							// be thrown
							content = object.addNode("jcr:content", "nt:resource");
						} catch (javax.jcr.PathNotFoundException pfn2) {
						}
					}
				} catch (javax.jcr.PathNotFoundException pfn2) {
					Node object = instance.addNode(id, "nt:file");
					content = object.addNode("jcr:content", "nt:resource");
				}
			} catch (javax.jcr.PathNotFoundException pfn1) {
				Node instance = study.addNode(instanceID, "nt:folder");
				Node object = instance.addNode(id, "nt:file");
				content = object.addNode("jcr:content", "nt:resource");
			}
		} catch (javax.jcr.PathNotFoundException pfn) {
			Node study = rootNode.addNode(studyID, "nt:folder");
			Node instance = study.addNode(instanceID, "nt:folder");
			Node object = instance.addNode(id, "nt:file");
			content = object.addNode("jcr:content", "nt:resource");
		}

		Binary binary = session.getValueFactory().createBinary(fileStream);
		content.setProperty("jcr:data", binary);
		content.setProperty("jcr:mimeType", mimeType);
		fileStream.close();
		session.save();
		String path = content.getParent().getPath();
		session.logout();
		return path;
	}

	/*
	 * @return the jcr Node to the node where the zip file is stored
	 */

	public String addZipToCMS(String studyID, String instanceID, String txtID, File zipFile) throws Exception {
		Session session = repository.login(new SimpleCredentials(icmaCMSUserID, icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		Node rootNode = session.getRootNode().getNode("ICMADOCS");
		final String mimeType = "application/x-gzip";
		InputStream fileStream = new FileInputStream(zipFile);

		String id = txtID;
		Node content = null;
		try {
			Node study = rootNode.getNode(studyID);
			try {
				Node instance = study.getNode(instanceID);
				try {
					Node object = instance.getNode(id);
					if (object.getNode("jcr:content") == null) {
						content = object.addNode("jcr:content", "nt:resource");
					} else {
						try {
							content = object.getNode("jcr:content");
							content.remove();
							// Without this Constraint violation exception will
							// be thrown
							content = object.addNode("jcr:content", "nt:resource");
						} catch (javax.jcr.PathNotFoundException pfn2) {
						}
					}
				} catch (javax.jcr.PathNotFoundException pfn2) {
					Node object = instance.addNode(id, "nt:file");
					content = object.addNode("jcr:content", "nt:resource");
				}
			} catch (javax.jcr.PathNotFoundException pfn1) {
				Node instance = study.addNode(instanceID, "nt:folder");
				Node object = instance.addNode(id, "nt:file");
				content = object.addNode("jcr:content", "nt:resource");
			}
		} catch (javax.jcr.PathNotFoundException pfn) {
			Node study = rootNode.addNode(studyID, "nt:folder");
			Node instance = study.addNode(instanceID, "nt:folder");
			Node object = instance.addNode(id, "nt:file");
			content = object.addNode("jcr:content", "nt:resource");
		}

		Binary binary = session.getValueFactory().createBinary(fileStream);
		content.setProperty("jcr:data", binary);
		content.setProperty("jcr:mimeType", mimeType);
		fileStream.close();
		session.save();
		String path = content.getParent().getPath();
		session.logout();
		return path;
	}

	public String getZipFromCMS(String studyID, String instanceID, String id) throws Exception {
		Session session = repository.login(new SimpleCredentials(icmaCMSUserID, icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		Node rootNode = session.getRootNode().getNode("ICMADOCS");
		try {
			Node study = rootNode.getNode(studyID);
			Node instance = study.getNode(instanceID);

			Node object = instance.getNode(id);
			if (object.getNode("jcr:content") != null) {
				return object.getPath();
			}
		} catch (javax.jcr.PathNotFoundException pfn) {

		}
		return null;
	}

	/*
	 * @return the jcr Node to the node where the text is stored
	 */
	public String addBytesToCMS(String studyID, String instanceID, String bID, String mimeType, byte[] array) throws Exception {

		InputStream fileStream = new ByteArrayInputStream(array);
		Session session = repository.login(new SimpleCredentials(icmaCMSUserID, icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		Node rootNode = session.getRootNode().getNode("ICMADOCS");

		String id = bID;
		Node content = null;
		try {
			Node study = rootNode.getNode(studyID);
			try {
				Node instance = study.getNode(instanceID);
				try {
					Node object = instance.getNode(id);
					if (object.getNode("jcr:content") == null) {
						content = object.addNode("jcr:content", "nt:resource");
					} else {
						try {
							content = object.getNode("jcr:content");
							content.remove();
							// Without this Constraint violation exception will
							// be thrown
							content = object.addNode("jcr:content", "nt:resource");
						} catch (javax.jcr.PathNotFoundException pfn2) {
						}
					}
				} catch (javax.jcr.PathNotFoundException pfn2) {
					Node object = instance.addNode(id, "nt:file");
					content = object.addNode("jcr:content", "nt:resource");
				}
			} catch (javax.jcr.PathNotFoundException pfn1) {
				Node instance = study.addNode(instanceID, "nt:folder");
				Node object = instance.addNode(id, "nt:file");
				content = object.addNode("jcr:content", "nt:resource");
			}
		} catch (javax.jcr.PathNotFoundException pfn) {
			Node study = rootNode.addNode(studyID, "nt:folder");
			Node instance = study.addNode(instanceID, "nt:folder");
			Node object = instance.addNode(id, "nt:file");
			content = object.addNode("jcr:content", "nt:resource");
		}

		Binary binary = session.getValueFactory().createBinary(fileStream);
		content.setProperty("jcr:data", binary);
		content.setProperty("jcr:mimeType", mimeType);
		fileStream.close();
		session.save();
		String path = content.getParent().getPath();
		session.logout();
		return path;
	}

	public String addMovieToCMS(String studyID, String instanceID, String bID, File input) throws Exception {
		String mimeType = "video/mpeg";
		// Serve with video mimetypes
		if (bID.endsWith("webm")) {
			mimeType = "video/webm";
		} else if (bID.endsWith("mp4")) {
			mimeType = "video/mp4";
		} else if (bID.endsWith("oggv")) {
			mimeType = "video/ogg";
		}
		Session session = repository.login(new SimpleCredentials(icmaCMSUserID, icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		Node rootNode = session.getRootNode().getNode("ICMADOCS");

		FileInputStream fileStream = new FileInputStream(input);
		String id = bID;
		Node content = null;
		try {
			Node study = rootNode.getNode(studyID);
			try {
				Node instance = study.getNode(instanceID);
				try {
					Node object = instance.getNode(id);
					if (object.getNode("jcr:content") == null) {
						content = object.addNode("jcr:content", "nt:resource");
					} else {
						try {
							content = object.getNode("jcr:content");
							content.remove();
							// Without this Constraint violation exception will
							// be thrown
							content = object.addNode("jcr:content", "nt:resource");
						} catch (javax.jcr.PathNotFoundException pfn2) {
						}
					}
				} catch (javax.jcr.PathNotFoundException pfn2) {
					Node object = instance.addNode(id, "nt:file");
					content = object.addNode("jcr:content", "nt:resource");
				}
			} catch (javax.jcr.PathNotFoundException pfn1) {
				Node instance = study.addNode(instanceID, "nt:folder");
				Node object = instance.addNode(id, "nt:file");
				content = object.addNode("jcr:content", "nt:resource");
			}
		} catch (javax.jcr.PathNotFoundException pfn) {
			Node study = rootNode.addNode(studyID, "nt:folder");
			Node instance = study.addNode(instanceID, "nt:folder");
			Node object = instance.addNode(id, "nt:file");
			content = object.addNode("jcr:content", "nt:resource");
		}

		Binary binary = session.getValueFactory().createBinary(fileStream);
		content.setProperty("jcr:data", binary);
		content.setProperty("jcr:mimeType", mimeType);
		fileStream.close();
		session.save();
		String path = content.getParent().getPath();
		session.logout();
		return path;
	}

	public String addUSERDoc(String username, String fileID, String mimeType, File file) throws Exception {
		Session session = repository.login(new SimpleCredentials(icmaCMSUserID, icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		Node rootNode = session.getRootNode().getNode("ICMAUSERDOCS");

		FileInputStream fileStream = new FileInputStream(file);
		String id = fileID;
		Node content = null;
		try {
			Node user = rootNode.getNode(username);
			try {
				Node instance = user.getNode("Analysis");
				try {
					Node object = instance.getNode(id);
					if (object.getNode("jcr:content") == null) {
						content = object.addNode("jcr:content", "nt:resource");
					} else {
						try {
							content = object.getNode("jcr:content");
							content.remove();
							// Without this Constraint violation exception will
							// be thrown
							content = object.addNode("jcr:content", "nt:resource");
						} catch (javax.jcr.PathNotFoundException pfn2) {
						}
					}
				} catch (javax.jcr.PathNotFoundException pfn2) {
					Node object = instance.addNode(id, "nt:file");
					content = object.addNode("jcr:content", "nt:resource");
				}
			} catch (javax.jcr.PathNotFoundException pfn1) {
				Node instance = user.addNode("Analysis", "nt:folder");
				Node object = instance.addNode(id, "nt:file");
				content = object.addNode("jcr:content", "nt:resource");
			}
		} catch (javax.jcr.PathNotFoundException pfn) {
			Node user = rootNode.addNode(username, "nt:folder");
			Node instance = user.addNode("Analysis", "nt:folder");
			Node object = instance.addNode(id, "nt:file");
			content = object.addNode("jcr:content", "nt:resource");
		}

		Binary binary = session.getValueFactory().createBinary(fileStream);
		content.setProperty("jcr:data", binary);
		content.setProperty("jcr:mimeType", mimeType);
		fileStream.close();
		session.save();
		String path = content.getParent().getPath();
		session.logout();
		return path;
	}

	/*
	 * @return the jcr Node to the node where the text is stored
	 */
	public String addWorkFlowDocs(String workflowID, String timeStamp, String bID, String mimeType, byte[] array) throws Exception {

		InputStream fileStream = new ByteArrayInputStream(array);
		Session session = repository.login(new SimpleCredentials(icmaCMSUserID, icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		Node rootNode = session.getRootNode().getNode("ICMAWORKFLOWDOCS");

		String id = bID;
		Node content = null;
		try {
			Node study = rootNode.getNode(workflowID);
			try {
				Node instance = study.getNode(timeStamp);
				try {
					Node object = instance.getNode(id);
					if (object.getNode("jcr:content") == null) {
						content = object.addNode("jcr:content", "nt:resource");
					} else {
						try {
							content = object.getNode("jcr:content");
							content.remove();
							// Without this Constraint violation exception will
							// be thrown
							content = object.addNode("jcr:content", "nt:resource");
						} catch (javax.jcr.PathNotFoundException pfn2) {
						}
					}
				} catch (javax.jcr.PathNotFoundException pfn2) {
					Node object = instance.addNode(id, "nt:file");
					content = object.addNode("jcr:content", "nt:resource");
				}
			} catch (javax.jcr.PathNotFoundException pfn1) {
				Node instance = study.addNode(timeStamp, "nt:folder");
				Node object = instance.addNode(id, "nt:file");
				content = object.addNode("jcr:content", "nt:resource");
			}
		} catch (javax.jcr.PathNotFoundException pfn) {
			Node study = rootNode.addNode(workflowID, "nt:folder");
			Node instance = study.addNode(timeStamp, "nt:folder");
			Node object = instance.addNode(id, "nt:file");
			content = object.addNode("jcr:content", "nt:resource");
		}

		Binary binary = session.getValueFactory().createBinary(fileStream);
		content.setProperty("jcr:data", binary);
		content.setProperty("jcr:mimeType", mimeType);
		fileStream.close();
		session.save();
		String path = content.getParent().getPath();
		session.logout();
		return path;
	}

	public List<String> getWorkFlowDocs(String workflowID, String timeStamp) throws Exception {
		Session localSession = repository.login(new SimpleCredentials(icmaCMSUserID, icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		Node localRootNode = localSession.getRootNode().getNode("ICMAWORKFLOWDOCS");
		Node target = localRootNode.getNode(workflowID);
		target = target.getNode(timeStamp);

		ArrayList<String> paths = new ArrayList<String>();
		getLeafNodes(target, paths);
		localSession.logout();
		return paths;
	}

	public List<String> getUserDocs(String username, String dir) throws Exception {
		Session localSession = repository.login(new SimpleCredentials(icmaCMSUserID, icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		Node localRootNode = localSession.getRootNode().getNode("ICMAUSERDOCS");
		Node target = localRootNode.getNode(username);
		if (dir != null) {
			target = target.getNode(dir);
		}

		ArrayList<String> paths = new ArrayList<String>();
		getLeafNodes(target, paths);
		localSession.logout();
		return paths;
	}

	private void getLeafNodes(Node n, ArrayList<String> paths) {
		try {
			boolean isfile = n.getPath().endsWith("jcr:content");
			if (!isfile) {
				javax.jcr.NodeIterator nodeItr = n.getNodes();
				while (nodeItr.hasNext()) {
					Node nx = nodeItr.nextNode();
					getLeafNodes(nx, paths);
				}
			} else {
				paths.add(n.getParent().getPath());
			}
		} catch (Exception exx) {
			exx.printStackTrace();
		}
	}

	public void removeUSERDoc(Vector<String> nodes) throws Exception {
		Session localSession = repository.login(new SimpleCredentials(icmaCMSUserID, icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		Node localRootNode = localSession.getRootNode().getNode("ICMAUSERDOCS");
		for (String node : nodes) {
			try {
				if (node.startsWith("/ICMAUSERDOCS")) {
					node = node.substring(node.indexOf("/ICMAUSERDOCS") + 14);
				}
				Node target = localRootNode.getNode(node);
				target.remove();
				localSession.save();
			} catch (Exception exx) {

			}
		}
		localSession.logout();
	}

	public void removeCMSNodes(Vector<String> nodes) throws Exception {
		Session localSession = repository.login(new SimpleCredentials(icmaCMSUserID, icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		Node localRootNode = localSession.getRootNode().getNode("ICMADOCS");
		for (String node : nodes) {
			try {
				Node target = localRootNode.getNode(node);
				target.remove();
				localSession.save();
			} catch (Exception exx) {

			}
		}
		localSession.logout();
	}

	public String getDiskScratchSpace() {
		return diskScratchSpace;
	}

	public CMSContent getCMSNodeAt(String path) throws Exception {
		Session localSession = repository.login(new SimpleCredentials(icmaCMSUserID, icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		Node localRootNode = null;
		if (path.startsWith("/ICMAUSERDOCS"))
			localRootNode = localSession.getRootNode().getNode("ICMAUSERDOCS");
		else if (path.startsWith("/ICMAWORKFLOWDOCS"))
			localRootNode = localSession.getRootNode().getNode("ICMAWORKFLOWDOCS");
		else
			localRootNode = localSession.getRootNode().getNode("ICMADOCS");
		int offset = 0;
		String paths[] = path.split("/");

		if (path.startsWith("/ICMADOCS") || path.startsWith("/ICMAUSERDOCS") || path.startsWith("/ICMAWORKFLOWDOCS"))
			offset += 2; // skip empty space and ICMADOCS
		Node study = null;
		Node instance = null;
		Node object = null;
		Node xNode = null;
		try {
			study = localRootNode.getNode(paths[offset++]);
			instance = study.getNode(paths[offset++]);
			object = instance.getNode(paths[offset++]);
			xNode = object.getNode("jcr:content");
			String mimeType = xNode.getProperty("jcr:mimeType").getString();
			Binary stream = xNode.getProperty("jcr:data").getBinary();
			CMSContent content = new CMSContent(mimeType, stream.getStream());
			localSession.logout();
			return content;
		} catch (Exception exx) {

/*			if (study == null) {
				System.out.println("Study not found");
			}
			if (instance == null) {
				System.out.println("Instance not found");
				NodeIterator itr = study.getNodes();
				while (itr.hasNext()) {
					System.out.println("\t" + itr.next().toString());
				}
			}
			if (object == null) {
				System.out.println("Object not found");
				NodeIterator itr = instance.getNodes();
				while (itr.hasNext()) {
					System.out.println("\t" + itr.next().toString());
				}
			}*/

			throw exx;
		}
	}

	public boolean removeStudy(String studyID) throws Exception {
		Session session = repository.login(new SimpleCredentials(icmaCMSUserID, icmaCMSPasswd.toCharArray()), icmaCMSRepositoryName);
		Node rootNode = session.getRootNode().getNode("ICMADOCS");
		try {
			Node study = rootNode.getNode(studyID);
			study.remove();
			session.save();
			session.logout();
		} catch (Exception exx) {
			log.log(Level.INFO, "Failed to access CMS record for study " + studyID);
			for (NodeIterator i = rootNode.getNodes(); i.hasNext();) {
				Node nn = i.nextNode();
				log.log(Level.FINE, nn.getPath() + " # " + nn.getName());
				for (PropertyIterator pi = nn.getProperties(); pi.hasNext();) {
					Property p = pi.nextProperty();
					log.log(Level.FINE, "\t" + p.getName());
				}
			}
			throw exx;
		}
		return true;
	}

	// Removes models that have not been saved
	public void purgeUnsavedWSWorkflowModels(ICMADatabaseAdministrationRemote admin) {
		long timeStamp = System.currentTimeMillis();
		Enumeration<String> workflowInsts = wsWorkFlows.keys();
		while (workflowInsts.hasMoreElements()) {
			String key = workflowInsts.nextElement();
			WSWorkflowInfo thread = wsWorkFlows.get(key);
			double tStamp = (timeStamp - thread.getTimeStamp()) / 60000; // minutes
			if (tStamp >= 60) {
				try {
					// admin.removeModel(thread.instanceID, "purge");
					femmodelsBean.removeModel(thread.getInstanceID(), "purge");
					thread.cleanup();
					wsWorkFlows.remove(key);
				} catch (Exception exx) {

				}
			}
		}
	}

	public void addWsWorkflow(String id, WSWorkflowInfo info) {
		wsWorkFlows.put(id, info);
	}

	public WSWorkflowInfo getWsWorkFlowInfo(String id) {
		return wsWorkFlows.get(id);
	}

	public void removeWsWorkFlow(String id) {
		WSWorkflowInfo info = wsWorkFlows.get(id);
		info.cleanup();
		wsWorkFlows.remove(id);
	}

	public String findModelWsWorkFlow(String modelID) throws Exception {
		for (String infoKey : wsWorkFlows.keySet()) {
			WSWorkflowInfo info = wsWorkFlows.get(infoKey);
			if (info.getInstanceID().equalsIgnoreCase(modelID)) {
				return infoKey;
			}
		}
		return null;
	}

	public boolean syncModelsWithPACS() {
		if (icmaSyncModelsToPacs != null) {
			if (icmaSyncModelsToPacs.trim().equalsIgnoreCase("TRUE")) {
				return true;
			}
		}
		return false;
	}

	public static boolean removeDirectory(String file) {
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

	public FEMModelsBeanRemote getModelManager() {
		return femmodelsBean;
	}

	public void addToCache(String key, Object obj) {
		cache.put(key, obj);
	}

}
