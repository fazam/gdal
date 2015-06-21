/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Utilities
 * Purpose:  GDAL Image Translator Program
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1998, 2002, Frank Warmerdam
 * Copyright (c) 2007-2014, Even Rouault <even dot rouault at mines-paris dot org>
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

#include "cpl_string.h"
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "commonutils.h"
#include "gdal_utils.h"

CPL_CVSID("$Id$");

static int ArgIsNumeric( const char * );

/*  ******************************************************************* */
/*                               Usage()                                */
/* ******************************************************************** */

static void Usage(const char* pszErrorMsg = NULL, int bShort = TRUE)

{
    int iDr;
        
    printf( "Usage: gdal_translate [--help-general] [--long-usage]\n"
            "       [-ot {Byte/Int16/UInt16/UInt32/Int32/Float32/Float64/\n"
            "             CInt16/CInt32/CFloat32/CFloat64}] [-strict]\n"
            "       [-of format] [-b band] [-mask band] [-expand {gray|rgb|rgba}]\n"
            "       [-outsize xsize[%%]|0 ysize[%%]|0] [-tr xres yres]\n"
            "       [-r {nearest,bilinear,cubic,cubicspline,lanczos,average,mode}]\n"
            "       [-unscale] [-scale[_bn] [src_min src_max [dst_min dst_max]]]* [-exponent[_bn] exp_val]*\n"
            "       [-srcwin xoff yoff xsize ysize] [-epo] [-eco]\n"
            "       [-projwin ulx uly lrx lry] [-projwin_srs srs_def]\n"
            "       [-a_srs srs_def] [-a_ullr ulx uly lrx lry] [-a_nodata value]\n"
            "       [-gcp pixel line easting northing [elevation]]*\n" 
            "       [-mo \"META-TAG=VALUE\"]* [-q] [-sds]\n"
            "       [-co \"NAME=VALUE\"]* [-stats] [-norat]\n"
            "       [-oo NAME=VALUE]*\n"
            "       src_dataset dst_dataset\n" );

    if( !bShort )
    {
        printf( "\n%s\n\n", GDALVersionInfo( "--version" ) );
        printf( "The following format drivers are configured and support output:\n" );
        for( iDr = 0; iDr < GDALGetDriverCount(); iDr++ )
        {
            GDALDriverH hDriver = GDALGetDriver(iDr);
            
            if( GDALGetMetadataItem( hDriver, GDAL_DCAP_RASTER, NULL) != NULL &&
                (GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATE, NULL ) != NULL
                || GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATECOPY, NULL ) != NULL) )
            {
                printf( "  %s: %s\n",
                        GDALGetDriverShortName( hDriver ),
                        GDALGetDriverLongName( hDriver ) );
            }
        }
    }

    if( pszErrorMsg != NULL )
        fprintf(stderr, "\nFAILURE: %s\n", pszErrorMsg);

    exit(1);
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/

#define CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(nExtraArg) \
    do { if (i + nExtraArg >= argc) \
        Usage(CPLSPrintf("%s option requires %d argument(s)", argv[i], nExtraArg)); } while(0)

int main( int argc, char ** argv )

