/*
 * Copyright (c) 1999, 2017 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.com/doc/english/licenseOverview.html
 *
 *
 * Portions of the Software have been derived from source code
 * developed by Silver Egg Technology under the following license:
 *
 * Copyright (c) 2001 Silver Egg Technology
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sub-license, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wrapper.h"
#include "wrapper_hashmap.h"
#include "property.h"
#include "logger.h"
#include "wrapper_jvminfo.h"
#include "wrapper_encoding.h"

#ifdef WIN32
 #define WIN_FILL(a, i, v) a[i] = v
#else
 #define WIN_FILL(a, i, v) /* nothing */
#endif

#define K_ENCODING_V_ENCODING           1   /* map any encoding (io or nio) to its corresponding encoding (io if key is nio, and nio if key is io). */
#define K_ENCODING_V_JVERSION           2   /* map any encoding (io or nio) to the Java version in which it was introduced. */

/**
 * Build a hashMap containing the encodings supported by Java.
 *  - On Windows, the keys are the canonical names for all APIs
 *    and the values are the corresponding code pages.
 *    If there there are no corresponding code page, 0 is set.
 *  - On UNIX, the keys are the canonical names for for java.io API and 
 *    java.lang API and the values are the canonical names for java.nio API. 
 *
 * @return The created hashmap or NULL on failure.
 */
