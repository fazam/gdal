/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Simple client for translating between formats.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
 * Copyright (c) 2008-2014, Even Rouault <even dot rouault at mines-paris dot org>
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

#include "ogrsf_frmts.h"
#include "ogr_p.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "ogr_api.h"
#include "gdal.h"
#include "gdal_utils.h"
#include "gdal_alg.h"
#include "commonutils.h"
#include <map>
#include <vector>

CPL_CVSID("$Id$");

static void Usage(int bShort = TRUE);
static void Usage(const char* pszAdditionalMsg, int bShort = TRUE);
int CPL_DLL GetFieldType(const char* pszArg, int* pnSubFieldType);

/************************************************************************/
/*                            IsNumber()                               */
/************************************************************************/

static int IsNumber(const char* pszStr)
{
    if (*pszStr == '-' || *pszStr == '+')
        pszStr ++;
    if (*pszStr == '.')
        pszStr ++;
    return (*pszStr >= '0' && *pszStr <= '9');
}

/************************************************************************/
/*                           IsFieldType()                              */
/************************************************************************/

static int IsFieldType(const char* pszArg)
{
    int iSubType;
    return GetFieldType(pszArg, &iSubType) >= 0 && iSubType >= 0;
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/

#define CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(nExtraArg) \
    do { if (iArg + nExtraArg >= nArgc) \
        Usage(CPLSPrintf("%s option requires %d argument(s)", papszArgv[iArg], nExtraArg)); } while(0)

int main( int nArgc, char ** papszArgv )
{
    const char *pszDataSource = NULL;
    const char *pszDestDataSource = NULL;
    GDALDatasetH hDS = NULL;
    GDALDatasetH hODS = NULL;
    const char *pszSelect;
    char **papszOpenOptions = NULL;
    OGR2OGROptions *psOptions = OGR2OGROptionsNew();
    int bUsageError = FALSE;
    int bCloseODS = TRUE;
    
    /* Check strict compilation and runtime library version as we use C++ API */
    if (! GDAL_CHECK_VERSION(papszArgv[0]))
        exit(1);

    EarlySetConfigOptions(nArgc, papszArgv);

/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
    OGRRegisterAll();

/* -------------------------------------------------------------------- */
/*      Processing command line arguments.                              */
/* -------------------------------------------------------------------- */
    nArgc = OGRGeneralCmdLineProcessor( nArgc, &papszArgv, 0 );
    
    if( nArgc < 1 )
        exit( -nArgc );

    for( int iArg = 1; iArg < nArgc; iArg++ )
    {
        if( EQUAL(papszArgv[iArg], "--utility_version") )
        {
            printf("%s was compiled against GDAL %s and is running against GDAL %s\n",
                   papszArgv[0], GDAL_RELEASE_NAME, GDALVersionInfo("RELEASE_NAME"));
            return 0;
        }
        else if( EQUAL(papszArgv[iArg],"--help") )
            Usage();
        else if ( EQUAL(papszArgv[iArg], "--long-usage") )
        {
            Usage(FALSE);
        }

        else if( EQUAL(papszArgv[iArg],"-q") || EQUAL(papszArgv[iArg],"-quiet") )
        {
            psOptions->bQuiet = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-f") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszFormat = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-dsco") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszDSCO = CSLAddString(psOptions->papszDSCO, papszArgv[++iArg] );
        }
        else if( EQUAL(papszArgv[iArg],"-lco") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszLCO = CSLAddString(psOptions->papszLCO, papszArgv[++iArg] );
        }
        else if( EQUAL(papszArgv[iArg],"-oo") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            papszOpenOptions = CSLAddString(papszOpenOptions, papszArgv[++iArg] );
        }
        else if( EQUAL(papszArgv[iArg],"-doo") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszDestOpenOptions = CSLAddString(psOptions->papszDestOpenOptions, papszArgv[++iArg] );
        }
        else if( EQUAL(papszArgv[iArg],"-preserve_fid") )
        {
            psOptions->bPreserveFID = TRUE;
        }
        else if( EQUALN(papszArgv[iArg],"-skip",5) )
        {
            psOptions->bSkipFailures = TRUE;
            psOptions->nGroupTransactions = 1; /* #2409 */
        }
        else if( EQUAL(papszArgv[iArg],"-append") )
        {
            psOptions->eAccessMode = ACCESS_APPEND;
        }
        else if( EQUAL(papszArgv[iArg],"-overwrite") )
        {
            psOptions->eAccessMode = ACCESS_OVERWRITE;
        }
        else if( EQUAL(papszArgv[iArg],"-addfields") )
        {
            psOptions->bAddMissingFields = TRUE;
            psOptions->eAccessMode = ACCESS_APPEND;
        }
        else if( EQUAL(papszArgv[iArg],"-update") )
        {
            psOptions->eAccessMode = ACCESS_UPDATE;
        }
        else if( EQUAL(papszArgv[iArg],"-relaxedFieldNameMatch") )
        {
            psOptions->bExactFieldNameMatch = FALSE;
        }
        else if( EQUAL(papszArgv[iArg],"-fid") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->nFIDToFetch = CPLAtoGIntBig(papszArgv[++iArg]);
        }
        else if( EQUAL(papszArgv[iArg],"-sql") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszSQLStatement = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-dialect") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszDialect = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-nln") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszNewLayerName = CPLStrdup(papszArgv[++iArg]);
        }
        else if( EQUAL(papszArgv[iArg],"-nlt") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            int bIs3D = FALSE;
            CPLString osGeomName = papszArgv[iArg+1];
            if (strlen(papszArgv[iArg+1]) > 3 &&
                EQUALN(papszArgv[iArg+1] + strlen(papszArgv[iArg+1]) - 3, "25D", 3))
            {
                bIs3D = TRUE;
                osGeomName.resize(osGeomName.size() - 3);
            }
            else if (strlen(papszArgv[iArg+1]) > 1 &&
                EQUALN(papszArgv[iArg+1] + strlen(papszArgv[iArg+1]) - 1, "Z", 1))
            {
                bIs3D = TRUE;
                osGeomName.resize(osGeomName.size() - 1);
            }
            if( EQUAL(osGeomName,"NONE") )
                psOptions->eGType = wkbNone;
            else if( EQUAL(osGeomName,"GEOMETRY") )
                psOptions->eGType = wkbUnknown;
            else if( EQUAL(osGeomName,"PROMOTE_TO_MULTI") )
                psOptions->eGeomConversion = GEOMTYPE_PROMOTE_TO_MULTI;
            else if( EQUAL(osGeomName,"CONVERT_TO_LINEAR") )
                psOptions->eGeomConversion = GEOMTYPE_CONVERT_TO_LINEAR;
            else if( EQUAL(osGeomName,"CONVERT_TO_CURVE") )
                psOptions->eGeomConversion = GEOMTYPE_CONVERT_TO_CURVE;
            else
            {
                psOptions->eGType = OGRFromOGCGeomType(osGeomName);
                if (psOptions->eGType == wkbUnknown)
                {
                    fprintf( stderr, "-nlt %s: type not recognised.\n",
                            papszArgv[iArg+1] );
                    exit( 1 );
                }
            }
            if (psOptions->eGType != -2 && psOptions->eGType != wkbNone && bIs3D)
                psOptions->eGType = wkbSetZ((OGRwkbGeometryType)psOptions->eGType);

            iArg++;
        }
        else if( EQUAL(papszArgv[iArg],"-dim")  )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            if( EQUAL(papszArgv[iArg+1], "layer_dim") )
                psOptions->nCoordDim = COORD_DIM_LAYER_DIM;
            else
            {
                psOptions->nCoordDim = atoi(papszArgv[iArg+1]);
                if( psOptions->nCoordDim != 2 && psOptions->nCoordDim != 3 )
                {
                    fprintf( stderr, "-dim %s: value not handled.\n",
                            papszArgv[iArg+1] );
                    exit( 1 );
                }
            }
            iArg ++;
        }
        else if( EQUAL(papszArgv[iArg],"-tg") ||
                 EQUAL(papszArgv[iArg],"-gt") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            ++iArg;
            if( EQUAL(papszArgv[iArg], "unlimited") )
                psOptions->nGroupTransactions = -1;
            else
                psOptions->nGroupTransactions = atoi(papszArgv[iArg]);
        }
        else if ( EQUAL(papszArgv[iArg],"-ds_transaction") )
        {
            psOptions->bLayerTransaction = FALSE;
            psOptions->bForceTransaction = TRUE;
        }
        /* Undocumented. Just a provision. Default behaviour should be OK */
        else if ( EQUAL(papszArgv[iArg],"-lyr_transaction") )
        {
            psOptions->bLayerTransaction = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-s_srs") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszSourceSRSDef = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-a_srs") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszOutputSRSDef = papszArgv[++iArg];
            if (EQUAL(psOptions->pszOutputSRSDef, "NULL") ||
                EQUAL(psOptions->pszOutputSRSDef, "NONE"))
            {
                psOptions->pszOutputSRSDef = NULL;
                psOptions->bNullifyOutputSRS = TRUE;
            }
        }
        else if( EQUAL(papszArgv[iArg],"-t_srs") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszOutputSRSDef = papszArgv[++iArg];
            psOptions->bTransform = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-spat") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(4);
            OGRLinearRing  oRing;

            oRing.addPoint( CPLAtof(papszArgv[iArg+1]), CPLAtof(papszArgv[iArg+2]) );
            oRing.addPoint( CPLAtof(papszArgv[iArg+1]), CPLAtof(papszArgv[iArg+4]) );
            oRing.addPoint( CPLAtof(papszArgv[iArg+3]), CPLAtof(papszArgv[iArg+4]) );
            oRing.addPoint( CPLAtof(papszArgv[iArg+3]), CPLAtof(papszArgv[iArg+2]) );
            oRing.addPoint( CPLAtof(papszArgv[iArg+1]), CPLAtof(papszArgv[iArg+2]) );

            psOptions->hSpatialFilter = (OGRGeometryH) OGRGeometryFactory::createGeometry(wkbPolygon);
            ((OGRPolygon *) psOptions->hSpatialFilter)->addRing( &oRing );
            iArg += 4;
        }
        else if( EQUAL(papszArgv[iArg],"-spat_srs") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszSpatSRSDef = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-geomfield") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszGeomField = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-where") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszWHERE = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-select") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            pszSelect = papszArgv[++iArg];
            psOptions->papszSelFields = CSLTokenizeStringComplex(pszSelect, " ,", 
                                                      FALSE, FALSE );
        }
        else if( EQUAL(papszArgv[iArg],"-segmentize") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->eGeomOp = GEOMOP_SEGMENTIZE;
            psOptions->dfGeomOpParam = CPLAtof(papszArgv[++iArg]);
        }
        else if( EQUAL(papszArgv[iArg],"-simplify") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->eGeomOp = GEOMOP_SIMPLIFY_PRESERVE_TOPOLOGY;
            psOptions->dfGeomOpParam = CPLAtof(papszArgv[++iArg]);
        }
        else if( EQUAL(papszArgv[iArg],"-fieldTypeToString") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszFieldTypesToString =
                    CSLTokenizeStringComplex(papszArgv[++iArg], " ,", 
                                             FALSE, FALSE );
            char** iter = psOptions->papszFieldTypesToString;
            while(*iter)
            {
                if (IsFieldType(*iter))
                {
                    /* Do nothing */
                }
                else if (EQUAL(*iter, "All"))
                {
                    CSLDestroy(psOptions->papszFieldTypesToString);
                    psOptions->papszFieldTypesToString = NULL;
                    psOptions->papszFieldTypesToString = CSLAddString(psOptions->papszFieldTypesToString, "All");
                    break;
                }
                else
                {
                    Usage(CPLSPrintf("Unhandled type for fieldTypeToString option : %s",
                            *iter));
                }
                iter ++;
            }
        }
        else if( EQUAL(papszArgv[iArg],"-mapFieldType") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszMapFieldType =
                    CSLTokenizeStringComplex(papszArgv[++iArg], " ,", 
                                             FALSE, FALSE );
            char** iter = psOptions->papszMapFieldType;
            while(*iter)
            {
                char* pszKey = NULL;
                const char* pszValue = CPLParseNameValue(*iter, &pszKey);
                if( pszKey && pszValue)
                {
                    if( !((IsFieldType(pszKey) || EQUAL(pszKey, "All")) && IsFieldType(pszValue)) )
                    {
                        Usage("Invalid value for -mapFieldType");
                    }
                }
                CPLFree(pszKey);
                iter ++;
            }
        }
        else if( EQUAL(papszArgv[iArg],"-unsetFieldWidth") )
        {
            psOptions->bUnsetFieldWidth = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-progress") )
        {
            psOptions->bDisplayProgress = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-wrapdateline") )
        {
            psOptions->bWrapDateline = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-datelineoffset") && iArg < nArgc-1 )
        {
            psOptions->nDateLineOffset = atoi(papszArgv[++iArg]);
        }        
        else if( EQUAL(papszArgv[iArg],"-clipsrc") )
        {
            if (iArg + 1 >= nArgc)
                Usage(CPLSPrintf("%s option requires 1 or 4 arguments", papszArgv[iArg]));

            VSIStatBufL  sStat;
            psOptions->bClipSrc = TRUE;
            if ( IsNumber(papszArgv[iArg+1])
                 && papszArgv[iArg+2] != NULL 
                 && papszArgv[iArg+3] != NULL 
                 && papszArgv[iArg+4] != NULL)
            {
                OGRLinearRing  oRing;

                oRing.addPoint( CPLAtof(papszArgv[iArg+1]), CPLAtof(papszArgv[iArg+2]) );
                oRing.addPoint( CPLAtof(papszArgv[iArg+1]), CPLAtof(papszArgv[iArg+4]) );
                oRing.addPoint( CPLAtof(papszArgv[iArg+3]), CPLAtof(papszArgv[iArg+4]) );
                oRing.addPoint( CPLAtof(papszArgv[iArg+3]), CPLAtof(papszArgv[iArg+2]) );
                oRing.addPoint( CPLAtof(papszArgv[iArg+1]), CPLAtof(papszArgv[iArg+2]) );

                psOptions->hClipSrc = (OGRGeometryH) OGRGeometryFactory::createGeometry(wkbPolygon);
                ((OGRPolygon *) psOptions->hClipSrc)->addRing( &oRing );
                iArg += 4;
            }
            else if ((EQUALN(papszArgv[iArg+1], "POLYGON", 7) ||
                      EQUALN(papszArgv[iArg+1], "MULTIPOLYGON", 12)) &&
                      VSIStatL(papszArgv[iArg+1], &sStat) != 0)
            {
                char* pszTmp = (char*) papszArgv[iArg+1];
                OGRGeometryFactory::createFromWkt(&pszTmp, NULL, (OGRGeometry **)&psOptions->hClipSrc);
                if (psOptions->hClipSrc == NULL)
                {
                    Usage("Invalid geometry. Must be a valid POLYGON or MULTIPOLYGON WKT");
                }
                iArg ++;
            }
            else if (EQUAL(papszArgv[iArg+1], "spat_extent") )
            {
                iArg ++;
            }
            else
            {
                psOptions->pszClipSrcDS = papszArgv[iArg+1];
                iArg ++;
            }
        }
        else if( EQUAL(papszArgv[iArg],"-clipsrcsql")  )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszClipSrcSQL = papszArgv[iArg+1];
            iArg ++;
        }
        else if( EQUAL(papszArgv[iArg],"-clipsrclayer") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszClipSrcLayer = papszArgv[iArg+1];
            iArg ++;
        }
        else if( EQUAL(papszArgv[iArg],"-clipsrcwhere") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszClipSrcWhere = papszArgv[iArg+1];
            iArg ++;
        }
        else if( EQUAL(papszArgv[iArg],"-clipdst") )
        {
            if (iArg + 1 >= nArgc)
                Usage(CPLSPrintf("%s option requires 1 or 4 arguments", papszArgv[iArg]));

            VSIStatBufL  sStat;
            if ( IsNumber(papszArgv[iArg+1])
                 && papszArgv[iArg+2] != NULL 
                 && papszArgv[iArg+3] != NULL 
                 && papszArgv[iArg+4] != NULL)
            {
                OGRLinearRing  oRing;

                oRing.addPoint( CPLAtof(papszArgv[iArg+1]), CPLAtof(papszArgv[iArg+2]) );
                oRing.addPoint( CPLAtof(papszArgv[iArg+1]), CPLAtof(papszArgv[iArg+4]) );
                oRing.addPoint( CPLAtof(papszArgv[iArg+3]), CPLAtof(papszArgv[iArg+4]) );
                oRing.addPoint( CPLAtof(papszArgv[iArg+3]), CPLAtof(papszArgv[iArg+2]) );
                oRing.addPoint( CPLAtof(papszArgv[iArg+1]), CPLAtof(papszArgv[iArg+2]) );

                psOptions->hClipDst = (OGRGeometryH) OGRGeometryFactory::createGeometry(wkbPolygon);
                ((OGRPolygon *) psOptions->hClipDst)->addRing( &oRing );
                iArg += 4;
            }
            else if ((EQUALN(papszArgv[iArg+1], "POLYGON", 7) ||
                      EQUALN(papszArgv[iArg+1], "MULTIPOLYGON", 12)) &&
                      VSIStatL(papszArgv[iArg+1], &sStat) != 0)
            {
                char* pszTmp = (char*) papszArgv[iArg+1];
                OGRGeometryFactory::createFromWkt(&pszTmp, NULL, (OGRGeometry **)&psOptions->hClipDst);
                if (psOptions->hClipDst == NULL)
                {
                    Usage("Invalid geometry. Must be a valid POLYGON or MULTIPOLYGON WKT");
                }
                iArg ++;
            }
            else
            {
                psOptions->pszClipDstDS = papszArgv[iArg+1];
                iArg ++;
            }
        }
        else if( EQUAL(papszArgv[iArg],"-clipdstsql") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszClipDstSQL = papszArgv[iArg+1];
            iArg ++;
        }
        else if( EQUAL(papszArgv[iArg],"-clipdstlayer") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszClipDstLayer = papszArgv[iArg+1];
            iArg ++;
        }
        else if( EQUAL(papszArgv[iArg],"-clipdstwhere") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszClipDstWhere = papszArgv[iArg+1];
            iArg ++;
        }
        else if( EQUAL(papszArgv[iArg],"-splitlistfields") )
        {
            psOptions->bSplitListFields = TRUE;
        }
        else if ( EQUAL(papszArgv[iArg],"-maxsubfields") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            if (IsNumber(papszArgv[iArg+1]))
            {
                int nTemp = atoi(papszArgv[iArg+1]);
                if (nTemp > 0)
                {
                    psOptions->nMaxSplitListSubFields = nTemp;
                    iArg ++;
                }
            }
        }
        else if( EQUAL(papszArgv[iArg],"-explodecollections") )
        {
            psOptions->bExplodeCollections = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-zfield") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->pszZField = papszArgv[iArg+1];
            iArg ++;
        }
        else if( EQUAL(papszArgv[iArg],"-gcp") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(4);
            char* endptr = NULL;
            /* -gcp pixel line easting northing [elev] */

            psOptions->nGCPCount++;
            psOptions->pasGCPs = (GDAL_GCP *) 
                CPLRealloc( psOptions->pasGCPs, sizeof(GDAL_GCP) * psOptions->nGCPCount );
            GDALInitGCPs( 1, psOptions->pasGCPs + psOptions->nGCPCount - 1 );

            psOptions->pasGCPs[psOptions->nGCPCount-1].dfGCPPixel = CPLAtof(papszArgv[++iArg]);
            psOptions->pasGCPs[psOptions->nGCPCount-1].dfGCPLine = CPLAtof(papszArgv[++iArg]);
            psOptions->pasGCPs[psOptions->nGCPCount-1].dfGCPX = CPLAtof(papszArgv[++iArg]);
            psOptions->pasGCPs[psOptions->nGCPCount-1].dfGCPY = CPLAtof(papszArgv[++iArg]);
            if( papszArgv[iArg+1] != NULL 
                && (CPLStrtod(papszArgv[iArg+1], &endptr) != 0.0 || papszArgv[iArg+1][0] == '0') )
            {
                /* Check that last argument is really a number and not a filename */
                /* looking like a number (see ticket #863) */
                if (endptr && *endptr == 0)
                    psOptions->pasGCPs[psOptions->nGCPCount-1].dfGCPZ = CPLAtof(papszArgv[++iArg]);
            }

            /* should set id and info? */
        }
        else if( EQUAL(papszArgv[iArg],"-tps") )
        {
            psOptions->nTransformOrder = -1;
        }
        else if( EQUAL(papszArgv[iArg],"-order") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->nTransformOrder = atoi( papszArgv[++iArg] );
        }
        else if( EQUAL(papszArgv[iArg],"-fieldmap") && papszArgv[iArg+1] != NULL)
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszFieldMap = CSLTokenizeStringComplex(papszArgv[++iArg], ",", 
                                                      FALSE, FALSE );
        }
        else if( EQUAL(papszArgv[iArg],"-forceNullable") )
        {
            psOptions->bForceNullable = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-unsetDefault") )
        {
            psOptions->bUnsetDefault = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-unsetFid") )
        {
            psOptions->bUnsetFid = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-nomd") )
        {
            psOptions->bCopyMD = FALSE;
        }
        else if( EQUAL(papszArgv[iArg],"-mo") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            psOptions->papszMetadataOptions = CSLAddString( psOptions->papszMetadataOptions,
                                                 papszArgv[++iArg] );
        }
        else if( papszArgv[iArg][0] == '-' )
        {
            Usage(CPLSPrintf("Unknown option name '%s'", papszArgv[iArg]));
        }
        else if( pszDestDataSource == NULL )
            pszDestDataSource = papszArgv[iArg];
        else if( pszDataSource == NULL )
            pszDataSource = papszArgv[iArg];
        else
            psOptions->papszLayers = CSLAddString( psOptions->papszLayers, papszArgv[iArg] );
    }

    if( pszDataSource == NULL )
    {
        if( pszDestDataSource == NULL )
            Usage("no target datasource provided");
        else
            Usage("no source datasource provided");
    }

/* -------------------------------------------------------------------- */
/*      Open data source.                                               */
/* -------------------------------------------------------------------- */

    hDS = GDALOpenEx( pszDataSource,
                      GDAL_OF_VECTOR, NULL, papszOpenOptions, NULL );

/* -------------------------------------------------------------------- */
/*      Report failure                                                  */
/* -------------------------------------------------------------------- */
    if( hDS == NULL )
    {
        OGRSFDriverRegistrar    *poR = OGRSFDriverRegistrar::GetRegistrar();
        
        fprintf( stderr, "FAILURE:\n"
                "Unable to open datasource `%s' with the following drivers.\n",
                pszDataSource );

        for( int iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
        {
            fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetDescription() );
        }

        exit( 1 );
    }


    hODS = OGR2OGR( pszDestDataSource, NULL, hDS, psOptions, &bUsageError, &bCloseODS );

    if(bUsageError==TRUE)
        Usage();

    if(hDS)
        GDALClose(hDS);
    if(bCloseODS)
        GDALClose(hODS);

    OGR2OGROptionsFree( psOptions );
    CSLDestroy( papszOpenOptions );

    OGRCleanupAll();

#ifdef DBMALLOC
    malloc_dump(1);
#endif

}



/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage(int bShort)
{
    Usage(NULL, bShort);
}

static void Usage(const char* pszAdditionalMsg, int bShort)

{
    OGRSFDriverRegistrar        *poR = OGRSFDriverRegistrar::GetRegistrar();


    printf( "Usage: ogr2ogr [--help-general] [-skipfailures] [-append] [-update]\n"
            "               [-select field_list] [-where restricted_where]\n"
            "               [-progress] [-sql <sql statement>] [-dialect dialect]\n"
            "               [-preserve_fid] [-fid FID]\n"
            "               [-spat xmin ymin xmax ymax] [-spat_srs srs_def] [-geomfield field]\n"
            "               [-a_srs srs_def] [-t_srs srs_def] [-s_srs srs_def]\n"
            "               [-f format_name] [-overwrite] [[-dsco NAME=VALUE] ...]\n"
            "               dst_datasource_name src_datasource_name\n"
            "               [-lco NAME=VALUE] [-nln name] \n"
            "               [-nlt type|PROMOTE_TO_MULTI|CONVERT_TO_LINEAR]\n"
            "               [-dim 2|3|layer_dim] [layer [layer ...]]\n"
            "\n"
            "Advanced options :\n"
            "               [-gt n] [-ds_transaction]\n"
            "               [[-oo NAME=VALUE] ...] [[-doo NAME=VALUE] ...]\n"
            "               [-clipsrc [xmin ymin xmax ymax]|WKT|datasource|spat_extent]\n"
            "               [-clipsrcsql sql_statement] [-clipsrclayer layer]\n"
            "               [-clipsrcwhere expression]\n"
            "               [-clipdst [xmin ymin xmax ymax]|WKT|datasource]\n"
            "               [-clipdstsql sql_statement] [-clipdstlayer layer]\n"
            "               [-clipdstwhere expression]\n"
            "               [-wrapdateline][-datelineoffset val]\n"
            "               [[-simplify tolerance] | [-segmentize max_dist]]\n"
            "               [-addfields] [-unsetFid]\n"
            "               [-relaxedFieldNameMatch] [-forceNullable] [-unsetDefault]\n"
            "               [-fieldTypeToString All|(type1[,type2]*)] [-unsetFieldWidth]\n"
            "               [-mapFieldType srctype|All=dsttype[,srctype2=dsttype2]*]\n"
            "               [-fieldmap identity | index1[,index2]*]\n"
            "               [-splitlistfields] [-maxsubfields val]\n"
            "               [-explodecollections] [-zfield field_name]\n"
            "               [-gcp pixel line easting northing [elevation]]* [-order n | -tps]\n"
            "               [-nomd] [-mo \"META-TAG=VALUE\"]*\n");

    if (bShort)
    {
        printf( "\nNote: ogr2ogr --long-usage for full help.\n");
        if( pszAdditionalMsg )
            fprintf(stderr, "\nFAILURE: %s\n", pszAdditionalMsg);
        exit( 1 );
    }

    printf("\n -f format_name: output file format name, possible values are:\n");

    for( int iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
    {
        GDALDriver *poDriver = poR->GetDriver(iDriver);

        if( CSLTestBoolean( CSLFetchNameValueDef(poDriver->GetMetadata(), GDAL_DCAP_CREATE, "FALSE") ) )
            printf( "     -f \"%s\"\n", poDriver->GetDescription() );
    }

    printf( " -append: Append to existing layer instead of creating new if it exists\n"
            " -overwrite: delete the output layer and recreate it empty\n"
            " -update: Open existing output datasource in update mode\n"
            " -progress: Display progress on terminal. Only works if input layers have the \n"
            "                                          \"fast feature count\" capability\n"
            " -select field_list: Comma-delimited list of fields from input layer to\n"
            "                     copy to the new layer (defaults to all)\n" 
            " -where restricted_where: Attribute query (like SQL WHERE)\n" 
            " -wrapdateline: split geometries crossing the dateline meridian\n"
            "                (long. = +/- 180deg)\n" 
            " -datelineoffset: offset from dateline in degrees\n"
            "                (default long. = +/- 10deg,\n"
            "                geometries within 170deg to -170deg will be splited)\n" 
            " -sql statement: Execute given SQL statement and save result.\n"
            " -dialect value: select a dialect, usually OGRSQL to avoid native sql.\n"
            " -skipfailures: skip features or layers that fail to convert\n"
            " -gt n: group n features per transaction (default 20000). n can be set to unlimited\n"
            " -spat xmin ymin xmax ymax: spatial query extents\n"
            " -simplify tolerance: distance tolerance for simplification.\n"
            " -segmentize max_dist: maximum distance between 2 nodes.\n"
            "                       Used to create intermediate points\n"
            " -dsco NAME=VALUE: Dataset creation option (format specific)\n"
            " -lco  NAME=VALUE: Layer creation option (format specific)\n"
            " -oo   NAME=VALUE: Input dataset open option (format specific)\n"
            " -doo  NAME=VALUE: Destination dataset open option (format specific)\n"
            " -nln name: Assign an alternate name to the new layer\n"
            " -nlt type: Force a geometry type for new layer.  One of NONE, GEOMETRY,\n"
            "      POINT, LINESTRING, POLYGON, GEOMETRYCOLLECTION, MULTIPOINT,\n"
            "      MULTIPOLYGON, or MULTILINESTRING, or PROMOTE_TO_MULTI.  Add \"25D\" for 3D layers.\n"
            "      Default is type of source layer.\n"
            " -dim dimension: Force the coordinate dimension to the specified value.\n"
            " -fieldTypeToString type1,...: Converts fields of specified types to\n"
            "      fields of type string in the new layer. Valid types are : Integer,\n"
            "      Integer64, Real, String, Date, Time, DateTime, Binary, IntegerList, Integer64List, RealList,\n"
            "      StringList. Special value All will convert all fields to strings.\n"
            " -fieldmap index1,index2,...: Specifies the list of field indexes to be\n"
            "      copied from the source to the destination. The (n)th value specified\n"
            "      in the list is the index of the field in the target layer definition\n"
            "      in which the n(th) field of the source layer must be copied. Index count\n"
            "      starts at zero. There must be exactly as many values in the list as\n"
            "      the count of the fields in the source layer. We can use the 'identity'\n"
            "      setting to specify that the fields should be transferred by using the\n"
            "      same order. This setting should be used along with the append setting.");

    printf(" -a_srs srs_def: Assign an output SRS\n"
           " -t_srs srs_def: Reproject/transform to this SRS on output\n"
           " -s_srs srs_def: Override source SRS\n"
           "\n" 
           " Srs_def can be a full WKT definition (hard to escape properly),\n"
           " or a well known definition (ie. EPSG:4326) or a file with a WKT\n"
           " definition.\n" );

    if( pszAdditionalMsg )
        fprintf(stderr, "\nFAILURE: %s\n", pszAdditionalMsg);

    exit( 1 );
}
