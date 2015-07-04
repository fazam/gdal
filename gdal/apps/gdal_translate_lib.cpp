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
#include "cpl_error.h"
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "vrtdataset.h"
#include "gdal_utils.h"

static void AttachMetadata( GDALDatasetH, char ** );
static void CopyBandInfo( GDALRasterBand * poSrcBand, GDALRasterBand * poDstBand,
                            int bCanCopyStatsMetadata, int bCopyScale, int bCopyNoData );

/************************************************************************/
/*                              SrcToDst()                              */
/************************************************************************/

static void SrcToDst( double dfX, double dfY,
                      int nSrcXOff, int nSrcYOff,
                      int nSrcXSize, int nSrcYSize,
                      int nDstXOff, int nDstYOff,
                      int nDstXSize, int nDstYSize,
                      double &dfXOut, double &dfYOut )

{
    dfXOut = ((dfX - nSrcXOff) / nSrcXSize) * nDstXSize + nDstXOff;
    dfYOut = ((dfY - nSrcYOff) / nSrcYSize) * nDstYSize + nDstYOff;
}

/************************************************************************/
/*                          GetSrcDstWindow()                           */
/************************************************************************/

static int FixSrcDstWindow( int* panSrcWin, int* panDstWin,
                            int nSrcRasterXSize,
                            int nSrcRasterYSize )

{
    const int nSrcXOff = panSrcWin[0];
    const int nSrcYOff = panSrcWin[1];
    const int nSrcXSize = panSrcWin[2];
    const int nSrcYSize = panSrcWin[3];

    const int nDstXOff = panDstWin[0];
    const int nDstYOff = panDstWin[1];
    const int nDstXSize = panDstWin[2];
    const int nDstYSize = panDstWin[3];

    int bModifiedX = FALSE, bModifiedY = FALSE;

    int nModifiedSrcXOff = nSrcXOff;
    int nModifiedSrcYOff = nSrcYOff;

    int nModifiedSrcXSize = nSrcXSize;
    int nModifiedSrcYSize = nSrcYSize;

/* -------------------------------------------------------------------- */
/*      Clamp within the bounds of the available source data.           */
/* -------------------------------------------------------------------- */
    if( nModifiedSrcXOff < 0 )
    {
        nModifiedSrcXSize += nModifiedSrcXOff;
        nModifiedSrcXOff = 0;

        bModifiedX = TRUE;
    }

    if( nModifiedSrcYOff < 0 )
    {
        nModifiedSrcYSize += nModifiedSrcYOff;
        nModifiedSrcYOff = 0;
        bModifiedY = TRUE;
    }

    if( nModifiedSrcXOff + nModifiedSrcXSize > nSrcRasterXSize )
    {
        nModifiedSrcXSize = nSrcRasterXSize - nModifiedSrcXOff;
        bModifiedX = TRUE;
    }

    if( nModifiedSrcYOff + nModifiedSrcYSize > nSrcRasterYSize )
    {
        nModifiedSrcYSize = nSrcRasterYSize - nModifiedSrcYOff;
        bModifiedY = TRUE;
    }

/* -------------------------------------------------------------------- */
/*      Don't do anything if the requesting region is completely off    */
/*      the source image.                                               */
/* -------------------------------------------------------------------- */
    if( nModifiedSrcXOff >= nSrcRasterXSize
        || nModifiedSrcYOff >= nSrcRasterYSize
        || nModifiedSrcXSize <= 0 || nModifiedSrcYSize <= 0 )
    {
        return FALSE;
    }

    panSrcWin[0] = nModifiedSrcXOff;
    panSrcWin[1] = nModifiedSrcYOff;
    panSrcWin[2] = nModifiedSrcXSize;
    panSrcWin[3] = nModifiedSrcYSize;

/* -------------------------------------------------------------------- */
/*      If we haven't had to modify the source rectangle, then the      */
/*      destination rectangle must be the whole region.                 */
/* -------------------------------------------------------------------- */
    if( !bModifiedX && !bModifiedY )
        return TRUE;

/* -------------------------------------------------------------------- */
/*      Now transform this possibly reduced request back into the       */
/*      destination buffer coordinates in case the output region is     */
/*      less than the whole buffer.                                     */
/* -------------------------------------------------------------------- */
    double dfDstULX, dfDstULY, dfDstLRX, dfDstLRY;

    SrcToDst( nModifiedSrcXOff, nModifiedSrcYOff,
              nSrcXOff, nSrcYOff,
              nSrcXSize, nSrcYSize,
              nDstXOff, nDstYOff,
              nDstXSize, nDstYSize,
              dfDstULX, dfDstULY );
    SrcToDst( nModifiedSrcXOff + nModifiedSrcXSize, nModifiedSrcYOff + nModifiedSrcYSize,
              nSrcXOff, nSrcYOff,
              nSrcXSize, nSrcYSize,
              nDstXOff, nDstYOff,
              nDstXSize, nDstYSize,
              dfDstLRX, dfDstLRY );

    int nModifiedDstXOff = nDstXOff;
    int nModifiedDstYOff = nDstYOff;
    int nModifiedDstXSize = nDstXSize;
    int nModifiedDstYSize = nDstYSize;

    if( bModifiedX )
    {
        nModifiedDstXOff = (int) ((dfDstULX - nDstXOff)+0.001);
        nModifiedDstXSize = (int) ((dfDstLRX - nDstXOff)+0.001)
            - nModifiedDstXOff;

        nModifiedDstXOff = MAX(0,nModifiedDstXOff);
        if( nModifiedDstXOff + nModifiedDstXSize > nDstXSize )
            nModifiedDstXSize = nDstXSize - nModifiedDstXOff;
    }

    if( bModifiedY )
    {
        nModifiedDstYOff = (int) ((dfDstULY - nDstYOff)+0.001);
        nModifiedDstYSize = (int) ((dfDstLRY - nDstYOff)+0.001)
            - nModifiedDstYOff;

        nModifiedDstYOff = MAX(0,nModifiedDstYOff);
        if( nModifiedDstYOff + nModifiedDstYSize > nDstYSize )
            nModifiedDstYSize = nDstYSize - nModifiedDstYOff;
    }

    if( nModifiedDstXSize < 1 || nModifiedDstYSize < 1 )
        return FALSE;
    else
    {
        panDstWin[0] = nModifiedDstXOff;
        panDstWin[1] = nModifiedDstYOff;
        panDstWin[2] = nModifiedDstXSize;
        panDstWin[3] = nModifiedDstYSize;

        return TRUE;
    }
}

/************************************************************************/
/*                             GDALTranslate()                          */
/************************************************************************/

/**
 * converts raster data between different formats.
 *
 * GDALTranslateOptions* must be allocated and freed with GDALTranslateOptionsNew()
 * and GDALTranslateOptionsFree() respectively.
 *
 * @param pszDest the destination dataset path.
 * @param hDataset the dataset handle.
 * @param psOptions the options struct for GDALTranslate().
 * @param bUsageError the pointer to int variable to determine any usage error has occured
 * @return the converted dataset.
 * It must be freed using GDALClose().
 */

GDALDatasetH GDALTranslate( const char *pszDest, GDALDatasetH hDataset, GDALTranslateOptions *psOptions, int *bUsageError )

