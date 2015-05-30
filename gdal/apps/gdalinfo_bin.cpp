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
#include "cpl_string.h"
#include "cpl_multiproc.h"
#include "commonutils.h"
#include "gdal_utils.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

void Usage(const char* pszErrorMsg)

{
    printf( "Usage: gdalinfo [--help-general] [-json] [-mm] [-stats] [-hist] [-nogcp] [-nomd]\n"
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
    GDALDatasetH hDataset = NULL;
    int i, nSubdataset = -1;
    const char *pszFilename = NULL;
    GDALInfoOptions *psOptions = NULL;
    char **papszFileList;
    char **papszOpenOptions = NULL;

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

    psOptions = GDALInfoOptionsNew();

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
            psOptions->eFormat = GDALINFO_FORMAT_JSON;
        else if( EQUAL(argv[i], "-mm") )
            psOptions->bComputeMinMax = TRUE;
        else if( EQUAL(argv[i], "-hist") )
            psOptions->bReportHistograms = TRUE;
        else if( EQUAL(argv[i], "-proj4") )
            psOptions->bReportProj4 = TRUE;
        else if( EQUAL(argv[i], "-stats") )
        {
            psOptions->bStats = TRUE;
            psOptions->bApproxStats = FALSE;
        }
        else if( EQUAL(argv[i], "-approx_stats") )
        {
            psOptions->bStats = TRUE;
            psOptions->bApproxStats = TRUE;
        }
        else if( EQUAL(argv[i], "-sample") )
            psOptions->bSample = TRUE;
        else if( EQUAL(argv[i], "-checksum") )
            psOptions->bComputeChecksum = TRUE;
        else if( EQUAL(argv[i], "-nogcp") )
            psOptions->bShowGCPs = FALSE;
        else if( EQUAL(argv[i], "-nomd") )
            psOptions->bShowMetadata = FALSE;
        else if( EQUAL(argv[i], "-norat") )
            psOptions->bShowRAT = FALSE;
        else if( EQUAL(argv[i], "-noct") )
            psOptions->bShowColorTable = FALSE;
        else if( EQUAL(argv[i], "-listmdd") )
            psOptions->bListMDD = TRUE;
        else if( EQUAL(argv[i], "-mdd") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszExtraMDDomains = CSLAddString( psOptions->papszExtraMDDomains,
                                                argv[++i] );
        }
        else if( EQUAL(argv[i], "-oo") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            papszOpenOptions = CSLAddString( papszOpenOptions,
                                                argv[++i] );
        }
        else if( EQUAL(argv[i], "-nofl") )
            psOptions->bShowFileList = FALSE;
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

        CSLDestroy( papszOpenOptions );
    
        GDALInfoOptionsFree( psOptions );

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

    printf( "%s\n", GDALInfo( hDataset, psOptions ) );

    GDALInfoOptionsFree( psOptions );

    GDALClose( hDataset );

    CSLDestroy( argv );
    
    GDALDumpOpenDatasets( stderr );

    GDALDestroyDriverManager();

    CPLDumpSharedList( NULL );
    CPLCleanupTLS();

    exit( 0 );

}