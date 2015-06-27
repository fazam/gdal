/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Utilities
 * Purpose:  GDAL Utilities Public Declarations.
 * Author:   Faza Mahamood, fazamhd at gmail dot com
 *
 * ****************************************************************************
 * Copyright (c) 1998, Frank Warmerdam
 * Copyright (c) 2007-2013, Even Rouault <even dot rouault at mines-paris dot org>
 * Copyright (c) 2015, Faza Mahamood
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

/**
 * \file gdal_utils.h
 *
 * Public (C callable) GDAL Utilities entry points.
 */

#include "cpl_port.h"

CPL_C_START

typedef enum {
    GDALINFO_FORMAT_TEXT = 0,
    GDALINFO_FORMAT_JSON = 1
} GDALInfoFormat;

/** GDALInfoOptions* must be allocated and freed with GDALInfoOptionsNew() and 
 * GDALInfoOptionsFree() respectively.
 */
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
    /** papszExtraMDDomains must not be directly set but through
     * GDALInfoOptionsAddExtraMDDomains().
     */
    char **papszExtraMDDomains;
} GDALInfoOptions;

GDALInfoOptions CPL_DLL *GDALInfoOptionsNew( void );

void CPL_DLL GDALInfoOptionsAddExtraMDDomains( GDALInfoOptions *psOptions,
                                               const char *pszDomain );

void CPL_DLL GDALInfoOptionsSetExtraMDDomains( GDALInfoOptions *psOptions,
                                               char **papszExtraMDDomains );

void CPL_DLL GDALInfoOptionsFree( GDALInfoOptions *psOptions );

char CPL_DLL *GDALInfo( GDALDatasetH hDataset, GDALInfoOptions *psOptions );

typedef enum
{
    MASK_DISABLED,
    MASK_AUTO,
    MASK_USER
} MaskMode;

typedef struct
{
    int     bScale;
    int     bHaveScaleSrc;
    double  dfScaleSrcMin, dfScaleSrcMax;
    double  dfScaleDstMin, dfScaleDstMax;
} ScaleParams;

typedef struct
{
    char *pszFormat;
    int bQuiet;
    GDALProgressFunc pfnProgress;
    void *pProgressData;
    GDALDataType eOutputType;
    MaskMode eMaskMode;
    char *pszDest;
    int nBandCount;
    int *panBandList; /* negative value of panBandList[i] means mask band of ABS(panBandList[i]) */
    int nOXSizePixel;
    int nOYSizePixel;
    double dfOXSizePct;
    double dfOYSizePct;
    char **papszCreateOptions;
    int anSrcWin[4];
    int bStrict;
    int bUnscale;
    int nScaleRepeat;
    ScaleParams *pasScaleParams;
    int bHasUsedExplicitScaleBand;
    int nExponentRepeat;
    double *padfExponent;
    int bHasUsedExplicitExponentBand;
    double dfULX;
    double dfULY;
    double dfLRX;
    double dfLRY;
    char **papszMetadataOptions;
    char *pszOutputSRS;
    int bGotBounds;
    int nGCPCount;
    GDAL_GCP *pasGCPs;
    double adfULLR[4];
    int bSetNoData;
    int bUnsetNoData;
    double dfNoDataReal;
    int nRGBExpand;
    int nMaskBand; /* negative value means mask band of ABS(nMaskBand) */
    int bStats;
    int bApproxStats;
    int bErrorOnPartiallyOutside;
    int bErrorOnCompletelyOutside;
    int bNoRAT;
    char *pszResampling;
    double dfXRes;
    double dfYRes;
    char *pszProjSRS;

} GDALTranslateOptions;

GDALTranslateOptions CPL_DLL *GDALTranslateOptionsNew(void);

void CPL_DLL GDALTranslateOptionsFree(GDALTranslateOptions *psOptions);

GDALDatasetH CPL_DLL GDALTranslate(char *pszDest, GDALDatasetH hDataset, GDALTranslateOptions *psOptions, int *pbUsageError);

CPL_C_END