{
    GDALDatasetH hOutDS;
    int			i;
    int			nRasterXSize, nRasterYSize;
    GDALDriverH		hDriver;    
    double		adfGeoTransform[6];
    int			nOXSize = 0, nOYSize = 0;
    const char          *pszProjection;
    const char *pszSource = NULL;
    int bGotBounds = FALSE;
    int bDefBands = TRUE;
    CPLString osProjSRS;

    if(bUsageError)
        *bUsageError = FALSE;

    if(psOptions->adfULLR[0] != 0.0 || psOptions->adfULLR[1] != 0.0 || psOptions->adfULLR[2] != 0.0 || psOptions->adfULLR[3] != 0.0)
        bGotBounds = TRUE;

    if(psOptions->panBandList != NULL)
    {
        if( psOptions->panBandList[psOptions->nBandCount-1] != psOptions->nBandCount )
                bDefBands = FALSE;
    }

    pszSource = GDALGetDescription(hDataset);

    if( pszDest == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, "No target dataset specified.");

        if(bUsageError)
            *bUsageError = TRUE;
        return NULL;
    }

    if ( strcmp(pszSource, pszDest) == 0)
    {
        CPLError( CE_Failure, CPLE_AppDefined, "Source and destination datasets must be different.");
        
        if(bUsageError)
            *bUsageError = TRUE;
        return NULL;
    }

    if(psOptions->pszProjSRS != NULL)
    {

        OGRSpatialReference oSRS;

        if( oSRS.SetFromUserInput( psOptions->pszProjSRS ) != OGRERR_NONE )
        {
            CPLError( CE_Failure, CPLE_AppDefined, "Failed to process SRS definition: %s\n", 
                      psOptions->pszProjSRS );
            return NULL;
        }

        char* pszSRS = NULL;
        oSRS.exportToWkt( &pszSRS );
        if( pszSRS )
            osProjSRS = pszSRS;
    }

    if(psOptions->pszOutputSRS != NULL)
    {
        OGRSpatialReference oOutputSRS;

        if( oOutputSRS.SetFromUserInput( psOptions->pszOutputSRS ) != OGRERR_NONE )
        {
            CPLError( CE_Failure, CPLE_AppDefined, "Failed to process SRS definition: %s\n", 
                      psOptions->pszOutputSRS );
            return NULL;
        }

        CPLFree(psOptions->pszOutputSRS);
        psOptions->pszOutputSRS = NULL;

        oOutputSRS.exportToWkt( &psOptions->pszOutputSRS );
    }

/* -------------------------------------------------------------------- */
/*      Check that incompatible options are not used                    */
/* -------------------------------------------------------------------- */

    if( (psOptions->nOXSizePixel != 0 || psOptions->dfOXSizePct != 0.0 || psOptions->nOYSizePixel != 0 ||
         psOptions->dfOYSizePct) && (psOptions->dfXRes != 0 && psOptions->dfYRes != 0) )
    {
        CPLError( CE_Failure, CPLE_IllegalArg, "-outsize and -tr options cannot be used at the same time.");
        if(bUsageError)
            *bUsageError = TRUE;
        return NULL;
    }
    if( bGotBounds &&  (psOptions->dfXRes != 0 && psOptions->dfYRes != 0) )
    {
        CPLError( CE_Failure, CPLE_IllegalArg, "-a_ullr and -tr options cannot be used at the same time.");
        if(bUsageError)
            *bUsageError = TRUE;
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Collect some information from the source file.                  */
/* -------------------------------------------------------------------- */
    nRasterXSize = GDALGetRasterXSize( hDataset );
    nRasterYSize = GDALGetRasterYSize( hDataset );

    if( psOptions->anSrcWin[2] == 0 && psOptions->anSrcWin[3] == 0 )
    {
        psOptions->anSrcWin[2] = nRasterXSize;
        psOptions->anSrcWin[3] = nRasterYSize;
    }

/* -------------------------------------------------------------------- */
/*	Build band list to translate					*/
/* -------------------------------------------------------------------- */
    if( psOptions->nBandCount == 0 )
    {
        psOptions->nBandCount = GDALGetRasterCount( hDataset );
        if( psOptions->nBandCount == 0 )
        {
            CPLError( CE_Failure, CPLE_AppDefined, "Input file has no bands, and so cannot be translated.\n" );
            return NULL;
        }

        psOptions->panBandList = (int *) CPLMalloc(sizeof(int)*psOptions->nBandCount);
        for( i = 0; i < psOptions->nBandCount; i++ )
            psOptions->panBandList[i] = i+1;
    }
    else
    {
        for( i = 0; i < psOptions->nBandCount; i++ )
        {
            if( ABS(psOptions->panBandList[i]) > GDALGetRasterCount(hDataset) )
            {
                CPLError( CE_Failure, CPLE_AppDefined,
                         "Band %d requested, but only bands 1 to %d available.\n",
                         ABS(psOptions->panBandList[i]), GDALGetRasterCount(hDataset) );
                return NULL;
            }
        }

        if( psOptions->nBandCount != GDALGetRasterCount( hDataset ) )
            bDefBands = FALSE;
    }

    if( psOptions->nScaleRepeat > psOptions->nBandCount )
    {
        if( !psOptions->bHasUsedExplicitScaleBand )
            CPLError( CE_Failure, CPLE_IllegalArg, "-scale has been specified more times than the number of output bands");
        else
            CPLError( CE_Failure, CPLE_IllegalArg, "-scale_XX has been specified with XX greater than the number of output bands");
        if(bUsageError)
            *bUsageError = TRUE;
        return NULL;
    }

    if( psOptions->nExponentRepeat > psOptions->nBandCount )
    {
        if( !psOptions->bHasUsedExplicitExponentBand )
            CPLError( CE_Failure, CPLE_IllegalArg, "-exponent has been specified more times than the number of output bands");
        else
            CPLError( CE_Failure, CPLE_IllegalArg, "-exponent_XX has been specified with XX greater than the number of output bands");
        if(bUsageError)
            *bUsageError = TRUE;
        return NULL;
    }
/* -------------------------------------------------------------------- */
/*      Compute the source window from the projected source window      */
/*      if the projected coordinates were provided.  Note that the      */
/*      projected coordinates are in ulx, uly, lrx, lry format,         */
/*      while the anSrcWin is xoff, yoff, xsize, ysize with the         */
/*      xoff,yoff being the ulx, uly in pixel/line.                     */
/* -------------------------------------------------------------------- */
    if( psOptions->dfULX != 0.0 || psOptions->dfULY != 0.0 
        || psOptions->dfLRX != 0.0 || psOptions->dfLRY != 0.0 )
    {
        double	adfGeoTransform[6];

        GDALGetGeoTransform( hDataset, adfGeoTransform );

        if( adfGeoTransform[2] != 0.0 || adfGeoTransform[4] != 0.0 )
        {
            CPLError( CE_Failure, CPLE_AppDefined,
                     "The -projwin option was used, but the geotransform is\n"
                     "rotated.  This configuration is not supported.\n" );
            return NULL;
        }

        if( osProjSRS.size() )
        {
            pszProjection = GDALGetProjectionRef( hDataset );
            if( pszProjection != NULL && strlen(pszProjection) > 0 )
            {
                OGRSpatialReference oSRSIn;
                OGRSpatialReference oSRSDS;
                oSRSIn.SetFromUserInput(osProjSRS);
                oSRSDS.SetFromUserInput(pszProjection);
                if( !oSRSIn.IsSame(&oSRSDS) )
                {
                    OGRCoordinateTransformation* poCT = OGRCreateCoordinateTransformation(&oSRSIn, &oSRSDS);
                    if( !(poCT &&
                        poCT->Transform(1, &psOptions->dfULX, &psOptions->dfULY) &&
                        poCT->Transform(1, &psOptions->dfLRX, &psOptions->dfLRY)) )
                    {
                        OGRCoordinateTransformation::DestroyCT(poCT);

                        CPLError( CE_Failure, CPLE_AppDefined, "-projwin_srs ignored since coordinate transformation failed.\n");
                        return NULL;
                    }
                    delete poCT;
                }
            }
            else
            {
                CPLError( CE_None, CPLE_None, "-projwin_srs ignored since the dataset has no projection.\n");
            }
        }

        psOptions->anSrcWin[0] = (int) 
            floor((psOptions->dfULX - adfGeoTransform[0]) / adfGeoTransform[1] + 0.001);
        psOptions->anSrcWin[1] = (int) 
            floor((psOptions->dfULY - adfGeoTransform[3]) / adfGeoTransform[5] + 0.001);

        psOptions->anSrcWin[2] = (int) ((psOptions->dfLRX - psOptions->dfULX) / adfGeoTransform[1] + 0.5);
        psOptions->anSrcWin[3] = (int) ((psOptions->dfLRY - psOptions->dfULY) / adfGeoTransform[5] + 0.5);

        /*if( !psOptions->bQuiet )
        {
            CPLError( CE_None, CPLE_None, "Computed -srcwin %d %d %d %d from projected window.\n",
                psOptions->anSrcWin[0], 
                psOptions->anSrcWin[1], 
                psOptions->anSrcWin[2], 
                psOptions->anSrcWin[3] );
        }*/
    }

/* -------------------------------------------------------------------- */
/*      Verify source window dimensions.                                */
/* -------------------------------------------------------------------- */
    if( psOptions->anSrcWin[2] <= 0 || psOptions->anSrcWin[3] <= 0 )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                 "Error: %s-srcwin %d %d %d %d has negative width and/or height.\n",
                 ( psOptions->dfULX != 0.0 || psOptions->dfULY != 0.0 || psOptions->dfLRX != 0.0 || psOptions->dfLRY != 0.0 ) ? "Computed " : "",
                 psOptions->anSrcWin[0],
                 psOptions->anSrcWin[1],
                 psOptions->anSrcWin[2],
                 psOptions->anSrcWin[3] );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Verify source window dimensions.                                */