PHashMap buildJvmEncodingsHashMap(int mode) {
    PHashMap hashMap;
    int i = 0;
    int          jv[163]; /* Java versions in which the encodings were introduced. */
    const TCHAR* e1[163]; /* Canonical Names for java.io API and java.lang API */
    const TCHAR* e2[163]; /* Canonical Names for java.nio API */
#ifdef WIN32
    int          cp[163]; /* Windows Code Pages */
    int          id[163]; /* Whether the code page is an ID to retrieve the encoding */
#endif
    TCHAR* key1;
    TCHAR* key2;
    
         jv[i] = 0; e1[i] = TEXT("Cp858");                 e2[i] = TEXT("IBM00858");            WIN_FILL(cp, i, 858);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp437");                 e2[i] = TEXT("IBM437");              WIN_FILL(cp, i, 437);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp775");                 e2[i] = TEXT("IBM775");              WIN_FILL(cp, i, 775);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp850");                 e2[i] = TEXT("IBM850");              WIN_FILL(cp, i, 850);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp852");                 e2[i] = TEXT("IBM852");              WIN_FILL(cp, i, 852);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp855");                 e2[i] = TEXT("IBM855");              WIN_FILL(cp, i, 855);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp857");                 e2[i] = TEXT("IBM857");              WIN_FILL(cp, i, 857);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp862");                 e2[i] = TEXT("IBM862");              WIN_FILL(cp, i, 862);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp866");                 e2[i] = TEXT("IBM866");              WIN_FILL(cp, i, 866);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("ISO8859_1");             e2[i] = TEXT("ISO-8859-1");          WIN_FILL(cp, i, 28591); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("ISO8859_2");             e2[i] = TEXT("ISO-8859-2");          WIN_FILL(cp, i, 28592); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("ISO8859_4");             e2[i] = TEXT("ISO-8859-4");          WIN_FILL(cp, i, 28594); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("ISO8859_5");             e2[i] = TEXT("ISO-8859-5");          WIN_FILL(cp, i, 28595); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("ISO8859_7");             e2[i] = TEXT("ISO-8859-7");          WIN_FILL(cp, i, 28597); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("ISO8859_9");             e2[i] = TEXT("ISO-8859-9");          WIN_FILL(cp, i, 28599); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("ISO8859_13");            e2[i] = TEXT("ISO-8859-13");         WIN_FILL(cp, i, 28603); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("ISO8859_15");            e2[i] = TEXT("ISO-8859-15");         WIN_FILL(cp, i, 28605); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("KOI8_R");                e2[i] = TEXT("KOI8-R");              WIN_FILL(cp, i, 20866); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("KOI8_U");                e2[i] = TEXT("KOI8-U");              WIN_FILL(cp, i, 21866); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("ASCII");                 e2[i] = TEXT("US-ASCII");            WIN_FILL(cp, i, 20127); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("UTF8");                  e2[i] = TEXT("UTF-8");               WIN_FILL(cp, i, 65001); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("UTF-16");                e2[i] = TEXT("UTF-16");              WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 7 */
    i++; jv[i] = 0; e1[i] = TEXT("UnicodeBigUnmarked");    e2[i] = TEXT("UTF-16BE");            WIN_FILL(cp, i, 1201);  WIN_FILL(id, i, TRUE);  /* NOTE 7 */
    i++; jv[i] = 0; e1[i] = TEXT("UnicodeLittleUnmarked"); e2[i] = TEXT("UTF-16LE");            WIN_FILL(cp, i, 1200);  WIN_FILL(id, i, TRUE);  /* NOTE 7 */
    i++; jv[i] = 0; e1[i] = TEXT("UTF_32");                e2[i] = TEXT("UTF-32");              WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined */
    i++; jv[i] = 0; e1[i] = TEXT("UTF_32BE");              e2[i] = TEXT("UTF-32BE");            WIN_FILL(cp, i, 12001); WIN_FILL(id, i, TRUE);  /* NOTE 7 */
    i++; jv[i] = 0; e1[i] = TEXT("UTF_32LE");              e2[i] = TEXT("UTF-32LE");            WIN_FILL(cp, i, 12000); WIN_FILL(id, i, TRUE);  /* NOTE 7 */
    i++; jv[i] = 0; e1[i] = TEXT("UTF_32BE_BOM");          e2[i] = TEXT("x-UTF-32BE-BOM");      WIN_FILL(cp, i, 12001); WIN_FILL(id, i, FALSE); /* NOTE 7 */
    i++; jv[i] = 0; e1[i] = TEXT("UTF_32LE_BOM");          e2[i] = TEXT("x-UTF-32LE-BOM");      WIN_FILL(cp, i, 12000); WIN_FILL(id, i, FALSE); /* NOTE 7 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp1250");                e2[i] = TEXT("windows-1250");        WIN_FILL(cp, i, 1250);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1251");                e2[i] = TEXT("windows-1251");        WIN_FILL(cp, i, 1251);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1252");                e2[i] = TEXT("windows-1252");        WIN_FILL(cp, i, 1252);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1253");                e2[i] = TEXT("windows-1253");        WIN_FILL(cp, i, 1253);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1254");                e2[i] = TEXT("windows-1254");        WIN_FILL(cp, i, 1254);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1257");                e2[i] = TEXT("windows-1257");        WIN_FILL(cp, i, 1257);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("UnicodeBig");            e2[i] = TEXT("Not available");       WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined */
    i++; jv[i] = 0; e1[i] = TEXT("Cp737");                 e2[i] = TEXT("x-IBM737");            WIN_FILL(cp, i, 737);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp874");                 e2[i] = TEXT("x-IBM874");            WIN_FILL(cp, i, 874);   WIN_FILL(id, i, FALSE);
    i++; jv[i] = 0; e1[i] = TEXT("UnicodeLittle");         e2[i] = TEXT("x-UTF-16LE-BOM");      WIN_FILL(cp, i, 1200);  WIN_FILL(id, i, FALSE); /* NOTE 7 */
    i++; jv[i] = 0; e1[i] = TEXT("Big5");                  e2[i] = TEXT("Big5");                WIN_FILL(cp, i, 950);   WIN_FILL(id, i, FALSE);
    i++; jv[i] = 0; e1[i] = TEXT("Big5_HKSCS");            e2[i] = TEXT("Big5-HKSCS");          WIN_FILL(cp, i, 951);   WIN_FILL(id, i, FALSE); /* NOTE 1 */
    i++; jv[i] = 0; e1[i] = TEXT("EUC_JP");                e2[i] = TEXT("EUC-JP");              WIN_FILL(cp, i, 20932); WIN_FILL(id, i, TRUE);  /* NOTE 2 */
    i++; jv[i] = 0; e1[i] = TEXT("EUC_KR");                e2[i] = TEXT("EUC-KR");              WIN_FILL(cp, i, 51949); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("GB18030");               e2[i] = TEXT("GB18030");             WIN_FILL(cp, i, 54936); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("EUC_CN");                e2[i] = TEXT("GB2312");              WIN_FILL(cp, i, 51936); WIN_FILL(id, i, TRUE);  /* NOTE 3 */
    i++; jv[i] = 0; e1[i] = TEXT("GBK");                   e2[i] = TEXT("GBK");                 WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined */
    i++; jv[i] = 0; e1[i] = TEXT("Cp838");                 e2[i] = TEXT("IBM-Thai");            WIN_FILL(cp, i, 20838); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1140");                e2[i] = TEXT("IBM01140");            WIN_FILL(cp, i, 1140);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1141");                e2[i] = TEXT("IBM01141");            WIN_FILL(cp, i, 1141);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1142");                e2[i] = TEXT("IBM01142");            WIN_FILL(cp, i, 1142);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1143");                e2[i] = TEXT("IBM01143");            WIN_FILL(cp, i, 1143);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1144");                e2[i] = TEXT("IBM01144");            WIN_FILL(cp, i, 1144);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1145");                e2[i] = TEXT("IBM01145");            WIN_FILL(cp, i, 1145);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1146");                e2[i] = TEXT("IBM01146");            WIN_FILL(cp, i, 1146);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1147");                e2[i] = TEXT("IBM01147");            WIN_FILL(cp, i, 1147);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1148");                e2[i] = TEXT("IBM01148");            WIN_FILL(cp, i, 1148);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1149");                e2[i] = TEXT("IBM01149");            WIN_FILL(cp, i, 1149);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp037");                 e2[i] = TEXT("IBM037");              WIN_FILL(cp, i, 037);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1026");                e2[i] = TEXT("IBM1026");             WIN_FILL(cp, i, 1026);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1047");                e2[i] = TEXT("IBM1047");             WIN_FILL(cp, i, 1047);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp273");                 e2[i] = TEXT("IBM273");              WIN_FILL(cp, i, 20273); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp277");                 e2[i] = TEXT("IBM277");              WIN_FILL(cp, i, 20277); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp278");                 e2[i] = TEXT("IBM278");              WIN_FILL(cp, i, 20278); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp280");                 e2[i] = TEXT("IBM280");              WIN_FILL(cp, i, 20280); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp284");                 e2[i] = TEXT("IBM284");              WIN_FILL(cp, i, 20284); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp285");                 e2[i] = TEXT("IBM285");              WIN_FILL(cp, i, 20285); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 9; e1[i] = TEXT("Cp290");                 e2[i] = TEXT("IBM290");              WIN_FILL(cp, i, 20290); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp297");                 e2[i] = TEXT("IBM297");              WIN_FILL(cp, i, 20297); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 9; e1[i] = TEXT("Cp300");                 e2[i] = TEXT("IBM300");              WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined */
    i++; jv[i] = 0; e1[i] = TEXT("Cp420");                 e2[i] = TEXT("IBM420");              WIN_FILL(cp, i, 20420); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp424");                 e2[i] = TEXT("IBM424");              WIN_FILL(cp, i, 20424); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp500");                 e2[i] = TEXT("IBM500");              WIN_FILL(cp, i, 500);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp860");                 e2[i] = TEXT("IBM860");              WIN_FILL(cp, i, 860);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp861");                 e2[i] = TEXT("IBM861");              WIN_FILL(cp, i, 861);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp863");                 e2[i] = TEXT("IBM863");              WIN_FILL(cp, i, 863);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp864");                 e2[i] = TEXT("IBM864");              WIN_FILL(cp, i, 864);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp865");                 e2[i] = TEXT("IBM865");              WIN_FILL(cp, i, 865);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp868");                 e2[i] = TEXT("IBM868");              WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp869");                 e2[i] = TEXT("IBM869");              WIN_FILL(cp, i, 869);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp870");                 e2[i] = TEXT("IBM870");              WIN_FILL(cp, i, 870);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp871");                 e2[i] = TEXT("IBM871");              WIN_FILL(cp, i, 20871); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp918");                 e2[i] = TEXT("IBM918");              WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("ISO2022CN");             e2[i] = TEXT("ISO-2022-CN");         WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 5 */
    i++; jv[i] = 0; e1[i] = TEXT("ISO2022JP");             e2[i] = TEXT("ISO-2022-JP");         WIN_FILL(cp, i, 50222); WIN_FILL(id, i, FALSE);
    i++; jv[i] = 0; e1[i] = TEXT("ISO2022KR");             e2[i] = TEXT("ISO-2022-KR");         WIN_FILL(cp, i, 50225); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("ISO8859_3");             e2[i] = TEXT("ISO-8859-3");          WIN_FILL(cp, i, 28593); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("ISO8859_6");             e2[i] = TEXT("ISO-8859-6");          WIN_FILL(cp, i, 28596); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("ISO8859_8");             e2[i] = TEXT("ISO-8859-8");          WIN_FILL(cp, i, 28598); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("JIS_X0201");             e2[i] = TEXT("JIS_X0201");           WIN_FILL(cp, i, 50221); WIN_FILL(id, i, FALSE); /* NOTE 4 */
    i++; jv[i] = 0; e1[i] = TEXT("JIS_X0212-1990");        e2[i] = TEXT("JIS_X0212-1990");      WIN_FILL(cp, i, 20932); WIN_FILL(id, i, FALSE); /* NOTE 6 */
    i++; jv[i] = 0; e1[i] = TEXT("SJIS");                  e2[i] = TEXT("Shift_JIS");           WIN_FILL(cp, i, 932);   WIN_FILL(id, i, FALSE);
    i++; jv[i] = 0; e1[i] = TEXT("TIS620");                e2[i] = TEXT("TIS-620");             WIN_FILL(cp, i, 28601); WIN_FILL(id, i, TRUE);  /* NOTE 1 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp1255");                e2[i] = TEXT("windows-1255");        WIN_FILL(cp, i, 1255);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1256");                e2[i] = TEXT("windows-1256");        WIN_FILL(cp, i, 1256);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1258");                e2[i] = TEXT("windows-1258");        WIN_FILL(cp, i, 1258);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MS932");                 e2[i] = TEXT("windows-31j");         WIN_FILL(cp, i, 932);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Big5_Solaris");          e2[i] = TEXT("x-Big5-Solaris");      WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined */
    i++; jv[i] = 0; e1[i] = TEXT("EUC_JP_LINUX");          e2[i] = TEXT("x-euc-jp-linux");      WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined */
    i++; jv[i] = 0; e1[i] = TEXT("EUC_TW");                e2[i] = TEXT("x-EUC-TW");            WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined */
    i++; jv[i] = 0; e1[i] = TEXT("EUC_JP_Solaris");        e2[i] = TEXT("x-eucJP-Open");        WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined */
    i++; jv[i] = 0; e1[i] = TEXT("Cp1006");                e2[i] = TEXT("x-IBM1006");           WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp1025");                e2[i] = TEXT("x-IBM1025");           WIN_FILL(cp, i, 21025); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp1046");                e2[i] = TEXT("x-IBM1046");           WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp1097");                e2[i] = TEXT("x-IBM1097");           WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp1098");                e2[i] = TEXT("x-IBM1098");           WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp1112");                e2[i] = TEXT("x-IBM1112");           WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp1122");                e2[i] = TEXT("x-IBM1122");           WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp1123");                e2[i] = TEXT("x-IBM1123");           WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp1124");                e2[i] = TEXT("x-IBM1124");           WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp1381");                e2[i] = TEXT("x-IBM1381");           WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp1383");                e2[i] = TEXT("x-IBM1383");           WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp33722");               e2[i] = TEXT("x-IBM33722");          WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp834");                 e2[i] = TEXT("x-IBM834");            WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp856");                 e2[i] = TEXT("x-IBM856");            WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp875");                 e2[i] = TEXT("x-IBM875");            WIN_FILL(cp, i, 875);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp921");                 e2[i] = TEXT("x-IBM921");            WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp922");                 e2[i] = TEXT("x-IBM922");            WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp930");                 e2[i] = TEXT("x-IBM930");            WIN_FILL(cp, i, 50930); WIN_FILL(id, i, TRUE); 
    i++; jv[i] = 0; e1[i] = TEXT("Cp933");                 e2[i] = TEXT("x-IBM933");            WIN_FILL(cp, i, 50933); WIN_FILL(id, i, TRUE); 
    i++; jv[i] = 0; e1[i] = TEXT("Cp935");                 e2[i] = TEXT("x-IBM935");            WIN_FILL(cp, i, 50935); WIN_FILL(id, i, TRUE); 
    i++; jv[i] = 0; e1[i] = TEXT("Cp937");                 e2[i] = TEXT("x-IBM937");            WIN_FILL(cp, i, 50937); WIN_FILL(id, i, TRUE); 
    i++; jv[i] = 0; e1[i] = TEXT("Cp939");                 e2[i] = TEXT("x-IBM939");            WIN_FILL(cp, i, 50939); WIN_FILL(id, i, TRUE); 
    i++; jv[i] = 0; e1[i] = TEXT("Cp942");                 e2[i] = TEXT("x-IBM942");            WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp942C");                e2[i] = TEXT("x-IBM942C");           WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp943");                 e2[i] = TEXT("x-IBM943");            WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp943C");                e2[i] = TEXT("x-IBM943C");           WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp948");                 e2[i] = TEXT("x-IBM948");            WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp949");                 e2[i] = TEXT("x-IBM949");            WIN_FILL(cp, i, 949);   WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp949C");                e2[i] = TEXT("x-IBM949C");           WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp950");                 e2[i] = TEXT("x-IBM950");            WIN_FILL(cp, i, 950);   WIN_FILL(id, i, FALSE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp964");                 e2[i] = TEXT("x-IBM964");            WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp970");                 e2[i] = TEXT("x-IBM970");            WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 0 */
    i++; jv[i] = 0; e1[i] = TEXT("ISCII91");               e2[i] = TEXT("x-ISCII91");           WIN_FILL(cp, i, 57002); WIN_FILL(id, i, FALSE); /* undefined - NOTE 8 */
    i++; jv[i] = 0; e1[i] = TEXT("ISO2022_CN_CNS");        e2[i] = TEXT("x-ISO2022-CN-CNS");    WIN_FILL(cp, i, 50229); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("ISO2022_CN_GB");         e2[i] = TEXT("x-ISO2022-CN-GB");     WIN_FILL(cp, i, 50227); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("x-iso-8859-11");         e2[i] = TEXT("x-iso-8859-11");       WIN_FILL(cp, i, 874);   WIN_FILL(id, i, FALSE); /* NOTE 9 */
    i++; jv[i] = 0; e1[i] = TEXT("x-JIS0208");             e2[i] = TEXT("x-JIS0208");           WIN_FILL(cp, i, 20932); WIN_FILL(id, i, FALSE);
    i++; jv[i] = 0; e1[i] = TEXT("JISAutoDetect");         e2[i] = TEXT("x-JISAutoDetect");     WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 12 */
    i++; jv[i] = 0; e1[i] = TEXT("x-Johab");               e2[i] = TEXT("x-Johab");             WIN_FILL(cp, i, 1361);  WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MacArabic");             e2[i] = TEXT("x-MacArabic");         WIN_FILL(cp, i, 10004); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MacCentralEurope");      e2[i] = TEXT("x-MacCentralEurope");  WIN_FILL(cp, i, 10029); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MacCroatian");           e2[i] = TEXT("x-MacCroatian");       WIN_FILL(cp, i, 10082); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MacCyrillic");           e2[i] = TEXT("x-MacCyrillic");       WIN_FILL(cp, i, 10007); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MacDingbat");            e2[i] = TEXT("x-MacDingbat");        WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined */
    i++; jv[i] = 0; e1[i] = TEXT("MacGreek");              e2[i] = TEXT("x-MacGreek");          WIN_FILL(cp, i, 10006); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MacHebrew");             e2[i] = TEXT("x-MacHebrew");         WIN_FILL(cp, i, 10005); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MacIceland");            e2[i] = TEXT("x-MacIceland");        WIN_FILL(cp, i, 10079); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MacRoman");              e2[i] = TEXT("x-MacRoman");          WIN_FILL(cp, i, 10000); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MacRomania");            e2[i] = TEXT("x-MacRomania");        WIN_FILL(cp, i, 10010); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MacSymbol");             e2[i] = TEXT("x-MacSymbol");         WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined */
    i++; jv[i] = 0; e1[i] = TEXT("MacThai");               e2[i] = TEXT("x-MacThai");           WIN_FILL(cp, i, 10021); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MacTurkish");            e2[i] = TEXT("x-MacTurkish");        WIN_FILL(cp, i, 10081); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MacUkraine");            e2[i] = TEXT("x-MacUkraine");        WIN_FILL(cp, i, 10017); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MS950_HKSCS");           e2[i] = TEXT("x-MS950-HKSCS");       WIN_FILL(cp, i, 951);   WIN_FILL(id, i, TRUE);  /* NOTE 1 */
    i++; jv[i] = 0; e1[i] = TEXT("MS936");                 e2[i] = TEXT("x-mswin-936");         WIN_FILL(cp, i, 936);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("PCK");                   e2[i] = TEXT("x-PCK");               WIN_FILL(cp, i, 932);   WIN_FILL(id, i, FALSE); /* NOTE 10 */
    i++; jv[i] = 7; e1[i] = TEXT("x-SJIS_0213");           e2[i] = TEXT("x-SJIS_0213");         WIN_FILL(cp, i, -1);    WIN_FILL(id, i, FALSE); /* undefined - NOTE 11 */
    i++; jv[i] = 0; e1[i] = TEXT("Cp50220");               e2[i] = TEXT("x-windows-50220");     WIN_FILL(cp, i, 50220); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("Cp50221");               e2[i] = TEXT("x-windows-50221");     WIN_FILL(cp, i, 50221); WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MS874");                 e2[i] = TEXT("x-windows-874");       WIN_FILL(cp, i, 874);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MS949");                 e2[i] = TEXT("x-windows-949");       WIN_FILL(cp, i, 949);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("MS950");                 e2[i] = TEXT("x-windows-950");       WIN_FILL(cp, i, 950);   WIN_FILL(id, i, TRUE);
    i++; jv[i] = 0; e1[i] = TEXT("x-windows-iso2022jp");   e2[i] = TEXT("x-windows-iso2022jp"); WIN_FILL(cp, i, 50220); WIN_FILL(id, i, FALSE);

    hashMap = newHashMap(16);
    if (!hashMap) {
        return NULL;
    }
    
    if (mode == K_ENCODING_V_ENCODING) {
        for (; i >= 0; i--) {
            key1 = toLower(e1[i]);
            if (!key1) {
                freeHashMap(hashMap);
                return NULL;
            }
            key2 = toLower(e2[i]);
            if (!key2) {
                free(key1);
                freeHashMap(hashMap);
                return NULL;
            }
#ifdef WIN32
            hashMapPutKWVI(hashMap, key1, cp[i]);
#else
            hashMapPutKWVW(hashMap, key1, e2[i]);
#endif
            if (_tcscmp(key1, key2) != 0) {
#ifdef WIN32
                hashMapPutKWVI(hashMap, key2, cp[i]);
#else
                hashMapPutKWVW(hashMap, key2, e1[i]);
#endif
            }
            free(key1);
            free(key2);
        }
    } else if (mode == K_ENCODING_V_JVERSION) {
        for (; i >= 0; i--) {
            key1 = toLower(e1[i]);
            if (!key1) {
                freeHashMap(hashMap);
                return NULL;
            }
            key2 = toLower(e2[i]);
            if (!key2) {
                free(key1);
                freeHashMap(hashMap);
                return NULL;
            }
            hashMapPutKWVI(hashMap, key1, jv[i]);
            if (_tcscmp(key1, key2) != 0) {
                hashMapPutKWVI(hashMap, key2, jv[i]);
            }
            free(key1);
            free(key2);
        }
    }
    return hashMap;
}

