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
package org.mozilla.jss.asn1;

import java.io.CharConversionException;

public class IA5String extends CharacterString implements ASN1Value {

    public IA5String(char[] chars) throws CharConversionException {
        super(chars);
    }

    public IA5String(String s) throws CharConversionException {
        super(s);
    }

    CharConverter getCharConverter() {
        return new IA5Converter();
    }

    public static final Tag TAG = new Tag( Tag.Class.UNIVERSAL, 22 );

    public Tag getTag() {
        return TAG;
    }

    public static Template getTemplate() {
        return templateInstance;
    }
    private static final Template templateInstance = new Template();

// nested class
public static class Template
    extends CharacterString.Template implements ASN1Template
{
    public Tag getTag() {
        return IA5String.TAG;
    }
    public boolean tagMatch(Tag tag) {
        return( tag.equals( IA5String.TAG ));
    }

    protected CharConverter getCharConverter() {
        return new IA5Converter();
    }

    protected CharacterString generateInstance(char[] chars)
        throws CharConversionException
    {
        return new IA5String(chars);
    }

    protected String typeName() {
        return "IA5String";
    }
}

// nested class
private static class IA5Converter implements CharConverter {

    public char[] byteToChar(byte[] bytes, int offset, int len)
        throws CharConversionException
    {
        char[] chars = new char[len];

        int c; // char index
        int b; // byte index
        for(b = offset, c=0; c < len; b++, c++) {
            if( (bytes[b] & 0x80) != 0 ) {
                throw new CharConversionException("Invalid character: "+
                    bytes[b]);
            }
            chars[c] = (char) (bytes[b] & 0x7f);
        }
        return chars;
    }

    public byte[] charToByte(char[] chars, int offset, int len)
        throws CharConversionException
    {
        byte[] bytes = new byte[len];

        int c; // char index
        int b; // byte index
        for(c = offset, b = 0; b < len; c++, b++) {
            if( (chars[c] & 0x7f) != chars[c] ) {
                throw new CharConversionException("Invalid character: "+
                    chars[c]);
            }
            bytes[b] = (byte) (chars[c] & 0x7f);
        }

        return bytes;
    }
}

}