/* -------------------------------------------------------------------- */
    else if( psOptions->anSrcWin[0] < 0 || psOptions->anSrcWin[1] < 0 
        || psOptions->anSrcWin[0] + psOptions->anSrcWin[2] > GDALGetRasterXSize(hDataset)
        || psOptions->anSrcWin[1] + psOptions->anSrcWin[3] > GDALGetRasterYSize(hDataset) )
    {
        int bCompletelyOutside = psOptions->anSrcWin[0] + psOptions->anSrcWin[2] <= 0 ||
                                    psOptions->anSrcWin[1] + psOptions->anSrcWin[3] <= 0 ||
                                    psOptions->anSrcWin[0] >= GDALGetRasterXSize(hDataset) ||
                                    psOptions->anSrcWin[1] >= GDALGetRasterYSize(hDataset);
        int bIsError = psOptions->bErrorOnPartiallyOutside || (bCompletelyOutside && psOptions->bErrorOnCompletelyOutside);
        if( !psOptions->bQuiet || bIsError )
        {
            CPLErr eErr = CE_Warning;

            if(bIsError)
                eErr = CE_Failure;

            CPLError( eErr, CPLE_AppDefined,
                 "%s-srcwin %d %d %d %d falls %s outside raster extent.%s\n",
                 ( psOptions->dfULX != 0.0 || psOptions->dfULY != 0.0 || psOptions->dfLRX != 0.0 || psOptions->dfLRY != 0.0 ) ? "Computed " : "",
                 psOptions->anSrcWin[0],
                 psOptions->anSrcWin[1],
                 psOptions->anSrcWin[2],
                 psOptions->anSrcWin[3],
                 (bCompletelyOutside) ? "completely" : "partially",
                 (bIsError) ? "" : " Going on however." );
        }
        if( bIsError )
            return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Find the output driver.                                         */
/* -------------------------------------------------------------------- */
    hDriver = GDALGetDriverByName( psOptions->pszFormat );
    if( hDriver == NULL )
    {
        int	iDr;
        
        CPLError( CE_Failure, CPLE_IllegalArg, "Output driver `%s' not recognised.\n", psOptions->pszFormat);
        CPLError( CE_Failure, CPLE_IllegalArg, "The following format drivers are configured and support output:\n" );
        for( iDr = 0; iDr < GDALGetDriverCount(); iDr++ )
        {
            GDALDriverH hDriver = GDALGetDriver(iDr);

            if( GDALGetMetadataItem( hDriver, GDAL_DCAP_RASTER, NULL) != NULL &&
                (GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATE, NULL ) != NULL
                 || GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATECOPY, NULL ) != NULL) )
            {
                CPLError( CE_Failure, CPLE_IllegalArg, "  %s: %s\n",
                        GDALGetDriverShortName( hDriver  ),
                        GDALGetDriverLongName( hDriver ) );
            }
        }
        CPLError( CE_Failure, CPLE_IllegalArg, "\n" );
        if(bUsageError)
            *bUsageError = TRUE;
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      The short form is to CreateCopy().  We use this if the input    */
/*      matches the whole dataset.  Eventually we should rewrite        */
/*      this entire program to use virtual datasets to construct a      */
/*      virtual input source to copy from.                              */
/* -------------------------------------------------------------------- */


    int bSpatialArrangementPreserved = (
           psOptions->anSrcWin[0] == 0 && psOptions->anSrcWin[1] == 0
        && psOptions->anSrcWin[2] == GDALGetRasterXSize(hDataset)
        && psOptions->anSrcWin[3] == GDALGetRasterYSize(hDataset)
        && psOptions->nOXSizePixel == 0 && psOptions->dfOXSizePct == 0.0
        && psOptions->nOYSizePixel == 0 && psOptions->dfOYSizePct == 0.0 && psOptions->dfXRes == 0.0 );

    if( psOptions->eOutputType == GDT_Unknown 
        && psOptions->nScaleRepeat == 0 && psOptions->nExponentRepeat == 0 && !psOptions->bUnscale
        && CSLCount(psOptions->papszMetadataOptions) == 0 && bDefBands 
        && psOptions->eMaskMode == MASK_AUTO
        && bSpatialArrangementPreserved
        && psOptions->nGCPCount == 0 && !bGotBounds
        && psOptions->pszOutputSRS == NULL && !psOptions->bSetNoData && !psOptions->bUnsetNoData
        && psOptions->nRGBExpand == 0 && !psOptions->bStats && !psOptions->bNoRAT )
    {
        
        hOutDS = GDALCreateCopy( hDriver, pszDest, hDataset, 
                                 psOptions->bStrict, psOptions->papszCreateOptions, 
                                 psOptions->pfnProgress, psOptions->pProgressData );


        return hOutDS;
    }

/* -------------------------------------------------------------------- */
/*      Establish some parameters.                                      */
/* -------------------------------------------------------------------- */
    if( psOptions->dfXRes != 0.0 )
    {
        if( !(GDALGetGeoTransform( hDataset, adfGeoTransform ) == CE_None &&
              psOptions->nGCPCount == 0 &&
              adfGeoTransform[2] == 0.0 && adfGeoTransform[4] == 0.0) )
        {
            CPLError( CE_Failure, CPLE_IllegalArg,
                     "The -tr option was used, but there's no geotransform or it is\n"
                     "rotated.  This configuration is not supported.\n" );
            return NULL;
        }
        nOXSize = int(psOptions->anSrcWin[2] / psOptions->dfXRes * adfGeoTransform[1] + 0.5);
        nOYSize = int(psOptions->anSrcWin[3] / psOptions->dfYRes * fabs(adfGeoTransform[5]) + 0.5);
    }
    else if( psOptions->nOXSizePixel == 0 && psOptions->dfOXSizePct == 0.0 && psOptions->nOYSizePixel == 0 && psOptions->dfOYSizePct == 0.0)
    {
        nOXSize = psOptions->anSrcWin[2];
        nOYSize = psOptions->anSrcWin[3];
    }
    else
    {
        if( !(psOptions->nOXSizePixel == 0 && psOptions->dfOXSizePct == 0.0) )
        {
            if(psOptions->nOXSizePixel != 0)
                nOXSize = psOptions->nOXSizePixel;
            else
                nOXSize = (int) psOptions->dfOXSizePct/100*psOptions->anSrcWin[2];
        }

        if( !(psOptions->nOYSizePixel == 0 && psOptions->dfOYSizePct == 0.0) )
        {
            if(psOptions->nOYSizePixel != 0)
                nOYSize = psOptions->nOYSizePixel;
            else
                nOYSize = (int) psOptions->dfOYSizePct/100*psOptions->anSrcWin[3];
        }

        if( psOptions->nOXSizePixel == 0 && psOptions->dfOXSizePct == 0.0 )
            nOXSize = (int)((double)nOYSize * psOptions->anSrcWin[2] / psOptions->anSrcWin[3] + 0.5);
        else if( psOptions->nOYSizePixel == 0 && psOptions->dfOYSizePct == 0.0 )
            nOYSize = (int)((double)nOXSize * psOptions->anSrcWin[3] / psOptions->anSrcWin[2] + 0.5);
    }

    if( nOXSize == 0 || nOYSize == 0 )
    {
        CPLError( CE_Failure, CPLE_IllegalArg, "Attempt to create %dx%d dataset is illegal.\n", nOXSize, nOYSize);
        return NULL;
    }