static PHashMap hashMapJvmEncoding = NULL;
static PHashMap hashMapJavaVersions = NULL;

/**
 * Should be called on exit to release the hashmap containing the JVM encodings.
 */
void disposeHashMapJvmEncoding() {
    if (hashMapJvmEncoding) {
        freeHashMap(hashMapJvmEncoding);
        hashMapJvmEncoding = NULL;
    }
    if (hashMapJavaVersions) {
        freeHashMap(hashMapJavaVersions);
        hashMapJavaVersions = NULL;
    }
}

/**
 * Check if the given encoding is supported for a specific version of Java.
 *
 * @encoding the encoding to search
 * @javaVersion current java version
 *
 * @return TRUE if the encoding exists, FALSE otherwise.
 */
int checkEncodingJavaVersion(const TCHAR* encoding, int javaVersion, int *pRequiredJavaVersion) {
    TCHAR* encodingLwr;
    int outRequiredJavaVersion;
    int result = FALSE;
    
    if (encoding) {
        encodingLwr = toLower(encoding);
        if (encodingLwr) {
            if (!hashMapJavaVersions) {
                /* Create a hashmap containing the Java versions in which each encoding was introduced. Keep it as a static global variable. */
                hashMapJavaVersions = buildJvmEncodingsHashMap(K_ENCODING_V_JVERSION);
            }
            if (hashMapJavaVersions) {
                /* Return TRUE if the Java version is greater than or equal to the required Java version for the encoding. */
                outRequiredJavaVersion = hashMapGetKWVI(hashMapJavaVersions, encodingLwr);
                result = (javaVersion >= outRequiredJavaVersion);
                if (pRequiredJavaVersion) {
                    *pRequiredJavaVersion = outRequiredJavaVersion;
                }
            }
            free(encodingLwr);
        }
    }
    return result;
}

