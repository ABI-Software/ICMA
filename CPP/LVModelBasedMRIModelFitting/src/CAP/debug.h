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
 *  Copyright (C) 2007-2010 by the University of Auckland.
 *  All Rights Reserved.
 *
 *  Contributor(s): J Chung
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
 * "2014"
 *******************************************************************************/

#ifndef _CAP_DEBUG_H
#define _CAP_DEBUG_H

#include <string>
#include <sstream>

#ifdef _MSC_VER
# define WINDOWS_LEAN_AND_MEAN
# define NOMINMAX
# include <windows.h>
#else
# include <iostream>
#endif

/**
 * Print debug message, uses printf on *nix based systems
 * and OutputDebugString on windows if using Visual Studio.
 *
 * @see dbgn
 * @param	msg	The message to print out to console.
 */
static void dbg(const std::string& msg) {
#ifdef debug
#ifdef _MSC_VER
	std::string out = msg + "\n";
	OutputDebugString(out.c_str());
#else
	std::cout << msg << std::endl;
#endif
#endif
}

/**
 * Print debug message with no newline, uses printf on *nix 
 * based systems and OutputDebugString on windows if using 
 * Visual Studio.
 *
 * @see dbg
 * @param	msg	The message to print out to console.
 */
static void dbgn(const std::string& msg) {
#ifdef _MSC_VER
	OutputDebugString(msg.c_str());
#else
	std::cout << msg;
#endif
}

#endif