/* ==================================================================== */
/*      Create a virtual dataset.                                       */
/* ==================================================================== */
    VRTDataset *poVDS;
        
/* -------------------------------------------------------------------- */
/*      Make a virtual clone.                                           */
/* -------------------------------------------------------------------- */
    poVDS = (VRTDataset *) VRTCreate( nOXSize, nOYSize );

    if( psOptions->nGCPCount == 0 )
    {
        if( psOptions->pszOutputSRS != NULL )
        {
            poVDS->SetProjection( psOptions->pszOutputSRS );
        }
        else
        {
            pszProjection = GDALGetProjectionRef( hDataset );
            if( pszProjection != NULL && strlen(pszProjection) > 0 )
                poVDS->SetProjection( pszProjection );
        }
    }

    if( bGotBounds )
    {
        adfGeoTransform[0] = psOptions->adfULLR[0];
        adfGeoTransform[1] = (psOptions->adfULLR[2] - psOptions->adfULLR[0]) / nOXSize;
        adfGeoTransform[2] = 0.0;
        adfGeoTransform[3] = psOptions->adfULLR[1];
        adfGeoTransform[4] = 0.0;
        adfGeoTransform[5] = (psOptions->adfULLR[3] - psOptions->adfULLR[1]) / nOYSize;

        poVDS->SetGeoTransform( adfGeoTransform );
    }

    else if( GDALGetGeoTransform( hDataset, adfGeoTransform ) == CE_None 
        && psOptions->nGCPCount == 0 )
    {
        adfGeoTransform[0] += psOptions->anSrcWin[0] * adfGeoTransform[1]
            + psOptions->anSrcWin[1] * adfGeoTransform[2];
        adfGeoTransform[3] += psOptions->anSrcWin[0] * adfGeoTransform[4]
            + psOptions->anSrcWin[1] * adfGeoTransform[5];
        
        adfGeoTransform[1] *= psOptions->anSrcWin[2] / (double) nOXSize;
        adfGeoTransform[2] *= psOptions->anSrcWin[3] / (double) nOYSize;
        adfGeoTransform[4] *= psOptions->anSrcWin[2] / (double) nOXSize;
        adfGeoTransform[5] *= psOptions->anSrcWin[3] / (double) nOYSize;
        
        if( psOptions->dfXRes != 0.0 )
        {
            adfGeoTransform[1] = psOptions->dfXRes;
            adfGeoTransform[5] = (adfGeoTransform[5] > 0) ? psOptions->dfYRes : -psOptions->dfYRes;
        }

        poVDS->SetGeoTransform( adfGeoTransform );
    }

    if( psOptions->nGCPCount != 0 )
    {
        const char *pszGCPProjection = psOptions->pszOutputSRS;

        if( pszGCPProjection == NULL )
            pszGCPProjection = GDALGetGCPProjection( hDataset );
        if( pszGCPProjection == NULL )
            pszGCPProjection = "";

        poVDS->SetGCPs( psOptions->nGCPCount, psOptions->pasGCPs, pszGCPProjection );

        GDALDeinitGCPs( psOptions->nGCPCount, psOptions->pasGCPs );
    }

    else if( GDALGetGCPCount( hDataset ) > 0 )
    {
        GDAL_GCP *pasGCPs;
        int       nGCPs = GDALGetGCPCount( hDataset );

        pasGCPs = GDALDuplicateGCPs( nGCPs, GDALGetGCPs( hDataset ) );

        for( i = 0; i < nGCPs; i++ )
        {
            pasGCPs[i].dfGCPPixel -= psOptions->anSrcWin[0];
            pasGCPs[i].dfGCPLine  -= psOptions->anSrcWin[1];
            pasGCPs[i].dfGCPPixel *= (nOXSize / (double) psOptions->anSrcWin[2] );
            pasGCPs[i].dfGCPLine  *= (nOYSize / (double) psOptions->anSrcWin[3] );
        }
            
        poVDS->SetGCPs( nGCPs, pasGCPs,
                        GDALGetGCPProjection( hDataset ) );

        GDALDeinitGCPs( nGCPs, pasGCPs );
        CPLFree( pasGCPs );
    }

/* -------------------------------------------------------------------- */
/*      To make the VRT to look less awkward (but this is optional      */
/*      in fact), avoid negative values.                                */
/* -------------------------------------------------------------------- */
    int anDstWin[4];
    anDstWin[0] = 0;
    anDstWin[1] = 0;
    anDstWin[2] = nOXSize;
    anDstWin[3] = nOYSize;

    FixSrcDstWindow( psOptions->anSrcWin, anDstWin,
                     GDALGetRasterXSize(hDataset),
                     GDALGetRasterYSize(hDataset) );

/* -------------------------------------------------------------------- */
/*      Transfer generally applicable metadata.                         */
/* -------------------------------------------------------------------- */
    char** papszMetadata = CSLDuplicate(((GDALDataset*)hDataset)->GetMetadata());
    if ( psOptions->nScaleRepeat > 0 || psOptions->bUnscale || psOptions->eOutputType != GDT_Unknown )
    {
        /* Remove TIFFTAG_MINSAMPLEVALUE and TIFFTAG_MAXSAMPLEVALUE */
        /* if the data range may change because of options */
        char** papszIter = papszMetadata;
        while(papszIter && *papszIter)
        {
            if (EQUALN(*papszIter, "TIFFTAG_MINSAMPLEVALUE=", 23) ||
                EQUALN(*papszIter, "TIFFTAG_MAXSAMPLEVALUE=", 23))
            {
                CPLFree(*papszIter);
                memmove(papszIter, papszIter+1, sizeof(char*) * (CSLCount(papszIter+1)+1));
            }
            else
                papszIter++;
        }
    }
    poVDS->SetMetadata( papszMetadata );
    CSLDestroy( papszMetadata );
    AttachMetadata( (GDALDatasetH) poVDS, psOptions->papszMetadataOptions );

    const char* pszInterleave = GDALGetMetadataItem(hDataset, "INTERLEAVE", "IMAGE_STRUCTURE");
    if (pszInterleave)
        poVDS->SetMetadataItem("INTERLEAVE", pszInterleave, "IMAGE_STRUCTURE");