int checkEquivalentEncodings(TCHAR* encoding1, TCHAR* encoding2) {
#ifndef WIN32
    const TCHAR* value;
#endif
    int result = FALSE;
    TCHAR* enc1Lower;
    TCHAR* enc2Lower;
    
    enc1Lower = toLower(encoding1);
    if (!enc1Lower) {
        return FALSE;
    }
    enc2Lower = toLower(encoding2);
    if (!enc2Lower) {
        free(enc1Lower);
        return FALSE;
    }

    if (_tcscmp(enc1Lower, enc2Lower) == 0) {
        result = TRUE;
    } else {
        if (!hashMapJvmEncoding) {
            hashMapJvmEncoding = buildJvmEncodingsHashMap(K_ENCODING_V_ENCODING);
        }
        if (hashMapJvmEncoding &&
#ifdef WIN32
            (hashMapGetKWVI(hashMapJvmEncoding, enc1Lower) == hashMapGetKWVI(hashMapJvmEncoding, enc2Lower))
#else
            (((value = hashMapGetKWVW(hashMapJvmEncoding, enc1Lower)) != NULL) && strcmpIgnoreCase(value, enc2Lower) == 0)
#endif
        ) {
            result = TRUE;
        }
    }
    free(enc1Lower);
    free(enc2Lower);
    return result;
}

