/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Utilities
 * Purpose:  GDAL Utilities Public Declarations.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 * ****************************************************************************
 * Copyright (c) 1998, Frank Warmerdam
 * Copyright (c) 2007-2013, Even Rouault <even dot rouault at mines-paris dot org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "cpl_port.h"

typedef enum {
    GDALINFO_FORMAT_TEXT = 0,
    GDALINFO_FORMAT_JSON = 1
} GDALInfoFormat;

typedef struct
{
    GDALInfoFormat eFormat;
    int bComputeMinMax;
    int bReportHistograms;
    int bReportProj4;
    int bStats;
    int bApproxStats;
    int bSample;
    int bComputeChecksum;
    int bShowGCPs;
    int bShowMetadata;
    int bShowRAT;
    int bShowColorTable;
    int bListMDD;
    int bShowFileList;
    int bAllMetadata;
    char **papszExtraMDDomains;
} GDALInfoOptions;

GDALInfoOptions CPL_DLL *GDALInfoOptionsNew( void );

void CPL_DLL GDALInfoOptionsFree( GDALInfoOptions *psOptions );

char CPL_DLL *GDALInfo( GDALDatasetH hDataset, GDALInfoOptions *psOptions );