/* -------------------------------------------------------------------- */
/*      Transfer metadata that remains valid if the spatial             */
/*      arrangement of the data is unaltered.                           */
/* -------------------------------------------------------------------- */
    if( bSpatialArrangementPreserved )
    {
        char **papszMD;

        papszMD = ((GDALDataset*)hDataset)->GetMetadata("RPC");
        if( papszMD != NULL )
            poVDS->SetMetadata( papszMD, "RPC" );

        papszMD = ((GDALDataset*)hDataset)->GetMetadata("GEOLOCATION");
        if( papszMD != NULL )
            poVDS->SetMetadata( papszMD, "GEOLOCATION" );
    }
    else
    {
        char **papszMD;

        papszMD = ((GDALDataset*)hDataset)->GetMetadata("RPC");
        if( papszMD != NULL )
        {
            papszMD = CSLDuplicate(papszMD);
 
            double dfSAMP_OFF = CPLAtof(CSLFetchNameValueDef(papszMD, "SAMP_OFF", "0"));
            double dfLINE_OFF = CPLAtof(CSLFetchNameValueDef(papszMD, "LINE_OFF", "0"));
            double dfSAMP_SCALE = CPLAtof(CSLFetchNameValueDef(papszMD, "SAMP_SCALE", "1"));
            double dfLINE_SCALE = CPLAtof(CSLFetchNameValueDef(papszMD, "LINE_SCALE", "1"));

            dfSAMP_OFF -= psOptions->anSrcWin[0];
            dfLINE_OFF -= psOptions->anSrcWin[1];
            dfSAMP_OFF *= (nOXSize / (double) psOptions->anSrcWin[2] );
            dfLINE_OFF *= (nOYSize / (double) psOptions->anSrcWin[3] );
            dfSAMP_SCALE *= (nOXSize / (double) psOptions->anSrcWin[2] );
            dfLINE_SCALE *= (nOYSize / (double) psOptions->anSrcWin[3] );

            CPLString osField;
            osField.Printf( "%.15g", dfLINE_OFF );
            papszMD = CSLSetNameValue( papszMD, "LINE_OFF", osField );

            osField.Printf( "%.15g", dfSAMP_OFF );
            papszMD = CSLSetNameValue( papszMD, "SAMP_OFF", osField );

            osField.Printf( "%.15g", dfLINE_SCALE );
            papszMD = CSLSetNameValue( papszMD, "LINE_SCALE", osField );

            osField.Printf( "%.15g", dfSAMP_SCALE );
            papszMD = CSLSetNameValue( papszMD, "SAMP_SCALE", osField );

            poVDS->SetMetadata( papszMD, "RPC" );
            CSLDestroy(papszMD);
        }
    }

    int nSrcBandCount = psOptions->nBandCount;

    if (psOptions->nRGBExpand != 0)
    {
        GDALRasterBand  *poSrcBand;
        poSrcBand = ((GDALDataset *) 
                     hDataset)->GetRasterBand(ABS(psOptions->panBandList[0]));
        if (psOptions->panBandList[0] < 0)
            poSrcBand = poSrcBand->GetMaskBand();
        GDALColorTable* poColorTable = poSrcBand->GetColorTable();
        if (poColorTable == NULL)
        {
            CPLError( CE_Failure, CPLE_AppDefined, "Error : band %d has no color table\n", ABS(psOptions->panBandList[0]));
            GDALClose((GDALDatasetH) poVDS);
            return NULL;
        }
        
        /* Check that the color table only contains gray levels */
        /* when using -expand gray */
        if (psOptions->nRGBExpand == 1)
        {
            int nColorCount = poColorTable->GetColorEntryCount();
            int nColor;
            for( nColor = 0; nColor < nColorCount; nColor++ )
            {
                const GDALColorEntry* poEntry = poColorTable->GetColorEntry(nColor);
                if (poEntry->c1 != poEntry->c2 || poEntry->c1 != poEntry->c3)
                {
                    CPLError( CE_Warning, CPLE_AppDefined, "Warning : color table contains non gray levels colors\n");
                    break;
                }
            }
        }

        if (psOptions->nBandCount == 1)
            psOptions->nBandCount = psOptions->nRGBExpand;
        else if (psOptions->nBandCount == 2 && (psOptions->nRGBExpand == 3 || psOptions->nRGBExpand == 4))
            psOptions->nBandCount = psOptions->nRGBExpand;
        else
        {
            CPLError( CE_Failure, CPLE_IllegalArg, "Error : invalid use of -expand option.\n");
            GDALClose((GDALDatasetH) poVDS);
            return NULL;
        }
    }

    // Can be set to TRUE in the band loop too
    int bFilterOutStatsMetadata =
        (psOptions->nScaleRepeat > 0 || psOptions->bUnscale || !bSpatialArrangementPreserved || psOptions->nRGBExpand != 0);