int getJvmSunEncodingSupport(int javaVersion, int jvmMaker) {
    int result = 0;
    
    if (jvmMaker == JVM_MAKER_IBM) {
        result |= SUN_ENCODING_UNSUPPORTED_JVM_MAKER;
    } else if ((jvmMaker != JVM_MAKER_ORACLE) || (jvmMaker != JVM_MAKER_OPENJDK)) {
        result |= SUN_ENCODING_SUPPORT_UNKNOWN;
    }
    if (javaVersion < 8) {
        result |= SUN_ENCODING_UNSUPPORTED_JAVA_VERSION;
    }
    if (result == 0) {
        result |= SUN_ENCODING_SUPPORTED;
    }
    return result;
}

void tryGetSunSystemProperty(const TCHAR* name, int javaVersion, int jvmMaker, TCHAR* propValue, TCHAR* buffer, int* result) {
    static int warnedPropsNotSupported[] = { FALSE, FALSE }; /* Remember if we logged warnings for each property, in the order: stdout, stderr */
    int propIndex;
    TCHAR argBase[23];
    int sunSupport;
    
#ifdef UNIT_TESTS
    if (resetStaticVariables) {
        warnedPropsNotSupported[0] = FALSE;
        warnedPropsNotSupported[1] = FALSE;
        resetStaticVariables = FALSE;
    }
#endif
    _sntprintf(argBase, 23, TEXT("-D%s="), name);
    if (_tcsstr(propValue, argBase) == propValue) {
        sunSupport = getJvmSunEncodingSupport(javaVersion, jvmMaker);
        if ((sunSupport & SUN_ENCODING_UNSUPPORTED_JVM_MAKER) == SUN_ENCODING_UNSUPPORTED_JVM_MAKER) {
            propIndex = (_tcscmp(name, TEXT("sun.stdout.encoding")) == 0) ? 0 : 1;
            if (!warnedPropsNotSupported[propIndex]) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                    TEXT("Found %s among the JVM parameters but this system property is not\n  supported on this implementation of Java."),
                    name);
                warnedPropsNotSupported[propIndex] = TRUE;
            }
        } else if ((sunSupport & SUN_ENCODING_UNSUPPORTED_JAVA_VERSION) == SUN_ENCODING_UNSUPPORTED_JAVA_VERSION) {
            propIndex = (_tcscmp(name, TEXT("sun.stdout.encoding")) == 0) ? 0 : 1;
            if (!warnedPropsNotSupported[propIndex]) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                    TEXT("Found %s among the JVM parameters but this system property is not\n  supported by this version of Java.\n  Requires Java 8 or above, using Java %d."),
                    name,
                    javaVersion);
                warnedPropsNotSupported[propIndex] = TRUE;
            }
        } else {
            if (buffer[0] == 0) {
                /* This is the first time we found this property. */
                _tcsncpy(buffer, propValue + 22, ENCODING_BUFFER_SIZE);
                buffer[ENCODING_BUFFER_SIZE - 1] = 0;
                /* Set the result to SUN_ENCODING to avoid searching for file.encoding. We may change it later if the sun properties are not configured correctly. */
                *result = SUN_ENCODING;
            } else if (!checkEquivalentEncodings(buffer, propValue + 22)) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    TEXT("Found multiple occurrences of %s set with different values\n  among the JVM parameters. Cannot resolve the JVM output encoding."),
                    name);
                *result = UNRESOLVED_ENCODING;
            }   /* The remaining case is when the value of the current system property is different than one of a previously defined,
                 *  but both values point to the same encoding. If this happen, leave the result unchanged and continue. */
        } 
    }
}

