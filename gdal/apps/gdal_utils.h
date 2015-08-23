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

    /*! the range of output pixel values. If GDALTranslateScaleParams::dfScaleDstMin
        and GDALTranslateScaleParams::dfScaleDstMax are not set, then the output
        range is 0 to 255. */
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

    /*! the progress function to use */
    GDALProgressFunc pfnProgress;
    
    /*! pointer to the progress data variable */
    void *pProgressData;

    /*! for the output bands to be of the indicated data type */
    GDALDataType eOutputType;

    MaskMode eMaskMode;

    /*! number of input bands to write to the output file, or to reorder bands */
    int nBandCount;

    /*! list of input bands to write to the output file, or to reorder bands. The
        value 1 corresponds to the 1st band. */
    int *panBandList; /* negative value of panBandList[i] means mask band of ABS(panBandList[i]) */
    
    /*! size of the output file. GDALTranslateOptions::nOXSizePixel is in pixels and
        GDALTranslateOptions::nOYSizePixel is in lines. If one of the two values is
        set to 0, its value will be determined from the other one, while maintaining
        the aspect ratio of the source dataset */
    int nOXSizePixel;
    int nOYSizePixel;

    /*! size of the output file. GDALTranslateOptions::dfOXSizePct and GDALTranslateOptions::dfOYSizePct
        are fraction of the input image size. The value 100 means 100%. If one of the two values is set
        to 0, its value will be determined from the other one, while maintaining the aspect ratio of the
        source dataset */
    double dfOXSizePct;
    double dfOYSizePct;

    /*! list of creation options to the output format driver */
    char **papszCreateOptions;

    /*! subwindow from the source image for copying based on pixel/line location */
    int anSrcWin[4];

    /*! don't be forgiving of mismatches and lost data when translating to the output format */
    int bStrict;

    /*! apply the scale/offset metadata for the bands to convert scaled values to unscaled values.
     *  It is also often necessary to reset the output datatype with GDALTranslateOptions::eOutputType */
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
        function (must be positive). This option must be used with GDALTranslateOptions::pasScaleParams. If
        GDALTranslateOptions::nExponentRepeat is 1, it is applied to all bands of the output image. */
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

    /*! set a nodata value specified in GDALTranslateOptions::dfNoDataReal to the output bands */
    int bSetNoData;

    /*! avoid setting a nodata value to the output file if one exists for the source file */
    int bUnsetNoData;

    /*! Assign a specified nodata value to output bands ( GDALTranslateOptions::bSetNoData option
        should be set). Note that if the input dataset has a nodata value, this does not cause
        pixel values that are equal to that nodata value to be changed to the value specified. */
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

    /*! If this option is set, GDALTranslateOptions::anSrcWin or (GDALTranslateOptions::dfULX,
        GDALTranslateOptions::dfULY, GDALTranslateOptions::dfLRX, GDALTranslateOptions::dfLRY)
        values that falls partially outside the source raster extent will be considered
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
        Both must be positive values. This is exclusive with GDALTranslateOptions::nOXSizePixel
        (or GDALTranslateOptions::dfOXSizePct), GDALTranslateOptions::nOYSizePixel
        (or GDALTranslateOptions::dfOYSizePct) and GDALTranslateOptions::adfULLR */
    double dfXRes;
    double dfYRes;

    /*! subwindow from the source image for copying (like GDALTranslateOptions::anSrcWin)
        but with the corners given in georeferenced coordinates (by default
        expressed in the SRS of the dataset. Can be changed with
        pszProjSRS) */
    double dfULX;
    double dfULY;
    double dfLRX;
    double dfLRY;

    /*! SRS in which to interpret the coordinates given with GDALTranslateOptions::dfULX,
        GDALTranslateOptions::dfULY, GDALTranslateOptions::dfLRX, GDALTranslateOptions::dfLRY.
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
    /*! thin plate spline transformer based on available GCPs */ GWTM_GCP_TPS,
    /*! RPCs */ GWTM_RPC,
    /*! Geolocation Arrays */ GWTM_GEOLOC_ARRAY
} GDALWarpTransformerMethod;

/************************************************************************/
/*                        GDALWarpAppOptions                            */
/************************************************************************/

/** Options for use with GDALWarp(). GDALWarpAppOptions* must be allocated and
 * freed with GDALWarpAppOptionsNew() and GDALWarpAppOptionsFree() respectively.
 */