{
    GDALDatasetH    hDataset, hOutDS;
    int         i;
    int                 bCopySubDatasets = FALSE;
    int                 bParsedMaskArgument = FALSE;
    char              **papszOpenOptions = NULL;
    int bFormatExplicitlySet = FALSE;
    GDALTranslateOptions *psOptions = NULL;

    /* Check strict compilation and runtime library version as we use C++ API */
    if (! GDAL_CHECK_VERSION(argv[0]))
        exit(1);

    EarlySetConfigOptions(argc, argv);

/* -------------------------------------------------------------------- */
/*      Register standard GDAL drivers, and process generic GDAL        */
/*      command options.                                                */
/* -------------------------------------------------------------------- */
    GDALAllRegister();
    argc = GDALGeneralCmdLineProcessor( argc, &argv, 0 );
    if( argc < 1 )
        exit( -argc );

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

    psOptions = GDALTranslateOptionsNew();

/* -------------------------------------------------------------------- */
/*      Handle command line arguments.                                  */
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
            Usage();
        else if ( EQUAL(argv[i], "--long-usage") )
        {
            Usage(NULL, FALSE);
        }
        else if( EQUAL(argv[i],"-of") && i < argc-1 )
        {
            psOptions->pszFormat = argv[++i];
            bFormatExplicitlySet = TRUE;
        }

        else if( EQUAL(argv[i],"-q") || EQUAL(argv[i],"-quiet") )
        {
            psOptions->bQuiet = TRUE;
            psOptions->pfnProgress = GDALDummyProgress;
        }

        else if( EQUAL(argv[i],"-ot") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            int iType;
            
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
                Usage(CPLSPrintf("Unknown output pixel type: %s.", argv[i+1] ));
            }
            i++;
        }
        else if( EQUAL(argv[i],"-b") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            const char* pszBand = argv[i+1];
            int bMask = FALSE;
            if (EQUAL(pszBand, "mask"))
                pszBand = "mask,1";
            if (EQUALN(pszBand, "mask,", 5))
            {
                bMask = TRUE;
                pszBand += 5;
                /* If we use the source mask band as a regular band */
                /* don't create a target mask band by default */
                if( !bParsedMaskArgument )
                    psOptions->eMaskMode = MASK_DISABLED;
            }
            int nBand = atoi(pszBand);
            if( nBand < 1 )
            {
                Usage(CPLSPrintf( "Unrecognizable band number (%s).", argv[i+1] ));
            }
            i++;

            psOptions->nBandCount++;
            psOptions->panBandList = (int *) 
                CPLRealloc(psOptions->panBandList, sizeof(int) * psOptions->nBandCount);
            psOptions->panBandList[psOptions->nBandCount-1] = nBand;
            if (bMask)
                psOptions->panBandList[psOptions->nBandCount-1] *= -1;

            if( psOptions->panBandList[psOptions->nBandCount-1] != psOptions->nBandCount )
                psOptions->bDefBands = FALSE;
        }
        else if( EQUAL(argv[i],"-mask") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            bParsedMaskArgument = TRUE;
            const char* pszBand = argv[i+1];
            if (EQUAL(pszBand, "none"))
            {
                psOptions->eMaskMode = MASK_DISABLED;
            }
            else if (EQUAL(pszBand, "auto"))
            {
                psOptions->eMaskMode = MASK_AUTO;
            }
            else
            {
                int bMask = FALSE;
                if (EQUAL(pszBand, "mask"))
                    pszBand = "mask,1";
                if (EQUALN(pszBand, "mask,", 5))
                {
                    bMask = TRUE;
                    pszBand += 5;
                }
                int nBand = atoi(pszBand);
                if( nBand < 1 )
                {
                    Usage(CPLSPrintf( "Unrecognizable band number (%s).", argv[i+1] ));
                }
                
                psOptions->eMaskMode = MASK_USER;
                psOptions->nMaskBand = nBand;
                if (bMask)
                    psOptions->nMaskBand *= -1;
            }
            i ++;
        }
        else if( EQUAL(argv[i],"-not_strict")  )
            psOptions->bStrict = FALSE;
            
        else if( EQUAL(argv[i],"-strict")  )
            psOptions->bStrict = TRUE;
            
        else if( EQUAL(argv[i],"-sds")  )
            bCopySubDatasets = TRUE;
            
        else if( EQUAL(argv[i],"-gcp") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(4);
            char* endptr = NULL;
            /* -gcp pixel line easting northing [elev] */

            psOptions->nGCPCount++;
            psOptions->pasGCPs = (GDAL_GCP *) 
                CPLRealloc( psOptions->pasGCPs, sizeof(GDAL_GCP) * psOptions->nGCPCount );
            GDALInitGCPs( 1, psOptions->pasGCPs + psOptions->nGCPCount - 1 );

            psOptions->pasGCPs[psOptions->nGCPCount-1].dfGCPPixel = CPLAtofM(argv[++i]);
            psOptions->pasGCPs[psOptions->nGCPCount-1].dfGCPLine = CPLAtofM(argv[++i]);
            psOptions->pasGCPs[psOptions->nGCPCount-1].dfGCPX = CPLAtofM(argv[++i]);
            psOptions->pasGCPs[psOptions->nGCPCount-1].dfGCPY = CPLAtofM(argv[++i]);
            if( argv[i+1] != NULL 
                && (CPLStrtod(argv[i+1], &endptr) != 0.0 || argv[i+1][0] == '0') )
            {
                /* Check that last argument is really a number and not a filename */
                /* looking like a number (see ticket #863) */
                if (endptr && *endptr == 0)
                    psOptions->pasGCPs[psOptions->nGCPCount-1].dfGCPZ = CPLAtofM(argv[++i]);
            }

            /* should set id and info? */
        }   

        else if( EQUAL(argv[i],"-a_nodata") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            if (EQUAL(argv[i+1], "none"))
            {
                psOptions->bUnsetNoData = TRUE;
            }
            else
            {
                psOptions->bSetNoData = TRUE;
                psOptions->dfNoDataReal = CPLAtofM(argv[i+1]);
            }
            i += 1;
        }   

        else if( EQUAL(argv[i],"-a_ullr") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(4);
            psOptions->adfULLR[0] = CPLAtofM(argv[i+1]);
            psOptions->adfULLR[1] = CPLAtofM(argv[i+2]);
            psOptions->adfULLR[2] = CPLAtofM(argv[i+3]);
            psOptions->adfULLR[3] = CPLAtofM(argv[i+4]);

            psOptions->bGotBounds = TRUE;
            
            i += 4;
        }   

        else if( EQUAL(argv[i],"-co") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszCreateOptions = CSLAddString( psOptions->papszCreateOptions, argv[++i] );
        }   

        else if( EQUAL(argv[i],"-scale") || EQUALN(argv[i],"-scale_", 7) )
        {
            int nIndex = 0;
            if( EQUALN(argv[i],"-scale_", 7) )
            {
                if( !psOptions->bHasUsedExplicitScaleBand && psOptions->nScaleRepeat != 0 )
                    Usage("Cannot mix -scale and -scale_XX syntax");
                psOptions->bHasUsedExplicitScaleBand = TRUE;
                nIndex = atoi(argv[i] + 7);
                if( nIndex <= 0 || nIndex > 65535 )
                    Usage(CPLSPrintf( "Invalid parameter name: %s", argv[i] ));
                nIndex --;
            }
            else
            {
                if( psOptions->bHasUsedExplicitScaleBand )
                    Usage("Cannot mix -scale and -scale_XX syntax");
                nIndex = psOptions->nScaleRepeat;
            }

            if( nIndex >= psOptions->nScaleRepeat )
            {
                psOptions->pasScaleParams = (ScaleParams*)CPLRealloc(psOptions->pasScaleParams,
                    (nIndex + 1) * sizeof(ScaleParams));
                if( nIndex > psOptions->nScaleRepeat )
                    memset(psOptions->pasScaleParams + psOptions->nScaleRepeat, 0,
                        sizeof(ScaleParams) * (nIndex - psOptions->nScaleRepeat));
                psOptions->nScaleRepeat = nIndex + 1;
            }
            psOptions->pasScaleParams[nIndex].bScale = TRUE;
            if( i < argc-2 && ArgIsNumeric(argv[i+1]) )
            {
                psOptions->pasScaleParams[nIndex].bHaveScaleSrc = TRUE;
                psOptions->pasScaleParams[nIndex].dfScaleSrcMin = CPLAtofM(argv[i+1]);
                psOptions->pasScaleParams[nIndex].dfScaleSrcMax = CPLAtofM(argv[i+2]);
                i += 2;
            }
            if( i < argc-2 && psOptions->pasScaleParams[nIndex].bHaveScaleSrc && ArgIsNumeric(argv[i+1]) )
            {
                psOptions->pasScaleParams[nIndex].dfScaleDstMin = CPLAtofM(argv[i+1]);
                psOptions->pasScaleParams[nIndex].dfScaleDstMax = CPLAtofM(argv[i+2]);
                i += 2;
            }
            else
            {
                psOptions->pasScaleParams[nIndex].dfScaleDstMin = 0.0;
                psOptions->pasScaleParams[nIndex].dfScaleDstMax = 255.999;
            }
        }

        else if( EQUAL(argv[i],"-exponent") || EQUALN(argv[i],"-exponent_",10) )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);

            int nIndex = 0;
            if( EQUALN(argv[i],"-exponent_", 10) )
            {
                if( !psOptions->bHasUsedExplicitExponentBand && psOptions->nExponentRepeat != 0 )
                    Usage("Cannot mix -exponent and -exponent_XX syntax");
                psOptions->bHasUsedExplicitExponentBand = TRUE;
                nIndex = atoi(argv[i] + 10);
                if( nIndex <= 0 || nIndex > 65535 )
                    Usage(CPLSPrintf( "Invalid parameter name: %s", argv[i] ));
                nIndex --;
            }
            else
            {
                if( psOptions->bHasUsedExplicitExponentBand )
                    Usage("Cannot mix -exponent and -exponent_XX syntax");
                nIndex = psOptions->nExponentRepeat;
            }

            if( nIndex >= psOptions->nExponentRepeat )
            {
                psOptions->padfExponent = (double*)CPLRealloc(psOptions->padfExponent,
                    (nIndex + 1) * sizeof(double));
                if( nIndex > psOptions->nExponentRepeat )
                    memset(psOptions->padfExponent + psOptions->nExponentRepeat, 0,
                        sizeof(double) * (nIndex - psOptions->nExponentRepeat));
                psOptions->nExponentRepeat = nIndex + 1;
            }
            double dfExponent = CPLAtofM(argv[++i]);
            psOptions->padfExponent[nIndex] = dfExponent;
        }

        else if( EQUAL(argv[i], "-unscale") )
        {
            psOptions->bUnscale = TRUE;
        }

        else if( EQUAL(argv[i],"-mo") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszMetadataOptions = CSLAddString( psOptions->papszMetadataOptions,
                                                 argv[++i] );
        }

        else if( EQUAL(argv[i],"-outsize") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(2);
            psOptions->pszOXSize = argv[++i];
            psOptions->pszOYSize = argv[++i];
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
        }

        else if( EQUAL(argv[i],"-srcwin") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(4);
            psOptions->anSrcWin[0] = atoi(argv[++i]);
            psOptions->anSrcWin[1] = atoi(argv[++i]);
            psOptions->anSrcWin[2] = atoi(argv[++i]);
            psOptions->anSrcWin[3] = atoi(argv[++i]);
        }   

        else if( EQUAL(argv[i],"-projwin") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(4);
            psOptions->dfULX = CPLAtofM(argv[++i]);
            psOptions->dfULY = CPLAtofM(argv[++i]);
            psOptions->dfLRX = CPLAtofM(argv[++i]);
            psOptions->dfLRY = CPLAtofM(argv[++i]);
        }   
        
        else if( EQUAL(argv[i],"-projwin_srs") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            OGRSpatialReference oSRS;

            if( oSRS.SetFromUserInput( argv[i+1] ) != OGRERR_NONE )
            {
                fprintf( stderr, "Failed to process SRS definition: %s\n", 
                         argv[i+1] );
                GDALDestroyDriverManager();
                exit( 1 );
            }

            char* pszSRS = NULL;
            oSRS.exportToWkt( &pszSRS );
            if( pszSRS )
                psOptions->osProjSRS = pszSRS;
            i++;
        }

        else if( EQUAL(argv[i],"-epo") )
        {
            psOptions->bErrorOnPartiallyOutside = TRUE;
            psOptions->bErrorOnCompletelyOutside = TRUE;
        }

        else  if( EQUAL(argv[i],"-eco") )
        {
            psOptions->bErrorOnCompletelyOutside = TRUE;
        }
    
        else if( EQUAL(argv[i],"-a_srs") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            OGRSpatialReference oOutputSRS;

            if( oOutputSRS.SetFromUserInput( argv[i+1] ) != OGRERR_NONE )
            {
                fprintf( stderr, "Failed to process SRS definition: %s\n", 
                         argv[i+1] );
                GDALDestroyDriverManager();
                exit( 1 );
            }

            oOutputSRS.exportToWkt( &psOptions->pszOutputSRS );
            i++;
        }   

        else if( EQUAL(argv[i],"-expand") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            if (EQUAL(argv[i+1], "gray"))
                psOptions->nRGBExpand = 1;
            else if (EQUAL(argv[i+1], "rgb"))
                psOptions->nRGBExpand = 3;
            else if (EQUAL(argv[i+1], "rgba"))
                psOptions->nRGBExpand = 4;
            else
            {
                Usage(CPLSPrintf( "Value %s unsupported. Only gray, rgb or rgba are supported.", 
                    argv[i] ));
            }
            i++;
        }

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
        else if( EQUAL(argv[i], "-norat") )
        {
            psOptions->bNoRAT = TRUE;
        }
        else if( EQUAL(argv[i], "-oo") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            papszOpenOptions = CSLAddString( papszOpenOptions,
                                                argv[++i] );
        }
        else if( EQUAL(argv[i],"-r") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszResampling = argv[++i];
        }  
        else if( argv[i][0] == '-' )
        {
            Usage(CPLSPrintf("Unknown option name '%s'", argv[i]));
        }
        else if( psOptions->pszSource == NULL )
        {
            psOptions->pszSource = argv[i];
        }
        else if( psOptions->pszDest == NULL )
        {
            psOptions->pszDest = argv[i];
        }

        else
        {
            Usage("Too many command options.");
        }
    }

    if (!psOptions->bQuiet && !bFormatExplicitlySet)
        CheckExtensionConsistency(psOptions->pszDest, psOptions->pszFormat);

