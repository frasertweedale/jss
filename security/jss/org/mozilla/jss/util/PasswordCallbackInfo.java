/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape Security Services for Java.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

package org.mozilla.jss.util;

/**
 * An object of this class is passed to a PasswordCallback to give it
 * information about the token that is being logged into.
 */
public class PasswordCallbackInfo {

    /**
     * @param name The name of the file or token that is being logged into.
     * @param type The type of object (<code>FILE</code> or
     *      <code>TOKEN</code>) that is being logged into.
     */
	public PasswordCallbackInfo(String name, int type) {
		Assert._assert(type==FILE || type==TOKEN);
		this.name = name;
		this.type = type;
	}

	/**
	 * The name of the file or token that is being logged into.
	 */
	public String getName() {
		return name;
	}

	/**
	 * The type of object that is being logged into, <code>FILE</code>
     *  or <code>TOKEN</code>.
	 */
	public int getType() {
		return type;
	}

    protected String name;
    protected int type;

    /**
     * Logging into a file.
     */
    static public final int FILE = 0;

    /**
     * Logging into a PKCS #11 token.
     */
    static public final int TOKEN = 1;
}