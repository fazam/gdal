/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Utilities
 * Purpose:  Commandline application to list info about a file.
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

#include "gdal.h"
#include "gdal_alg.h"
#include "ogr_srs_api.h"
#include "cpl_string.h"
#include "cpl_conv.h"
#include "cpl_multiproc.h"
#include "commonutils.h"
#include "json.h"

CPL_CVSID("$Id$");

static int
GDALInfoReportCorner( GDALDatasetH hDataset,
                      OGRCoordinateTransformationH hTransform,
                      const char * corner_name,
                      double x, double y );

static void
GDALInfoReportMetadata( GDALMajorObjectH hObject,
                        int bListMDD,
                        int bShowMetadata,
                        char **papszExtraMDDomains,
                        int bIsBand );

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

void Usage(const char* pszErrorMsg)

{
    printf( "Usage: gdalinfo [--help-general] [-mm] [-stats] [-hist] [-nogcp] [-nomd]\n"
            "                [-norat] [-noct] [-nofl] [-checksum] [-proj4]\n"
            "                [-listmdd] [-mdd domain|`all`]*\n"
            "                [-sd subdataset] [-oo NAME=VALUE]* datasetname\n" );

    if( pszErrorMsg != NULL )
        fprintf(stderr, "\nFAILURE: %s\n", pszErrorMsg);

    exit( 1 );
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/

#define CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(nExtraArg) \
    do { if (i + nExtraArg >= argc) \
        Usage(CPLSPrintf("%s option requires %d argument(s)", argv[i], nExtraArg)); } while(0)

int main( int argc, char ** argv )

{
    GDALDatasetH	hDataset = NULL;
    GDALRasterBandH	hBand = NULL;
    int			i, iBand;
    double		adfGeoTransform[6];
    GDALDriverH		hDriver;
    int                 bComputeMinMax = FALSE, bSample = FALSE;
    int                 bShowGCPs = TRUE, bShowMetadata = TRUE, bShowRAT=TRUE;
    int                 bStats = FALSE, bApproxStats = TRUE;
    int                 bShowColorTable = TRUE, bComputeChecksum = FALSE;
    int                 bReportHistograms = FALSE;
    int                 bReportProj4 = FALSE;
    int                 nSubdataset = -1;
    const char          *pszFilename = NULL;
    char              **papszExtraMDDomains = NULL, **papszFileList;
    int                 bListMDD = FALSE;
    const char  *pszProjection = NULL;
    OGRCoordinateTransformationH hTransform = NULL;
    int             bShowFileList = TRUE;
    char              **papszOpenOptions = NULL;

    int bJson = FALSE;
    json_object *poJsonObject = json_object_new_object();
    json_object *poDescription, *poDriverShortName, *poDriverLongName,
                *poFiles, *poSize, *poCoordinateSystem, *poGeoTransform,
                *poGCPs, *poCornerCoordinates, *poBands, *poBand, *poHistogram,
                *poColorTable;

    /* Check that we are running against at least GDAL 1.5 */
    /* Note to developers : if we use newer API, please change the requirement */
    if (atoi(GDALVersionInfo("VERSION_NUM")) < 1500)
    {
        fprintf(stderr, "At least, GDAL >= 1.5.0 is required for this version of %s, "
                "which was compiled against GDAL %s\n", argv[0], GDAL_RELEASE_NAME);
        exit(1);
    }

    EarlySetConfigOptions(argc, argv);

    GDALAllRegister();

    argc = GDALGeneralCmdLineProcessor( argc, &argv, 0 );
    if( argc < 1 )
        exit( -argc );

/* -------------------------------------------------------------------- */
/*      Parse arguments.                                                */
/* -------------------------------------------------------------------- */
    for( i = 1; i < argc; i++ )
    {
        if( EQUAL(argv[i], "--utility_version") )
        {
            printf("%s was compiled against GDAL %s and is running against GDAL %s\n",
                   argv[0], GDAL_RELEASE_NAME, GDALVersionInfo("RELEASE_NAME"));
            return 0;
        }
        else if( EQUAL(argv[i],"--help") )
            Usage(NULL);
        else if( EQUAL(argv[i],"-json") )
            bJson = TRUE;
        else if( EQUAL(argv[i], "-mm") )
            bComputeMinMax = TRUE;
        else if( EQUAL(argv[i], "-hist") )
            bReportHistograms = TRUE;
        else if( EQUAL(argv[i], "-proj4") )
            bReportProj4 = TRUE;
        else if( EQUAL(argv[i], "-stats") )
        {
            bStats = TRUE;
            bApproxStats = FALSE;
        }
        else if( EQUAL(argv[i], "-approx_stats") )
        {
            bStats = TRUE;
            bApproxStats = TRUE;
        }
        else if( EQUAL(argv[i], "-sample") )
            bSample = TRUE;
        else if( EQUAL(argv[i], "-checksum") )
            bComputeChecksum = TRUE;
        else if( EQUAL(argv[i], "-nogcp") )
            bShowGCPs = FALSE;
        else if( EQUAL(argv[i], "-nomd") )
            bShowMetadata = FALSE;
        else if( EQUAL(argv[i], "-norat") )
            bShowRAT = FALSE;
        else if( EQUAL(argv[i], "-noct") )
            bShowColorTable = FALSE;
        else if( EQUAL(argv[i], "-listmdd") )
            bListMDD = TRUE;
        else if( EQUAL(argv[i], "-mdd") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            papszExtraMDDomains = CSLAddString( papszExtraMDDomains,
                                                argv[++i] );
        }
        else if( EQUAL(argv[i], "-oo") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            papszOpenOptions = CSLAddString( papszOpenOptions,
                                                argv[++i] );
        }
        else if( EQUAL(argv[i], "-nofl") )
            bShowFileList = FALSE;
        else if( EQUAL(argv[i], "-sd") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            nSubdataset = atoi(argv[++i]);
        }
        else if( argv[i][0] == '-' )
            Usage(CPLSPrintf("Unknown option name '%s'", argv[i]));
        else if( pszFilename == NULL )
            pszFilename = argv[i];
        else
            Usage("Too many command options.");
    }

    if( pszFilename == NULL )
        Usage("No datasource specified.");

/* -------------------------------------------------------------------- */
/*      Open dataset.                                                   */
/* -------------------------------------------------------------------- */
    hDataset = GDALOpenEx( pszFilename, GDAL_OF_READONLY | GDAL_OF_RASTER, NULL,
                           (const char* const* )papszOpenOptions, NULL );

    if( hDataset == NULL )
    {
        fprintf( stderr,
                 "gdalinfo failed - unable to open '%s'.\n",
                 pszFilename );

/* -------------------------------------------------------------------- */
/*      If argument is a VSIFILE, then print its contents               */
/* -------------------------------------------------------------------- */
        if ( strncmp( pszFilename, "/vsizip/", 8 ) == 0 ||
             strncmp( pszFilename, "/vsitar/", 8 ) == 0 )
        {
            papszFileList = VSIReadDirRecursive( pszFilename );
            if ( papszFileList )
            {
                int nCount = CSLCount( papszFileList );
                fprintf( stdout,
                         "Unable to open source `%s' directly.\n"
                         "The archive contains %d files:\n",
                         pszFilename, nCount );
                for ( i = 0; i < nCount; i++ )
                {
                    fprintf( stdout, "       %s/%s\n", pszFilename, papszFileList[i] );
                }
                CSLDestroy( papszFileList );
                papszFileList = NULL;
            }
        }

        CSLDestroy( argv );
        CSLDestroy( papszExtraMDDomains );
        CSLDestroy( papszOpenOptions );

        GDALDumpOpenDatasets( stderr );

        GDALDestroyDriverManager();

        CPLDumpSharedList( NULL );

        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Read specified subdataset if requested.                         */
/* -------------------------------------------------------------------- */
    if ( nSubdataset > 0 )
    {
        char **papszSubdatasets = GDALGetMetadata( hDataset, "SUBDATASETS" );
        int nSubdatasets = CSLCount( papszSubdatasets );

        if ( nSubdatasets > 0 && nSubdataset <= nSubdatasets )
        {
            char szKeyName[1024];
            char *pszSubdatasetName;

            snprintf( szKeyName, sizeof(szKeyName),
                      "SUBDATASET_%d_NAME", nSubdataset );
            szKeyName[sizeof(szKeyName) - 1] = '\0';
            pszSubdatasetName =
                CPLStrdup( CSLFetchNameValue( papszSubdatasets, szKeyName ) );
            GDALClose( hDataset );
            hDataset = GDALOpen( pszSubdatasetName, GA_ReadOnly );
            CPLFree( pszSubdatasetName );
        }
        else
        {
            fprintf( stderr,
                     "gdalinfo warning: subdataset %d of %d requested. "
                     "Reading the main dataset.\n",
                     nSubdataset, nSubdatasets );

        }
    }

/* -------------------------------------------------------------------- */
/*      Report general info.                                            */
/* -------------------------------------------------------------------- */
    hDriver = GDALGetDatasetDriver( hDataset );
    if(bJson)
    {
        poDescription = json_object_new_string(GDALGetDescription(hDataset));
        poDriverShortName = json_object_new_string(GDALGetDriverShortName(hDriver));
        poDriverLongName = json_object_new_string(GDALGetDriverLongName(hDriver));
        json_object_object_add(poJsonObject, "description", poDescription);
        json_object_object_add(poJsonObject, "driverShortName", poDriverShortName);
        json_object_object_add(poJsonObject, "driverLongName", poDriverLongName);
    }
    else
    {
        printf( "Driver: %s/%s\n",
                GDALGetDriverShortName( hDriver ),
                GDALGetDriverLongName( hDriver ) );

    }

    papszFileList = GDALGetFileList( hDataset );
    if( CSLCount(papszFileList) == 0 )
    {
        if(bJson)
        {

        }
        else
            printf( "Files: none associated\n" );
    }
    else
    {
        if(bJson)
        {
            json_object *poFile;
            poFiles = json_object_new_array();
            for(i = 0; papszFileList[i] != NULL; i++)
            {
                poFile = json_object_new_string(papszFileList[i]);
                json_object_array_add(poFiles, poFile);
            }
            json_object_object_add(poJsonObject, "files", poFiles);
        }
        else
        {
            printf( "Files: %s\n", papszFileList[0] );
            if( bShowFileList )
            {
                for( i = 1; papszFileList[i] != NULL; i++ )
                    printf( "       %s\n", papszFileList[i] );
            }
        }

    }
    CSLDestroy( papszFileList );

    if(bJson)
    {
        json_object *poSizeX, *poSizeY;
        poSize = json_object_new_array();
        poSizeX = json_object_new_int(GDALGetRasterXSize(hDataset));
        poSizeY = json_object_new_int(GDALGetRasterYSize(hDataset));
        json_object_array_add(poSize, poSizeX);
        json_object_array_add(poSize, poSizeY);
        json_object_object_add(poJsonObject, "size", poSize);
    }
    else
        printf( "Size is %d, %d\n",
            GDALGetRasterXSize( hDataset ),
            GDALGetRasterYSize( hDataset ) );

/* -------------------------------------------------------------------- */
/*      Report projection.                                              */
/* -------------------------------------------------------------------- */
    if( GDALGetProjectionRef( hDataset ) != NULL )
    {
        OGRSpatialReferenceH  hSRS;
        char		      *pszProjection;

        pszProjection = (char *) GDALGetProjectionRef( hDataset );

        hSRS = OSRNewSpatialReference(NULL);
        if( OSRImportFromWkt( hSRS, &pszProjection ) == CE_None )
        {
            char	*pszPrettyWkt = NULL;

            OSRExportToPrettyWkt( hSRS, &pszPrettyWkt, FALSE );
            if(bJson)
            {
                json_object *wkt;
                poCoordinateSystem = json_object_new_object();
                wkt = json_object_new_string(pszPrettyWkt);
                json_object_object_add(poCoordinateSystem, "wkt", wkt);
            }
            else
                printf( "Coordinate System is:\n%s\n", pszPrettyWkt );
            CPLFree( pszPrettyWkt );
        }
        else
        {
            if(bJson)
            {
                json_object *wkt;
                poCoordinateSystem = json_object_new_object();
                wkt = json_object_new_string(GDALGetProjectionRef(hDataset));
                json_object_object_add(poCoordinateSystem, "wkt", wkt);
            }
            else
                printf( "Coordinate System is `%s'\n",
                    GDALGetProjectionRef( hDataset ) );

        }

        if ( bReportProj4 )
        {
            char *pszProj4 = NULL;
            OSRExportToProj4( hSRS, &pszProj4 );
            if(bJson)
            {
                json_object *proj;
                proj = json_object_new_string(pszProj4);
                json_object_object_add(poCoordinateSystem, "proj", proj);
            }
            else
                printf("PROJ.4 string is:\n\'%s\'\n",pszProj4);
            CPLFree( pszProj4 );
        }

        if(bJson)
            json_object_object_add(poJsonObject, "coordinateSystem", poCoordinateSystem);

        OSRDestroySpatialReference( hSRS );
    }

/* -------------------------------------------------------------------- */
/*      Report Geotransform.                                            */
/* -------------------------------------------------------------------- */
    if( GDALGetGeoTransform( hDataset, adfGeoTransform ) == CE_None )
    {
        if(bJson)
        {
            json_object *poAdfGeoTransform;
            poGeoTransform = json_object_new_array();
            for(i = 0; i < 6; i++)
            {
                poAdfGeoTransform = json_object_new_double(adfGeoTransform[i]);
                json_object_array_add(poGeoTransform, poAdfGeoTransform);
            }
            json_object_object_add(poJsonObject, "geoTransform", poGeoTransform);
        }
        else
        {
            if( adfGeoTransform[2] == 0.0 && adfGeoTransform[4] == 0.0 )
            {
                CPLprintf( "Origin = (%.15f,%.15f)\n",
                        adfGeoTransform[0], adfGeoTransform[3] );

                CPLprintf( "Pixel Size = (%.15f,%.15f)\n",
                        adfGeoTransform[1], adfGeoTransform[5] );
            }
            else
                CPLprintf( "GeoTransform =\n"
                        "  %.16g, %.16g, %.16g\n"
                        "  %.16g, %.16g, %.16g\n",
                        adfGeoTransform[0],
                        adfGeoTransform[1],
                        adfGeoTransform[2],
                        adfGeoTransform[3],
                        adfGeoTransform[4],
                        adfGeoTransform[5] );
        }

    }

/* -------------------------------------------------------------------- */
/*      Report GCPs.                                                    */
/* -------------------------------------------------------------------- */
    if( bShowGCPs && GDALGetGCPCount( hDataset ) > 0 )
    {
        json_object *poGCPList;
        if(bJson)
            poGCPs = json_object_new_object();
        if (GDALGetGCPProjection(hDataset) != NULL)
        {
            json_object *poGCPCoordinateSystem;
            OGRSpatialReferenceH  hSRS;
            char		      *pszProjection;

            pszProjection = (char *) GDALGetGCPProjection( hDataset );

            hSRS = OSRNewSpatialReference(NULL);
            if( OSRImportFromWkt( hSRS, &pszProjection ) == CE_None )
            {
                char	*pszPrettyWkt = NULL;

                OSRExportToPrettyWkt( hSRS, &pszPrettyWkt, FALSE );
                if(bJson)
                {
                    json_object *poWkt;
                    poGCPCoordinateSystem = json_object_new_object();
                    poWkt = json_object_new_string(pszPrettyWkt);
                    json_object_object_add(poGCPCoordinateSystem, "wkt", poWkt);
                }
                else
                    printf( "GCP Projection = \n%s\n", pszPrettyWkt );
                CPLFree( pszPrettyWkt );
            }
            else
            {
                if(bJson)
                {
                    json_object *poWkt;
                    poGCPCoordinateSystem = json_object_new_object();
                    poWkt = json_object_new_string(GDALGetGCPProjection(hDataset));
                    json_object_object_add(poGCPCoordinateSystem, "wkt", poWkt);
                }
                else
                    printf( "GCP Projection = %s\n",
                        GDALGetGCPProjection( hDataset ) );

            }
            if(bJson)
                json_object_object_add(poGCPs, "coordinateSystem", poGCPCoordinateSystem);
            OSRDestroySpatialReference( hSRS );
        }

        if(bJson)
            poGCPList = json_object_new_array();
        for( i = 0; i < GDALGetGCPCount(hDataset); i++ )
        {
            const GDAL_GCP	*psGCP;

            psGCP = GDALGetGCPs( hDataset ) + i;
            if(bJson)
            {
                json_object *poGCP, *poId, *poInfo, *poPixel, *poLine, *poX, *poY, *poZ;
                poGCP = json_object_new_object();
                poId = json_object_new_string(psGCP->pszId);
                poInfo = json_object_new_string(psGCP->pszInfo);
                poPixel = json_object_new_double(psGCP->dfGCPPixel);
                poLine = json_object_new_double(psGCP->dfGCPLine);
                poX = json_object_new_double(psGCP->dfGCPX);
                poY = json_object_new_double(psGCP->dfGCPY);
                poZ = json_object_new_double(psGCP->dfGCPZ);
                json_object_object_add(poGCP, "id", poId);
                json_object_object_add(poGCP, "info", poInfo);
                json_object_object_add(poGCP, "pixel", poPixel);
                json_object_object_add(poGCP, "line", poLine);
                json_object_object_add(poGCP, "x", poX);
                json_object_object_add(poGCP, "y", poY);
                json_object_object_add(poGCP, "z", poZ);
                json_object_array_add(poGCPList, poGCP);
            }
            else
                CPLprintf( "GCP[%3d]: Id=%s, Info=%s\n"
                    "          (%.15g,%.15g) -> (%.15g,%.15g,%.15g)\n",
                    i, psGCP->pszId, psGCP->pszInfo,
                    psGCP->dfGCPPixel, psGCP->dfGCPLine,
                    psGCP->dfGCPX, psGCP->dfGCPY, psGCP->dfGCPZ );
        }
        if(bJson)
        {
            json_object_object_add(poGCPs, "gcpList", poGCPList);
            json_object_object_add(poJsonObject, "gcps", poGCPs);
        }
    }

/* -------------------------------------------------------------------- */
/*      Report metadata.                                                */
/* -------------------------------------------------------------------- */

    GDALInfoReportMetadata( hDataset, bListMDD, bShowMetadata, papszExtraMDDomains, FALSE);


/* -------------------------------------------------------------------- */
/*      Setup projected to lat/long transform if appropriate.           */
/* -------------------------------------------------------------------- */
    if( GDALGetGeoTransform( hDataset, adfGeoTransform ) == CE_None )
        pszProjection = GDALGetProjectionRef(hDataset);

    if( pszProjection != NULL && strlen(pszProjection) > 0 )
    {
        OGRSpatialReferenceH hProj, hLatLong = NULL;

        hProj = OSRNewSpatialReference( pszProjection );
        if( hProj != NULL )
            hLatLong = OSRCloneGeogCS( hProj );

        if( hLatLong != NULL )
        {
            CPLPushErrorHandler( CPLQuietErrorHandler );
            hTransform = OCTNewCoordinateTransformation( hProj, hLatLong );
            CPLPopErrorHandler();

            OSRDestroySpatialReference( hLatLong );
        }

        if( hProj != NULL )
            OSRDestroySpatialReference( hProj );
    }

/* -------------------------------------------------------------------- */
/*      Report corners.                                                 */
/* -------------------------------------------------------------------- */
    if(bJson)
    {

    }
    else
    {
        printf( "Corner Coordinates:\n" );
        GDALInfoReportCorner( hDataset, hTransform, "Upper Left",
                              0.0, 0.0 );
        GDALInfoReportCorner( hDataset, hTransform, "Lower Left",
                              0.0, GDALGetRasterYSize(hDataset));
        GDALInfoReportCorner( hDataset, hTransform, "Upper Right",
                              GDALGetRasterXSize(hDataset), 0.0 );
        GDALInfoReportCorner( hDataset, hTransform, "Lower Right",
                              GDALGetRasterXSize(hDataset),
                              GDALGetRasterYSize(hDataset) );
        GDALInfoReportCorner( hDataset, hTransform, "Center",
                              GDALGetRasterXSize(hDataset)/2.0,
                              GDALGetRasterYSize(hDataset)/2.0 );

    }

    if( hTransform != NULL )
    {
        OCTDestroyCoordinateTransformation( hTransform );
        hTransform = NULL;
    }

    if(bJson)
        poBands = json_object_new_array();
/* ==================================================================== */
/*      Loop over bands.                                                */
/* ==================================================================== */
    for( iBand = 0; iBand < GDALGetRasterCount( hDataset ); iBand++ )
    {
        double      dfMin, dfMax, adfCMinMax[2], dfNoData;
        int         bGotMin, bGotMax, bGotNodata, bSuccess;
        int         nBlockXSize, nBlockYSize, nMaskFlags;
        double      dfMean, dfStdDev;
        GDALColorTableH	hTable;
        CPLErr      eErr;

        hBand = GDALGetRasterBand( hDataset, iBand+1 );

        if( bSample )
        {
            float afSample[10000];
            int   nCount;

            nCount = GDALGetRandomRasterSample( hBand, 10000, afSample );
            if(bJson)
            {

            }
            else
                printf( "Got %d samples.\n", nCount );
        }

        GDALGetBlockSize( hBand, &nBlockXSize, &nBlockYSize );
        if(bJson)
        {
            json_object *poBandNumber, *poBlock, *poType, *poColorInterp;
            poBand = json_object_new_object();
            poBandNumber = json_object_new_int(iBand+1);
            poBlock = json_object_new_array();
            json_object_array_add(poBlock, json_object_new_int(nBlockXSize));
            json_object_array_add(poBlock, json_object_new_int(nBlockYSize));
            poType = json_object_new_string(GDALGetDataTypeName(GDALGetRasterDataType(hBand)));
            poColorInterp = json_object_new_string(GDALGetColorInterpretationName(
                GDALGetRasterColorInterpretation(hBand)));
            json_object_object_add(poBand, "band", poBandNumber);
            json_object_object_add(poBand, "block", poBlock);
            json_object_object_add(poBand, "type", poType);
            json_object_object_add(poBand, "colorInterp", poColorInterp);
        }
        else
            printf( "Band %d Block=%dx%d Type=%s, ColorInterp=%s\n", iBand+1,
                nBlockXSize, nBlockYSize,
                GDALGetDataTypeName(
                    GDALGetRasterDataType(hBand)),
                GDALGetColorInterpretationName(
                    GDALGetRasterColorInterpretation(hBand)) );

        if( GDALGetDescription( hBand ) != NULL
            && strlen(GDALGetDescription( hBand )) > 0 )
        {
            if(bJson)
            {
                json_object *poBandDescription;
                poBandDescription = json_object_new_string(GDALGetDescription(hBand));
                json_object_object_add(poBand, "description", poBandDescription);
            }
            else
                printf( "  Description = %s\n", GDALGetDescription(hBand) );
        }

        dfMin = GDALGetRasterMinimum( hBand, &bGotMin );
        dfMax = GDALGetRasterMaximum( hBand, &bGotMax );
        if( bGotMin || bGotMax || bComputeMinMax )
        {
            if(!bJson)
                printf( "  " );
            if( bGotMin )
            {
                if(bJson)
                {
                    json_object *poMin;
                    poMin = json_object_new_double(dfMin);
                    json_object_object_add(poBand, "min", poMin);
                }
                else
                    CPLprintf( "Min=%.3f ", dfMin );
            }
            if( bGotMax )
            {
                if(bJson)
                {
                    json_object *poMax;
                    poMax = json_object_new_double(dfMax);
                    json_object_object_add(poBand, "max", poMax);
                }
                else
                    CPLprintf( "Max=%.3f ", dfMax );
            }

            if( bComputeMinMax )
            {
                CPLErrorReset();
                GDALComputeRasterMinMax( hBand, FALSE, adfCMinMax );
                if (CPLGetLastErrorType() == CE_None)
                {
                    if(bJson)
                    {
                        json_object *poComputedMin, *poComputedMax;
                        poComputedMin = json_object_new_double(adfCMinMax[0]);
                        poComputedMax = json_object_new_double(adfCMinMax[1]);
                        json_object_object_add(poBand, "computedMin", poComputedMin);
                        json_object_object_add(poBand, "computedMax", poComputedMax);
                    }
                    else
                        CPLprintf( "  Computed Min/Max=%.3f,%.3f",
                          adfCMinMax[0], adfCMinMax[1] );
                }
            }
            if(!bJson)
                printf( "\n" );
        }

        eErr = GDALGetRasterStatistics( hBand, bApproxStats, bStats,
                                        &dfMin, &dfMax, &dfMean, &dfStdDev );
        if( eErr == CE_None )
        {
            if(bJson)
            {
                json_object *poMinimum, *poMaximum, *poMean, *poStdDev;
                poMinimum = json_object_new_double(dfMin);
                poMaximum = json_object_new_double(dfMax);
                poMean = json_object_new_double(dfMean);
                poStdDev = json_object_new_double(dfStdDev);
                json_object_object_add(poBand, "minimum", poMinimum);
                json_object_object_add(poBand, "maximum", poMaximum);
                json_object_object_add(poBand, "mean", poMean);
                json_object_object_add(poBand, "stdDev", poStdDev);
            }
            else
                CPLprintf( "  Minimum=%.3f, Maximum=%.3f, Mean=%.3f, StdDev=%.3f\n",
                    dfMin, dfMax, dfMean, dfStdDev );
        }

        if( bReportHistograms )
        {
            int nBucketCount, *panHistogram = NULL;

            eErr = GDALGetDefaultHistogram( hBand, &dfMin, &dfMax,
                                            &nBucketCount, &panHistogram,
                                            TRUE, GDALTermProgress, NULL );
            if( eErr == CE_None )
            {
                int iBucket;

                if(bJson)
                {
                    json_object *poCount, *poMin, *poMax;
                    poHistogram = json_object_new_object();
                    poCount = json_object_new_int(nBucketCount);
                    poMin = json_object_new_double(dfMin);
                    poMax = json_object_new_double(dfMax);
                    json_object_object_add(poHistogram, "count", poCount);
                    json_object_object_add(poHistogram, "min", poMin);
                    json_object_object_add(poHistogram, "max", poMax);
                }
                else
                    printf( "  %d buckets from %g to %g:\n  ",
                        nBucketCount, dfMin, dfMax );
                json_object *poBuckets;
                if(bJson)
                    poBuckets = json_object_new_array();
                for( iBucket = 0; iBucket < nBucketCount; iBucket++ )
                {
                    if(bJson)
                    {
                        json_object *poBucket;
                        poBucket = json_object_new_int(panHistogram[iBucket]);
                        json_object_array_add(poBuckets, poBucket);
                    }
                    else
                        printf( "%d ", panHistogram[iBucket] );
                }
                if(bJson)
                {
                    json_object_object_add(poHistogram, "buckets", poBuckets);
                    json_object_object_add(poBand, "histogram", poHistogram);
                }
                else
                    printf( "\n" );
                CPLFree( panHistogram );
            }
        }

        if ( bComputeChecksum)
        {
            if(bJson)
            {
                json_object *poChecksum;
                poChecksum = json_object_new_int(GDALChecksumImage(hBand, 0, 0,
                    GDALGetRasterXSize(hDataset),GDALGetRasterYSize(hDataset)));
                json_object_object_add(poBand, "checksum", poChecksum);
            }
            else
                printf( "  Checksum=%d\n",
                    GDALChecksumImage(hBand, 0, 0,
                                      GDALGetRasterXSize(hDataset),
                                      GDALGetRasterYSize(hDataset)));
        }

        dfNoData = GDALGetRasterNoDataValue( hBand, &bGotNodata );
        if( bGotNodata )
        {
            if (CPLIsNan(dfNoData))
                printf( "  NoData Value=nan\n" );
            else
                CPLprintf( "  NoData Value=%.18g\n", dfNoData );
        }

        if( GDALGetOverviewCount(hBand) > 0 )
        {
            int		iOverview;

            printf( "  Overviews: " );
            for( iOverview = 0;
                 iOverview < GDALGetOverviewCount(hBand);
                 iOverview++ )
            {
                GDALRasterBandH	hOverview;
                const char *pszResampling = NULL;

                if( iOverview != 0 )
                    printf( ", " );

                hOverview = GDALGetOverview( hBand, iOverview );
                if (hOverview != NULL)
                {
                    printf( "%dx%d",
                            GDALGetRasterBandXSize( hOverview ),
                            GDALGetRasterBandYSize( hOverview ) );

                    pszResampling =
                        GDALGetMetadataItem( hOverview, "RESAMPLING", "" );

                    if( pszResampling != NULL
                        && EQUALN(pszResampling,"AVERAGE_BIT2",12) )
                        printf( "*" );
                }
                else
                    printf( "(null)" );
            }
            printf( "\n" );

            if ( bComputeChecksum)
            {
                printf( "  Overviews checksum: " );
                for( iOverview = 0;
                    iOverview < GDALGetOverviewCount(hBand);
                    iOverview++ )
                {
                    GDALRasterBandH	hOverview;

                    if( iOverview != 0 )
                        printf( ", " );

                    hOverview = GDALGetOverview( hBand, iOverview );
                    if (hOverview)
                        printf( "%d",
                                GDALChecksumImage(hOverview, 0, 0,
                                        GDALGetRasterBandXSize(hOverview),
                                        GDALGetRasterBandYSize(hOverview)));
                    else
                        printf( "(null)" );
                }
                printf( "\n" );
            }
        }

        if( GDALHasArbitraryOverviews( hBand ) )
        {
            printf( "  Overviews: arbitrary\n" );
        }

        nMaskFlags = GDALGetMaskFlags( hBand );
        if( (nMaskFlags & (GMF_NODATA|GMF_ALL_VALID)) == 0 )
        {
            GDALRasterBandH hMaskBand = GDALGetMaskBand(hBand) ;

            printf( "  Mask Flags: " );
            if( nMaskFlags & GMF_PER_DATASET )
                printf( "PER_DATASET " );
            if( nMaskFlags & GMF_ALPHA )
                printf( "ALPHA " );
            if( nMaskFlags & GMF_NODATA )
                printf( "NODATA " );
            if( nMaskFlags & GMF_ALL_VALID )
                printf( "ALL_VALID " );
            printf( "\n" );

            if( hMaskBand != NULL &&
                GDALGetOverviewCount(hMaskBand) > 0 )
            {
                int		iOverview;

                printf( "  Overviews of mask band: " );
                for( iOverview = 0;
                     iOverview < GDALGetOverviewCount(hMaskBand);
                     iOverview++ )
                {
                    GDALRasterBandH	hOverview;

                    if( iOverview != 0 )
                        printf( ", " );

                    hOverview = GDALGetOverview( hMaskBand, iOverview );
                    printf( "%dx%d",
                            GDALGetRasterBandXSize( hOverview ),
                            GDALGetRasterBandYSize( hOverview ) );
                }
                printf( "\n" );
            }
        }

        if( strlen(GDALGetRasterUnitType(hBand)) > 0 )
        {
            if(bJson)
            {
                json_object *poUnit;
                poUnit = json_object_new_string(GDALGetRasterUnitType(hBand));
                json_object_object_add(poBand, "unit", poUnit);
            }
            else
                printf( "  Unit Type: %s\n", GDALGetRasterUnitType(hBand) );
        }

        if( GDALGetRasterCategoryNames(hBand) != NULL )
        {
            char **papszCategories = GDALGetRasterCategoryNames(hBand);
            int i;
            if(bJson)
            {

            }
            else
            {
                printf( "  Categories:\n" );
                for( i = 0; papszCategories[i] != NULL; i++ )
                    printf( "    %3d: %s\n", i, papszCategories[i] );
            }

        }

        if( GDALGetRasterScale( hBand, &bSuccess ) != 1.0
            || GDALGetRasterOffset( hBand, &bSuccess ) != 0.0 )
            if(bJson)
            {
                json_object *poOffset, *poScale;
                poOffset = json_object_new_double(GDALGetRasterOffset(hBand, &bSuccess));
                poScale = json_object_new_double(GDALGetRasterScale(hBand, &bSuccess));
                json_object_object_add(poBand, "offset", poOffset);
                json_object_object_add(poBand, "scale", poScale);
            }
            else
                CPLprintf( "  Offset: %.15g,   Scale:%.15g\n",
                    GDALGetRasterOffset( hBand, &bSuccess ),
                    GDALGetRasterScale( hBand, &bSuccess ) );

        GDALInfoReportMetadata( hBand, bListMDD, bShowMetadata, papszExtraMDDomains, TRUE);

        if( GDALGetRasterColorInterpretation(hBand) == GCI_PaletteIndex
            && (hTable = GDALGetRasterColorTable( hBand )) != NULL )
        {
            int			i;
            poColorTable = json_object_new_object();
            if(bJson)
            {
                json_object *poPalette, *poCount;
                poColorTable = json_object_new_object();
                poPalette = json_object_new_string(GDALGetPaletteInterpretationName(
                    GDALGetPaletteInterpretation(hTable)));
                poCount = json_object_new_int(GDALGetColorEntryCount(hTable));
                json_object_object_add(poColorTable, "palette", poPalette);
                json_object_object_add(poColorTable, "count", poCount);
            }
            else
                printf( "  Color Table (%s with %d entries)\n",
                    GDALGetPaletteInterpretationName(
                        GDALGetPaletteInterpretation( hTable )),
                    GDALGetColorEntryCount( hTable ) );

            if (bShowColorTable)
            {
                json_object *poEntries;
                if(bJson)
                    poEntries = json_object_new_array();
                for( i = 0; i < GDALGetColorEntryCount( hTable ); i++ )
                {
                    GDALColorEntry	sEntry;

                    GDALGetColorEntryAsRGB( hTable, i, &sEntry );

                    if(bJson)
                    {
                        json_object *poEntry, *poC1, *poC2, *poC3, *poC4;
                        poEntry = json_object_new_array();
                        poC1 = json_object_new_int(sEntry.c1);
                        poC2 = json_object_new_int(sEntry.c2);
                        poC3 = json_object_new_int(sEntry.c3);
                        poC4 = json_object_new_int(sEntry.c4);
                        json_object_array_add(poEntry, poC1);
                        json_object_array_add(poEntry, poC2);
                        json_object_array_add(poEntry, poC3);
                        json_object_array_add(poEntry, poC4);
                        json_object_array_add(poEntries, poEntry);
                    }
                    else
                        printf( "  %3d: %d,%d,%d,%d\n",
                            i,
                            sEntry.c1,
                            sEntry.c2,
                            sEntry.c3,
                            sEntry.c4 );
                }
                if(bJson)
                    json_object_object_add(poColorTable, "entries", poEntries);
            }
            if(bJson)
                json_object_object_add(poBand, "colorTable", poColorTable);
        }

        if( bShowRAT && GDALGetDefaultRAT( hBand ) != NULL )
        {
            GDALRasterAttributeTableH hRAT = GDALGetDefaultRAT( hBand );

            GDALRATDumpReadable( hRAT, NULL );
        }
        if(bJson)
            json_object_array_add(poBands, poBand);
    }

    json_object_object_add(poJsonObject, "bands", poBands);
    printf("%s", json_object_to_json_string_ext(poJsonObject, JSON_C_TO_STRING_PRETTY));
    GDALClose( hDataset );

    CSLDestroy( papszExtraMDDomains );
    CSLDestroy( papszOpenOptions );
    CSLDestroy( argv );

    GDALDumpOpenDatasets( stderr );

    GDALDestroyDriverManager();

    CPLDumpSharedList( NULL );
    CPLCleanupTLS();

    exit( 0 );
}

/************************************************************************/
/*                        GDALInfoReportCorner()                        */
/************************************************************************/

static int
GDALInfoReportCorner( GDALDatasetH hDataset,
                      OGRCoordinateTransformationH hTransform,
                      const char * corner_name,
                      double x, double y )

{
    double	dfGeoX, dfGeoY;
    double	adfGeoTransform[6];

    printf( "%-11s ", corner_name );

/* -------------------------------------------------------------------- */
/*      Transform the point into georeferenced coordinates.             */
/* -------------------------------------------------------------------- */
    if( GDALGetGeoTransform( hDataset, adfGeoTransform ) == CE_None )
    {
        dfGeoX = adfGeoTransform[0] + adfGeoTransform[1] * x
            + adfGeoTransform[2] * y;
        dfGeoY = adfGeoTransform[3] + adfGeoTransform[4] * x
            + adfGeoTransform[5] * y;
    }

    else
    {
        CPLprintf( "(%7.1f,%7.1f)\n", x, y );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Report the georeferenced coordinates.                           */
/* -------------------------------------------------------------------- */
    if( ABS(dfGeoX) < 181 && ABS(dfGeoY) < 91 )
    {
        CPLprintf( "(%12.7f,%12.7f) ", dfGeoX, dfGeoY );
    }
    else
    {
        CPLprintf( "(%12.3f,%12.3f) ", dfGeoX, dfGeoY );
    }

/* -------------------------------------------------------------------- */
/*      Transform to latlong and report.                                */
/* -------------------------------------------------------------------- */
    if( hTransform != NULL
        && OCTTransform(hTransform,1,&dfGeoX,&dfGeoY,NULL) )
    {

        printf( "(%s,", GDALDecToDMS( dfGeoX, "Long", 2 ) );
        printf( "%s)", GDALDecToDMS( dfGeoY, "Lat", 2 ) );
    }

    printf( "\n" );

    return TRUE;
}


/************************************************************************/
/*                       GDALInfoPrintMetadata()                        */
/************************************************************************/
static void GDALInfoPrintMetadata( GDALMajorObjectH hObject,
                                   const char *pszDomain,
                                   const char *pszDisplayedname,
                                   const char *pszIndent)
{
    int i;
    char **papszMetadata;
    int bIsxml = FALSE;

    if (pszDomain != NULL && EQUALN(pszDomain, "xml:", 4))
        bIsxml = TRUE;

    papszMetadata = GDALGetMetadata( hObject, pszDomain );
    if( CSLCount(papszMetadata) > 0 )
    {
        printf( "%s%s:\n", pszIndent, pszDisplayedname );
        for( i = 0; papszMetadata[i] != NULL; i++ )
        {
            if (bIsxml)
                printf( "%s%s\n", pszIndent, papszMetadata[i] );
            else
                printf( "%s  %s\n", pszIndent, papszMetadata[i] );
        }
    }
}

/************************************************************************/
/*                       GDALInfoReportMetadata()                       */
/************************************************************************/
static void GDALInfoReportMetadata( GDALMajorObjectH hObject,
                                    int bListMDD,
                                    int bShowMetadata,
                                    char **papszExtraMDDomains,
                                    int bIsBand )
{
    const char* pszIndent = "";
    if( bIsBand )
        pszIndent = "  ";

    /* -------------------------------------------------------------------- */
    /*      Report list of Metadata domains                                 */
    /* -------------------------------------------------------------------- */
    if( bListMDD )
    {
        char** papszMDDList = GDALGetMetadataDomainList( hObject );
        char** papszIter = papszMDDList;

        if( papszMDDList != NULL )
            printf( "%sMetadata domains:\n", pszIndent );
        while( papszIter != NULL && *papszIter != NULL )
        {
            if( EQUAL(*papszIter, "") )
                printf( "%s  (default)\n", pszIndent);
            else
                printf( "%s  %s\n", pszIndent, *papszIter );
            papszIter ++;
        }
        CSLDestroy(papszMDDList);
    }

    if (!bShowMetadata)
        return;

    /* -------------------------------------------------------------------- */
    /*      Report default Metadata domain.                                 */
    /* -------------------------------------------------------------------- */
    GDALInfoPrintMetadata( hObject, NULL, "Metadata", pszIndent );

    /* -------------------------------------------------------------------- */
    /*      Report extra Metadata domains                                   */
    /* -------------------------------------------------------------------- */
    if (papszExtraMDDomains != NULL) {
        char **papszExtraMDDomainsExpanded = NULL;
        int iMDD;

        if( EQUAL(papszExtraMDDomains[0], "all") &&
            papszExtraMDDomains[1] == NULL )
        {
            char** papszMDDList = GDALGetMetadataDomainList( hObject );
            char** papszIter = papszMDDList;

            while( papszIter != NULL && *papszIter != NULL )
            {
                if( !EQUAL(*papszIter, "") &&
                    !EQUAL(*papszIter, "IMAGE_STRUCTURE") &&
                    !EQUAL(*papszIter, "SUBDATASETS") &&
                    !EQUAL(*papszIter, "GEOLOCATION") &&
                    !EQUAL(*papszIter, "RPC") )
                {
                    papszExtraMDDomainsExpanded = CSLAddString(papszExtraMDDomainsExpanded, *papszIter);
                }
                papszIter ++;
            }
            CSLDestroy(papszMDDList);
        }
        else
        {
            papszExtraMDDomainsExpanded = CSLDuplicate(papszExtraMDDomains);
        }

        for( iMDD = 0; iMDD < CSLCount(papszExtraMDDomainsExpanded); iMDD++ )
        {
            char pszDisplayedname[256];
            snprintf(pszDisplayedname, 256, "Metadata (%s)", papszExtraMDDomainsExpanded[iMDD]);
            GDALInfoPrintMetadata( hObject, papszExtraMDDomainsExpanded[iMDD], pszDisplayedname, pszIndent );
        }

        CSLDestroy(papszExtraMDDomainsExpanded);
    }

    /* -------------------------------------------------------------------- */
    /*      Report various named metadata domains.                          */
    /* -------------------------------------------------------------------- */
    GDALInfoPrintMetadata( hObject, "IMAGE_STRUCTURE", "Image Structure Metadata", pszIndent );

    if (!bIsBand)
    {
        GDALInfoPrintMetadata( hObject, "SUBDATASETS", "Subdatasets", pszIndent );
        GDALInfoPrintMetadata( hObject, "GEOLOCATION", "Geolocation", pszIndent );
        GDALInfoPrintMetadata( hObject, "RPC", "RPC Metadata", pszIndent );
    }

}