typedef struct
{
    /*! set georeferenced extents of output file to be created (in target SRS by default,
        or in the SRS specified with pszTE_SRS) */
    double dfMinX;
    double dfMinY;
    double dfMaxX;
    double dfMaxY;

    /*! the SRS in which to interpret the coordinates given in GDALWarpAppOptions::dfMinX,
        GDALWarpAppOptions::dfMinY, GDALWarpAppOptions::dfMaxX and GDALWarpAppOptions::dfMaxY.
        The SRS may be any of the usual GDAL/OGR forms,
        complete WKT, PROJ.4, EPSG:n or a file containing the WKT. It is a
        conveniency e.g. when knowing the output coordinates in a
        geodetic long/lat SRS, but still wanting a result in a projected
        coordinate system. */
    char *pszTE_SRS;

    /*! set output file resolution (in target georeferenced units) */
    double dfXRes;
    double dfYRes;

    /*! align the coordinates of the extent of the output file to the values of the
        GDALWarpAppOptions::dfXRes and GDALWarpAppOptions::dfYRes, such that the
        aligned extent includes the minimum extent. */
    int bTargetAlignedPixels;
    
    /*! set output file size in pixels and lines. If GDALWarpAppOptions::nForcePixels
        or GDALWarpAppOptions::nForceLines is set to 0, the other dimension will be
        guessed from the computed resolution. Note that GDALWarpAppOptions::nForcePixels and
        GDALWarpAppOptions::nForceLines cannot be used with GDALWarpAppOptions::dfXRes and
        GDALWarpAppOptions::dfYRes. */
    int nForcePixels;
    int nForceLines;
    
    /*! allow or suppress progress monitor and other non-error output */
    int bQuiet;

    /*! creates an output alpha band to identify nodata (unset/transparent) pixels
        when set to TRUE */
    int bEnableDstAlpha;

    int bEnableSrcAlpha;

    /*! output format. The default is GeoTIFF (GTiff). Use the short format name. */
    char *pszFormat;

    int bCreateOutput;

    /*! list of warp options. ("NAME1=VALUE1","NAME2=VALUE2",...). The
        GDALWarpOptions::papszWarpOptions docs show all options. */
    char **papszWarpOptions;

    double dfErrorThreshold;

    /*! the amount of memory (in megabytes) that the warp API is allowed
        to use for caching. */
    double dfWarpMemoryLimit;

    /*! list of create options for the output format driver. See format
        specific documentation for legal creation options for each format. */
    char **papszCreateOptions;

    /*! the data type of the output bands */
    GDALDataType eOutputType;

    /*! working pixel data type. The data type of pixels in the source
        image and destination image buffers. */
    GDALDataType eWorkingType;

    /*! the resampling method. Available methods are: near, bilinear,
        cubic, cubicspline, lanczos, average, mode, max, min, med,
        q1, q3 */
    GDALResampleAlg eResampleAlg;
    
    /*! nodata masking values for input bands (different values can be supplied
        for each band). ("value1 value2 ..."). Masked values will not be used
        in interpolation. Use a value of "None" to ignore intrinsic nodata
        settings on the source dataset. */
    char *pszSrcNodata;

    /*! nodata values for output bands (different values can be supplied for
        each band). ("value1 value2 ..."). New files will be initialized to
        this value and if possible the nodata value will be recorded in the
        output file. Use a value of "None" to ensure that nodata is not defined.
        If this argument is not used then nodata values will be copied from
        the source dataset. */
    char *pszDstNodata;

    /*! use multithreaded warping implementation. Multiple threads will be used
        to process chunks of image and perform input/output operation simultaneously. */
    int bMulti;

    /*! list of transformer options suitable to pass to GDALCreateGenImgProjTransformer2().
        ("NAME1=VALUE1","NAME2=VALUE2",...) */
    char **papszTO;

    /*! enable use of a blend cutline from the name OGR support pszCutlineDSName */
    char *pszCutlineDSName;

    /*! the named layer to be selected from the cutline datasource */
    char *pszCLayer;

    /*! restrict desired cutline features based on attribute query */
    char *pszCWHERE;

    /*! SQL query to select the cutline features instead of from a layer
        with pszCLayer */
    char *pszCSQL;

    /*! crop the extent of the target dataset to the extent of the cutline */
    int bCropToCutline;

    /*! overwrite the target dataset if it already exists */
    int bOverwrite;

    /*! copy dataset and band metadata will be copied from the first source dataset. Items that differ between
        source datasets will be set "*" (see GDALWarpAppOptions::pszMDConflictValue) */
    int bCopyMetadata;

    /*! copy band information from the first source dataset */
    int bCopyBandInfo;

    /*! value to set metadata items that conflict between source datasets (default is "*").
        Use "" to remove conflicting items. */
    char *pszMDConflictValue;

    /*! set the color interpretation of the bands of the target dataset from the source dataset */
    int bSetColorInterpretation;

    /*! output dataset open option (format specific) */
    char **papszDestOpenOptions;

    /*! overview level of source files to be used */
    int nOvLevel;
    

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

void CPL_DLL GDALWarpAppOptionsSetMethod( GDALWarpAppOptions *psOptions, GDALWarpTransformerMethod eTransformerMethod );

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

    /*! use layer level transaction. If set to FALSE, then it is interpreted as dataset level transaction. */
    int bLayerTransaction;

    /*! force the use of particular transaction type based on OGR2OGR::bLayerTransaction */
    int bForceTransaction;

    /*! group nGroupTransactions features per transaction (default 20000). Increase the value for better
        performance when writing into DBMS drivers that have transaction support. nGroupTransactions can
        be set to -1 to load the data into a single transaction */
    int nGroupTransactions;

    /*! If provided, only the feature with this feature id will be reported. Operates exclusive of
        the spatial or attribute queries. Note: if you want to select several features based on their
        feature id, you can also use the fact the 'fid' is a special field recognized by OGR SQL.
        So OGR2OGROptions::pszWHERE = "fid in (1,3,5)" would select features 1, 3 and 5. */
    GIntBig nFIDToFetch;
    
    /*! allow or suppress progress monitor and other non-error output */
    int bQuiet;

    /*! output file format name (default is ESRI Shapefile) */
    char *pszFormat;

    /*! list of layers of the source dataset which needs to be selected */
    char **papszLayers;

    /*! dataset creation option (format specific) */
    char **papszDSCO;

    /*! layer creation option (format specific) */
    char **papszLCO;

    /*! access modes */
    AccessMode eAccessMode;

    /*! It has the effect of adding, to existing target layers, the new fields found in source layers.
        This option is useful when merging files that have non-strictly identical structures. This might
        not work for output formats that don't support adding fields to existing non-empty layers. */
    int bAddMissingFields;

    /*! It must be set to TRUE to trigger reprojection, otherwise only SRS assignment is done. */
    int bTransform;

    /*! output SRS. OGR2OGROptions::bTransform must be set to TRUE to trigger reprojection,
        otherwise only SRS assignment is done. */
    char *pszOutputSRSDef;

    /*! override source SRS */
    char *pszSourceSRSDef;

    int bNullifyOutputSRS;

    /*! If set to FALSE, then field name matching between source and existing target layer is done
        in a more relaxed way if the target driver has an implementation for it. */
    int bExactFieldNameMatch;

    /*! an alternate name to the new layer */
    char *pszNewLayerName;

    /*! attribute query (like SQL WHERE) */
    char *pszWHERE;

    /*! name of the geometry field on which the spatial filter operates on. */
    char *pszGeomField;

    /*! list of fields from input layer to copy to the new layer. A field is skipped if
        mentioned previously in the list even if the input layer has duplicate field names.
        (Defaults to all; any field is skipped if a subsequent field with same name is
        found.) Geometry fields can also be specified in the list. */
    char **papszSelFields;

    /*! SQL statement to execute. The resulting table/layer will be saved to the output. */
    char *pszSQLStatement;

    /*! SQL dialect. In some cases can be used to use (unoptimized) OGR SQL instead of the
        native SQL of an RDBMS by using "OGRSQL". The "SQLITE" dialect can also be used with
        any datasource. */
    char *pszDialect;

    /*! the geometry type for the created layer */
    int eGType;

    GeomType eGeomConversion;

    /*! Geomertric operation to perform */
    GeomOperation eGeomOp;

    /*! the parameter to geometric operation */
    double dfGeomOpParam;

    /*! list of field types to convert to a field of type string in the destination layer.
        Valid types are: Integer, Integer64, Real, String, Date, Time, DateTime, Binary,
        IntegerList, Integer64List, RealList, StringList. Special value "All" can be
        used to convert all fields to strings. This is an alternate way to using the CAST
        operator of OGR SQL, that may avoid typing a long SQL query. Note that this does
        not influence the field types used by the source driver, and is only an afterwards
        conversion. */
    char **papszFieldTypesToString;

    /*! list of field types and the field type after conversion in the destination layer.
        ("srctype1=dsttype1","srctype2=dsttype2",...).
        Valid types are : Integer, Integer64, Real, String, Date, Time, DateTime, Binary,
        IntegerList, Integer64List, RealList, StringList. Types can also include subtype
        between parenthesis, such as Integer(Boolean), Real(Float32), ... Special value 
        "All" can be used to convert all fields to another type. This is an alternate way to
        using the CAST operator of OGR SQL, that may avoid typing a long SQL query.
        This is a generalization of OGR2OGROptions::papszFieldTypeToString. Note that this does not influence
        the field types used by the source driver, and is only an afterwards conversion. */
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

    /*! clip geometries when it is set to TRUE */
    int bClipSrc;

    OGRGeometryH hClipSrc;
    
    /*! clip datasource */
    char *pszClipSrcDS;

    /*! select desired geometries using an SQL query */
    char *pszClipSrcSQL;

    /*! selected named layer from the source clip datasource */
    char *pszClipSrcLayer;

    /*! restrict desired geometries based on attribute query */
    char *pszClipSrcWhere;

    OGRGeometryH hClipDst;

    /*! destination clip datasource */
    char *pszClipDstDS;

    /*! select desired geometries using an SQL query */
    char *pszClipDstSQL;

    /*! selected named layer from the destination clip datasource */
    char *pszClipDstLayer;

    /*! restrict desired geometries based on attribute query */
    char *pszClipDstWhere;

    /*! split fields of type StringList, RealList or IntegerList into as many fields
        of type String, Real or Integer as necessary. */
    int bSplitListFields;

    /*! limit the number of subfields created for each split field. */
    int nMaxSplitListSubFields;

    /*! produce one feature for each geometry in any kind of geometry collection in the
        source file */
    int bExplodeCollections;

    /*! uses the specified field to fill the Z coordinates of geometries */
    char *pszZField;

    /*! the list of field indexes to be copied from the source to the destination. The (n)th value
        specified in the list is the index of the field in the target layer definition in which the
        n(th) field of the source layer must be copied. Index count starts at zero. There must be
        exactly as many values in the list as the count of the fields in the source layer. 
        We can use the "identity" option to specify that the fields should be transferred by using
        the same order. This option should be used along with the
        OGR2OGROptions::eAccessMode = ACCESS_APPEND option. */
    char **papszFieldMap;

    /*! force the coordinate dimension to nCoordDim (valid values are 2 or 3). This affects both
        the layer geometry type, and feature geometries. */
    int nCoordDim;

    /*! destination dataset open option (format specific), only valid in update mode */
    char **papszDestOpenOptions;

    /*! If set to TRUE, does not propagate not-nullable constraints to target layer if they exist
        in source layer */
    int bForceNullable;

    /*! If set to TRUE, does not propagate default field values to target layer if they exist in
        source layer */
    int bUnsetDefault;

    /*! to prevent the new default behaviour that consists in, if the output driver has a FID layer
        creation option and we are not in append mode, to preserve the name of the source FID column
        and source feature IDs */
    int bUnsetFid;

    /*! use the FID of the source features instead of letting the output driver to automatically
        assign a new one. If not in append mode, this behaviour becomes the default if the output
        driver has a FID layer creation option. In which case the name of the source FID column will
        be used and source feature IDs will be attempted to be preserved. This behaviour can be
        disabled by option OGR2OGROptions::bUnsetFid */
    int bPreserveFID;

    /*! set it to FALSE to disable copying of metadata from source dataset and layers into target dataset and
        layers, when supported by output driver. */
    int bCopyMD;

    /*! list of metadata key and value to set on the output dataset, when supported by output driver.
        ("META-TAG1=VALUE1","META-TAG2=VALUE2") */
    char **papszMetadataOptions;

    /*! override spatial filter SRS */
    char *pszSpatSRSDef;

    /*! size of the list OGR2OGROptions::pasGCPs */
    int nGCPCount;

    /*! list of ground control points to be added */
    GDAL_GCP *pasGCPs;

    /*! order of polynomial used for warping (1 to 3). The default is to select a polynomial
        order based on the number of GCPs */
    int nTransformOrder;

    /*! spatial query extents, in the SRS of the source layer(s) (or the one specified with
        OGR2OGROptions::pszSpatSRSDef). Only features whose geometry intersects the extents
        will be selected. The geometries will not be clipped unless OGR2OGROptions::bClipSrc
        is specified. */
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
