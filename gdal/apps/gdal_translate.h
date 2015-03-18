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
#include "cpl_vsi.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "vrtdataset.h"
#include "commonutils.h"

#define CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(nExtraArg) \
    do { if (i + nExtraArg >= argc) \
        Usage(CPLSPrintf("%s option requires %d argument(s)", argv[i], nExtraArg)); } while(0)

CPL_CVSID("$Id$");

static int ArgIsNumeric( const char * );
static void AttachMetadata( GDALDatasetH, char ** );
static void CopyBandInfo( GDALRasterBand * poSrcBand, GDALRasterBand * poDstBand,
                            int bCanCopyStatsMetadata, int bCopyScale, int bCopyNoData );

enum
{
    MASK_DISABLED,
    MASK_AUTO,
    MASK_USER
};

typedef struct
{
    int     bScale;
    int     bHaveScaleSrc;
    double  dfScaleSrcMin, dfScaleSrcMax;
    double  dfScaleDstMin, dfScaleDstMax;
} ScaleParams;

class GDALTranslate
{
public:

    GDALDatasetH hOutDS;
    int	nRasterXSize, nRasterYSize;
    const char *pszSource, *pszDest, *pszFormat;
    int bFormatExplicitlySet;
    GDALDriverH	hDriver;
    int	*panBandList; /* negative value of panBandList[i] means mask band of ABS(panBandList[i]) */
    int nBandCount, bDefBands;
    double adfGeoTransform[6];
    GDALDataType eOutputType;
    int	nOXSize, nOYSize;
    char *pszOXSize, *pszOYSize;
    char **papszCreateOptions;
    int  anSrcWin[4], bStrict;
    const char *pszProjection;

    int bUnscale;
    int nScaleRepeat;
    ScaleParams *pasScaleParams;
    int bHasUsedExplictScaleBand;
    int nExponentRepeat;
    double *padfExponent;
    int bHasUsedExplictExponentBand;

    double dfULX, dfULY, dfLRX, dfLRY;
    char **papszMetadataOptions;
    char *pszOutputSRS;
    int bQuiet, bGotBounds;
    GDALProgressFunc pfnProgress;
    int nGCPCount;
    GDAL_GCP *pasGCPs;
    int iSrcFileArg, iDstFileArg;
    int bCopySubDatasets;
    double adfULLR[4];
    int bSetNoData;
    int bUnsetNoData;
    double dfNoDataReal;
    int nRGBExpand;
    int bParsedMaskArgument;
    int eMaskMode;
    int nMaskBand; /* negative value means mask band of ABS(nMaskBand) */
    int bStats, bApproxStats;
    int bErrorOnPartiallyOutside;
    int bErrorOnCompletelyOutside;
    int bNoRAT;
    char **papszOpenOptions;
    const char *pszResampling;
    double dfXRes, dfYRes;

    GDALTranslate();
    ~GDALTranslate();
    CPLErr parse(int argc, char ** argv);
    CPLErr validate();
    void setOutputFormat(char *pszFormat);
    void setQuiet();
    CPLErr setOutputType(char *pszOutputType);
    CPLErr setBand(char *pszBand);
    CPLErr setMask(char *pszBand);
    void setStrict(bool bStrict);
    void setCopySubDatasets();
    void setNodataValue(char *pszNodataValue);
    void setGeoreferenceBounds(char *pszUlx, char *pszUly, char *pszLrx, char *pszLry);
    void addCreationOption(char *pszCreationOption);
    CPLErr setExpand(char *pszExpand);
    void addMetadata(char *pszMetadata);
    void setOutSize(char *pszxsize, char *pszysize);
    CPLErr setTargetResolution(char *pszxres, char *pszyres);
    void setSrcwin(char *pszxoff, char *pszyoff, char *pszxsize, char *pszysize);
    void setProjwin(char *pszulx, char *pszuly, char *pszlrx, char *pszlry);
    void setErrorOnPartiallyOutside();
    void setErrorOnCompletelyOutside();
    CPLErr setSRS(char *pszSRS);
    void setApproxStats(bool bApproxStats);
    void setNoRAT();
    void setOpenOptions(char *pszOpenOption);
    void setResampling(char *pszResampling);
    void setSource(char *pszSource);
    void setDest(char *pszDest);
    CPLErr gdal_translate(GDALDatasetH hDataset, GDALDatasetH *phOutDataset,
        GDALProgressFunc pfnProgress, void *pProgressArg);

};
