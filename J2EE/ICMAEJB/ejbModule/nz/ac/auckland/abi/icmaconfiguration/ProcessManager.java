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

import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.annotation.PostConstruct;
import javax.annotation.Resource;
import javax.ejb.AccessTimeout;
import javax.ejb.LocalBean;
import javax.ejb.Singleton;

/**
 * Session Bean implementation class ProcessManager
 */
@Singleton
@LocalBean
public class ProcessManager {

	@Resource(name="java:global/ICMAMAXSERVERTHREADS")
	private String serverThreads;
	
	@Resource(name="java:global/ICMAAVGPROCESSTIME")
	private String avgProcessTime;
	
	private int maxNoOFActiveProcesses;
	private int currentlyActiveNoOFProcesses;
	private int waitTime;
	
	Logger log = null;
	

	@PostConstruct
	public void init() throws Exception {
		log = Logger.getLogger(this.getClass().getSimpleName());
	   	currentlyActiveNoOFProcesses = 0;
        try{
        	maxNoOFActiveProcesses = Integer.parseInt(serverThreads);
        }catch(Exception exx){
        	maxNoOFActiveProcesses = 2;
        }
        if(maxNoOFActiveProcesses<1){
        	maxNoOFActiveProcesses=1;
        }
        try{
        	waitTime = Integer.parseInt(avgProcessTime);
        }catch(Exception exx){
        	waitTime = 120000;
        }
        if(waitTime<60000){
        	waitTime = 120000;
        }
        log.log(Level.INFO,"Max active process threads is set to "+maxNoOFActiveProcesses);
        log.log(Level.INFO,"Average processing time is set to "+waitTime+" milliseconds");
		log.log(Level.INFO,"Resource Manager instantiated");
	}
	
	@AccessTimeout(value = 1, unit = TimeUnit.DAYS)
	 public void processInQueue(Runnable instance) throws Exception{
	    	while(currentlyActiveNoOFProcesses>=maxNoOFActiveProcesses){
	    		try{
	    			Thread.sleep(waitTime); 
	    		}catch(Exception exx){
	    			
	    		}
	    	}
	    	synchronized (this) {
		    	currentlyActiveNoOFProcesses++;				
			}

	    	Thread myThread = new Thread(instance);
	    	try{
	    		myThread.start();
	    		try{
	    			Thread.sleep(waitTime); //2 minutes
	    		}catch(Exception exx){
	    			
	    		}
	    		myThread.join();
	    		synchronized (this) {
			    	currentlyActiveNoOFProcesses--;				
				}
	    	}catch(Exception exx){
	    		synchronized (this) {
			    	currentlyActiveNoOFProcesses--;				
				}
	    		throw exx;
	    	}
	}

}