/**
 * Retrieved the value of file.encoding (or sun.std*.encoding) if defined in the java additional properties.
 *  The buffer is set to an empty string if the value could not be found.
 *  disposeHashMapJvmEncoding() should be called before calling this function.
 *
 * @buffer buffer in which the encoding should be copied
 * @javaVersion current java version
 * @jvmMaker    current java implementation (Oracle, IBM, etc.)
 *
 * @return LOCALE_ENCODING if no encoding was specified in the JVM arguments
 *         FILE_ENCODING if the encoding was resolved to the value of file.encoding
 *         SUN_ENCODING if the encoding was resolved to the value of the sun.std*.encoding properties
 *         UNRESOLVED_ENCODING if there was any error. A FATAL message will be printed before returning
 */
int getJvmArgumentsEncoding(TCHAR* buffer, int javaVersion, int jvmMaker) {
    TCHAR** propNames;
    TCHAR** propValues;
    TCHAR bufferSunOut[ENCODING_BUFFER_SIZE];
    TCHAR bufferSunErr[ENCODING_BUFFER_SIZE];
    long unsigned int* propIndices;
    TCHAR* propValue;
    int i;
    int result = LOCALE_ENCODING; /* We can move the result to a higher value but never decrease it. The order is LOCALE_ENCODING -> FILE_ENCODING -> SUN_ENCODING -> UNRESOLVED_ENCODING. */
    int foundMultipleFileEncoding = FALSE;

    buffer[0] = 0;
    if (getStringProperties(properties, TEXT("wrapper.java.additional."), TEXT(""), wrapperData->ignoreSequenceGaps, FALSE, &propNames, &propValues, &propIndices)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Failed to retrieve the values of the wrapper.java.additional.* properties."));
        return UNRESOLVED_ENCODING;
    }

    bufferSunOut[0] = 0;
    bufferSunErr[0] = 0;
    for (i = 0; propValues[i]; i++){
        propValue = propValues[i];
        tryGetSunSystemProperty(TEXT("sun.stdout.encoding"), javaVersion, jvmMaker, propValue, bufferSunOut, &result);
        if (result == UNRESOLVED_ENCODING) {
            break;
        }
        tryGetSunSystemProperty(TEXT("sun.stderr.encoding"), javaVersion, jvmMaker, propValue, bufferSunErr, &result);
        if (result == UNRESOLVED_ENCODING) {
            break;
        }
        if ((result != SUN_ENCODING) && (_tcsstr(propValue, TEXT("-Dfile.encoding=")) == propValue)) {
            if (buffer[0] == 0) {
                /* This is the first time we found this property. */
                _tcsncpy(buffer, propValue + 16, ENCODING_BUFFER_SIZE);
                buffer[ENCODING_BUFFER_SIZE - 1] = 0;
                result = FILE_ENCODING;
            } else if (!checkEquivalentEncodings(buffer, propValue + 16)) {
                /* Keep a flag and log later. We will ignore this case if the sun properties are set. */
                foundMultipleFileEncoding = TRUE;
            }   /* The remaining case is when the value of the current system property is different than one of a previously defined,
                 *  but both values point to the same encoding. If this happen, leave the result unchanged and continue. */
        }
    }
    
    if ((result == FILE_ENCODING) && foundMultipleFileEncoding) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
            TEXT("Found multiple occurrences of %s set with different values\n  among the JVM parameters. Cannot resolve the JVM output encoding."),
            TEXT("file.encoding"));
        result = UNRESOLVED_ENCODING;
    } else if (result == SUN_ENCODING) {
        /* For clarity, the sun.*.encoding propeties, when defined, should be both present. We don't even accept cases
         *  where only one would be defined along with file.encoding also set to the same value (although this is valid). */
        if (bufferSunOut[0] == 0) {
            if (bufferSunErr[0] != 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    TEXT("Found sun.stderr.encoding but sun.stdout.encoding was not specified.\n  Please add sun.stdout.encoding to the list of JVM parameters and set its encoding to the same value as sun.stderr.encoding (%s)."), bufferSunErr);
                result = UNRESOLVED_ENCODING;
            }
        } else { /* bufferSunOut[0] != 0 */
            if (bufferSunErr[0] == 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    TEXT("Found sun.stdout.encoding but sun.stderr.encoding was not specified.\n  Please add sun.stderr.encoding to the list of JVM parameters and set its encoding to the same value as sun.stdout.encoding (%s)."), bufferSunOut);
                result = UNRESOLVED_ENCODING;
            } else if (!checkEquivalentEncodings(bufferSunOut, bufferSunErr)) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    TEXT("The encodings of sun.stdout.encoding (%s) and sun.stderr.encoding (%s) don't match.\n  Please set both system properties to the same value."), bufferSunOut, bufferSunErr);
                result = UNRESOLVED_ENCODING;
            } else {
                /* Both sun properties are defined and set to the same value (or equivalent values pointing to the same encoding). */
                _tcsncpy(buffer, bufferSunOut, ENCODING_BUFFER_SIZE);
            }
        }
    }
    
    if (result == UNRESOLVED_ENCODING) {
        buffer[0] = 0;
    }

    freeStringProperties(propNames, propValues, propIndices);
    return result;
}