/* ==================================================================== */
/*      Process all bands.                                              */
/* ==================================================================== */
    for( i = 0; i < psOptions->nBandCount; i++ )
    {
        VRTSourcedRasterBand   *poVRTBand;
        GDALRasterBand  *poSrcBand;
        GDALDataType    eBandType;
        int             nComponent = 0;

        int nSrcBand;
        if (psOptions->nRGBExpand != 0)
        {
            if (nSrcBandCount == 2 && psOptions->nRGBExpand == 4 && i == 3)
                nSrcBand = psOptions->panBandList[1];
            else
            {
                nSrcBand = psOptions->panBandList[0];
                nComponent = i + 1;
            }
        }
        else
            nSrcBand = psOptions->panBandList[i];

        poSrcBand = ((GDALDataset *) hDataset)->GetRasterBand(ABS(nSrcBand));

/* -------------------------------------------------------------------- */
/*      Select output data type to match source.                        */
/* -------------------------------------------------------------------- */
        if( psOptions->eOutputType == GDT_Unknown )
            eBandType = poSrcBand->GetRasterDataType();
        else
        {
            eBandType = psOptions->eOutputType;
            
            // Check that we can copy existing statistics
            GDALDataType eSrcBandType = poSrcBand->GetRasterDataType();
            const char* pszMin = poSrcBand->GetMetadataItem("STATISTICS_MINIMUM");
            const char* pszMax = poSrcBand->GetMetadataItem("STATISTICS_MAXIMUM");
            if( !bFilterOutStatsMetadata && eBandType != eSrcBandType &&
                pszMin != NULL && pszMax != NULL )
            {
                int bSrcIsInteger = ( eSrcBandType == GDT_Byte ||
                                      eSrcBandType == GDT_Int16 ||
                                      eSrcBandType == GDT_UInt16 ||
                                      eSrcBandType == GDT_Int32 ||
                                      eSrcBandType == GDT_UInt32 );
                int bDstIsInteger = ( eBandType == GDT_Byte ||
                                      eBandType == GDT_Int16 ||
                                      eBandType == GDT_UInt16 ||
                                      eBandType == GDT_Int32 ||
                                      eBandType == GDT_UInt32 );
                if( bSrcIsInteger && bDstIsInteger )
                {
                    GInt32 nDstMin = 0;
                    GUInt32 nDstMax = 0;
                    switch( eBandType )
                    {
                        case GDT_Byte:
                            nDstMin = 0;
                            nDstMax = 255;
                            break;
                        case GDT_UInt16:
                            nDstMin = 0;
                            nDstMax = 65535;
                            break;
                        case GDT_Int16:
                            nDstMin = -32768;
                            nDstMax = 32767;
                            break;
                        case GDT_UInt32:
                            nDstMin = 0;
                            nDstMax = 0xFFFFFFFFU;
                            break;
                        case GDT_Int32:
                            nDstMin = 0x80000000;
                            nDstMax = 0x7FFFFFFF;
                            break;
                        default:
                            CPLAssert(FALSE);
                            break;
                    }

                    GInt32 nMin = atoi(pszMin);
                    GUInt32 nMax = (GUInt32)strtoul(pszMax, NULL, 10);
                    if( nMin < nDstMin || nMax > nDstMax )
                        bFilterOutStatsMetadata = TRUE;
                }
                // Float64 is large enough to hold all integer <= 32 bit or float32 values
                // there might be other OK cases, but ere on safe side for now
                else if( !((bSrcIsInteger || eSrcBandType == GDT_Float32) && eBandType == GDT_Float64) )
                {
                    bFilterOutStatsMetadata = TRUE;
                }
            }
        }

/* -------------------------------------------------------------------- */
/*      Create this band.                                               */
/* -------------------------------------------------------------------- */
        poVDS->AddBand( eBandType, NULL );
        poVRTBand = (VRTSourcedRasterBand *) poVDS->GetRasterBand( i+1 );
        if (nSrcBand < 0)
        {
            poVRTBand->AddMaskBandSource(poSrcBand,
                                         psOptions->anSrcWin[0], psOptions->anSrcWin[1],
                                         psOptions->anSrcWin[2], psOptions->anSrcWin[3],
                                         anDstWin[0], anDstWin[1],
                                         anDstWin[2], anDstWin[3]);
            continue;
        }

        // Preserve nbits if no option change values
        const char* pszNBits = poSrcBand->GetMetadataItem("NBITS", "IMAGE_STRUCTURE");
        if( pszNBits && psOptions->nRGBExpand == 0 && psOptions->nScaleRepeat == 0 &&
            !psOptions->bUnscale && psOptions->eOutputType == GDT_Unknown && psOptions->pszResampling == NULL )
        {
            poVRTBand->SetMetadataItem("NBITS", pszNBits, "IMAGE_STRUCTURE");
        }

/* -------------------------------------------------------------------- */
/*      Do we need to collect scaling information?                      */
/* -------------------------------------------------------------------- */
        double dfScale=1.0, dfOffset=0.0;
        int    bScale = FALSE, bHaveScaleSrc = FALSE;
        double dfScaleSrcMin = 0.0, dfScaleSrcMax = 0.0;
        double dfScaleDstMin = 0.0, dfScaleDstMax = 0.0;
        int    bExponentScaling = FALSE;
        double dfExponent = 0.0;

        if( i < psOptions->nScaleRepeat && psOptions->pasScaleParams[i].bScale )
        {
            bScale = psOptions->pasScaleParams[i].bScale;
            bHaveScaleSrc = psOptions->pasScaleParams[i].bHaveScaleSrc;
            dfScaleSrcMin = psOptions->pasScaleParams[i].dfScaleSrcMin;
            dfScaleSrcMax = psOptions->pasScaleParams[i].dfScaleSrcMax;
            dfScaleDstMin = psOptions->pasScaleParams[i].dfScaleDstMin;
            dfScaleDstMax = psOptions->pasScaleParams[i].dfScaleDstMax;
        }
        else if( psOptions->nScaleRepeat == 1 && !psOptions->bHasUsedExplicitScaleBand )
        {
            bScale = psOptions->pasScaleParams[0].bScale;
            bHaveScaleSrc = psOptions->pasScaleParams[0].bHaveScaleSrc;
            dfScaleSrcMin = psOptions->pasScaleParams[0].dfScaleSrcMin;
            dfScaleSrcMax = psOptions->pasScaleParams[0].dfScaleSrcMax;
            dfScaleDstMin = psOptions->pasScaleParams[0].dfScaleDstMin;
            dfScaleDstMax = psOptions->pasScaleParams[0].dfScaleDstMax;
        }

        if( i < psOptions->nExponentRepeat && psOptions->padfExponent[i] != 0.0 )
        {
            bExponentScaling = TRUE;
            dfExponent = psOptions->padfExponent[i];
        }
        else if( psOptions->nExponentRepeat == 1 && !psOptions->bHasUsedExplicitExponentBand )
        {
            bExponentScaling = TRUE;
            dfExponent = psOptions->padfExponent[0];
        }

        if( bExponentScaling && !bScale )
        {
            CPLError( CE_Failure, CPLE_IllegalArg, "For band %d, -scale should be specified when -exponent is specified.", i + 1);
            if(bUsageError)
                *bUsageError = TRUE;
            return NULL;
        }

        if( bScale && !bHaveScaleSrc )
        {
            double	adfCMinMax[2];
            GDALComputeRasterMinMax( poSrcBand, TRUE, adfCMinMax );
            dfScaleSrcMin = adfCMinMax[0];
            dfScaleSrcMax = adfCMinMax[1];
        }

        if( bScale )
        {
            /* To avoid a divide by zero */
            if( dfScaleSrcMax == dfScaleSrcMin )
                dfScaleSrcMax += 0.1;

            if( !bExponentScaling )
            {
                dfScale = (dfScaleDstMax - dfScaleDstMin) 
                    / (dfScaleSrcMax - dfScaleSrcMin);
                dfOffset = -1 * dfScaleSrcMin * dfScale + dfScaleDstMin;
            }
        }

        if( psOptions->bUnscale )
        {
            dfScale = poSrcBand->GetScale();
            dfOffset = poSrcBand->GetOffset();
        }

/* -------------------------------------------------------------------- */
/*      Create a simple or complex data source depending on the         */
/*      translation type required.                                      */
/* -------------------------------------------------------------------- */
        VRTSimpleSource* poSimpleSource;
        if( psOptions->bUnscale || bScale || (psOptions->nRGBExpand != 0 && i < psOptions->nRGBExpand) )
        {
            VRTComplexSource* poSource = new VRTComplexSource();

        /* -------------------------------------------------------------------- */
        /*      Set complex parameters.                                         */
        /* -------------------------------------------------------------------- */

            if( dfOffset != 0.0 || dfScale != 1.0 )
            {
                poSource->SetLinearScaling(dfOffset, dfScale);
            }
            else if( bExponentScaling )
            {
                poSource->SetPowerScaling(dfExponent,
                                          dfScaleSrcMin,
                                          dfScaleSrcMax,
                                          dfScaleDstMin,
                                          dfScaleDstMax);
            }

            poSource->SetColorTableComponent(nComponent);

            poSimpleSource = poSource;
        }
        else
            poSimpleSource = new VRTSimpleSource();

        poSimpleSource->SetResampling(psOptions->pszResampling);
        poVRTBand->ConfigureSource( poSimpleSource,
                                    poSrcBand,
                                    FALSE,
                                    psOptions->anSrcWin[0], psOptions->anSrcWin[1],
                                    psOptions->anSrcWin[2], psOptions->anSrcWin[3],
                                    anDstWin[0], anDstWin[1],
                                    anDstWin[2], anDstWin[3] );

        poVRTBand->AddSource( poSimpleSource );

/* -------------------------------------------------------------------- */
/*      In case of color table translate, we only set the color         */
/*      interpretation other info copied by CopyBandInfo are            */
/*      not relevant in RGB expansion.                                  */
/* -------------------------------------------------------------------- */
        if (psOptions->nRGBExpand == 1)
        {
            poVRTBand->SetColorInterpretation( GCI_GrayIndex );
        }
        else if (psOptions->nRGBExpand != 0 && i < psOptions->nRGBExpand)
        {
            poVRTBand->SetColorInterpretation( (GDALColorInterp) (GCI_RedBand + i) );
        }

/* -------------------------------------------------------------------- */
/*      copy over some other information of interest.                   */
/* -------------------------------------------------------------------- */
        else
        {
            CopyBandInfo( poSrcBand, poVRTBand,
                          !psOptions->bStats && !bFilterOutStatsMetadata,
                          !psOptions->bUnscale,
                          !psOptions->bSetNoData && !psOptions->bUnsetNoData );
        }

/* -------------------------------------------------------------------- */
/*      Set a forcable nodata value?                                    */
/* -------------------------------------------------------------------- */
        if( psOptions->bSetNoData )
        {
            double dfVal = psOptions->dfNoDataReal;
            int bClamped = FALSE, bRounded = FALSE;

#define CLAMP(val,type,minval,maxval) \
    do { if (val < minval) { bClamped = TRUE; val = minval; } \
    else if (val > maxval) { bClamped = TRUE; val = maxval; } \
    else if (val != (type)val) { bRounded = TRUE; val = (type)(val + 0.5); } } \
    while(0)

            switch(eBandType)
            {
                case GDT_Byte:
                    CLAMP(dfVal, GByte, 0.0, 255.0);
                    break;
                case GDT_Int16:
                    CLAMP(dfVal, GInt16, -32768.0, 32767.0);
                    break;
                case GDT_UInt16:
                    CLAMP(dfVal, GUInt16, 0.0, 65535.0);
                    break;
                case GDT_Int32:
                    CLAMP(dfVal, GInt32, -2147483648.0, 2147483647.0);
                    break;
                case GDT_UInt32:
                    CLAMP(dfVal, GUInt32, 0.0, 4294967295.0);
                    break;
                default:
                    break;
            }
                
            if (bClamped)
            {
                CPLError( CE_Warning, CPLE_AppDefined, "for band %d, nodata value has been clamped "
                       "to %.0f, the original value being out of range.\n",
                       i + 1, dfVal);
            }
            else if(bRounded)
            {
                CPLError( CE_Warning, CPLE_AppDefined, "for band %d, nodata value has been rounded "
                       "to %.0f, %s being an integer datatype.\n",
                       i + 1, dfVal,
                       GDALGetDataTypeName(eBandType));
            }
            
            poVRTBand->SetNoDataValue( dfVal );
        }

        if (psOptions->eMaskMode == MASK_AUTO &&
            (GDALGetMaskFlags(GDALGetRasterBand(hDataset, 1)) & GMF_PER_DATASET) == 0 &&
            (poSrcBand->GetMaskFlags() & (GMF_ALL_VALID | GMF_NODATA)) == 0)
        {
            if (poVRTBand->CreateMaskBand(poSrcBand->GetMaskFlags()) == CE_None)
            {
                VRTSourcedRasterBand* hMaskVRTBand =
                    (VRTSourcedRasterBand*)poVRTBand->GetMaskBand();
                hMaskVRTBand->AddMaskBandSource(poSrcBand,
                                        psOptions->anSrcWin[0], psOptions->anSrcWin[1],
                                        psOptions->anSrcWin[2], psOptions->anSrcWin[3],
                                        anDstWin[0], anDstWin[1],
                                        anDstWin[2], anDstWin[3] );
            }
        }
    }

    if (psOptions->eMaskMode == MASK_USER)
    {
        GDALRasterBand *poSrcBand =
            (GDALRasterBand*)GDALGetRasterBand(hDataset, ABS(psOptions->nMaskBand));
        if (poSrcBand && poVDS->CreateMaskBand(GMF_PER_DATASET) == CE_None)
        {
            VRTSourcedRasterBand* hMaskVRTBand = (VRTSourcedRasterBand*)
                GDALGetMaskBand(GDALGetRasterBand((GDALDatasetH)poVDS, 1));
            if (psOptions->nMaskBand > 0)
                hMaskVRTBand->AddSimpleSource(poSrcBand,
                                        psOptions->anSrcWin[0], psOptions->anSrcWin[1],
                                        psOptions->anSrcWin[2], psOptions->anSrcWin[3],
                                        anDstWin[0], anDstWin[1],
                                        anDstWin[2], anDstWin[3] );
            else
                hMaskVRTBand->AddMaskBandSource(poSrcBand,
                                        psOptions->anSrcWin[0], psOptions->anSrcWin[1],
                                        psOptions->anSrcWin[2], psOptions->anSrcWin[3],
                                        anDstWin[0], anDstWin[1],
                                        anDstWin[2], anDstWin[3] );
        }
    }
    else
    if (psOptions->eMaskMode == MASK_AUTO && nSrcBandCount > 0 &&
        GDALGetMaskFlags(GDALGetRasterBand(hDataset, 1)) == GMF_PER_DATASET)
    {
        if (poVDS->CreateMaskBand(GMF_PER_DATASET) == CE_None)
        {
            VRTSourcedRasterBand* hMaskVRTBand = (VRTSourcedRasterBand*)
                GDALGetMaskBand(GDALGetRasterBand((GDALDatasetH)poVDS, 1));
            hMaskVRTBand->AddMaskBandSource((GDALRasterBand*)GDALGetRasterBand(hDataset, 1),
                                        psOptions->anSrcWin[0], psOptions->anSrcWin[1],
                                        psOptions->anSrcWin[2], psOptions->anSrcWin[3],
                                        anDstWin[0], anDstWin[1],
                                        anDstWin[2], anDstWin[3] );
        }
    }

