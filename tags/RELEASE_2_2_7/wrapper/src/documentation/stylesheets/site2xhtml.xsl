<?xml version="1.0"?>

<html xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xsl:version="1.0">
    
<!-- ********************************************************************
     $Id$
     ********************************************************************

     Based on Jakarta Avalon Documentation Style sheets.
     See http://jakarta.apache.org/avalon

     ******************************************************************** -->
    
    <head><title><xsl:value-of select="/site/body/title"/></title></head>
    <body text="#000000" link="#525D76" vlink="#023264" alink="#023264"
        topmargin="4" leftmargin="4" marginwidth="4" marginheight="4"
        bgcolor="#ffffff">
        <table width="100%" cellspacing="0" cellpadding="0" border="0">
            <tr>
                <td valign="top" align="left">
                    <a href="http://www.silveregg.co.jp/">
                        <img hspace="0" vspace="0" border="0" src="images/silveregg-logo.png"/>
                    </a>
                </td>
                <td width="100%" valign="bottom" align="right" bgcolor="#ffffff">
                    <font size="+3" face="arial,helvetica,sanserif" color="gray">
                        <i><b>Java Service Wrapper</b></i>
                    </font>
                </td>
            </tr>
        </table>
        
        <table width="100%" cellspacing="0" cellpadding="0" border="0">
            <tr>
                <td colspan="7" bgcolor="gray"><img src="images/spacer.png" width="1" height="1"/></td>
            </tr>
            <tr>
                <td width="5" valign="top"><img src="images/spacer.png" width="5" height="1"/></td>
                <td width="150" valign="top" nowrap="1">
                    <br/>
                    <font face="arial,helvetica,sanserif">
                        <br/>
                        <xsl:copy-of select="/site/menu/node()|@*"/>
                        <br/>
                        Hosted by:<br/>
                        <A href="http://sourceforge.net/projects/wrapper/">
                            <IMG src="http://sourceforge.net/sflogo.php?group_id=39428" width="88" height="31" border="0" alt="SourceForge Logo"/>
                        </A>
                        <br/>
                    </font>
                </td>
                <td width="2" valign="top"><img src="images/spacer.png" width="2" height="1"/></td>
                <td width="1" valign="top" bgcolor="gray"><img src="images/spacer.png" width="1" height="1"/></td>
                <td width="10" valign="top"><img src="images/spacer.png" width="10" height="1"/></td>
                <td width="*" valign="top" align="left">
                    <br/>
                    <xsl:copy-of select="/site/body/node()|@*"/>
                </td>
                <td width="5" valign="top"><img src="images/spacer.png" width="5" height="1"/></td>
            </tr>
            <tr>
                <td colspan="7" bgcolor="gray"><img src="images/spacer.png" width="1" height="1"/></td>
            </tr>
        </table>
        <table width="100%" border="0" cellspacing="0" cellpadding="0">
            <tr>
                <td align="center">
                    <font face="arial,helvetica,sanserif" size="-1" color="#525D76">
                        <i>
                            Copyright &#169;2000-2001 by Silver Egg Technology Co., Ltd.
                            All Rights Reserved.
                        </i>
                    </font>
                </td>
            </tr>
        </table>
    </body>
</html>

