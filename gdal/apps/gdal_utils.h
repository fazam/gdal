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

/*! output format */
typedef enum {
    /*! output in text format */ GDALINFO_FORMAT_TEXT = 0,
    /*! output in json format */ GDALINFO_FORMAT_JSON = 1
} GDALInfoFormat;

/************************************************************************/
/*                           GDALInfoOptions                            */
/************************************************************************/

/** Options for use with GDALInfo(). GDALInfoOptions* must be allocated and freed with GDALInfoOptionsNew() and 
 * GDALInfoOptionsFree() respectively.
 */
typedef struct
{

    /*! output format */
    GDALInfoFormat eFormat;
    int bComputeMinMax;

    /*! report histogram information for all bands */
    int bReportHistograms;

    /*! report a PROJ.4 string corresponding to the file's coordinate system */
    int bReportProj4;

    /*! read and display image statistics. Force computation if no statistics are stored in an image */
    int bStats;

    /*! read and display image statistics. Force computation if no statistics are stored in an image.
        However, they may be computed based on overviews or a subset of all tiles. Useful if you are
        in a hurry and don't want precise stats. */
    int bApproxStats;

    int bSample;

    /*! force computation of the checksum for each band in the dataset */
    int bComputeChecksum;

    /*! allow or suppress ground control points list printing. It may be useful for datasets with huge amount
        of GCPs, such as L1B AVHRR or HDF4 MODIS which contain thousands of them. */
    int bShowGCPs;

    /*! allow or suppress metadata printing. Some datasets may contain a lot of metadata strings. */
    int bShowMetadata;

    /*! allow or suppress printing of raster attribute table */
    int bShowRAT;

    /*! allow or suppress printing of color table */
    int bShowColorTable;

    /*! list all metadata domains available for the dataset */
    int bListMDD;

    /*! display the file list or the first file of the file list */
    int bShowFileList;

    /*! display all metadata */
    int bAllMetadata;

    /*! report metadata for the specified domains. "all" can be used to report metadata
        in all domains. papszExtraMDDomains must not be directly set but through
        GDALInfoOptionsAddExtraMDDomains(). 
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

/************************************************************************/
/*                         GDALTranslateScaleParams                     */
/************************************************************************/

/** scaling parameters for use in GDALTranslateOptions.
 */
typedef struct
{
    /*! scaling is done only if it is set to TRUE. This is helpful when there is a need to
        scale only certain bands. */
    int     bScale;

    /*! set it to TRUE if dfScaleSrcMin and dfScaleSrcMax is set. When it is FALSE, the
        input range is automatically computed from the source data. */
    int     bHaveScaleSrc;

    /*! the range of input pixel values which need to be scaled */
    double  dfScaleSrcMin, dfScaleSrcMax;

    /*! the range of output pixel values. If dfScaleDstMin and dfScaleDstMax are not set,
        then the output range is 0 to 255. */
    double  dfScaleDstMin, dfScaleDstMax;
} GDALTranslateScaleParams;

/************************************************************************/
/*                         GDALTranslateOptions                         */
/************************************************************************/

/** Options for use with GDALTranslate(). GDALTranslateOptions* must be allocated
 * and freed with GDALTranslateOptionsNew() and GDALTranslateOptionsFree() respectively.
 */
typedef struct
{

    /*! output format. The default is GeoTIFF(GTiff). Use the short format name. */
    char *pszFormat;

    /*! allow or suppress progress monitor and other non-error output */
    int bQuiet;

    /*! for the output bands to be of the indicated data type */
    GDALDataType eOutputType;

    MaskMode eMaskMode;

    /*! number of input bands to write to the output file, or to reorder bands */
    int nBandCount;

    /*! list of input bands to write to the output file, or to reorder bands */
    int *panBandList; /* negative value of panBandList[i] means mask band of ABS(panBandList[i]) */
    
    /*! size of the output file. nOXSizePixel is in pixels and nOYSizePixel is in lines.
        If one of the two values is set to 0, its value will be determined from the other one
        , while maintaining the aspect ratio of the source dataset */
    int nOXSizePixel;
    int nOYSizePixel;

    /*! size of the output file. dfOXSizePct and dfOYSizePct are fraction of the input image size.
        If one of the two values is set to 0, its value will be determined from the other one,
        while maintaining the aspect ratio of the source dataset */
    double dfOXSizePct;
    double dfOYSizePct;

    /*! list of creation options to the output format driver */
    char **papszCreateOptions;

    /*! subwindow from the source image for copying based on pixel/line location */
    int anSrcWin[4];

    /*! don't be forgiving of mismatches and lost data when translating to the output format */
    int bStrict;

    /*! apply the scale/offset metadata for the bands to convert scaled values to unscaled values.
     *  It is also often necessary to reset the output datatype with eOutputType */
    int bUnscale;

    /*! the size of pasScaleParams */
    int nScaleRepeat;

    /*! the list of scale parameters for each band. */
    GDALTranslateScaleParams *pasScaleParams;

    /*! It is set to TRUE, when scale parameters are specific to each band */
    int bHasUsedExplicitScaleBand;
    
    /*! the size of the list padfExponent */
    int nExponentRepeat;

    /*! to apply non-linear scaling with a power function. It is the list of exponents of the power
        function (must be positive). This option must be used with pasScaleParams. If
        nExponentRepeat is 1, it is applied to all bands of the output image. */
    double *padfExponent;

    int bHasUsedExplicitExponentBand;
    
    /*! list of metadata key and value to set on the output dataset if possible.
     *  GDALTranslateOptionsSetMetadataOptions() and GDALTranslateOptionsAddMetadataOptions()
     *  should be used */
    char **papszMetadataOptions;

    /*! override the projection for the output file. The SRS may be any of the usual
        GDAL/OGR forms, complete WKT, PROJ.4, EPSG:n or a file containing the WKT. */
    char *pszOutputSRS;

    /*! number of GCPS to be added to the output dataset */
    int nGCPCount;

    /*! list of GCPs to be added to the output dataset */
    GDAL_GCP *pasGCPs;

    /*! assign/override the georeferenced bounds of the output file. This assigns
        georeferenced bounds to the output file, ignoring what would have been
        derived from the source file. So this does not cause reprojection to the
        specified SRS. */
    double adfULLR[4];

    /*! set a nodata value specified in dfNoDataReal to the output bands */
    int bSetNoData;

    /*! avoid setting a nodata value to the output file if one exists for the source file */
    int bUnsetNoData;

    /*! Assign a specified nodata value to output bands ( bSetNoData option should be set). Note that if the input
        dataset has a nodata value, this does not cause pixel values that are
        equal to that nodata value to be changed to the value specified. */
    double dfNoDataReal;

    /*! to expose a dataset with 1 band with a color table as a dataset with
        3 (RGB) or 4 (RGBA) bands. Useful for output drivers such as JPEG,
        JPEG2000, MrSID, ECW that don't support color indexed datasets.
        The 1 value enables to expand a dataset with a color table that only
        contains gray levels to a gray indexed dataset. */
    int nRGBExpand;

    int nMaskBand; /* negative value means mask band of ABS(nMaskBand) */
    
    /*! force recomputation of statistics */
    int bStats;

    int bApproxStats;

    /*! If this option is set, anSrcWin or (dfULX, dfULY, dfLRX, dfLRY) values
        that falls partially outside the source raster extent will be considered
        as an error. The default behaviour is to accept such requests. */
    int bErrorOnPartiallyOutside;

    /*! Same as bErrorOnPartiallyOutside, except that the criterion for
        erroring out is when the request falls completely outside the
        source raster extent. */
    int bErrorOnCompletelyOutside;

    /*! does not copy source RAT into destination dataset (when TRUE) */
    int bNoRAT;

    /*! resampling algorithm
        nearest (default), bilinear, cubic, cubicspline, lanczos, average, mode */
    char *pszResampling;
    
    /*! target resolution. The values must be expressed in georeferenced units.
        Both must be positive values. This is exclusive with nOXSizePixel (or dfOXSizePct),
        nOYSizePixel (or dfOYSizePct) and adfULLR */
    double dfXRes;
    double dfYRes;

    /*! subwindow from the source image for copying (like anSrcWin) but
        with the corners given in georeferenced coordinates (by default
        expressed in the SRS of the dataset. Can be changed with
        pszProjSRS) */
    double dfULX;
    double dfULY;
    double dfLRX;
    double dfLRY;

    /*! SRS in which to interpret the coordinates given with dfULX, dfULY, dfLRX, dfLRY.
        The SRS may be any of the usual GDAL/OGR forms, complete WKT, PROJ.4, EPSG:n or
        a file containing the WKT. Note that this does not cause reprojection of the
        dataset to the specified SRS. */
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


typedef enum
{
    /*! thin plate spline transformer based on available GCPs */ GCP_TPS,
    /*! RPCs */ RPC,
    /*! Geolocation Arrays */ GEOLOC_ARRAY
} TransformerMethod;

/************************************************************************/
/*                        GDALWarpAppOptions                            */
/************************************************************************/

/** Options for use with GDALWarp(). GDALWarpAppOptions* must be allocated and
 * freed with GDALWarpAppOptionsNew() and GDALWarpAppOptionsFree() respectively.
 */
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

void CPL_DLL GDALWarpAppOptionsSetOrder( GDALWarpAppOptions *psOptions, int nOrder);

void CPL_DLL GDALWarpAppOptionsSetRefineGCPs( GDALWarpAppOptions *psOptions, int nTolerance, int nMinimumGCPs );

void CPL_DLL GDALWarpAppOptionsSetMethod( GDALWarpAppOptions *psOptions, TransformerMethod eTransformerMethod );

void CPL_DLL GDALWarpAppOptionsSetWarpOptions( GDALWarpAppOptions *psOptions, char **papszWarpOptions );

void CPL_DLL GDALWarpAppOptionsAddWarpOptions( GDALWarpAppOptions *psOptions, const char *pszWarpOption );

void CPL_DLL GDALWarpAppOptionsSetCreateOptions( GDALWarpAppOptions *psOptions, char **papszCreateOptions );

void CPL_DLL GDALWarpAppOptionsAddCreateOptions( GDALWarpAppOptions *psOptions, const char *pszCreateOption );

void CPL_DLL GDALWarpAppOptionsSetTransformerOptions( GDALWarpAppOptions *psOptions, char **papszTO );

void CPL_DLL GDALWarpAppOptionsAddTransformerOptions( GDALWarpAppOptions *psOptions, const char *pszTransformerOption );

void CPL_DLL GDALWarpAppOptionsSetDestOpenOptions( GDALWarpAppOptions *psOptions, char **papszDestOpenOptions );

void CPL_DLL GDALWarpAppOptionsAddDestOpenOptions( GDALWarpAppOptions *psOptions, const char *pszDestOpenOption );

typedef enum
{
    GEOMOP_NONE,
    GEOMOP_SEGMENTIZE,
    GEOMOP_SIMPLIFY_PRESERVE_TOPOLOGY,
} GeomOperation;

typedef enum
{
    GEOMTYPE_DEFAULT,
    GEOMTYPE_SET,
    GEOMTYPE_PROMOTE_TO_MULTI,
    GEOMTYPE_CONVERT_TO_LINEAR,
    GEOMTYPE_CONVERT_TO_CURVE,
} GeomType;

/*! Access modes */
typedef enum
{
    ACCESS_CREATION,
    /*! open existing output datasource in update mode rather than trying
    to create a new one */ ACCESS_UPDATE,
    /*! append to existing layer instead of creating new */ ACCESS_APPEND,
    /*! delete the output layer and recreate it empty */ ACCESS_OVERWRITE,
} AccessMode;

#define COORD_DIM_LAYER_DIM -2

/************************************************************************/
/*                           OGR2OGROptions                             */
/************************************************************************/

/** Options for use with OGR2OGR(). OGR2OGROptions* must be allocated and
 * freed with OGR2OGROptionsNew() and OGR2OGROptionsFree() respectively.
 */
typedef struct
{
    /*! continue after a failure, skipping the failured feature */
    int bSkipFailures;

    int bLayerTransaction;
    int bForceTransaction;

    /*! group n features per transaction (default 20000 in OGR 1.11, 200 in previous releases).
    Increase the value for better performance when writing into DBMS drivers that have transaction
    support. Starting with GDAL 2.0, n can be set to unlimited to load the data into a single transaction */
    int nGroupTransactions;
    GIntBig nFIDToFetch;
    int bQuiet;

    /*! output file format name (default is ESRI Shapefile) */
    char *pszFormat;
    char **papszLayers;
    char **papszDSCO;
    char **papszLCO;
    AccessMode eAccessMode;
    int bAddMissingFields;
    int bTransform; //bTransform must be set to TRUE to trigger reprojection, otherwise only SRS assignment is done.
    char *pszOutputSRSDef;
    char *pszSourceSRSDef;
    int bNullifyOutputSRS;
    int bExactFieldNameMatch;

    /*! an alternate name to the new layer */
    char *pszNewLayerName;
    char *pszWHERE;
    char *pszGeomField;
    char **papszSelFields;
    char *pszSQLStatement;
    char *pszDialect;
    int eGType;
    GeomType eGeomConversion;
    GeomOperation eGeomOp;
    double dfGeomOpParam;
    char **papszFieldTypesToString;
    char **papszMapFieldType;

    /*! set field width and precision to 0 */
    int bUnsetFieldWidth;

    /*! display progress on terminal. Only works if input layers have the "fast feature count"
    capability */
    int bDisplayProgress;

    /*! split geometries crossing the dateline meridian */
    int bWrapDateline;

    /*! offset from dateline in degrees (default long. = +/- 10deg, geometries
    within 170deg to -170deg will be split) */
    int nDateLineOffset;


    int bClipSrc;
    OGRGeometryH hClipSrc;
    char *pszClipSrcDS;

    /*! select desired geometries using an SQL query */
    char *pszClipSrcSQL;

    /*! selected named layer from the source clip datasource */
    char *pszClipSrcLayer;

    /*! restrict desired geometries based on attribute query */
    char *pszClipSrcWhere;

    OGRGeometryH hClipDst;

    char *pszClipDstDS;

    /*! select desired geometries using an SQL query */
    char *pszClipDstSQL;

    /*! selected named layer from the destination clip datasource */
    char *pszClipDstLayer;

    /*! restrict desired geometries based on attribute query */
    char *pszClipDstWhere;


    int bSplitListFields;
    int nMaxSplitListSubFields;
    int bExplodeCollections;
    char *pszZField;
    char **papszFieldMap;
    int nCoordDim;

    /*! destination dataset open option (format specific), only valid in update mode */
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

void CPL_DLL OGR2OGROptionsSetLayers( OGR2OGROptions *psOptions,
                                      char **papszLayers );

void CPL_DLL OGR2OGROptionsAddLayer( OGR2OGROptions *psOptions,
                                     const char *pszLayer );

void CPL_DLL OGR2OGROptionsSetDSCO( OGR2OGROptions *psOptions,
                                    char **papszDSCO );

void CPL_DLL OGR2OGROptionsAddDSCO( OGR2OGROptions *psOptions,
                                    const char *pszDSCO );

void CPL_DLL OGR2OGROptionsSetLCO( OGR2OGROptions *psOptions,
                                   char **papszLCO );

void CPL_DLL OGR2OGROptionsAddLCO( OGR2OGROptions *psOptions,
                                   const char *pszLCO );

void CPL_DLL OGR2OGROptionsSetSelFields( OGR2OGROptions *psOptions,
                                         char **papszSelFields );

void CPL_DLL OGR2OGROptionsAddSelFields( OGR2OGROptions *psOptions,
                                         const char *pszSelField );

void CPL_DLL OGR2OGROptionsSetFieldTypesToString( OGR2OGROptions *psOptions,
                                                  char **papszFieldTypesToString );

void CPL_DLL OGR2OGROptionsAddFieldTypesToString( OGR2OGROptions *psOptions,
                                                  const char *pszFieldType );

void CPL_DLL OGR2OGROptionsSetMapFieldType( OGR2OGROptions *psOptions,
                                            char **papszMapFieldType );

void CPL_DLL OGR2OGROptionsAddMapFieldType( OGR2OGROptions *psOptions,
                                            const char *pszFieldType );

void CPL_DLL OGR2OGROptionsSetFieldMap( OGR2OGROptions *psOptions,
                                        char **papszFieldMap );

void CPL_DLL OGR2OGROptionsAddFieldMap( OGR2OGROptions *psOptions,
                                        const char *pszField );

void CPL_DLL OGR2OGROptionsSetDestOpenOptions( OGR2OGROptions *psOptions,
                                               char **papszDestOpenOptions );

void CPL_DLL OGR2OGROptionsAddDestOpenOptions( OGR2OGROptions *psOptions,
                                               const char *pszDestOpenOption );

void CPL_DLL OGR2OGROptionsSetMetadataOptions( OGR2OGROptions *psOptions,
                                               char **papszMetadataOptions );

void CPL_DLL OGR2OGROptionsAddMetadataOptions( OGR2OGROptions *psOptions,
                                               const char *pszMetadataOption );

GDALDatasetH CPL_DLL OGR2OGR( const char *pszDest, GDALDatasetH hDstDS, GDALDatasetH hSrcDS,
                              OGR2OGROptions *psOptions, int *pbUsageError );

CPL_C_END