/* -------------------------------------------------------------------- */
/*      Compute stats if required.                                      */
/* -------------------------------------------------------------------- */
    if (psOptions->bStats)
    {
        for( i = 0; i < poVDS->GetRasterCount(); i++ )
        {
            double dfMin, dfMax, dfMean, dfStdDev;
            poVDS->GetRasterBand(i+1)->ComputeStatistics( psOptions->bApproxStats,
                    &dfMin, &dfMax, &dfMean, &dfStdDev, GDALDummyProgress, NULL );
        }
    }

/* -------------------------------------------------------------------- */
/*      Write to the output file using CopyCreate().                    */
/* -------------------------------------------------------------------- */
    hOutDS = GDALCreateCopy( hDriver, pszDest, (GDALDatasetH) poVDS,
                             psOptions->bStrict, psOptions->papszCreateOptions, 
                             psOptions->pfnProgress, psOptions->pProgressData );
    if( hOutDS != NULL )
    {
        int bHasGotErr = FALSE;
        CPLErrorReset();
        GDALFlushCache( hOutDS );
        if (CPLGetLastErrorType() != CE_None)
            bHasGotErr = TRUE;
        if (bHasGotErr)
            hOutDS = NULL;
    }
    
    GDALClose( (GDALDatasetH) poVDS );

    return hOutDS;
}


/************************************************************************/
/*                           AttachMetadata()                           */
/************************************************************************/

static void AttachMetadata( GDALDatasetH hDS, char **papszMetadataOptions )

{
    int nCount = CSLCount(papszMetadataOptions);
    int i;

    for( i = 0; i < nCount; i++ )
    {
        char    *pszKey = NULL;
        const char *pszValue;
        
        pszValue = CPLParseNameValue( papszMetadataOptions[i], &pszKey );
        GDALSetMetadataItem(hDS,pszKey,pszValue,NULL);
        CPLFree( pszKey );
    }

    CSLDestroy( papszMetadataOptions );
}

/************************************************************************/
/*                           CopyBandInfo()                            */
/************************************************************************/

/* A bit of a clone of VRTRasterBand::CopyCommonInfoFrom(), but we need */
/* more and more custom behaviour in the context of gdal_translate ... */

static void CopyBandInfo( GDALRasterBand * poSrcBand, GDALRasterBand * poDstBand,
                          int bCanCopyStatsMetadata, int bCopyScale, int bCopyNoData )

