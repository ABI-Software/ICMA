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
package nz.ac.auckland.abi.administration;

import java.security.MessageDigest;
import java.util.HashMap;
import java.util.List;
import java.util.Set;

import javax.ejb.LocalBean;
import javax.ejb.Singleton;
import javax.ejb.TransactionAttribute;
import javax.ejb.TransactionAttributeType;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.persistence.Query;
import javax.resource.spi.SecurityException;

import org.jboss.security.Base64Encoder;
import org.json.simple.JSONObject;

/**
 * Session Bean implementation class ICMAAccessManager
 */
@Singleton
@LocalBean
public class ICMAAccessManager {

	@PersistenceContext(unitName = "ICMADB")
	private EntityManager entityManager;
    /**
     * Default constructor. 
     */
    public ICMAAccessManager() {
        super();
    }

    @TransactionAttribute(TransactionAttributeType.REQUIRED)
    public void changeUserPassword(String username,String oldPass,String newPass) throws Exception{
    	String q = "SELECT u.passwd FROM Users u WHERE u.username='"+username+"'";
		Query nQuery = entityManager.createNativeQuery(q);
		List<String> pks = nQuery.getResultList();
		if(pks.size()!=0){
			if(pks.get(0).equalsIgnoreCase(oldPass)){
				q = "UPDATE Users SET passwd='"+newPass+"' where username='"+username+"'";
				entityManager.createNativeQuery(q).executeUpdate();
				entityManager.flush();
			}else{
				throw new SecurityException("Password Incorrect");
			}
		}else{
			throw new Exception("User "+username+" not found");
		}
    }
    
    
    @TransactionAttribute(TransactionAttributeType.REQUIRED)
    public void addNewUser(String username, String password) throws Exception{
		String q = "SELECT u.username FROM Users u WHERE u.username='"+username+"'";
		Query nQuery = entityManager.createNativeQuery(q);
		List<String> pks = nQuery.getResultList();
		if(pks.size()==0){
			MessageDigest md = MessageDigest.getInstance("SHA-1");
			md.update(password.getBytes());
			byte[] digest = md.digest();
			String hash = Base64Encoder.encode(digest);
			q = "INSERT INTO Users values ('"+username+"','"+hash+"')";
			entityManager.createNativeQuery(q).executeUpdate();
			q = "INSERT INTO UserRoles values ('"+username+"','ICMAUSER')";
			entityManager.createNativeQuery(q).executeUpdate();
			entityManager.flush();
		}else{
			throw new Exception("Username already exists");
		}
    }
    
    @TransactionAttribute(TransactionAttributeType.REQUIRED)
    public void assignAdmin(String username) throws Exception{
    	String q = "SELECT userRoles from UserRoles where username='"+username+"'";
		Query nQuery = entityManager.createNativeQuery(q);
		List<String> pks = nQuery.getResultList();
		boolean found = false;
		for(String role : pks){
			if(role.equalsIgnoreCase("ICMAADMIN")){
				found = true;
				break;
			}
		}
		if(!found){
			q = "INSERT INTO UserRoles values ('"+username+"','ICMAADMIN')";
			entityManager.createNativeQuery(q).executeUpdate();
			entityManager.flush();
		}
    }
    
    public void removeAdmin(String username) throws Exception{
    	String q = "SELECT userRoles from UserRoles where username='"+username+"'";
		Query nQuery = entityManager.createNativeQuery(q);
		List<String> pks = nQuery.getResultList();
		boolean found = false;
		for(String role : pks){
			if(role.equalsIgnoreCase("ICMAADMIN")){
				found = true;
				break;
			}
		}
		if(!found){
			q = "DELETE FROM icmadb.UserRoles where username='"+username+"' and userRoles='ICMAADMIN'";
			entityManager.createNativeQuery(q).executeUpdate();
			entityManager.flush();
		}
    }
    
    public void removeUser(String username) throws Exception{
		String q = "SELECT u.username FROM Users u WHERE u.username='"+username+"'";
		Query nQuery = entityManager.createNativeQuery(q);
		List<String> pks = nQuery.getResultList();
		if(pks.size()==0){
			throw new Exception("Username not found");
		}else{
/*			//Check if it will be the sole admin, if so dont delete it
			q = "SELECT distinct m.username FROM UserRoles p, Users m WHERE m.username=p.username and p.userRoles = 'ICMAADMIN'";
			nQuery = entityManager.createNativeQuery(q);
			pks = nQuery.getResultList();
			if(pks.size()>1){
				q = "DELETE FROM icmadb.Users where username='"+username+"'";
				entityManager.createNativeQuery(q).executeUpdate();
				q = "DELETE FROM icmadb.UserRoles where username='"+username+"'";
				entityManager.createNativeQuery(q).executeUpdate();
				entityManager.flush();
			}else{
				throw new Exception("Cannot delete the last remaining Admin");
			}*/
			//Since all users except the current user is listed in the Access table
			//the above check is not required as the current user is an admin
			q = "DELETE FROM icmadb.Users where username='"+username+"'";
			entityManager.createNativeQuery(q).executeUpdate();
			q = "DELETE FROM icmadb.UserRoles where username='"+username+"'";
			entityManager.createNativeQuery(q).executeUpdate();
			entityManager.flush();
		}
    }
    
    
    public JSONObject getUsers(){
    	String q = "SELECT u.username, r.userRoles FROM Users u, UserRoles r WHERE u.username = r.username";
		Query nQuery = entityManager.createNativeQuery(q);
		List<Object[]> pks = nQuery.getResultList();
		HashMap<String, String> users = new HashMap<String, String>();
		for(Object[] user: pks){
			String uid = (String)user[0];
			String role = (String)user[1];
			if(role.equalsIgnoreCase("ICMAADMIN")){
				users.put(uid, role);
			}else{
				if(users.containsKey(uid)){
					if(!users.get(uid).equalsIgnoreCase("ICMAADMIN")){
						users.put(uid, role);
					}
				}else{
					users.put(uid, role);
				}
			}
		}
		JSONObject obj = new JSONObject();
		Set<String> keys = users.keySet();
		for(String key: keys){
			obj.put(key, users.get(key));
		}
		return obj;
    }
    
}