#ifdef WIN32
static UINT jvmOutputCodePage;
/**
 * Get the code page used to encode the current JVM outputs.
 *  resolveJvmEncoding() should be called before using this function.
 *
 * @return UINT value of the code page (the code page of the current locale is returned by default)
 */
UINT getJvmOutputCodePage() {
    return jvmOutputCodePage;
}
#else
static TCHAR jvmOutputEncoding[ENCODING_BUFFER_SIZE];
static char jvmOutputEncodingMB[ENCODING_BUFFER_SIZE];
/**
 * Get the encoding used for the current JVM outputs.
 *  resolveJvmEncoding() should be called before using this function.
 *
 * @return String representation of the encoding if the value found in file.encoding is supported by iconv, NULL otherwise.
 *         The returned value doesn't need to be freed.
 */
const char* getJvmOutputEncodingMB() {
    if (jvmOutputEncodingMB[0] == 0) {
        return NULL;
    }
    return jvmOutputEncodingMB;
}
#endif

/**
 * Clear the Jvm encoding previously cached.
 *  This function can be called before getJvmOutputEncodingMB() to force using the encoding of the current locale.
 *  A call to resolveJvmEncoding() may then be necessary to restore the encoding.
 *
 * @debug TRUE to print a debug message, FALSE otherwise.
 */
void resetJvmOutputEncoding(int debug) {
    TCHAR buffer[ENCODING_BUFFER_SIZE];

    buffer[0] = 0;
#ifdef WIN32
    jvmOutputCodePage = GetACP();
    _sntprintf(buffer, ENCODING_BUFFER_SIZE, TEXT("%d"), jvmOutputCodePage);
#else
    jvmOutputEncoding[0] = 0;
    jvmOutputEncodingMB[0] = 0;
    if (debug) {
        getCurrentLocaleEncoding(buffer);
    }
#endif
    if (debug) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            TEXT("Reading the JVM output using the encoding of the current locale (%s)."), buffer);
    }
}

#define GET_ENCODING_SYSPROP(o)    o == FILE_ENCODING ? TEXT("file.encoding") : TEXT("sun.stdout.encoding and sun.stderr.encoding")