{
    int bSuccess;
    double dfNoData;

    if (bCanCopyStatsMetadata)
    {
        poDstBand->SetMetadata( poSrcBand->GetMetadata() );
    }
    else
    {
        char** papszMetadata = poSrcBand->GetMetadata();
        char** papszMetadataNew = NULL;
        for( int i = 0; papszMetadata != NULL && papszMetadata[i] != NULL; i++ )
        {
            if (strncmp(papszMetadata[i], "STATISTICS_", 11) != 0)
                papszMetadataNew = CSLAddString(papszMetadataNew, papszMetadata[i]);
        }
        poDstBand->SetMetadata( papszMetadataNew );
        CSLDestroy(papszMetadataNew);
    }

    poDstBand->SetColorTable( poSrcBand->GetColorTable() );
    poDstBand->SetColorInterpretation(poSrcBand->GetColorInterpretation());
    if( strlen(poSrcBand->GetDescription()) > 0 )
        poDstBand->SetDescription( poSrcBand->GetDescription() );

    if (bCopyNoData)
    {
        dfNoData = poSrcBand->GetNoDataValue( &bSuccess );
        if( bSuccess )
            poDstBand->SetNoDataValue( dfNoData );
    }

    if (bCopyScale)
    {
        poDstBand->SetOffset( poSrcBand->GetOffset() );
        poDstBand->SetScale( poSrcBand->GetScale() );
    }

    poDstBand->SetCategoryNames( poSrcBand->GetCategoryNames() );

    // Copy unit only if the range of pixel values is not modified
    if( bCanCopyStatsMetadata && bCopyScale && !EQUAL(poSrcBand->GetUnitType(),"") )
        poDstBand->SetUnitType( poSrcBand->GetUnitType() );
}

/************************************************************************/
/*                             GDALTranslateOptionsNew()                */
/************************************************************************/

/**
 * allocates a GDALTranslateOptions struct.
 *
 * @return pointer to the allocated GDALTranslateOptions struct.
 */

GDALTranslateOptions *GDALTranslateOptionsNew()
{
    GDALTranslateOptions *psOptions = (GDALTranslateOptions *) CPLCalloc( 1, sizeof(GDALTranslateOptions) );

    psOptions->pszFormat = CPLStrdup("GTiff");
    psOptions->bQuiet = TRUE;
    psOptions->pfnProgress = GDALDummyProgress;
    psOptions->pProgressData = NULL;
    psOptions->eOutputType = GDT_Unknown;
    psOptions->eMaskMode = MASK_AUTO;
    psOptions->nBandCount = 0;
    psOptions->panBandList = NULL;
    psOptions->nOXSizePixel = 0;
    psOptions->nOYSizePixel = 0;
    psOptions->dfOXSizePct = 0.0;
    psOptions->dfOYSizePct = 0.0;
    psOptions->papszCreateOptions = NULL;
    psOptions->anSrcWin[0] = 0;
    psOptions->anSrcWin[1] = 0;
    psOptions->anSrcWin[2] = 0;
    psOptions->anSrcWin[3] = 0;
    psOptions->bStrict = FALSE;
    psOptions->bUnscale = FALSE;
    psOptions->nScaleRepeat = 0;
    psOptions->pasScaleParams = NULL;
    psOptions->bHasUsedExplicitScaleBand = FALSE;
    psOptions->nExponentRepeat = 0;
    psOptions->padfExponent = NULL;
    psOptions->bHasUsedExplicitExponentBand = FALSE;
    psOptions->dfULX = 0.0;
    psOptions->dfULY = 0.0;
    psOptions->dfLRX = 0.0;
    psOptions->dfLRY = 0.0;
    psOptions->papszMetadataOptions = NULL;
    psOptions->pszOutputSRS = NULL;
    psOptions->nGCPCount = 0;
    psOptions->pasGCPs = NULL;
    psOptions->adfULLR[0] = 0;
    psOptions->adfULLR[1] = 0;
    psOptions->adfULLR[2] = 0;
    psOptions->adfULLR[3] = 0;
    psOptions->bSetNoData = FALSE;
    psOptions->bUnsetNoData = FALSE;
    psOptions->dfNoDataReal = 0.0;
    psOptions->nRGBExpand = 0;
    psOptions->nMaskBand = 0;
    psOptions->bStats = FALSE;
    psOptions->bApproxStats = FALSE;
    psOptions->bErrorOnPartiallyOutside = FALSE;
    psOptions->bErrorOnCompletelyOutside = FALSE;
    psOptions->bNoRAT = FALSE;
    psOptions->pszResampling = NULL;
    psOptions->dfXRes = 0.0;
    psOptions->dfYRes = 0.0;
    psOptions->pszProjSRS = NULL;

    return psOptions;
}

/************************************************************************/
/*                        GDALTranslateOptionsFree()                    */
/************************************************************************/

/**
 * frees the GDALTranslateOptions struct.
 *
 * @param psOptions the options struct for GDALTranslate().
 */

void GDALTranslateOptionsFree(GDALTranslateOptions *psOptions)
{
    CPLFree(psOptions->pszFormat);
    CPLFree(psOptions->panBandList);
    CSLDestroy(psOptions->papszCreateOptions);
    CPLFree(psOptions->pasScaleParams);
    CPLFree(psOptions->padfExponent);
    CSLDestroy(psOptions->papszMetadataOptions);
    CPLFree(psOptions->pszOutputSRS);
    CPLFree(psOptions->pasGCPs);
    CPLFree(psOptions->pszResampling);
    CPLFree(psOptions->pszProjSRS);

    CPLFree(psOptions);
}

/************************************************************************/
/*                   GDALTranslateOptionsSetCreateOptions()             */
/************************************************************************/

/**
 * Set function for papszCreateOptions.
 *
 * @param psOptions the options struct for GDALTranslate().
 * @param papszCreateOption.
 */

void GDALTranslateOptionsSetCreateOptions( GDALTranslateOptions *psOptions,
                                       char **papszCreateOptions )
{
    CSLDestroy( psOptions->papszCreateOptions );
    psOptions->papszCreateOptions = CSLDuplicate( papszCreateOptions );
}

/************************************************************************/
/*                  GDALTranslateOptionsAddCreateOptions()              */
/************************************************************************/

/**
 * Specify the particular create option which need to be added.
 *
 * @param psOptions the options struct for GDALTranslate().
 * @param pszCreateOption the specific create option which need to be added.
 */

void GDALTranslateOptionsAddCreateOptions( GDALTranslateOptions *psOptions,
                                       const char *pszCreateOption )
{
    psOptions->papszCreateOptions = CSLAddString( psOptions->papszCreateOptions, pszCreateOption );
}

/************************************************************************/
/*                   GDALTranslateOptionsSetMetadataOptions()           */
/************************************************************************/

/**
 * Set function for papszMetadataOptions.
 *
 * @param psOptions the options struct for GDALTranslate().
 * @param papszMetadataOption.
 */

void GDALTranslateOptionsSetMetadataOptions( GDALTranslateOptions *psOptions,
                                       char **papszMetadataOptions )
{
    CSLDestroy( psOptions->papszMetadataOptions );
    psOptions->papszMetadataOptions = CSLDuplicate( papszMetadataOptions );
}

/************************************************************************/
/*                   GDALTranslateOptionsAddMetadataOptions()           */
/************************************************************************/

/**
 * Specify the particular metadata option which need to be added.
 *
 * @param psOptions the options struct for GDALTranslate().
 * @param pszMetadataOption the specific metadata option which need to be added.
 */

void GDALTranslateOptionsAddMetadataOptions( GDALTranslateOptions *psOptions,
                                       const char *pszMetadataOption )
{
    psOptions->papszMetadataOptions = CSLAddString( psOptions->papszMetadataOptions, pszMetadataOption );
}

/************************************************************************/
/*                              GDALTranslateOptionsAddBand()           */
/************************************************************************/

/**
 * Specify the particular bands which need to be in the converted dataset.
 *
 * @param psOptions the options struct for GDALTranslate().
 * @param nBand the band number.
 * @param bIsMask TRUE if the band is a mask band.
 */

void GDALTranslateOptionsAddBand( GDALTranslateOptions *psOptions, int nBand, int bIsMask )
{
    psOptions->nBandCount++;
    psOptions->panBandList = (int *) CPLRealloc(psOptions->panBandList, sizeof(int) * psOptions->nBandCount);
    psOptions->panBandList[psOptions->nBandCount-1] = nBand;
    if(bIsMask)
        psOptions->panBandList[psOptions->nBandCount-1] *= -1;
}
