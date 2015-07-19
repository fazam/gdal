/******************************************************************************
 * $Id$
 *
 * Project:  High Performance Image Reprojector
 * Purpose:  Test program for high performance warper API.
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
 *
 ******************************************************************************
 * Copyright (c) 2002, i3 - information integration and imaging 
 *                          Fort Collin, CO
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

#include "gdalwarper.h"
#include "cpl_string.h"
#include "cpl_error.h"
#include "ogr_spatialref.h"
#include "ogr_api.h"
#include "commonutils.h"
#include "gdal_priv.h"
#include <vector>
#include "gdal_utils.h"

CPL_CVSID("$Id$");

/******************************************************************************/
/*! \page gdalwarp gdalwarp

image reprojection and warping utility

\section gdalwarp_synopsis SYNOPSIS

\htmlonly
Usage: 
\endhtmlonly

\verbatim
gdalwarp [--help-general] [--formats]
    [-s_srs srs_def] [-t_srs srs_def] [-to "NAME=VALUE"]
    [-order n | -tps | -rpc | -geoloc] [-et err_threshold]
    [-refine_gcps tolerance [minimum_gcps]]
    [-te xmin ymin xmax ymax] [-te_srs srs_def]
    [-tr xres yres] [-tap] [-ts width height]
    [-ovr level|AUTO|AUTO-n|NONE] [-wo "NAME=VALUE"] [-ot Byte/Int16/...] [-wt Byte/Int16]
    [-srcnodata "value [value...]"] [-dstnodata "value [value...]"] -dstalpha
    [-r resampling_method] [-wm memory_in_mb] [-multi] [-q]
    [-cutline datasource] [-cl layer] [-cwhere expression]
    [-csql statement] [-cblend dist_in_pixels] [-crop_to_cutline]
    [-of format] [-co "NAME=VALUE"]* [-overwrite]
    [-nomd] [-cvmd meta_conflict_value] [-setci] [-oo NAME=VALUE]*
    [-doo NAME=VALUE]*
    srcfile* dstfile
\endverbatim

\section gdalwarp_description DESCRIPTION

<p>
The gdalwarp utility is an image mosaicing, reprojection and warping
utility. The program can reproject to any supported projection,
and can also apply GCPs stored with the image if the image is "raw"
with control information.

<p>
<dl>
<dt> <b>-s_srs</b> <em>srs def</em>:</dt><dd> source spatial reference set.
The coordinate systems that can be passed are anything supported by the
OGRSpatialReference.SetFromUserInput() call, which includes EPSG PCS and GCSes
(ie. EPSG:4296), PROJ.4 declarations (as above), or the name of a .prf file
containing well known text.</dd>
<dt> <b>-t_srs</b> <em>srs_def</em>:</dt><dd> target spatial reference set.
The coordinate systems that can be passed are anything supported by the
OGRSpatialReference.SetFromUserInput() call, which includes EPSG PCS and GCSes
(ie. EPSG:4296), PROJ.4 declarations (as above), or the name of a .prf file
containing well known text.</dd>
<dt> <b>-to</b> <em>NAME=VALUE</em>:</dt><dd> set a transformer option suitable
to pass to GDALCreateGenImgProjTransformer2(). </dd>
<dt> <b>-order</b> <em>n</em>:</dt><dd> order of polynomial used for warping
(1 to 3). The default is to select a polynomial order based on the number of
GCPs.</dd>
<dt> <b>-tps</b>:</dt><dd>Force use of thin plate spline transformer based on
available GCPs.</dd>
<dt> <b>-rpc</b>:</dt> <dd>Force use of RPCs.</dd>
<dt> <b>-geoloc</b>:</dt><dd>Force use of Geolocation Arrays.</dd>
<dt> <b>-et</b> <em>err_threshold</em>:</dt><dd> error threshold for
transformation approximation (in pixel units - defaults to 0.125, unless, starting
with GDAL 2.1, the RPC_DEM warping option is specified, in which case, an exact
transformer, i.e. err_threshold=0, will be used).</dd>
<dt> <b>-refine_gcps</b> <em>tolerance minimum_gcps</em>:</dt><dd>  (GDAL >= 1.9.0) refines the GCPs by automatically eliminating outliers.
Outliers will be eliminated until minimum_gcps are left or when no outliers can be detected.
The tolerance is passed to adjust when a GCP will be eliminated.
Not that GCP refinement only works with polynomial interpolation.
The tolerance is in pixel units if no projection is available, otherwise it is in SRS units.
If minimum_gcps is not provided, the minimum GCPs according to the polynomial model is used.</dd>
<dt> <b>-te</b> <em>xmin ymin xmax ymax</em>:</dt><dd> set georeferenced
extents of output file to be created (in target SRS by default, or in the SRS
specified with -te_srs)
</dd>
<dt> <b>-te_srs</b> <i>srs_def</i>:</dt><dd> (GDAL >= 2.0) Specifies the SRS in
which to interpret the coordinates given with -te. The <i>srs_def</i> may
be any of the usual GDAL/OGR forms, complete WKT, PROJ.4, EPSG:n or a file
containing the WKT.
This must not be confused with -t_srs which is the target SRS of the output
dataset. -te_srs is a conveniency e.g. when knowing the output coordinates in a
geodetic long/lat SRS, but still wanting a result in a projected coordinate system.
</dd>
<dt> <b>-tr</b> <em>xres yres</em>:</dt><dd> set output file resolution (in
target georeferenced units)</dd>
<dt> <b>-tap</b>:</dt><dd> (GDAL >= 1.8.0) (target aligned pixels) align
the coordinates of the extent of the output file to the values of the -tr,
such that the aligned extent includes the minimum extent.</dd>
<dt> <b>-ts</b> <em>width height</em>:</dt><dd> set output file size in
pixels and lines. If width or height is set to 0, the other dimension will be
guessed from the computed resolution. Note that -ts cannot be used with -tr</dd>
<dt> <b>-ovr</b> <em>level|AUTO|AUTO-n|NONE></em>:</dt><dd>(GDAL >= 2.0) To
specify which overview level of source files must be used. The default choice,
AUTO, will select the overview level whose resolution is the closest to the
target resolution. Specify an integer value (0-based, i.e. 0=1st overview level) 
to select a particular level. Specify AUTO-n where n is an integer greater or
equal to 1, to select an overview level below the AUTO one. Or specify NONE to
force the base resolution to be used.</dd>
<dt> <b>-wo</b> <em>"NAME=VALUE"</em>:</dt><dd> Set a warp option.  The 
GDALWarpOptions::papszWarpOptions docs show all options.  Multiple
 <b>-wo</b> options may be listed.</dd>
<dt> <b>-ot</b> <em>type</em>:</dt><dd> For the output bands to be of the
indicated data type.</dd>
<dt> <b>-wt</b> <em>type</em>:</dt><dd> Working pixel data type. The data type
of pixels in the source image and destination image buffers.</dd>
<dt> <b>-r</b> <em>resampling_method</em>:</dt><dd> Resampling method to use. Available methods are:
<dl>
<dt><b>near</b></dt>: <dd>nearest neighbour resampling (default, fastest
algorithm, worst interpolation quality).</dd>
<dt><b>bilinear</b></dt>: <dd>bilinear resampling.</dd>
<dt><b>cubic</b></dt>: <dd>cubic resampling.</dd>
<dt><b>cubicspline</b></dt>: <dd>cubic spline resampling.</dd>
<dt><b>lanczos</b></dt>: <dd>Lanczos windowed sinc resampling.</dd>
<dt><b>average</b></dt>: <dd>average resampling, computes the average of all non-NODATA contributing pixels. (GDAL >= 1.10.0)</dd>
<dt><b>mode</b></dt>: <dd>mode resampling, selects the value which appears most often of all the sampled points. (GDAL >= 1.10.0)</dd>
<dt><b>max</b></dt>: <dd>maximum resampling, selects the maximum value from all non-NODATA contributing pixels. (GDAL >= 2.0.0)</dd>
<dt><b>min</b></dt>: <dd>minimum resampling, selects the minimum value from all non-NODATA contributing pixels. (GDAL >= 2.0.0)</dd>
<dt><b>med</b></dt>: <dd>median resampling, selects the median value of all non-NODATA contributing pixels. (GDAL >= 2.0.0)</dd>
<dt><b>q1</b></dt>: <dd>first quartile resampling, selects the first quartile value of all non-NODATA contributing pixels. (GDAL >= 2.0.0)</dd>
<dt><b>q3</b></dt>: <dd>third quartile resampling, selects the third quartile value of all non-NODATA contributing pixels. (GDAL >= 2.0.0)</dd>
</dl>
<dt> <b>-srcnodata</b> <em>value [value...]</em>:</dt><dd> Set nodata masking
values for input bands (different values can be supplied for each band).  If 
more than one value is supplied all values should be quoted to keep them 
together as a single operating system argument.  Masked values will not be 
used in interpolation.  Use a value of <tt>None</tt> to ignore intrinsic nodata settings on the source dataset.</dd>
<dt> <b>-dstnodata</b> <em>value [value...]</em>:</dt><dd> Set nodata values
for output bands (different values can be supplied for each band).  If more
than one value is supplied all values should be quoted to keep them together
as a single operating system argument.  New files will be initialized to this
value and if possible the nodata value will be recorded in the output
file. Use a value of <tt>None</tt> to ensure that nodata is not defined (GDAL>=1.11).
If this argument is not used then nodata values will be copied from the source dataset (GDAL>=1.11).</dd>
<dt> <b>-dstalpha</b>:</dt><dd> Create an output alpha band to identify 
nodata (unset/transparent) pixels. </dd>
<dt> <b>-wm</b> <em>memory_in_mb</em>:</dt><dd> Set the amount of memory (in
megabytes) that the warp API is allowed to use for caching.</dd>
<dt> <b>-multi</b>:</dt><dd> Use multithreaded warping implementation.
Multiple threads will be used to process chunks of image and perform
input/output operation simultaneously.</dd>
<dt> <b>-q</b>:</dt><dd> Be quiet.</dd>
<dt> <b>-of</b> <em>format</em>:</dt><dd> Select the output format. The default is GeoTIFF (GTiff). Use the short format name. </dd>
<dt> <b>-co</b> <em>"NAME=VALUE"</em>:</dt><dd> passes a creation option to
the output format driver. Multiple <b>-co</b> options may be listed. See
format specific documentation for legal creation options for each format.
</dd>

<dt> <b>-cutline</b> <em>datasource</em>:</dt><dd>Enable use of a blend cutline from the name OGR support datasource.</dd>
<dt> <b>-cl</b> <em>layername</em>:</dt><dd>Select the named layer from the 
cutline datasource.</dd>
<dt> <b>-cwhere</b> <em>expression</em>:</dt><dd>Restrict desired cutline features based on attribute query.</dd>
<dt> <b>-csql</b> <em>query</em>:</dt><dd>Select cutline features using an SQL query instead of from a layer with -cl.</dd>
<dt> <b>-cblend</b> <em>distance</em>:</dt><dd>Set a blend distance to use to blend over cutlines (in pixels).</dd>
<dt> <b>-crop_to_cutline</b>:</dt><dd>(GDAL >= 1.8.0) Crop the extent of the target dataset to the extent of the cutline.</dd>
<dt> <b>-overwrite</b>:</dt><dd>(GDAL >= 1.8.0) Overwrite the target dataset if it already exists.</dd>
<dt> <b>-nomd</b>:</dt><dd>(GDAL >= 1.10.0) Do not copy metadata. Without this option, dataset and band metadata 
(as well as some band information) will be copied from the first source dataset. 
Items that differ between source datasets will be set to * (see -cvmd option).</dd>
<dt> <b>-cvmd</b> <em>meta_conflict_value</em>:</dt><dd>(GDAL >= 1.10.0) 
Value to set metadata items that conflict between source datasets (default is "*"). Use "" to remove conflicting items. </dd>
<dt> <b>-setci</b>:</dt><dd>(GDAL >= 1.10.0) 
Set the color interpretation of the bands of the target dataset from the source dataset.</dd>
<dt> <b>-oo</b> <em>NAME=VALUE</em>:</dt><dd>(starting with GDAL 2.0) Dataset open option (format specific)</dd>
<dt> <b>-doo</b> <em>NAME=VALUE</em>:</dt><dd>(starting with GDAL 2.1) Output dataset open option (format specific)</dd>

<dt> <em>srcfile</em>:</dt><dd> The source file name(s). </dd>
<dt> <em>dstfile</em>:</dt><dd> The destination file name. </dd>
</dl>

Mosaicing into an existing output file is supported if the output file 
already exists. The spatial extent of the existing file will not
be modified to accommodate new data, so you may have to remove it in that case, or
use the -overwrite option.

Polygon cutlines may be used as a mask to restrict the area of the destination file
that may be updated, including blending.  If the OGR layer containing the cutline
features has no explicit SRS, the cutline features must be in the SRS of the
destination file. When outputing to a not yet existing target dataset,
its extent will be the one of the original raster unless -te or -crop_to_cutline are
specified.

<p>
\section gdalwarp_example EXAMPLE

For instance, an eight bit spot scene stored in GeoTIFF with
control points mapping the corners to lat/long could be warped to a UTM
projection with a command like this:<p>

\verbatim
gdalwarp -t_srs '+proj=utm +zone=11 +datum=WGS84' raw_spot.tif utm11.tif
\endverbatim

For instance, the second channel of an ASTER image stored in HDF with
control points mapping the corners to lat/long could be warped to a UTM
projection with a command like this:<p>

\verbatim
gdalwarp HDF4_SDS:ASTER_L1B:"pg-PR1B0000-2002031402_100_001":2 pg-PR1B0000-2002031402_100_001_2.tif
\endverbatim

<p>
\section gdalwarp_seealso SEE ALSO

\if man
http://trac.osgeo.org/gdal/wiki/UserDocs/GdalWarp :
\else
<a href="http://trac.osgeo.org/gdal/wiki/UserDocs/GdalWarp">
\endif
Wiki page discussing options and behaviours of gdalwarp
\if man
\else
</a>
\endif

\if man
\section gdalwarp_author AUTHORS
Frank Warmerdam <warmerdam@pobox.com>, Silke Reimer <silke@intevation.de>
\endif
*/