/**
 * Resolve the Java output encoding using system properties and an internal hashMap containing the supported encoding.
 *  This function should be called prior to using getJvmOutputCodePage() and getJvmOutputEncodingMB()
 *
 * @javaVersion current java version
 * @jvmMaker    current java implementation (Oracle, IBM, etc.)
 *
 * @return TRUE if there is any error (misconfiguration of system properties or unsuported encoding), FALSE otherwise.
 */
int resolveJvmEncoding(int javaVersion, int jvmMaker) {
    TCHAR buffer[ENCODING_BUFFER_SIZE];
    int jvmEncodingOrigin;
#ifndef WIN32
    const TCHAR* encoding;
    const TCHAR* altEncoding;
#endif
    int requiredJavaVersion = 0;
    TCHAR* encodingLwr;
    
    jvmEncodingOrigin = getJvmArgumentsEncoding(buffer, javaVersion, jvmMaker);
    if (jvmEncodingOrigin == UNRESOLVED_ENCODING) {
        /* Unresolved encoding - any error has already been logged */
        return TRUE;
    } else if (jvmEncodingOrigin != LOCALE_ENCODING) {
        /* The encoding was specified in a system property passed to the Java command line. */
        if (!hashMapJvmEncoding) {
            hashMapJvmEncoding = buildJvmEncodingsHashMap(K_ENCODING_V_ENCODING);
            if (!hashMapJvmEncoding) {
                return TRUE;
            }
        }
        encodingLwr = toLower(buffer);
        if (!encodingLwr) {
            return TRUE;
        }
#ifdef WIN32
        jvmOutputCodePage = (UINT)hashMapGetKWVI(hashMapJvmEncoding, encodingLwr);
        free(encodingLwr);
        if (jvmOutputCodePage == 0) {
            /* The value was not found in the hasmap. We have no way to know if the encoding is invalid or if it was
             *  added after this version of the Wrapper was released, so log a message to indicate both possibilities. */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("'%s' is not a valid value for %s\n  or is not supported by this version of the Wrapper."),
                buffer,
                GET_ENCODING_SYSPROP(jvmEncodingOrigin));
            return TRUE;
        } else if (jvmOutputCodePage == -1) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("The value '%s' of %s is not supported on Windows."),
                buffer,
                GET_ENCODING_SYSPROP(jvmEncodingOrigin));
            jvmOutputCodePage = 0;
            return TRUE;
        } else if (!IsValidCodePage(jvmOutputCodePage)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("The value '%s' of %s is not a valid code page."),
                buffer,
                GET_ENCODING_SYSPROP(jvmEncodingOrigin));
            jvmOutputCodePage = 0;
            return TRUE;
        }
        
        if (!checkEncodingJavaVersion(buffer, javaVersion, &requiredJavaVersion)) {
            /* The value exist for a more recent version of Java. */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("The value '%s' of %s is supported from Java %d. The current version of Java is %d."),
                buffer,
                GET_ENCODING_SYSPROP(jvmEncodingOrigin),
                requiredJavaVersion,
                javaVersion);
            return TRUE;
        }

        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            TEXT("Reading the JVM output using the value of %s: %s (resolved to code page %d)."),
            GET_ENCODING_SYSPROP(jvmEncodingOrigin),
            buffer,
            jvmOutputCodePage);
#else
        altEncoding = hashMapGetKWVW(hashMapJvmEncoding, encodingLwr);
        free(encodingLwr);
        if (!altEncoding || (_tcscmp(buffer, TEXT("Not available")) == 0)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("'%s' is not a valid value for %s\n  or is not supported by this version of the Wrapper."),
                buffer,
                GET_ENCODING_SYSPROP(jvmEncodingOrigin));
            return TRUE;
        } else if (getIconvEncodingSupport(buffer) == ICONV_ENCODING_NOT_SUPPORTED) {
            if ((_tcscmp(altEncoding, TEXT("Not available")) == 0) || (getIconvEncodingSupport(altEncoding) == ICONV_ENCODING_NOT_SUPPORTED)) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    TEXT("The value '%s' of %s is not supported by iconv."),
                    buffer,
                    GET_ENCODING_SYSPROP(jvmEncodingOrigin));
                return TRUE;
            }
            encoding = altEncoding;
        } else {
            encoding = buffer;
        }
        
        if (!checkEncodingJavaVersion(buffer, javaVersion, &requiredJavaVersion)) {
            /* The value exist for a more recent version of Java. */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("The value '%s' of %s is supported from Java %d. The current version of Java is %d."),
                buffer,
                GET_ENCODING_SYSPROP(jvmEncodingOrigin),
                requiredJavaVersion,
                javaVersion);
            return TRUE;
        }

        _tcsncpy(jvmOutputEncoding, encoding, ENCODING_BUFFER_SIZE);
        if (wcstombs(jvmOutputEncodingMB, jvmOutputEncoding, ENCODING_BUFFER_SIZE) == (size_t)-1) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Error when converting the JVM output encoding '%s'."), jvmOutputEncoding);
            return TRUE;
        }
        if (_tcscmp(buffer, jvmOutputEncoding) == 0) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("Reading the JVM output using the value of %s: %s."),
                GET_ENCODING_SYSPROP(jvmEncodingOrigin),
                buffer);
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("Reading the JVM output using the value of %s: %s (resolved to %s)."),
                GET_ENCODING_SYSPROP(jvmEncodingOrigin),
                buffer,
                jvmOutputEncoding);
        }
#endif
    } else {
        /* The encoding of the current locale should be used. */
        resetJvmOutputEncoding(wrapperData->isDebugging);
    }

    return FALSE;
}
