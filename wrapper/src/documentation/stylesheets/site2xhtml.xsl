<?xml version="1.0"?>

<html xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xsl:version="1.0">
    
    <!-- ********************************************************************
    $Id$
    ********************************************************************
    
    Based on Jakarta Avalon Documentation Style sheets.
    See http://jakarta.apache.org/avalon
    
    ******************************************************************** -->
    
    <head>
        <title>Java Service Wrapper - <xsl:value-of select="/site/body/title"/></title>
        <style type="text/css" media="all">
            @import url("./style/wrapper.css");
        </style>
    </head>
    
    <body text="#000000" link="#525D76" vlink="#023264" alink="#023264"
        topmargin="0" leftmargin="0" marginwidth="0" marginheight="0"
        bgcolor="#ffffff">
        
        <table border="0" width="100%" cellspacing="0" cellpadding="0">
            <tr>
                <td rowspan="6" bgcolor="#7fc1e6" width="282" height="115"><a href="http://wrapper.tanukisoftware.org"><img src="images/WrapperLogo.png" width="282" height="115" border="0"/></a></td>
                <td bgcolor="#7fc1e6" width="67" height="50"><img src="images/spacer.gif"/></td>
                <td bgcolor="#7fc1e6" align="right" width="*" height="50"><a href="http://www.tanukisoftware.org"><img src="images/TanukiSoftwareOrgLogo.png" width="259" height="33" border="0"/></a></td>
            </tr>
            <tr>
                <td rowspan="7" width="67" height="70" align="right" valign="top" background="images/BannerBackground.png"><img src="images/BannerAngle.png" width="67" height="70"/></td>
                <td width="*" height="3"><img src="images/spacer.gif"/></td>
            </tr>
            <tr>
                <td bgcolor="#000000" width="*" height="2"><img src="images/spacer.gif"/></td>
            </tr>
            <tr>
                <td bgcolor="#c7d9e2" align="right" width="*" height="12"><a href="donate.html"><img src="images/DonationRequest.png" width="201" height="12" border="0"/></a></td>
            </tr>
            <tr>
                <td bgcolor="#115b77" width="*" height="1"><img src="images/spacer.gif"/></td>
            </tr>
            <tr>
                <td width="*" height="46" nowrap="true"><font size="5" color="#115b77"><b><xsl:value-of select="/site/body/title"/></b></font><img src="images/TitleDefinition.png" align="center"/></td>
            </tr>
            <tr>
                <td width="282" height="3"><img src="images/spacer.gif"/></td>
                <td width="*" height="3"><img src="images/spacer.gif"/></td>
            </tr>
            <tr>
                <td bgcolor="#000000" width="282" height="2"><img src="images/spacer.gif"/></td>
                <td width="*" height="2"><img src="images/spacer.gif"/></td>
            </tr>
        </table>
        
        <table border="0" width="100%" cellspacing="0" cellpadding="0">
            <tr>
                <td bgcolor="#c7d9e2" width="180" valign="top">
                    <img src="images/spacer.gif" width="180" height="1"/>
                    <table border="0" width="100%" cellspacing="0" cellpadding="4">
                        <tr>
                            <td nowrap="true">
                                <xsl:copy-of select="/site/menu/node()|@*"/>
                                <form action="http://www.google.com/search" method="get">
                                    <p>
                                        <b>Search This Site:</b><br/>
                                        <input type="text" name="q" size="20" value="" style="width:175"/>
                                        <br/>
                                        <input type="submit" value="Search"/>
                                        <input type="hidden" name="sitesearch" value="wrapper.tanukisoftware.org"/>
                                    </p>
                                </form>
                                <p>
                                    <b>Hosted by:</b><br/>
                                    <a href="http://sourceforge.net/projects/wrapper/">
                                        <img src="http://sourceforge.net/sflogo.php?group_id=39428" width="88" height="31" border="0" alt="SourceForge"/>
                                    </a>
                                </p>
                            </td>
                        </tr>
                    </table>
                </td>
                <td bgcolor="#115b77" width="1"><img src="images/spacer.gif" width="1" height="300"/></td>
                <td valign="top" width="*">
                    <table border="0" width="100%" cellspacing="0" cellpadding="4">
                        <tr>
                            <td>
                                <xsl:copy-of select="/site/body/node()|@*"/>
                            </td>
                        </tr>
                        <tr>
                            <td id="author" align="right">
                                <p><i>by Leif Mortenson</i></p>
                            </td>
                        </tr>
                    </table>
                </td>
            </tr>
        </table>
        
        <table width="100%" border="0" cellspacing="0" cellpadding="0">
            <tr>
                <td colspan="2" bgcolor="#115b77"><img src="images/spacer.gif"/></td>
            </tr>
            <tr>
                <td align="left" id="copyright">
                    <font face="arial,helvetica,sanserif" size="-1" color="#525D76">
                        <i>
                            Copyright &#169;1999-2003 by <a href="http://www.tanukisoftware.org">TanukiSoftware.org</a>.
                            All Rights Reserved.
                        </i>
                    </font>
                </td>
                <td align="right">
                    <font face="arial,helvetica,sanserif" size="-1" color="#525D76">
                        <i>
                            last modified:
                            <script language="JavaScript">
                                <![CDATA[<!-- 
                                document.write(document.lastModified); 
                                //  -->]]>
                            </script>
                        </i>
                    </font>
                </td>
            </tr>
        </table>
    </body>
</html>