/* -------------------------------------------------------------------- */
/*      Attempt to open source file.                                    */
/* -------------------------------------------------------------------- */

    hDataset = GDALOpenEx( psOptions->pszSource, GDAL_OF_RASTER, NULL,
                           (const char* const* )papszOpenOptions, NULL );
    
    if( hDataset == NULL )
    {
        fprintf( stderr,
                 "GDALOpen failed - %d\n%s\n",
                 CPLGetLastErrorNo(), CPLGetLastErrorMsg() );
        GDALDestroyDriverManager();
        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Handle subdatasets.                                             */
/* -------------------------------------------------------------------- */
    if( !bCopySubDatasets 
        && CSLCount(GDALGetMetadata( hDataset, "SUBDATASETS" )) > 0 
        && GDALGetRasterCount(hDataset) == 0 )
    {
        fprintf( stderr,
                 "Input file contains subdatasets. Please, select one of them for reading.\n" );
        GDALClose( hDataset );
        GDALDestroyDriverManager();
        exit( 1 );
    }

    if( CSLCount(GDALGetMetadata( hDataset, "SUBDATASETS" )) > 0 
        && bCopySubDatasets )
    {
        char **papszSubdatasets = GDALGetMetadata(hDataset,"SUBDATASETS");
        char *pszSubDest = (char *) CPLMalloc(strlen(psOptions->pszDest)+32);
        int i;

        CPLString osPath = CPLGetPath(psOptions->pszDest);
        CPLString osBasename = CPLGetBasename(psOptions->pszDest);
        CPLString osExtension = CPLGetExtension(psOptions->pszDest);
        CPLString osTemp;

        const char* pszFormat = NULL;
        if ( CSLCount(papszSubdatasets)/2 < 10 )
        {
            pszFormat = "%s_%d";
        }
        else if ( CSLCount(papszSubdatasets)/2 < 100 )
        {
            pszFormat = "%s_%002d";
        }
        else
        {
            pszFormat = "%s_%003d";
        }

        psOptions->pszDest = pszSubDest;

        for( i = 0; papszSubdatasets[i] != NULL; i += 2 )
        {
            psOptions->pszSource = CPLStrdup(strstr(papszSubdatasets[i],"=")+1);
            osTemp = CPLSPrintf( pszFormat, osBasename.c_str(), i/2 + 1 );
            osTemp = CPLFormFilename( osPath, osTemp, osExtension ); 
            strcpy( pszSubDest, osTemp.c_str() );
            hDataset = GDALOpenEx( psOptions->pszSource, GDAL_OF_RASTER, NULL,
                           (const char* const* )papszOpenOptions, NULL );
            hOutDS = GDALTranslate( hDataset, psOptions);
            if (hOutDS == NULL)
                break;
            GDALClose(hOutDS);
        }

        GDALClose(hDataset);

        GDALDestroyDriverManager();
        return 0;

    }

    hOutDS = GDALTranslate( hDataset, psOptions);

    if(hOutDS == NULL)
        Usage();

    GDALClose(hDataset);
    GDALClose(hOutDS);
    
    GDALDestroyDriverManager();

}


/************************************************************************/
/*                            ArgIsNumeric()                            */
/************************************************************************/

int ArgIsNumeric( const char *pszArg )

{
    return CPLGetValueType(pszArg) != CPL_VALUE_STRING;
}
