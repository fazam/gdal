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
#include "gdalwarper.h"

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
} GDALTranslateScaleParams;

typedef struct
{
    char *pszFormat;
    int bQuiet;
    GDALProgressFunc pfnProgress;
    void *pProgressData;
    GDALDataType eOutputType;
    MaskMode eMaskMode;
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
    GDALTranslateScaleParams *pasScaleParams;
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

void CPL_DLL GDALTranslateOptionsSetCreateOptions( GDALTranslateOptions *psOptions,
                                                   char **papszCreateOptions );

void  CPL_DLL GDALTranslateOptionsAddCreateOptions( GDALTranslateOptions *psOptions,
                                                    const char *pszCreateOption );

void CPL_DLL GDALTranslateOptionsSetMetadataOptions( GDALTranslateOptions *psOptions,
                                                     char **papszMetadataOptions );

void  CPL_DLL GDALTranslateOptionsAddMetadataOptions( GDALTranslateOptions *psOptions,
                                                      const char *pszMetadataOption );

void CPL_DLL GDALTranslateOptionsAddBand( GDALTranslateOptions *psOptions, int nBand,
                                          int bIsMask );

GDALDatasetH CPL_DLL GDALTranslate(const char *pszDest, GDALDatasetH hDataset, GDALTranslateOptions *psOptions, int *pbUsageError);

typedef struct
{
    double dfMinX;
    double dfMinY;
    double dfMaxX;
    double dfMaxY;
    double dfXRes;
    double dfYRes;
    int bTargetAlignedPixels;
    int nForcePixels;
    int nForceLines;
    int bQuiet;
    int bEnableDstAlpha;
    int bEnableSrcAlpha;
    char *pszFormat;
    int bCreateOutput;
    char **papszWarpOptions;
    double dfErrorThreshold;
    double dfWarpMemoryLimit;
    char **papszCreateOptions;
    GDALDataType eOutputType;
    GDALDataType eWorkingType;
    GDALResampleAlg eResampleAlg;
    char *pszSrcNodata;
    char *pszDstNodata;
    int bMulti;
    char **papszTO;
    char *pszCutlineDSName;
    char *pszCLayer;
    char *pszCWHERE;
    char *pszCSQL;
    int bCropToCutline;
    int bOverwrite;
    int bCopyMetadata;
    int bCopyBandInfo;
    char *pszMDConflictValue;
    int bSetColorInterpretation;
    char **papszDestOpenOptions;
    int nOvLevel;
    char *pszTE_SRS;

} GDALWarpAppOptions;

GDALDatasetH CPL_DLL GDALWarp( const char *pszDest, GDALDatasetH hDstDS, int nSrcCount,
                       GDALDatasetH *pahSrcDS, GDALWarpAppOptions *psOptions, int *pbUsageError );

GDALWarpAppOptions CPL_DLL *GDALWarpAppOptionsNew( void );

void CPL_DLL GDALWarpAppOptionsFree( GDALWarpAppOptions *psOptions );

char CPL_DLL *SanitizeSRS( const char *pszUserInput );

void CPL_DLL GDALWarpAppOptionsSetSrcSRS( GDALWarpAppOptions *psOptions, const char *pszSrcSRS );

void CPL_DLL GDALWarpAppOptionsSetDstSRS( GDALWarpAppOptions *psOptions, const char *pszDstSRS );

void CPL_DLL GDALWarpAppOptionsSetOrder( GDALWarpAppOptions *psOptions, const char *pszN);

void CPL_DLL GDALWarpAppOptionsSetRefineGCPs( GDALWarpAppOptions *psOptions, const char *pszTolerance, const char *pszMinimumGCPs );

void CPL_DLL GDALWarpAppOptionsSetMethod( GDALWarpAppOptions *psOptions, const char *pszMethod );

void CPL_DLL GDALWarpAppOptionsSetTransformerOption( GDALWarpAppOptions *psOptions, const char *pszTransformerOption );

void CPL_DLL GDALWarpAppOptionsSetWarpOptions( GDALWarpAppOptions *psOptions, char **papszWarpOptions );

void CPL_DLL GDALWarpAppOptionsSetCreateOptions( GDALWarpAppOptions *psOptions, char **papszCreateOptions );

void CPL_DLL GDALWarpAppOptionsSetTO( GDALWarpAppOptions *psOptions, char **papszTO );

void CPL_DLL GDALWarpAppOptionsSetDestOpenOptions( GDALWarpAppOptions *psOptions, char **papszDestOpenOptions );

typedef enum
{
    NONE,
    SEGMENTIZE,
    SIMPLIFY_PRESERVE_TOPOLOGY,
} GeomOperation;

typedef struct
{
    int bPromoteToMulti;
    int bConvertToLinear;
    int bConvertToCurve;
} GeometryConversion;

#define COORD_DIM_LAYER_DIM -2

typedef struct
{
    int bSkipFailures;
    int bLayerTransaction;
    int bForceTransaction;
    int nGroupTransactions;
    GIntBig nFIDToFetch;
    int bQuiet;
    char *pszFormat;
    char **papszLayers;
    char **papszDSCO;
    char **papszLCO;
    int bTransform;
    int bAppend;
    int bUpdate;
    int bOverwrite;
    int bAddMissingFields;
    char *pszOutputSRSDef;
    char *pszSourceSRSDef;
    int bNullifyOutputSRS;
    int bExactFieldNameMatch;
    char *pszNewLayerName;
    char *pszWHERE;
    char *pszGeomField;
    char **papszSelFields;
    char *pszSQLStatement;
    char *pszDialect;
    int eGType;
    GeometryConversion sGeomConversion;
    GeomOperation eGeomOp;
    double dfGeomOpParam;
    char **papszFieldTypesToString;
    char **papszMapFieldType;
    int bUnsetFieldWidth;
    int bDisplayProgress;
    int bWrapDateline;
    char *pszDateLineOffset;
    int bClipSrc;
    OGRGeometryH hClipSrc;
    char *pszClipSrcDS;
    char *pszClipSrcSQL;
    char *pszClipSrcLayer;
    char *pszClipSrcWhere;
    OGRGeometryH hClipDst;
    char *pszClipDstDS;
    char *pszClipDstSQL;
    char *pszClipDstLayer;
    char *pszClipDstWhere;
    int bSplitListFields;
    int nMaxSplitListSubFields;
    int bExplodeCollections;
    char *pszZField;
    char *pszFieldMap;
    char **papszFieldMap;
    int nCoordDim;
    char **papszDestOpenOptions;
    int bForceNullable;
    int bUnsetDefault;
    int bUnsetFid;
    int bPreserveFID;
    int bCopyMD;
    char **papszMetadataOptions;
    char *pszSpatSRSDef;
    int nGCPCount;
    GDAL_GCP *pasGCPs;
    int nTransformOrder;
    OGRGeometryH hSpatialFilter;

} OGR2OGROptions;

OGR2OGROptions CPL_DLL *OGR2OGROptionsNew( void );

void CPL_DLL OGR2OGROptionsFree( OGR2OGROptions *psOptions );

GDALDatasetH CPL_DLL OGR2OGR( const char *pszDest, GDALDatasetH hDataset, OGR2OGROptions *psOptions, int *bUsageError );

CPL_C_END