/************************************************************************/
/*                               GDALExit()                             */
/*  This function exits and cleans up GDAL and OGR resources            */
/*  Perhaps it should be added to C api and used in all apps?           */
/************************************************************************/

static int GDALExit( int nCode )
{
  const char  *pszDebug = CPLGetConfigOption("CPL_DEBUG",NULL);
  if( pszDebug && (EQUAL(pszDebug,"ON") || EQUAL(pszDebug,"") ) )
  {  
    GDALDumpOpenDatasets( stderr );
    CPLDumpSharedList( NULL );
  }

  GDALDestroyDriverManager();

#ifdef OGR_ENABLED
  OGRCleanupAll();
#endif

  exit( nCode );
}

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage(const char* pszErrorMsg = NULL)

{
    printf( 
        "Usage: gdalwarp [--help-general] [--formats]\n"
        "    [-s_srs srs_def] [-t_srs srs_def] [-to \"NAME=VALUE\"]\n"
        "    [-order n | -tps | -rpc | -geoloc] [-et err_threshold]\n"
        "    [-refine_gcps tolerance [minimum_gcps]]\n"
        "    [-te xmin ymin xmax ymax] [-tr xres yres] [-tap] [-ts width height]\n"
        "    [-ovr level|AUTO|AUTO-n|NONE] [-wo \"NAME=VALUE\"] [-ot Byte/Int16/...] [-wt Byte/Int16]\n"
        "    [-srcnodata \"value [value...]\"] [-dstnodata \"value [value...]\"] -dstalpha\n" 
        "    [-r resampling_method] [-wm memory_in_mb] [-multi] [-q]\n"
        "    [-cutline datasource] [-cl layer] [-cwhere expression]\n"
        "    [-csql statement] [-cblend dist_in_pixels] [-crop_to_cutline]\n"
        "    [-of format] [-co \"NAME=VALUE\"]* [-overwrite]\n"
        "    [-nomd] [-cvmd meta_conflict_value] [-setci] [-oo NAME=VALUE]*\n"
        "    [-doo NAME=VALUE]*\n"
        "    srcfile* dstfile\n"
        "\n"
        "Available resampling methods:\n"
        "    near (default), bilinear, cubic, cubicspline, lanczos, average, mode,  max, min, med, Q1, Q3.\n" );

    if( pszErrorMsg != NULL )
        fprintf(stderr, "\nFAILURE: %s\n", pszErrorMsg);

    GDALExit( 1 );
}

/************************************************************************/
/*                             SanitizeSRS                              */
/************************************************************************/

char *SanitizeSRS( const char *pszUserInput )

{
    OGRSpatialReferenceH hSRS;
    char *pszResult = NULL;

    CPLErrorReset();
    
    hSRS = OSRNewSpatialReference( NULL );
    if( OSRSetFromUserInput( hSRS, pszUserInput ) == OGRERR_NONE )
        OSRExportToWkt( hSRS, &pszResult );
    else
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Translating source or target SRS failed:\n%s",
                  pszUserInput );
        GDALExit( 1 );
    }
    
    OSRDestroySpatialReference( hSRS );

    return pszResult;
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/

#define CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(nExtraArg) \
    do { if (i + nExtraArg >= argc) \
        Usage(CPLSPrintf("%s option requires %d argument(s)", argv[i], nExtraArg)); } while(0)

int main( int argc, char ** argv )

{
    GDALDatasetH hDstDS = NULL;
    GDALDatasetH *pahSrcDS = NULL;
    int nSrcCount = 0;
    int i;
    char **papszSrcFiles;
    char **papszOpenOptions;
    char *pszDstFilename;
    GDALWarpAppOptions *psOptions = GDALWarpAppOptionsNew();

    /* Check that we are running against at least GDAL 1.6 */
    /* Note to developers : if we use newer API, please change the requirement */
    if (atoi(GDALVersionInfo("VERSION_NUM")) < 1600)
    {
        fprintf(stderr, "At least, GDAL >= 1.6.0 is required for this version of %s, "
                "which was compiled against GDAL %s\n", argv[0], GDAL_RELEASE_NAME);
        GDALExit(1);
    }

    EarlySetConfigOptions(argc, argv);

/* -------------------------------------------------------------------- */
/*      Register standard GDAL drivers, and process generic GDAL        */
/*      command options.                                                */
/* -------------------------------------------------------------------- */
    GDALAllRegister();
    argc = GDALGeneralCmdLineProcessor( argc, &argv, 0 );
    if( argc < 1 )
        GDALExit( -argc );

/* -------------------------------------------------------------------- */
/*      Set optimal setting for best performance with huge input VRT.   */
/*      The rationale for 450 is that typical Linux process allow       */
/*      only 1024 file descriptors per process and we need to keep some */
/*      spare for shared libraries, etc. so let's go down to 900.       */
/*      And some datasets may need 2 file descriptors, so divide by 2   */
/*      for security.                                                   */
/* -------------------------------------------------------------------- */
    if( CPLGetConfigOption("GDAL_MAX_DATASET_POOL_SIZE", NULL) == NULL )
    {
        CPLSetConfigOption("GDAL_MAX_DATASET_POOL_SIZE", "450");
    }

/* -------------------------------------------------------------------- */
/*      Parse arguments.                                                */
/* -------------------------------------------------------------------- */
    for( i = 1; i < argc; i++ )
    {
        if( EQUAL(argv[i],"-tps") || EQUAL(argv[i],"-rpc") || EQUAL(argv[i],"-geoloc")  )
        {
            const char* pszMethod = CSLFetchNameValue(psOptions->papszTO, "METHOD");
            if (pszMethod)
                fprintf(stderr, "Warning: only one METHOD can be used. Method %s is already defined.\n",
                        pszMethod);
            const char* pszMAX_GCP_ORDER = CSLFetchNameValue(psOptions->papszTO, "MAX_GCP_ORDER");
            if (pszMAX_GCP_ORDER)
                fprintf(stderr, "Warning: only one METHOD can be used. -order %s option was specified, so it is likely that GCP_POLYNOMIAL was implied.\n",
                        pszMAX_GCP_ORDER);
        }

        if( EQUAL(argv[i], "--utility_version") )
        {
            printf("%s was compiled against GDAL %s and is running against GDAL %s\n",
                   argv[0], GDAL_RELEASE_NAME, GDALVersionInfo("RELEASE_NAME"));
            return 0;
        }
        else if( EQUAL(argv[i],"--help") )
            Usage();
        else if( EQUAL(argv[i],"-co") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszCreateOptions = CSLAddString( psOptions->papszCreateOptions, argv[++i] );
            psOptions->bCreateOutput = TRUE;
        }   
        else if( EQUAL(argv[i],"-wo") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszWarpOptions = CSLAddString( psOptions->papszWarpOptions, argv[++i] );
        }   
        else if( EQUAL(argv[i],"-multi") )
        {
            psOptions->bMulti = TRUE;
        }   
        else if( EQUAL(argv[i],"-q") || EQUAL(argv[i],"-quiet"))
        {
            psOptions->bQuiet = TRUE;
        }   
        else if( EQUAL(argv[i],"-dstalpha") )
        {
            psOptions->bEnableDstAlpha = TRUE;
        }
        else if( EQUAL(argv[i],"-srcalpha") )
        {
            psOptions->bEnableSrcAlpha = TRUE;
        }
        else if( EQUAL(argv[i],"-of") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszFormat = CPLStrdup(argv[++i]);
            psOptions->bFormatExplicitlySet = TRUE;
            psOptions->bCreateOutput = TRUE;
            if( EQUAL(psOptions->pszFormat,"VRT") )
                psOptions->bVRT = TRUE;
        }
        else if( EQUAL(argv[i],"-t_srs") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            char *pszSRS = SanitizeSRS(argv[++i]);
            psOptions->papszTO = CSLSetNameValue( psOptions->papszTO, "DST_SRS", pszSRS );
            CPLFree( pszSRS );
        }
        else if( EQUAL(argv[i],"-s_srs") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            char *pszSRS = SanitizeSRS(argv[++i]);
            psOptions->papszTO = CSLSetNameValue( psOptions->papszTO, "SRC_SRS", pszSRS );
            CPLFree( pszSRS );
        }
        else if( EQUAL(argv[i],"-order") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            const char* pszMethod = CSLFetchNameValue(psOptions->papszTO, "METHOD");
            if (pszMethod)
                fprintf(stderr, "Warning: only one METHOD can be used. Method %s is already defined\n",
                        pszMethod);
            psOptions->papszTO = CSLSetNameValue( psOptions->papszTO, "MAX_GCP_ORDER", argv[++i] );
        }
        else if( EQUAL(argv[i],"-refine_gcps") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszTO = CSLSetNameValue( psOptions->papszTO, "REFINE_TOLERANCE", argv[++i] );
            if(CPLAtof(argv[i]) < 0)
            {
                Usage("The tolerance for -refine_gcps may not be negative.");
            }
            if (i < argc-1 && atoi(argv[i+1]) >= 0 && isdigit(argv[i+1][0]))
            {
                psOptions->papszTO = CSLSetNameValue( psOptions->papszTO, "REFINE_MINIMUM_GCPS", argv[++i] );
            }
            else
            {
                psOptions->papszTO = CSLSetNameValue( psOptions->papszTO, "REFINE_MINIMUM_GCPS", "-1" );
            }
        }
        else if( EQUAL(argv[i],"-tps") )
        {
            psOptions->papszTO = CSLSetNameValue( psOptions->papszTO, "METHOD", "GCP_TPS" );
        }
        else if( EQUAL(argv[i],"-rpc") )
        {
            psOptions->papszTO = CSLSetNameValue( psOptions->papszTO, "METHOD", "RPC" );
        }
        else if( EQUAL(argv[i],"-geoloc") )
        {
            psOptions->papszTO = CSLSetNameValue( psOptions->papszTO, "METHOD", "GEOLOC_ARRAY" );
        }
        else if( EQUAL(argv[i],"-to") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszTO = CSLAddString( psOptions->papszTO, argv[++i] );
        }
        else if( EQUAL(argv[i],"-et") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->dfErrorThreshold = CPLAtofM(argv[++i]);
            psOptions->papszWarpOptions = CSLAddString( psOptions->papszWarpOptions, CPLSPrintf("ERROR_THRESHOLD=%.16g", psOptions->dfErrorThreshold) );
        }
        else if( EQUAL(argv[i],"-wm") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            if( CPLAtofM(argv[i+1]) < 10000 )
                psOptions->dfWarpMemoryLimit = CPLAtofM(argv[i+1]) * 1024 * 1024;
            else
                psOptions->dfWarpMemoryLimit = CPLAtofM(argv[i+1]);
            i++;
        }
        else if( EQUAL(argv[i],"-srcnodata") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszSrcNodata = argv[++i];
        }
        else if( EQUAL(argv[i],"-dstnodata") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszDstNodata = argv[++i];
        }
        else if( EQUAL(argv[i],"-tr") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(2);
            psOptions->dfXRes = CPLAtofM(argv[++i]);
            psOptions->dfYRes = fabs(CPLAtofM(argv[++i]));
            if( psOptions->dfXRes == 0 || psOptions->dfYRes == 0 )
            {
                Usage("Wrong value for -tr parameters.");
            }
            psOptions->bCreateOutput = TRUE;
        }
        else if( EQUAL(argv[i],"-tap") )
        {
            psOptions->bTargetAlignedPixels = TRUE;
        }
        else if( EQUAL(argv[i],"-ot") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            int	iType;
            
            for( iType = 1; iType < GDT_TypeCount; iType++ )
            {
                if( GDALGetDataTypeName((GDALDataType)iType) != NULL
                    && EQUAL(GDALGetDataTypeName((GDALDataType)iType),
                             argv[i+1]) )
                {
                    psOptions->eOutputType = (GDALDataType) iType;
                }
            }

            if( psOptions->eOutputType == GDT_Unknown )
            {
                Usage(CPLSPrintf( "Unknown output pixel type: %s.", argv[i+1] ));
            }
            i++;
            psOptions->bCreateOutput = TRUE;
        }
        else if( EQUAL(argv[i],"-wt") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            int	iType;
            
            for( iType = 1; iType < GDT_TypeCount; iType++ )
            {
                if( GDALGetDataTypeName((GDALDataType)iType) != NULL
                    && EQUAL(GDALGetDataTypeName((GDALDataType)iType),
                             argv[i+1]) )
                {
                    psOptions->eWorkingType = (GDALDataType) iType;
                }
            }

            if( psOptions->eWorkingType == GDT_Unknown )
            {
                Usage(CPLSPrintf( "Unknown working pixel type: %s.", argv[i+1] ));
            }
            i++;
        }
        else if( EQUAL(argv[i],"-ts") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(2);
            psOptions->nForcePixels = atoi(argv[++i]);
            psOptions->nForceLines = atoi(argv[++i]);
            psOptions->bCreateOutput = TRUE;
        }
        else if( EQUAL(argv[i],"-te") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(4);
            psOptions->dfMinX = CPLAtofM(argv[++i]);
            psOptions->dfMinY = CPLAtofM(argv[++i]);
            psOptions->dfMaxX = CPLAtofM(argv[++i]);
            psOptions->dfMaxY = CPLAtofM(argv[++i]);
            psOptions->bCreateOutput = TRUE;
        }
        else if( EQUAL(argv[i],"-te_srs") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            char *pszSRS = SanitizeSRS(argv[++i]);
            psOptions->pszTE_SRS = CPLStrdup(pszSRS);
            CPLFree(pszSRS);
            psOptions->bCreateOutput = TRUE;
        }
        else if( EQUAL(argv[i],"-rn") )
            psOptions->eResampleAlg = GRA_NearestNeighbour;

        else if( EQUAL(argv[i],"-rb") )
            psOptions->eResampleAlg = GRA_Bilinear;

        else if( EQUAL(argv[i],"-rc") )
            psOptions->eResampleAlg = GRA_Cubic;

        else if( EQUAL(argv[i],"-rcs") )
            psOptions->eResampleAlg = GRA_CubicSpline;

        else if( EQUAL(argv[i],"-rl") )
            psOptions->eResampleAlg = GRA_Lanczos;

        else if( EQUAL(argv[i],"-ra") )
            psOptions->eResampleAlg = GRA_Average;

        else if( EQUAL(argv[i],"-rm") )
            psOptions->eResampleAlg = GRA_Mode;

        else if( EQUAL(argv[i],"-r") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            if ( EQUAL(argv[++i], "near") )
                psOptions->eResampleAlg = GRA_NearestNeighbour;
            else if ( EQUAL(argv[i], "bilinear") )
                psOptions->eResampleAlg = GRA_Bilinear;
            else if ( EQUAL(argv[i], "cubic") )
                psOptions->eResampleAlg = GRA_Cubic;
            else if ( EQUAL(argv[i], "cubicspline") )
                psOptions->eResampleAlg = GRA_CubicSpline;
            else if ( EQUAL(argv[i], "lanczos") )
                psOptions->eResampleAlg = GRA_Lanczos;
            else if ( EQUAL(argv[i], "average") )
                psOptions->eResampleAlg = GRA_Average;
            else if ( EQUAL(argv[i], "mode") )
                psOptions->eResampleAlg = GRA_Mode;
            else if ( EQUAL(argv[i], "max") )
                psOptions->eResampleAlg = GRA_Max;
            else if ( EQUAL(argv[i], "min") )
                psOptions->eResampleAlg = GRA_Min;
            else if ( EQUAL(argv[i], "med") )
                psOptions->eResampleAlg = GRA_Med;
            else if ( EQUAL(argv[i], "q1") )
                psOptions->eResampleAlg = GRA_Q1;
            else if ( EQUAL(argv[i], "q3") )
                psOptions->eResampleAlg = GRA_Q3;
            else
            {
                Usage(CPLSPrintf( "Unknown resampling method: \"%s\".", argv[i] ));
            }
        }

        else if( EQUAL(argv[i],"-cutline") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszCutlineDSName = CPLStrdup(argv[++i]);
        }
        else if( EQUAL(argv[i],"-cwhere") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszCWHERE = CPLStrdup(argv[++i]);
        }
        else if( EQUAL(argv[i],"-cl") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszCLayer = CPLStrdup(argv[++i]);
        }
        else if( EQUAL(argv[i],"-csql") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszCSQL = CPLStrdup(argv[++i]);
        }
        else if( EQUAL(argv[i],"-cblend") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszWarpOptions = 
                CSLSetNameValue( psOptions->papszWarpOptions, 
                                 "CUTLINE_BLEND_DIST", argv[++i] );
        }
        else if( EQUAL(argv[i],"-crop_to_cutline")  )
        {
            psOptions->bCropToCutline = TRUE;
            psOptions->bCreateOutput = TRUE;
        }
        else if( EQUAL(argv[i],"-overwrite") )
            psOptions->bOverwrite = TRUE;
        else if( EQUAL(argv[i],"-nomd") )
        {
            psOptions->bCopyMetadata = FALSE;
            psOptions->bCopyBandInfo = FALSE;
        }   
        else if( EQUAL(argv[i],"-cvmd") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszMDConflictValue = CPLStrdup(argv[++i]);
        }
        else if( EQUAL(argv[i],"-setci") )
            psOptions->bSetColorInterpretation = TRUE;
        else if( EQUAL(argv[i], "-oo") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            papszOpenOptions = CSLAddString( papszOpenOptions,
                                                argv[++i] );
        }
        else if( EQUAL(argv[i], "-doo") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszDestOpenOptions = CSLAddString( psOptions->papszDestOpenOptions,
                                                argv[++i] );
        }
        else if( EQUAL(argv[i], "-ovr") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            const char* pszOvLevel = argv[++i];
            if( EQUAL(pszOvLevel, "AUTO") )
                psOptions->nOvLevel = -2;
            else if( EQUALN(pszOvLevel,"AUTO-",5) )
                psOptions->nOvLevel = -2-atoi(pszOvLevel + 5);
            else if( EQUAL(pszOvLevel, "NONE") )
                psOptions->nOvLevel = -1;
            else if( CPLGetValueType(pszOvLevel) == CPL_VALUE_INTEGER )
                psOptions->nOvLevel = atoi(pszOvLevel); 
            else
                Usage(CPLSPrintf("Invalid value '%s' for -ov option", pszOvLevel));
        }
        else if( argv[i][0] == '-' )
            Usage(CPLSPrintf("Unknown option name '%s'", argv[i]));

        else 
            papszSrcFiles = CSLAddString( papszSrcFiles, argv[i] );
    }

/* -------------------------------------------------------------------- */
/*      The last filename in the file list is really our destination    */
/*      file.                                                           */
/* -------------------------------------------------------------------- */
    if( CSLCount(papszSrcFiles) > 1 )
    {
        pszDstFilename = papszSrcFiles[CSLCount(papszSrcFiles)-1];
        papszSrcFiles[CSLCount(papszSrcFiles)-1] = NULL;
    }

    if( pszDstFilename == NULL )
    {
        Usage("No target filename specified.");
    }
        
    if( psOptions->bVRT && CSLCount(papszSrcFiles) > 1 )
    {
        CPLError(CE_Warning, CPLE_AppDefined, "gdalwarp -of VRT just takes into account "
                        "the first source dataset.\nIf all source datasets "
                        "are in the same projection, try making a mosaic of\n"
                        "them with gdalbuildvrt, and use the resulting "
                        "VRT file as the input of\ngdalwarp -of VRT.\n");
    }

    if ( CSLCount(papszSrcFiles) == 1 &&
         strcmp(papszSrcFiles[0], pszDstFilename) == 0 && psOptions->bOverwrite)
    {
        CPLError(CE_Failure, CPLE_IllegalArg, "Source and destination datasets must be different.\n");
        GDALExit(1);
    }
     
/* -------------------------------------------------------------------- */
/*      Open Source files.                                              */
/* -------------------------------------------------------------------- */
    for(i = 0; papszSrcFiles[i] != NULL; i++)
    {
        nSrcCount++;
        pahSrcDS = (GDALDatasetH *) CPLRealloc(pahSrcDS, sizeof(GDALDatasetH) * nSrcCount);
        pahSrcDS[nSrcCount-1] = GDALOpenEx( papszSrcFiles[i], GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR, NULL,
                                            (const char* const* )papszOpenOptions, NULL );
    
        if( pahSrcDS[nSrcCount-1] == NULL )
            GDALExit(2);
    }

    hDstDS = GDALWarp(pszDstFilename, NULL, nSrcCount, pahSrcDS, psOptions, NULL);

    GDALWarpAppOptionsFree(psOptions);

    CSLDestroy(papszSrcFiles);
    CPLFree(pszDstFilename);

    GDALClose( hDstDS );

    GDALDumpOpenDatasets( stderr );

    GDALDestroyDriverManager();

#ifdef OGR_ENABLED
    OGRCleanupAll();
#endif

}
