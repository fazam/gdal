/******************************************************************************
 * $Id$
 *
 * Name:     gdal.i
 * Project:  GDAL Python Interface
 * Purpose:  GDAL Core SWIG Interface declarations.
 * Author:   Kevin Ruland, kruland@ku.edu
 *
 ******************************************************************************
 * Copyright (c) 2005, Kevin Ruland
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
 *****************************************************************************/

%include constraints.i

#ifdef PERL_CPAN_NAMESPACE
%module "Geo::GDAL"
#elif defined(SWIGCSHARP)
%module Gdal
#else
%module gdal
#endif

#ifdef SWIGCSHARP
%include swig_csharp_extensions.i
#endif

#ifndef SWIGJAVA
%feature ("compactdefaultargs");
#endif

//
// We register all the drivers upon module initialization
//

%{
#include <iostream>
using namespace std;

#include "cpl_port.h"
#include "cpl_string.h"
#include "cpl_multiproc.h"
#include "cpl_http.h"

#include "gdal.h"
#include "gdal_priv.h"
#include "gdal_alg.h"
#include "gdalwarper.h"

typedef void GDALMajorObjectShadow;
typedef void GDALDriverShadow;
typedef void GDALDatasetShadow;
typedef void GDALRasterBandShadow;
typedef void GDALColorTableShadow;
typedef void GDALRasterAttributeTableShadow;
typedef void GDALTransformerInfoShadow;
typedef void GDALAsyncReaderShadow;
%}

#if defined(SWIGPYTHON) || defined(SWIGJAVA)
%{
#ifdef DEBUG 
typedef struct OGRSpatialReferenceHS OSRSpatialReferenceShadow;
typedef struct OGRLayerHS OGRLayerShadow;
typedef struct OGRGeometryHS OGRGeometryShadow;
#else
typedef void OSRSpatialReferenceShadow;
typedef void OGRLayerShadow;
typedef void OGRGeometryShadow;
#endif
typedef struct OGRStyleTableHS OGRStyleTableShadow;
%}
#endif /* #if defined(SWIGPYTHON) || defined(SWIGJAVA) */

#if defined(SWIGCSHARP)
typedef int OGRErr;
#endif

%{
/* use this to not return the int returned by GDAL */
typedef int RETURN_NONE;
/* return value that is used for VSI methods that return -1 on error (and set errno) */
typedef int VSI_RETVAL;

%}

//************************************************************************
//
// Enums.
//
//************************************************************************

typedef int GDALTileOrganization;
#ifndef SWIGCSHARP
typedef int GDALPaletteInterp;
typedef int GDALColorInterp;
typedef int GDALAccess;
typedef int GDALDataType;
typedef int CPLErr;
typedef int GDALResampleAlg;
typedef int GDALAsyncStatusType;
typedef int GDALRWFlag;
typedef int GDALRIOResampleAlg;
#else
/*! Pixel data types */
%rename (DataType) GDALDataType;
typedef enum {
    GDT_Unknown = 0,
    /*! Eight bit unsigned integer */           GDT_Byte = 1,
    /*! Sixteen bit unsigned integer */         GDT_UInt16 = 2,
    /*! Sixteen bit signed integer */           GDT_Int16 = 3,
    /*! Thirty two bit unsigned integer */      GDT_UInt32 = 4,
    /*! Thirty two bit signed integer */        GDT_Int32 = 5,
    /*! Thirty two bit floating point */        GDT_Float32 = 6,
    /*! Sixty four bit floating point */        GDT_Float64 = 7,
    /*! Complex Int16 */                        GDT_CInt16 = 8,
    /*! Complex Int32 */                        GDT_CInt32 = 9,
    /*! Complex Float32 */                      GDT_CFloat32 = 10,
    /*! Complex Float64 */                      GDT_CFloat64 = 11,
    GDT_TypeCount = 12          /* maximum type # + 1 */
} GDALDataType;

/*! Types of color interpretation for raster bands. */
%rename (ColorInterp) GDALColorInterp;
typedef enum
{
    GCI_Undefined=0,
    /*! Greyscale */                                      GCI_GrayIndex=1,
    /*! Paletted (see associated color table) */          GCI_PaletteIndex=2,
    /*! Red band of RGBA image */                         GCI_RedBand=3,
    /*! Green band of RGBA image */                       GCI_GreenBand=4,
    /*! Blue band of RGBA image */                        GCI_BlueBand=5,
    /*! Alpha (0=transparent, 255=opaque) */              GCI_AlphaBand=6,
    /*! Hue band of HLS image */                          GCI_HueBand=7,
    /*! Saturation band of HLS image */                   GCI_SaturationBand=8,
    /*! Lightness band of HLS image */                    GCI_LightnessBand=9,
    /*! Cyan band of CMYK image */                        GCI_CyanBand=10,
    /*! Magenta band of CMYK image */                     GCI_MagentaBand=11,
    /*! Yellow band of CMYK image */                      GCI_YellowBand=12,
    /*! Black band of CMLY image */                       GCI_BlackBand=13,
    /*! Y Luminance */                                    GCI_YCbCr_YBand=14,
    /*! Cb Chroma */                                      GCI_YCbCr_CbBand=15,
    /*! Cr Chroma */                                      GCI_YCbCr_CrBand=16,
    /*! Max current value */                              GCI_Max=16
} GDALColorInterp;

/*! Types of color interpretations for a GDALColorTable. */
%rename (PaletteInterp) GDALPaletteInterp;
typedef enum 
{
  /*! Grayscale (in GDALColorEntry.c1) */                      GPI_Gray=0,
  /*! Red, Green, Blue and Alpha in (in c1, c2, c3 and c4) */  GPI_RGB=1,
  /*! Cyan, Magenta, Yellow and Black (in c1, c2, c3 and c4)*/ GPI_CMYK=2,
  /*! Hue, Lightness and Saturation (in c1, c2, and c3) */     GPI_HLS=3
} GDALPaletteInterp;

/*! Flag indicating read/write, or read-only access to data. */
%rename (Access) GDALAccess;
typedef enum {
    /*! Read only (no update) access */ GA_ReadOnly = 0,
    /*! Read/write access. */           GA_Update = 1
} GDALAccess;

/*! Read/Write flag for RasterIO() method */
%rename (RWFlag) GDALRWFlag;
typedef enum {
    /*! Read data */   GF_Read = 0,
    /*! Write data */  GF_Write = 1
} GDALRWFlag;

%rename (RIOResampleAlg) GDALRIOResampleAlg;
typedef enum
{
    /*! Nearest neighbour */                GRIORA_NearestNeighbour = 0,
    /*! Bilinear (2x2 kernel) */            GRIORA_Bilinear = 1,
    /*! Cubic Convolution Approximation  */ GRIORA_Cubic = 2,
    /*! Cubic B-Spline Approximation */     GRIORA_CubicSpline = 3,
    /*! Lanczos windowed sinc interpolation (6x6 kernel) */ GRIORA_Lanczos = 4,
    /*! Average */                          GRIORA_Average = 5,
    /*! Mode (selects the value which appears most often of all the sampled points) */
                                            GRIORA_Mode = 6,
    /*! Gauss bluring */                    GRIORA_Gauss = 7
} GDALRIOResampleAlg;

/*! Warp Resampling Algorithm */
%rename (ResampleAlg) GDALResampleAlg;
typedef enum {
  /*! Nearest neighbour (select on one input pixel) */ GRA_NearestNeighbour=0,
  /*! Bilinear (2x2 kernel) */                         GRA_Bilinear=1,
  /*! Cubic Convolution Approximation (4x4 kernel) */  GRA_Cubic=2,
  /*! Cubic B-Spline Approximation (4x4 kernel) */     GRA_CubicSpline=3,
  /*! Lanczos windowed sinc interpolation (6x6 kernel) */ GRA_Lanczos=4,
  /*! Average (computes the average of all non-NODATA contributing pixels) */ GRA_Average=5, 
  /*! Mode (selects the value which appears most often of all the sampled points) */ GRA_Mode=6
} GDALResampleAlg;

%rename (AsyncStatusType) GDALAsyncStatusType;
typedef enum {
	GARIO_PENDING = 0,
	GARIO_UPDATE = 1,
	GARIO_ERROR = 2,
	GARIO_COMPLETE = 3
} GDALAsyncStatusType;
#endif

#if defined(SWIGPYTHON)
%include "gdal_python.i"
#elif defined(SWIGRUBY)
%include "gdal_ruby.i"
#elif defined(SWIGPHP4)
%include "gdal_php.i"
#elif defined(SWIGCSHARP)
%include "gdal_csharp.i"
#elif defined(SWIGPERL)
%include "gdal_perl.i"
#elif defined(SWIGJAVA)
%include "gdal_java.i"
#else
%include "gdal_typemaps.i"
#endif


/* Default memberin typemaps required to support SWIG 1.3.39 and above */
%typemap(memberin) char *Info %{
/* char* Info memberin typemap */
$1;
%}

%typemap(memberin) char *Id %{
/* char* Info memberin typemap */
$1;
%}

//************************************************************************
// Apply NONNULL to all utf8_path's. 
%apply Pointer NONNULL { const char* utf8_path };

//************************************************************************
//
// Define the exposed CPL functions.
//
//************************************************************************
%include "cpl.i"

//************************************************************************
//
// Define the XMLNode object
//
//************************************************************************
#if defined(SWIGCSHARP) || defined(SWIGJAVA)
%include "XMLNode.i"
#endif

//************************************************************************
//
// Define the MajorObject object
//
//************************************************************************
%include "MajorObject.i"

//************************************************************************
//
// Define the Driver object.
//
//************************************************************************
%include "Driver.i"



#if defined(SWIGPYTHON) || defined(SWIGJAVA) || defined(SWIGPERL)
/*
 * We need to import ogr.i and osr.i for OGRLayer and OSRSpatialRefrerence
 */
#define FROM_GDAL_I
%import ogr.i
#endif /* #if defined(SWIGPYTHON) || defined(SWIGJAVA) */


//************************************************************************
//
// Define renames.
//
//************************************************************************
%rename (GCP) GDAL_GCP;

#ifdef SWIGRUBY
%rename (all_register) GDALAllRegister;
%rename (get_cache_max) wrapper_GDALGetCacheMax;
%rename (set_cache_max) wrapper_GDALSetCacheMax;
%rename (get_cache_used) wrapper_GDALGetCacheUsed;
%rename (get_data_type_size) GDALGetDataTypeSize;
%rename (data_type_is_complex) GDALDataTypeIsComplex;
%rename (gcps_to_geo_transform) GDALGCPsToGeoTransform;
%rename (get_data_type_name) GDALGetDataTypeName;
%rename (get_data_type_by_name) GDALGetDataTypeByName;
%rename (get_color_interpretation_name) GDALGetColorInterpretationName;
%rename (get_palette_interpretation_name) GDALGetPaletteInterpretationName;
%rename (dec_to_dms) GDALDecToDMS;
%rename (packed_dms_to_dec) GDALPackedDMSToDec;
%rename (dec_to_packed_dms) GDALDecToPackedDMS;
%rename (parse_xml_string) CPLParseXMLString;
%rename (serialize_xml_tree) CPLSerializeXMLTree;
#else
%rename (GCP) GDAL_GCP;
%rename (GCPsToGeoTransform) GDALGCPsToGeoTransform;
%rename (ApplyGeoTransform) GDALApplyGeoTransform;
%rename (InvGeoTransform) GDALInvGeoTransform;
%rename (VersionInfo) GDALVersionInfo;
%rename (AllRegister) GDALAllRegister;
%rename (GetCacheMax) wrapper_GDALGetCacheMax;
%rename (SetCacheMax) wrapper_GDALSetCacheMax;
%rename (GetCacheUsed) wrapper_GDALGetCacheUsed;
%rename (GetDataTypeSize) GDALGetDataTypeSize;
%rename (DataTypeIsComplex) GDALDataTypeIsComplex;
%rename (GetDataTypeName) GDALGetDataTypeName;
%rename (GetDataTypeByName) GDALGetDataTypeByName;
%rename (GetColorInterpretationName) GDALGetColorInterpretationName;
%rename (GetPaletteInterpretationName) GDALGetPaletteInterpretationName;
%rename (DecToDMS) GDALDecToDMS;
%rename (PackedDMSToDec) GDALPackedDMSToDec;
%rename (DecToPackedDMS) GDALDecToPackedDMS;
%rename (ParseXMLString) CPLParseXMLString;
%rename (SerializeXMLTree) CPLSerializeXMLTree;
%rename (GetJPEG2000Structure) GDALGetJPEG2000Structure;
#endif
#ifdef SWIGPERL
%include "gdal_perl_rename.i"
#endif


//************************************************************************
//
// GDALColorEntry
//
//************************************************************************
#if !defined(SWIGPERL) && !defined(SWIGJAVA)
%rename (ColorEntry) GDALColorEntry;
typedef struct
{
    /*! gray, red, cyan or hue */
    short      c1;      
    /*! green, magenta, or lightness */    
    short      c2;      
    /*! blue, yellow, or saturation */
    short      c3;      
    /*! alpha or blackband */
    short      c4;      
} GDALColorEntry;
#endif

//************************************************************************
//
// Define the Ground Control Point structure.
//
//************************************************************************
// GCP - class?  serialize() method missing.
struct GDAL_GCP {
%extend {
%mutable;
  double GCPX;
  double GCPY;
  double GCPZ;
  double GCPPixel;
  double GCPLine;
  char *Info;
  char *Id;
%immutable;

#ifdef SWIGJAVA
  GDAL_GCP( double x, double y, double z,
            double pixel, double line,
            const char *info, const char *id) {
#else
  GDAL_GCP( double x = 0.0, double y = 0.0, double z = 0.0,
            double pixel = 0.0, double line = 0.0,
            const char *info = "", const char *id = "" ) {
#endif
    GDAL_GCP *self = (GDAL_GCP*) CPLMalloc( sizeof( GDAL_GCP ) );
    self->dfGCPX = x;
    self->dfGCPY = y;
    self->dfGCPZ = z;
    self->dfGCPPixel = pixel;
    self->dfGCPLine = line;
    self->pszInfo =  CPLStrdup( (info == 0) ? "" : info );
    self->pszId = CPLStrdup( (id==0)? "" : id );
    return self;
  }

  ~GDAL_GCP() {
    if ( self->pszInfo )
      CPLFree( self->pszInfo );
    if ( self->pszId )
      CPLFree( self->pszId );
    CPLFree( self );
  }


} /* extend */
}; /* GDAL_GCP */

%apply Pointer NONNULL {GDAL_GCP *gcp};
%inline %{

double GDAL_GCP_GCPX_get( GDAL_GCP *gcp ) {
  return gcp->dfGCPX;
}
void GDAL_GCP_GCPX_set( GDAL_GCP *gcp, double dfGCPX ) {
  gcp->dfGCPX = dfGCPX;
}
double GDAL_GCP_GCPY_get( GDAL_GCP *gcp ) {
  return gcp->dfGCPY;
}
void GDAL_GCP_GCPY_set( GDAL_GCP *gcp, double dfGCPY ) {
  gcp->dfGCPY = dfGCPY;
}
double GDAL_GCP_GCPZ_get( GDAL_GCP *gcp ) {
  return gcp->dfGCPZ;
}
void GDAL_GCP_GCPZ_set( GDAL_GCP *gcp, double dfGCPZ ) {
  gcp->dfGCPZ = dfGCPZ;
}
double GDAL_GCP_GCPPixel_get( GDAL_GCP *gcp ) {
  return gcp->dfGCPPixel;
}
void GDAL_GCP_GCPPixel_set( GDAL_GCP *gcp, double dfGCPPixel ) {
  gcp->dfGCPPixel = dfGCPPixel;
}
double GDAL_GCP_GCPLine_get( GDAL_GCP *gcp ) {
  return gcp->dfGCPLine;
}
void GDAL_GCP_GCPLine_set( GDAL_GCP *gcp, double dfGCPLine ) {
  gcp->dfGCPLine = dfGCPLine;
}
const char * GDAL_GCP_Info_get( GDAL_GCP *gcp ) {
  return gcp->pszInfo;
}
void GDAL_GCP_Info_set( GDAL_GCP *gcp, const char * pszInfo ) {
  if ( gcp->pszInfo ) 
    CPLFree( gcp->pszInfo );
  gcp->pszInfo = CPLStrdup(pszInfo);
}
const char * GDAL_GCP_Id_get( GDAL_GCP *gcp ) {
  return gcp->pszId;
}
void GDAL_GCP_Id_set( GDAL_GCP *gcp, const char * pszId ) {
  if ( gcp->pszId ) 
    CPLFree( gcp->pszId );
  gcp->pszId = CPLStrdup(pszId);
}
%} //%inline 

#if defined(SWIGCSHARP)
%inline %{
/* Duplicate, but transposed names for C# because 
*  the C# module outputs backwards names
*/
double GDAL_GCP_get_GCPX( GDAL_GCP *gcp ) {
  return gcp->dfGCPX;
}
void GDAL_GCP_set_GCPX( GDAL_GCP *gcp, double dfGCPX ) {
  gcp->dfGCPX = dfGCPX;
}
double GDAL_GCP_get_GCPY( GDAL_GCP *gcp ) {
  return gcp->dfGCPY;
}
void GDAL_GCP_set_GCPY( GDAL_GCP *gcp, double dfGCPY ) {
  gcp->dfGCPY = dfGCPY;
}
double GDAL_GCP_get_GCPZ( GDAL_GCP *gcp ) {
  return gcp->dfGCPZ;
}
void GDAL_GCP_set_GCPZ( GDAL_GCP *gcp, double dfGCPZ ) {
  gcp->dfGCPZ = dfGCPZ;
}
double GDAL_GCP_get_GCPPixel( GDAL_GCP *gcp ) {
  return gcp->dfGCPPixel;
}
void GDAL_GCP_set_GCPPixel( GDAL_GCP *gcp, double dfGCPPixel ) {
  gcp->dfGCPPixel = dfGCPPixel;
}
double GDAL_GCP_get_GCPLine( GDAL_GCP *gcp ) {
  return gcp->dfGCPLine;
}
void GDAL_GCP_set_GCPLine( GDAL_GCP *gcp, double dfGCPLine ) {
  gcp->dfGCPLine = dfGCPLine;
}
const char * GDAL_GCP_get_Info( GDAL_GCP *gcp ) {
  return gcp->pszInfo;
}
void GDAL_GCP_set_Info( GDAL_GCP *gcp, const char * pszInfo ) {
  if ( gcp->pszInfo ) 
    CPLFree( gcp->pszInfo );
  gcp->pszInfo = CPLStrdup(pszInfo);
}
const char * GDAL_GCP_get_Id( GDAL_GCP *gcp ) {
  return gcp->pszId;
}
void GDAL_GCP_set_Id( GDAL_GCP *gcp, const char * pszId ) {
  if ( gcp->pszId ) 
    CPLFree( gcp->pszId );
  gcp->pszId = CPLStrdup(pszId);
}
%} //%inline 
#endif //if defined(SWIGCSHARP)

%clear GDAL_GCP *gcp;

#ifdef SWIGJAVA
%rename (GCPsToGeoTransform) wrapper_GDALGCPsToGeoTransform;
%inline
{
int wrapper_GDALGCPsToGeoTransform( int nGCPs, GDAL_GCP const * pGCPs, 
    	                             double argout[6], int bApproxOK = 1 )
{
    return GDALGCPsToGeoTransform(nGCPs, pGCPs, argout, bApproxOK);
}
}
#else
%apply (IF_FALSE_RETURN_NONE) { (RETURN_NONE) };
RETURN_NONE GDALGCPsToGeoTransform( int nGCPs, GDAL_GCP const * pGCPs, 
    	                             double argout[6], int bApproxOK = 1 ); 
%clear (RETURN_NONE);
#endif

%include "cplvirtualmem.i"

//************************************************************************
//
// Define the Dataset object.
//
//************************************************************************
%include "Dataset.i"

//************************************************************************
//
// Define the Band object.
//
//************************************************************************
%include "Band.i"

//************************************************************************
//
// Define the ColorTable object.
//
//************************************************************************
%include "ColorTable.i"

//************************************************************************
//
// Define the RasterAttributeTable object.
//
//************************************************************************
%include "RasterAttributeTable.i"

//************************************************************************
//
// Raster Operations
//
//************************************************************************
%include "Operations.i"

%apply (double argin[ANY]) {(double padfGeoTransform[6])};
%apply (double *OUTPUT) {(double *pdfGeoX)};
%apply (double *OUTPUT) {(double *pdfGeoY)};
void GDALApplyGeoTransform( double padfGeoTransform[6], 
                            double dfPixel, double dfLine, 
                            double *pdfGeoX, double *pdfGeoY );
%clear (double *padfGeoTransform);
%clear (double *pdfGeoX);
%clear (double *pdfGeoY);

%apply (double argin[ANY]) {double gt_in[6]};
%apply (double argout[ANY]) {double gt_out[6]};
#ifdef SWIGJAVA
// FIXME: we should implement correctly the IF_FALSE_RETURN_NONE typemap
int GDALInvGeoTransform( double gt_in[6], double gt_out[6] );
#else
%apply (IF_FALSE_RETURN_NONE) { (RETURN_NONE) };
RETURN_NONE GDALInvGeoTransform( double gt_in[6], double gt_out[6] );
%clear (RETURN_NONE);
#endif
%clear (double *gt_in);
%clear (double *gt_out);

#ifdef SWIGJAVA
%apply (const char* stringWithDefaultValue) {const char *request};
%rename (VersionInfo) wrapper_GDALVersionInfo;
%inline {
const char *wrapper_GDALVersionInfo( const char *request = "VERSION_NUM" )
{
    return GDALVersionInfo(request ? request : "VERSION_NUM");
}
}
%clear (const char* request);
#else
const char *GDALVersionInfo( const char *request = "VERSION_NUM" );
#endif

void GDALAllRegister();

void GDALDestroyDriverManager();

#ifdef SWIGPYTHON
%inline {
GIntBig wrapper_GDALGetCacheMax()
{
    return GDALGetCacheMax64();
}
}

%inline {
GIntBig wrapper_GDALGetCacheUsed()
{
    return GDALGetCacheUsed64();
}
}

%inline {
void wrapper_GDALSetCacheMax(GIntBig nBytes)
{
    return GDALSetCacheMax64(nBytes);
}
}

#else
%inline {
int wrapper_GDALGetCacheMax()
{
    return GDALGetCacheMax();
}
}

%inline {
int wrapper_GDALGetCacheUsed()
{
    return GDALGetCacheUsed();
}
}

%inline {
void wrapper_GDALSetCacheMax(int nBytes)
{
    return GDALSetCacheMax(nBytes);
}
}
#endif

int GDALGetDataTypeSize( GDALDataType eDataType );

int GDALDataTypeIsComplex( GDALDataType eDataType );

const char *GDALGetDataTypeName( GDALDataType eDataType );

GDALDataType GDALGetDataTypeByName( const char * pszDataTypeName );

const char *GDALGetColorInterpretationName( GDALColorInterp eColorInterp );

const char *GDALGetPaletteInterpretationName( GDALPaletteInterp ePaletteInterp );

#ifdef SWIGJAVA
%apply (const char* stringWithDefaultValue) {const char *request};
%rename (DecToDMS) wrapper_GDALDecToDMS;
%inline {
const char *wrapper_GDALDecToDMS( double dfAngle, const char * pszAxis,
                                  int nPrecision = 2 )
{
    return GDALDecToDMS(dfAngle, pszAxis, nPrecision);
}
}
%clear (const char* request);
#else
const char *GDALDecToDMS( double, const char *, int = 2 );
#endif

double GDALPackedDMSToDec( double dfPacked );

double GDALDecToPackedDMS( double dfDec );


#if defined(SWIGCSHARP) || defined(SWIGJAVA)
%newobject CPLParseXMLString;
#endif
CPLXMLNode *CPLParseXMLString( char * pszXMLString );

#if defined(SWIGJAVA) || defined(SWIGCSHARP) || defined(SWIGPYTHON) || defined(SWIGPERL)
retStringAndCPLFree *CPLSerializeXMLTree( CPLXMLNode *xmlnode );
#else
char *CPLSerializeXMLTree( CPLXMLNode *xmlnode );
#endif

#if defined(SWIGPYTHON)
%newobject GDALGetJPEG2000Structure;
CPLXMLNode *GDALGetJPEG2000Structure( const char* pszFilename, char** options = NULL );
#endif

%inline {
retStringAndCPLFree *GetJPEG2000StructureAsString( const char* pszFilename, char** options = NULL )
{
    CPLXMLNode* psNode = GDALGetJPEG2000Structure(pszFilename, options);
    if( psNode == NULL )
        return NULL;
    char* pszXML = CPLSerializeXMLTree(psNode);
    CPLDestroyXMLNode(psNode);
    return pszXML;
}
}


//************************************************************************
//
// Define the factory functions for Drivers and Datasets
//
//************************************************************************

// Missing
// GetDriverList

%inline %{
int GetDriverCount() {
  return GDALGetDriverCount();
}
%}

%apply Pointer NONNULL { char const *name };
%inline %{
GDALDriverShadow* GetDriverByName( char const *name ) {
  return (GDALDriverShadow*) GDALGetDriverByName( name );
}
%}

%inline %{
GDALDriverShadow* GetDriver( int i ) {
  return (GDALDriverShadow*) GDALGetDriver( i );
}
%}

#ifdef SWIGJAVA
%newobject Open;
%inline %{
GDALDatasetShadow* Open( char const* utf8_path, GDALAccess eAccess) {
  CPLErrorReset();
  GDALDatasetShadow *ds = GDALOpen( utf8_path, eAccess );
  if( ds != NULL && CPLGetLastErrorType() == CE_Failure )
  {
      if ( GDALDereferenceDataset( ds ) <= 0 )
          GDALClose(ds);
      ds = NULL;
  }
  return (GDALDatasetShadow*) ds;
}
%}

%newobject Open;
%inline %{
GDALDatasetShadow* Open( char const* name ) {
  return Open( name, GA_ReadOnly );
}
%}

#else
%newobject Open;
%inline %{
GDALDatasetShadow* Open( char const* utf8_path, GDALAccess eAccess = GA_ReadOnly ) {
  CPLErrorReset();
  GDALDatasetShadow *ds = GDALOpen( utf8_path, eAccess );
  if( ds != NULL && CPLGetLastErrorType() == CE_Failure )
  {
      if ( GDALDereferenceDataset( ds ) <= 0 )
          GDALClose(ds);
      ds = NULL;
  }
  return (GDALDatasetShadow*) ds;
}
%}

#endif

%newobject OpenEx;
#ifndef SWIGJAVA
%feature( "kwargs" ) OpenEx;
#endif
%apply (char **options) {char** allowed_drivers};
%apply (char **options) {char** open_options};
%apply (char **options) {char** sibling_files};
%inline %{
GDALDatasetShadow* OpenEx( char const* utf8_path, unsigned int nOpenFlags = 0,
                           char** allowed_drivers = NULL, char** open_options = NULL,
                           char** sibling_files = NULL ) {
  CPLErrorReset();
  GDALDatasetShadow *ds = GDALOpenEx( utf8_path, nOpenFlags, allowed_drivers,
                                      open_options, sibling_files );
  if( ds != NULL && CPLGetLastErrorType() == CE_Failure )
  {
      if ( GDALDereferenceDataset( ds ) <= 0 )
          GDALClose(ds);
      ds = NULL;
  }
  return (GDALDatasetShadow*) ds;
}
%}
%clear char** allowed_drivers;
%clear char** open_options;
%clear char** sibling_files;

%newobject OpenShared;
%inline %{
GDALDatasetShadow* OpenShared( char const* utf8_path, GDALAccess eAccess = GA_ReadOnly ) {
  CPLErrorReset();
  GDALDatasetShadow *ds = GDALOpenShared( utf8_path, eAccess );
  if( ds != NULL && CPLGetLastErrorType() == CE_Failure )
  {
      if ( GDALDereferenceDataset( ds ) <= 0 )
          GDALClose(ds);
      ds = NULL;
  }
  return (GDALDatasetShadow*) ds;
}
%}

%apply (char **options) {char **papszSiblings};
%inline %{
GDALDriverShadow *IdentifyDriver( const char *utf8_path, 
                                  char **papszSiblings = NULL ) {
    return (GDALDriverShadow *) GDALIdentifyDriver( utf8_path, 
	                                            papszSiblings );
}
%}
%clear char **papszSiblings;

//************************************************************************
//
// Define Algorithms
//
//************************************************************************

// Missing
// CreateAndReprojectImage
// GCPsToGeoTransform


#if defined(SWIGPYTHON) || defined(SWIGJAVA)
/* FIXME: other bindings should also use those typemaps to avoid memory leaks */
%apply (char **options) {char ** papszArgv};
%apply (char **CSL) {(char **)};
#else
%apply (char **options) {char **};
#endif

#ifdef SWIGJAVA
%inline %{
  char **GeneralCmdLineProcessor( char **papszArgv, int nOptions = 0 ) {
    int nResArgCount;
    
    /* We must add a 'dummy' element in front of the real argument list */
    /* as Java doesn't include the binary name as the first */
    /* argument, as C does... */
    char** papszArgvModBefore = CSLInsertString(CSLDuplicate(papszArgv), 0, "dummy");
    char** papszArgvModAfter = papszArgvModBefore;

    nResArgCount = 
      GDALGeneralCmdLineProcessor( CSLCount(papszArgvModBefore), &papszArgvModAfter, nOptions ); 

    CSLDestroy(papszArgvModBefore);

    if( nResArgCount <= 0 )
    {
        return NULL;
    }
    else
    {
        /* Now, remove the first dummy element */
        char** papszRet = CSLDuplicate(papszArgvModAfter + 1);
        CSLDestroy(papszArgvModAfter);
        return papszRet;
    }
  }
%}
#else
%inline %{
  char **GeneralCmdLineProcessor( char **papszArgv, int nOptions = 0 ) {
    int nResArgCount;

    nResArgCount = 
      GDALGeneralCmdLineProcessor( CSLCount(papszArgv), &papszArgv, nOptions ); 

    if( nResArgCount <= 0 )
        return NULL;
    else
        return papszArgv;
  }
%}
#endif
%clear char **;


//************************************************************************
//
// Language specific extensions
//
//************************************************************************

#ifdef SWIGCSHARP
%include "gdal_csharp_extend.i"
#endif

#ifdef SWIGPYTHON
/* Add a __version__ attribute to match the convention */
%pythoncode %{
__version__ = _gdal.VersionInfo("RELEASE_NAME") 
%}
#endif

//************************************************************************
//
// GDAL Utilities
//
//************************************************************************

#ifdef SWIGPYTHON
%{
#include "gdal_utils.h"   
%}

%rename (INFO_FORMAT_TEXT) GDALINFO_FORMAT_TEXT;
%rename (INFO_FORMAT_JSON) GDALINFO_FORMAT_JSON;

typedef enum {
    GDALINFO_FORMAT_TEXT = 0,
    GDALINFO_FORMAT_JSON = 1
} GDALInfoFormat;

%rename (InfoOptions) GDALInfoOptions;

%apply (char** options) {char** extraMDDomains};

struct GDALInfoOptions {
%extend {
%mutable;
    GDALInfoFormat format;
    int computeMinMax;
    int reportHistograms;
    int reportProj4;
    int stats;
    int approxStats;
    int sample;
    int computeChecksum;
    int showGCPs;
    int showMetadata;
    int showRAT;
    int showColorTable;
    int listMDD;
    int showFileList;
    int allMetadata;
    char** extraMDDomains;
%immutable;

    GDALInfoOptions() {
        GDALInfoOptions *self = GDALInfoOptionsNew();
        return self;
    }

    ~GDALInfoOptions() {
        GDALInfoOptionsFree( self );
    }
}
};

%clear char** extraMDDomains;

%apply Pointer NONNULL {GDALInfoOptions *infoOptions};
%inline %{

GDALInfoFormat GDALInfoOptions_format_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->eFormat;
}

void GDALInfoOptions_format_set( GDALInfoOptions *infoOptions, GDALInfoFormat eFormat )
{
    infoOptions->eFormat = eFormat;
}

bool GDALInfoOptions_computeMinMax_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->bComputeMinMax;
}

void GDALInfoOptions_computeMinMax_set( GDALInfoOptions *infoOptions, bool bComputeMinMax )
{
    infoOptions->bComputeMinMax = bComputeMinMax;
}

bool GDALInfoOptions_reportHistograms_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->bReportHistograms;
}

void GDALInfoOptions_reportHistograms_set( GDALInfoOptions *infoOptions, bool bReportHistograms )
{
    infoOptions->bReportHistograms = bReportHistograms;
}

bool GDALInfoOptions_reportProj4_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->bReportProj4;
}

void GDALInfoOptions_reportProj4_set( GDALInfoOptions *infoOptions, bool bReportProj4 )
{
    infoOptions->bReportProj4 = bReportProj4;
}

bool GDALInfoOptions_stats_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->bStats;
}

void GDALInfoOptions_stats_set( GDALInfoOptions *infoOptions, bool bStats )
{
    infoOptions->bStats = bStats;
}

bool GDALInfoOptions_approxStats_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->bApproxStats;
}

void GDALInfoOptions_approxStats_set( GDALInfoOptions *infoOptions, bool bApproxStats )
{
    infoOptions->bApproxStats = bApproxStats;
}

bool GDALInfoOptions_sample_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->bSample;
}

void GDALInfoOptions_sample_set( GDALInfoOptions *infoOptions, bool bSample )
{
    infoOptions->bSample = bSample;
}

bool GDALInfoOptions_computeChecksum_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->bComputeChecksum;
}

void GDALInfoOptions_computeChecksum_set( GDALInfoOptions *infoOptions, bool bComputeChecksum )
{
    infoOptions->bComputeChecksum = bComputeChecksum;
}

bool GDALInfoOptions_showGCPs_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->bShowGCPs;
}

void GDALInfoOptions_showGCPs_set( GDALInfoOptions *infoOptions, bool bShowGCPs )
{
    infoOptions->bShowGCPs = bShowGCPs;
}

bool GDALInfoOptions_showMetadata_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->bShowMetadata;
}

void GDALInfoOptions_showMetadata_set( GDALInfoOptions *infoOptions, bool bShowMetadata )
{
    infoOptions->bShowMetadata = bShowMetadata;
}

bool GDALInfoOptions_showRAT_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->bShowRAT;
}

void GDALInfoOptions_showRAT_set( GDALInfoOptions *infoOptions, bool bShowRAT )
{
    infoOptions->bShowRAT = bShowRAT;
}

bool GDALInfoOptions_showColorTable_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->bShowColorTable;
}

void GDALInfoOptions_showColorTable_set( GDALInfoOptions *infoOptions, bool bShowColorTable )
{
    infoOptions->bShowColorTable = bShowColorTable;
}

bool GDALInfoOptions_listMDD_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->bListMDD;
}

void GDALInfoOptions_listMDD_set( GDALInfoOptions *infoOptions, bool bListMDD )
{
    infoOptions->bListMDD = bListMDD;
}

bool GDALInfoOptions_showFileList_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->bShowFileList;
}

void GDALInfoOptions_showFileList_set( GDALInfoOptions *infoOptions, bool bShowFileList )
{
    infoOptions->bShowFileList = bShowFileList;
}

bool GDALInfoOptions_allMetadata_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->bAllMetadata;
}

void GDALInfoOptions_allMetadata_set( GDALInfoOptions *infoOptions, bool bAllMetadata )
{
    infoOptions->bAllMetadata = bAllMetadata;
}

char **GDALInfoOptions_extraMDDomains_get( GDALInfoOptions *infoOptions )
{
    return infoOptions->papszExtraMDDomains;
}

void GDALInfoOptions_extraMDDomains_set( GDALInfoOptions *infoOptions, char** papszExtraMDDomains )
{
    GDALInfoOptionsSetExtraMDDomains( infoOptions, papszExtraMDDomains );
}

%}

%clear GDALInfoOptions *infoOptions;

%rename (InfoOptionsAddExtraMDDomains) GDALInfoOptionsAddExtraMDDomains;
void GDALInfoOptionsAddExtraMDDomains( GDALInfoOptions *infoOptions,
                                               const char *pszDomain );

#ifdef SWIGPYTHON
%rename (InfoInternal) GDALInfo;
#else
%rename (Info) GDALInfo;
#endif
char *GDALInfo( GDALDatasetShadow *hDataset, GDALInfoOptions *infoOptions );


typedef enum
{
    MASK_DISABLED,
    MASK_AUTO,
    MASK_USER
} MaskMode;

%rename (TranslateScaleParams) GDALTranslateScaleParams;

struct GDALTranslateScaleParams
{
%extend {
%mutable;
    int     scale;
    int     haveScaleSrc;
    double  scaleSrcMin, scaleSrcMax;
    double  scaleDstMin, scaleDstMax;
%immutable;
    
    GDALTranslateScaleParams( int scale = 0, int haveScaleSrc = 0, double scaleSrcMin = 0.0,
                 double scaleSrcMax = 0.0, double scaleDstMin = 0.0, double scaleDstMax = 0.0 ) {
        GDALTranslateScaleParams *self = (GDALTranslateScaleParams*) CPLMalloc( sizeof( GDALTranslateScaleParams ) );
        self->bScale = scale;
        self->bHaveScaleSrc = haveScaleSrc;
        self->dfScaleSrcMin = scaleSrcMin;
        self->dfScaleSrcMax = scaleSrcMax;
        self->dfScaleDstMin = scaleDstMin;
        self->dfScaleDstMax = scaleDstMax;
        return self;
    }

    ~GDALTranslateScaleParams() {
        CPLFree( self );
    }
} /* extend */
};

%apply Pointer NONNULL {GDALTranslateScaleParams *scaleParams};
%inline %{
    
bool GDALTranslateScaleParams_scale_get( GDALTranslateScaleParams* scaleParams ) {
    return scaleParams->bScale;
}

void GDALTranslateScaleParams_scale_set( GDALTranslateScaleParams* scaleParams, bool bScale ) {
    scaleParams->bScale = bScale;
}

bool GDALTranslateScaleParams_haveScaleSrc_get( GDALTranslateScaleParams* scaleParams ) {
    return scaleParams->bHaveScaleSrc;
}

void GDALTranslateScaleParams_haveScaleSrc_set( GDALTranslateScaleParams* scaleParams, bool bHaveScaleSrc ) {
    scaleParams->bHaveScaleSrc = bHaveScaleSrc;
}

double GDALTranslateScaleParams_scaleSrcMin_get( GDALTranslateScaleParams* scaleParams ) {
    return scaleParams->dfScaleSrcMin;
}

void GDALTranslateScaleParams_scaleSrcMin_set( GDALTranslateScaleParams* scaleParams, double dfScaleSrcMin ) {
    scaleParams->dfScaleSrcMin = dfScaleSrcMin;
}

double GDALTranslateScaleParams_scaleSrcMax_get( GDALTranslateScaleParams* scaleParams ) {
    return scaleParams->dfScaleSrcMax;
}

void GDALTranslateScaleParams_scaleSrcMax_set( GDALTranslateScaleParams* scaleParams, double dfScaleSrcMax ) {
    scaleParams->dfScaleSrcMax = dfScaleSrcMax;
}

double GDALTranslateScaleParams_scaleDstMin_get( GDALTranslateScaleParams* scaleParams ) {
    return scaleParams->dfScaleDstMin;
}

void GDALTranslateScaleParams_scaleDstMin_set( GDALTranslateScaleParams* scaleParams, double dfScaleDstMin ) {
    scaleParams->dfScaleDstMin = dfScaleDstMin;
}

double GDALTranslateScaleParams_scaleDstMax_get( GDALTranslateScaleParams* scaleParams ) {
    return scaleParams->dfScaleDstMax;
}

void GDALTranslateScaleParams_scaleDstMax_set( GDALTranslateScaleParams* scaleParams, double dfScaleDstMax ) {
    scaleParams->dfScaleDstMax = dfScaleDstMax;
}

%}

%clear GDALTranslateScaleParams *scaleParams;

%rename (TranslateOptions) GDALTranslateOptions;

%apply (char** options) {char** createOptions};
%apply (char** options) {char** metadataOptions};

%feature("python:nondynamic") GDALTranslateOptions;

struct GDALTranslateOptions
{
%extend {
%mutable;
    char* format;
    int quiet;
    GDALDataType outputType;
    MaskMode maskMode;
    int oXSizePixel;
    int oYSizePixel;
    double oXSizePct;
    double oYSizePct;
    char** createOptions;
    int strict;
    int unscale;
    double uLX;
    double uLY;
    double lRX;
    double lRY;
    char** metadataOptions;
    char* outputSRS;
    int setNoData;
    int unsetNoData;
    double noDataReal;
    int rgbExpand;
    int maskBand; /* negative value means mask band of ABS(nMaskBand) */
    int stats;
    int approxStats;
    int errorOnPartiallyOutside;
    int errorOnCompletelyOutside;
    int noRAT;
    char* resampling;
    double xRes;
    double yRes;
    char* projSRS;
%immutable;
    GDALTranslateOptions() {
        GDALTranslateOptions *self = GDALTranslateOptionsNew();
        return self;
    }

    ~GDALTranslateOptions() {
        GDALTranslateOptionsFree( self );
    }

    void getBandList( int *nLen, const int **pList ) {
        *nLen = self->nBandCount;
        *pList = self->panBandList;
    }
    
    void setBandList( int nList, int *pList ) {
        CPLFree(self->panBandList);
        self->nBandCount = nList;
        self->panBandList = (int*)CPLMalloc(sizeof(int) * nList);
        memcpy(self->panBandList, pList, sizeof(int) * nList);
    }

    void getSrcWin( int *nLen, const int **pList ) {
        *nLen = 4;
        *pList = self->anSrcWin;
    }

    void setSrcWin( int nList, int *pList ) {
        if(nList != 4)
            CPLError(CE_Failure, CPLE_AppDefined, "srcWin can only accept list of 4 integers");
        else
            memcpy(self->anSrcWin, pList, sizeof(int) * nList);
    }

    void getExponent( int *nLen, const double **pList ) {
        *nLen = self->nExponentRepeat;
        *pList = self->padfExponent;
    }

    void setExponent( int nList, double *pList ) {
        CPLFree(self->padfExponent);
        self->nExponentRepeat = nList;
        self->padfExponent = (double*)CPLMalloc(sizeof(double) * nList);
        memcpy(self->padfExponent, pList, sizeof(double) * nList);
    }

    void getULLR( int *nLen, const double **pList ) {
        *nLen = 4;
        *pList = self->adfULLR;
    }

    void setULLR( int nList, double *pList ) {
        if(nList != 4)
            CPLError(CE_Failure, CPLE_AppDefined, "ULLR can only accept list of 4 floats");
        else
            memcpy(self->adfULLR, pList, sizeof(double) * nList);
    }

    void getGCPs( int *nGCPs, GDAL_GCP const **pGCPs ) {
        *nGCPs = self->nGCPCount;
        *pGCPs = self->pasGCPs;
    }

    void setGCPs( int nGCPs, GDAL_GCP const *pGCPs ) {
        if(self->nGCPCount > 0) {
            GDALDeinitGCPs( self->nGCPCount, self->pasGCPs );
            CPLFree( self->pasGCPs );
        }
        self->nGCPCount = nGCPs;
        self->pasGCPs = GDALDuplicateGCPs( nGCPs, pGCPs );
    }

    void getScaleParams( int *nScaleParams, GDALTranslateScaleParams const **pScaleParams ) {
        *nScaleParams = self->nScaleRepeat;
        *pScaleParams = self->pasScaleParams;
    }

    void setScaleParams( int nScaleParams, GDALTranslateScaleParams const *pScaleParams ) {
        CPLFree( self->pasScaleParams );
        self->nScaleRepeat = nScaleParams;
        self->pasScaleParams = (GDALTranslateScaleParams*)CPLMalloc(sizeof(GDALTranslateScaleParams) * nScaleParams);
        memcpy(self->pasScaleParams, pScaleParams, sizeof(GDALTranslateScaleParams) * nScaleParams);
    }

} /* extend */
};

%extend GDALTranslateOptions {
%pythoncode {
    if not '__swig_setmethods__' in dir():
        #This is to ensure that it builds in windows too since SWIG generates different files for linux and windows.
        __swig_getmethods__ = {}
        __swig_setmethods__ = {}
    __swig_setmethods__["bandList"] = _gdal.TranslateOptions_setBandList
    __swig_getmethods__["bandList"] = _gdal.TranslateOptions_getBandList
    __swig_setmethods__["srcWin"] = _gdal.TranslateOptions_setSrcWin
    __swig_getmethods__["srcWin"] = _gdal.TranslateOptions_getSrcWin
    __swig_setmethods__["exponent"] = _gdal.TranslateOptions_setExponent
    __swig_getmethods__["exponent"] = _gdal.TranslateOptions_getExponent
    __swig_setmethods__["ULLR"] = _gdal.TranslateOptions_setULLR
    __swig_getmethods__["ULLR"] = _gdal.TranslateOptions_getULLR
    __swig_setmethods__["GCPs"] = _gdal.TranslateOptions_setGCPs
    __swig_getmethods__["GCPs"] = _gdal.TranslateOptions_getGCPs
    __swig_setmethods__["scaleParams"] = _gdal.TranslateOptions_setScaleParams
    __swig_getmethods__["scaleParams"] = _gdal.TranslateOptions_getScaleParams
    if _newclass:bandList = _swig_property(_gdal.TranslateOptions_getBandList, _gdal.TranslateOptions_setBandList)
    if _newclass:srcWin = _swig_property(_gdal.TranslateOptions_getSrcWin, _gdal.TranslateOptions_setSrcWin)
    if _newclass:exponent = _swig_property(_gdal.TranslateOptions_getExponent, _gdal.TranslateOptions_setExponent)
    if _newclass:ULLR = _swig_property(_gdal.TranslateOptions_getULLR, _gdal.TranslateOptions_setULLR)
    if _newclass:GCPs = _swig_property(_gdal.TranslateOptions_getGCPs, _gdal.TranslateOptions_setGCPs)
    if _newclass:scaleParams = _swig_property(_gdal.TranslateOptions_getScaleParams, _gdal.TranslateOptions_setScaleParams)
}
}

%clear char** createOptions;
%clear char** metadataOptions;

%apply Pointer NONNULL {GDALTranslateOptions *translateOptions};
%inline %{

const char* GDALTranslateOptions_format_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->pszFormat;
}

void GDALTranslateOptions_format_set( GDALTranslateOptions *translateOptions, const char *pszFormat ) {
    CPLFree(translateOptions->pszFormat);
    translateOptions->pszFormat = CPLStrdup(pszFormat);
}

bool GDALTranslateOptions_quiet_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->bQuiet;
}

void GDALTranslateOptions_quiet_set( GDALTranslateOptions *translateOptions, bool bQuiet ) {
    translateOptions->bQuiet = bQuiet;
}

GDALDataType GDALTranslateOptions_outputType_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->eOutputType;
}

void GDALTranslateOptions_outputType_set( GDALTranslateOptions *translateOptions, GDALDataType eOutputType) {
    translateOptions->eOutputType = eOutputType;
}

MaskMode GDALTranslateOptions_maskMode_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->eMaskMode;
}

void GDALTranslateOptions_maskMode_set( GDALTranslateOptions *translateOptions, MaskMode eMaskMode ) {
    translateOptions->eMaskMode = eMaskMode;
}

int GDALTranslateOptions_oXSizePixel_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->nOXSizePixel;
}

void GDALTranslateOptions_oXSizePixel_set( GDALTranslateOptions *translateOptions, int nOXSizePixel ) {
    translateOptions->nOXSizePixel = nOXSizePixel;
}

int GDALTranslateOptions_oYSizePixel_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->nOYSizePixel;
}

void GDALTranslateOptions_oYSizePixel_set( GDALTranslateOptions *translateOptions, int nOYSizePixel ) {
    translateOptions->nOYSizePixel = nOYSizePixel;
}

double GDALTranslateOptions_oXSizePct_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->dfOXSizePct;
}

void GDALTranslateOptions_oXSizePct_set( GDALTranslateOptions *translateOptions, double dfOXSizePct ) {
    translateOptions->dfOXSizePct = dfOXSizePct;
}

double GDALTranslateOptions_oYSizePct_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->dfOYSizePct;
}

void GDALTranslateOptions_oYSizePct_set( GDALTranslateOptions *translateOptions, double dfOYSizePct ) {
    translateOptions->dfOYSizePct = dfOYSizePct;
}

char** GDALTranslateOptions_createOptions_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->papszCreateOptions;
}

void GDALTranslateOptions_createOptions_set( GDALTranslateOptions *translateOptions, char **papszCreateOptions ) {
    GDALTranslateOptionsSetCreateOptions( translateOptions, papszCreateOptions );
}

bool GDALTranslateOptions_strict_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->bStrict;
}

void GDALTranslateOptions_strict_set( GDALTranslateOptions *translateOptions, bool bStrict ) {
    translateOptions->bStrict = bStrict;
}

bool GDALTranslateOptions_unscale_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->bUnscale;
}

void GDALTranslateOptions_unscale_set( GDALTranslateOptions *translateOptions, bool bUnscale ) {
    translateOptions->bUnscale = bUnscale;
}

double GDALTranslateOptions_uLX_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->dfULX;
}

void GDALTranslateOptions_uLX_set( GDALTranslateOptions *translateOptions, double dfULX ) {
    translateOptions->dfULX = dfULX;
}

double GDALTranslateOptions_uLY_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->dfULY;
}

void GDALTranslateOptions_uLY_set( GDALTranslateOptions *translateOptions, double dfULY ) {
    translateOptions->dfULY = dfULY;
}

double GDALTranslateOptions_lRX_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->dfLRX;
}

void GDALTranslateOptions_lRX_set( GDALTranslateOptions *translateOptions, double dfLRX ) {
    translateOptions->dfLRX = dfLRX;
}

double GDALTranslateOptions_lRY_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->dfLRY;
}

void GDALTranslateOptions_lRY_set( GDALTranslateOptions *translateOptions, double dfLRY ) {
    translateOptions->dfLRY = dfLRY;
}

char** GDALTranslateOptions_metadataOptions_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->papszMetadataOptions;
}

void GDALTranslateOptions_metadataOptions_set( GDALTranslateOptions *translateOptions, char **papszMetadataOptions ) {
    GDALTranslateOptionsSetMetadataOptions( translateOptions, papszMetadataOptions );
}

const char* GDALTranslateOptions_outputSRS_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->pszOutputSRS;
}

void GDALTranslateOptions_outputSRS_set( GDALTranslateOptions *translateOptions, const char *pszOutputSRS ) {
    CPLFree(translateOptions->pszOutputSRS);
    translateOptions->pszOutputSRS = CPLStrdup(pszOutputSRS);
}

bool GDALTranslateOptions_setNoData_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->bSetNoData;
}

void GDALTranslateOptions_setNoData_set( GDALTranslateOptions *translateOptions, bool bSetNoData ) {
    translateOptions->bSetNoData = bSetNoData;
}

bool GDALTranslateOptions_unsetNoData_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->bUnsetNoData;
}

void GDALTranslateOptions_unsetNoData_set( GDALTranslateOptions *translateOptions, bool bUnsetNoData ) {
    translateOptions->bUnsetNoData = bUnsetNoData;
}

double GDALTranslateOptions_noDataReal_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->dfNoDataReal;
}

void GDALTranslateOptions_noDataReal_set( GDALTranslateOptions *translateOptions, double dfNoDataReal ) {
    translateOptions->dfNoDataReal = dfNoDataReal;
}

int GDALTranslateOptions_rgbExpand_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->nRGBExpand;
}

void GDALTranslateOptions_rgbExpand_set( GDALTranslateOptions *translateOptions, int nRGBExpand ) {
    translateOptions->nRGBExpand = nRGBExpand;
}

int GDALTranslateOptions_maskBand_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->nMaskBand;
}

void GDALTranslateOptions_maskBand_set( GDALTranslateOptions *translateOptions, int nMaskBand ) {
    translateOptions->nMaskBand = nMaskBand;
}

bool GDALTranslateOptions_stats_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->bStats;
}

void GDALTranslateOptions_stats_set( GDALTranslateOptions *translateOptions, bool bStats ) {
    translateOptions->bStats = bStats;
}

bool GDALTranslateOptions_approxStats_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->bApproxStats;
}

void GDALTranslateOptions_approxStats_set( GDALTranslateOptions *translateOptions, bool bApproxStats ) {
    translateOptions->bApproxStats = bApproxStats;
}

bool GDALTranslateOptions_errorOnPartiallyOutside_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->bErrorOnPartiallyOutside;
}

void GDALTranslateOptions_errorOnPartiallyOutside_set( GDALTranslateOptions *translateOptions, bool bErrorOnPartiallyOutside ) {
    translateOptions->bErrorOnPartiallyOutside = bErrorOnPartiallyOutside;
}

bool GDALTranslateOptions_errorOnCompletelyOutside_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->bErrorOnCompletelyOutside;
}

void GDALTranslateOptions_errorOnCompletelyOutside_set( GDALTranslateOptions *translateOptions, bool bErrorOnCompletelyOutside ) {
    translateOptions->bErrorOnCompletelyOutside = bErrorOnCompletelyOutside;
}

bool GDALTranslateOptions_noRAT_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->bNoRAT;
}

void GDALTranslateOptions_noRAT_set( GDALTranslateOptions *translateOptions, bool bNoRAT ) {
    translateOptions->bNoRAT = bNoRAT;
}

const char* GDALTranslateOptions_resampling_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->pszResampling;
}

void GDALTranslateOptions_resampling_set( GDALTranslateOptions *translateOptions, const char *pszResampling ) {
    CPLFree(translateOptions->pszResampling);
    translateOptions->pszResampling = CPLStrdup(pszResampling);
}

double GDALTranslateOptions_xRes_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->dfXRes;
}

void GDALTranslateOptions_xRes_set( GDALTranslateOptions *translateOptions, double dfXRes ) {
    translateOptions->dfXRes = dfXRes;
}

double GDALTranslateOptions_yRes_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->dfYRes;
}

void GDALTranslateOptions_yRes_set( GDALTranslateOptions *translateOptions, double dfYRes ) {
    translateOptions->dfYRes = dfYRes;
}

const char* GDALTranslateOptions_projSRS_get( GDALTranslateOptions *translateOptions ) {
    return translateOptions->pszProjSRS;
}

void GDALTranslateOptions_projSRS_set( GDALTranslateOptions *translateOptions, const char *pszProjSRS ) {
    CPLFree(translateOptions->pszProjSRS);
    translateOptions->pszProjSRS = CPLStrdup(pszProjSRS);
}

%}

%clear GDALTranslateOptions *translateOptions;

#ifdef SWIGPYTHON
%rename (TranslateInternal) wrapper_GDALTranslate;
#else
%rename (Translate) wrapper_GDALTranslate;
#endif

%newobject wrapper_GDALTranslate;

%inline %{
GDALDatasetShadow* wrapper_GDALTranslate( const char* dest, GDALDatasetShadow* dataset, GDALTranslateOptions* translateOptions)
{
    int usageError; /* ignored */
    return GDALTranslate(dest, dataset, translateOptions, &usageError);    
}
%}

%rename (WarpOptions) GDALWarpAppOptions;

%apply (char** options) {char** warpOptions};
%apply (char** options) {char** createOptions};
%apply (char** options) {char** TO};
%apply (char** options) {char** destOpenOptions};

struct GDALWarpAppOptions
{
%extend {
%mutable;
    double minX;
    double minY;
    double maxX;
    double maxY;
    double xRes;
    double yRes;
    int targetAlignedPixels;
    int forcePixels;
    int forceLines;
    int quiet;
    int enableDstAlpha;
    int enableSrcAlpha;
    char *format;
    int createOutput;
    char **warpOptions;
    double errorThreshold;
    double warpMemoryLimit;
    char **createOptions;
    GDALDataType outputType;
    GDALDataType workingType;
    GDALResampleAlg resampleAlg;
    char *srcNodata;
    char *dstNodata;
    int multi;
    char **TO;
    char *cutlineDSName;
    char *cLayer;
    char *cWHERE;
    char *cSQL;
    int cropToCutline;
    int overwrite;
    int copyMetadata;
    int copyBandInfo;
    char *MDConflictValue;
    int setColorInterpretation;
    char **destOpenOptions;
    int OvLevel;
    char *TE_SRS;
%immutable;
    GDALWarpAppOptions() {
        GDALWarpAppOptions *self = GDALWarpAppOptionsNew();
        return self;
    }

    ~GDALWarpAppOptions() {
        GDALWarpAppOptionsFree(self);
    }
}
};

%clear char** warpOptions;
%clear char** createOptions;
%clear char** TO;
%clear char** destOpenOptions;

%apply Pointer NONNULL {GDALWarpAppOptions *warpAppOptions};
%inline %{

double GDALWarpAppOptions_minX_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->dfMinX;
}

void GDALWarpAppOptions_minX_set( GDALWarpAppOptions *warpAppOptions, double dfMinX ) {
    warpAppOptions->dfMinX = dfMinX;
}

double GDALWarpAppOptions_minY_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->dfMinX;
}

void GDALWarpAppOptions_minY_set( GDALWarpAppOptions *warpAppOptions, double dfMinY ) {
    warpAppOptions->dfMinY = dfMinY;
}

double GDALWarpAppOptions_maxX_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->dfMaxX;
}

void GDALWarpAppOptions_maxX_set( GDALWarpAppOptions *warpAppOptions, double dfMaxX ) {
    warpAppOptions->dfMaxX = dfMaxX;
}

double GDALWarpAppOptions_maxY_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->dfMaxY;
}

void GDALWarpAppOptions_maxY_set( GDALWarpAppOptions *warpAppOptions, double dfMaxY ) {
    warpAppOptions->dfMaxY = dfMaxY;
}

double GDALWarpAppOptions_xRes_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->dfXRes;
}

void GDALWarpAppOptions_xRes_set( GDALWarpAppOptions *warpAppOptions, double dfXRes ) {
    warpAppOptions->dfXRes = dfXRes;
}

double GDALWarpAppOptions_yRes_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->dfYRes;
}

void GDALWarpAppOptions_yRes_set( GDALWarpAppOptions *warpAppOptions, double dfYRes ) {
    warpAppOptions->dfYRes = dfYRes;
}

bool GDALWarpAppOptions_targetAlignedPixels_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->bTargetAlignedPixels;
}

void GDALWarpAppOptions_targetAlignedPixels_set( GDALWarpAppOptions *warpAppOptions, bool bTargetAlignedPixels ) {
    warpAppOptions->bTargetAlignedPixels = bTargetAlignedPixels;
}

int GDALWarpAppOptions_forcePixels_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->nForcePixels;
}

void GDALWarpAppOptions_forcePixels_set( GDALWarpAppOptions *warpAppOptions, int nForcePixels ) {
    warpAppOptions->nForcePixels = nForcePixels;
}

int GDALWarpAppOptions_forceLines_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->nForceLines;
}

void GDALWarpAppOptions_forceLines_set( GDALWarpAppOptions *warpAppOptions, int nForceLines ) {
    warpAppOptions->nForceLines = nForceLines;
}

bool GDALWarpAppOptions_quiet_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->bQuiet;
}

void GDALWarpAppOptions_quiet_set( GDALWarpAppOptions *warpAppOptions, bool bQuiet ) {
    warpAppOptions->bQuiet = bQuiet;
}

bool GDALWarpAppOptions_enableDstAlpha_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->bEnableDstAlpha;
}

void GDALWarpAppOptions_enableDstAlpha_set( GDALWarpAppOptions *warpAppOptions, bool bEnableDstAlpha ) {
    warpAppOptions->bEnableDstAlpha = bEnableDstAlpha;
}

bool GDALWarpAppOptions_enableSrcAlpha_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->bEnableSrcAlpha;
}

void GDALWarpAppOptions_enableSrcAlpha_set( GDALWarpAppOptions *warpAppOptions, bool bEnableSrcAlpha ) {
    warpAppOptions->bEnableSrcAlpha = bEnableSrcAlpha;
}

char *GDALWarpAppOptions_format_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->pszFormat;
}

void GDALWarpAppOptions_format_set( GDALWarpAppOptions *warpAppOptions, char *pszFormat ) {
    CPLFree(warpAppOptions->pszFormat);
    warpAppOptions->pszFormat = CPLStrdup(pszFormat);
}

bool GDALWarpAppOptions_createOutput_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->bCreateOutput;
}

void GDALWarpAppOptions_createOutput_set( GDALWarpAppOptions *warpAppOptions, bool bCreateOutput ) {
    warpAppOptions->bCreateOutput = bCreateOutput;
}

char **GDALWarpAppOptions_warpOptions_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->papszWarpOptions;
}

void GDALWarpAppOptions_warpOptions_set( GDALWarpAppOptions *warpAppOptions, char **papszWarpOptions ) {
    GDALWarpAppOptionsSetWarpOptions( warpAppOptions, papszWarpOptions );
}

double GDALWarpAppOptions_errorThreshold_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->dfErrorThreshold;
}

void GDALWarpAppOptions_errorThreshold_set( GDALWarpAppOptions *warpAppOptions, double dfErrorThreshold ) {
    warpAppOptions->dfErrorThreshold = dfErrorThreshold;
}

double GDALWarpAppOptions_warpMemoryLimit_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->dfWarpMemoryLimit;
}

void GDALWarpAppOptions_warpMemoryLimit_set( GDALWarpAppOptions *warpAppOptions, double dfWarpMemoryLimit ) {
    warpAppOptions->dfWarpMemoryLimit = dfWarpMemoryLimit;
}

char **GDALWarpAppOptions_createOptions_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->papszCreateOptions;
}

void GDALWarpAppOptions_createOptions_set( GDALWarpAppOptions *warpAppOptions, char **papszCreateOptions ) {
    GDALWarpAppOptionsSetCreateOptions( warpAppOptions, papszCreateOptions );
}

GDALDataType GDALWarpAppOptions_outputType_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->eOutputType;
}

void GDALWarpAppOptions_outputType_set( GDALWarpAppOptions *warpAppOptions, GDALDataType eOutputType ) {
    warpAppOptions->eOutputType = eOutputType;
}

GDALDataType GDALWarpAppOptions_workingType_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->eWorkingType;
}

void GDALWarpAppOptions_workingType_set( GDALWarpAppOptions *warpAppOptions, GDALDataType eWorkingType ) {
    warpAppOptions->eWorkingType = eWorkingType;
}

GDALResampleAlg GDALWarpAppOptions_resampleAlg_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->eResampleAlg;
}

void GDALWarpAppOptions_resampleAlg_set( GDALWarpAppOptions *warpAppOptions, GDALResampleAlg eResampleAlg ) {
    warpAppOptions->eResampleAlg = eResampleAlg;
}

char *GDALWarpAppOptions_srcNodata_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->pszSrcNodata;
}

void GDALWarpAppOptions_srcNodata_set( GDALWarpAppOptions *warpAppOptions, char *pszSrcNodata ) {
    CPLFree(warpAppOptions->pszSrcNodata);
    warpAppOptions->pszSrcNodata = CPLStrdup(pszSrcNodata);
}

char *GDALWarpAppOptions_dstNodata_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->pszDstNodata;
}

void GDALWarpAppOptions_dstNodata_set( GDALWarpAppOptions *warpAppOptions, char *pszDstNodata ) {
    CPLFree(warpAppOptions->pszDstNodata);
    warpAppOptions->pszDstNodata = CPLStrdup(pszDstNodata);
}

bool GDALWarpAppOptions_multi_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->bMulti;
}

void GDALWarpAppOptions_multi_set( GDALWarpAppOptions *warpAppOptions, bool bMulti ) {
    warpAppOptions->bMulti = bMulti;
}

char **GDALWarpAppOptions_TO_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->papszTO;
}

void GDALWarpAppOptions_TO_set( GDALWarpAppOptions *warpAppOptions, char **papszTO ) {
    GDALWarpAppOptionsSetTO( warpAppOptions, papszTO );
}

char *GDALWarpAppOptions_cutlineDSName_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->pszCutlineDSName;
}

void GDALWarpAppOptions_cutlineDSName_set( GDALWarpAppOptions *warpAppOptions, char *pszCutlineDSName ) {
    CPLFree(warpAppOptions->pszCutlineDSName);
    warpAppOptions->pszCutlineDSName = CPLStrdup(pszCutlineDSName);
}

char *GDALWarpAppOptions_cLayer_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->pszCLayer;
}

void GDALWarpAppOptions_cLayer_set( GDALWarpAppOptions *warpAppOptions, char *pszCLayer ) {
    CPLFree(warpAppOptions->pszCLayer);
    warpAppOptions->pszCLayer = CPLStrdup(pszCLayer);
}

char *GDALWarpAppOptions_cWHERE_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->pszCWHERE;
}

void GDALWarpAppOptions_cWHERE_set( GDALWarpAppOptions *warpAppOptions, char *pszCWHERE ) {
    CPLFree(warpAppOptions->pszCWHERE);
    warpAppOptions->pszCWHERE = CPLStrdup(pszCWHERE);
}

char *GDALWarpAppOptions_cSQL_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->pszCSQL;
}

void GDALWarpAppOptions_cSQL_set( GDALWarpAppOptions *warpAppOptions, char *pszCSQL) {
    CPLFree(warpAppOptions->pszCSQL);
    warpAppOptions->pszCSQL = CPLStrdup(pszCSQL);
}

bool GDALWarpAppOptions_cropToCutline_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->bCropToCutline;
}

void GDALWarpAppOptions_cropToCutline_set( GDALWarpAppOptions *warpAppOptions, bool bCropToCutline ) {
    warpAppOptions->bCropToCutline = bCropToCutline;
}

bool GDALWarpAppOptions_overwrite_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->bOverwrite;
}

void GDALWarpAppOptions_overwrite_set( GDALWarpAppOptions *warpAppOptions, bool bOverwrite ) {
    warpAppOptions->bOverwrite = bOverwrite;
}

bool GDALWarpAppOptions_copyMetadata_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->bCopyMetadata;
}

void GDALWarpAppOptions_copyMetadata_set( GDALWarpAppOptions *warpAppOptions, bool bCopyMetadata ) {
    warpAppOptions->bCopyMetadata = bCopyMetadata;
}

bool GDALWarpAppOptions_copyBandInfo_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->bCopyBandInfo;
}

void GDALWarpAppOptions_copyBandInfo_set( GDALWarpAppOptions *warpAppOptions, bool bCopyBandInfo ) {
    warpAppOptions->bCopyBandInfo = bCopyBandInfo;
}

char *GDALWarpAppOptions_MDConflictValue_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->pszMDConflictValue;
}

void GDALWarpAppOptions_MDConflictValue_set( GDALWarpAppOptions *warpAppOptions, char *pszMDConflictValue ) {
    CPLFree(warpAppOptions->pszMDConflictValue);
    warpAppOptions->pszMDConflictValue = CPLStrdup(pszMDConflictValue);
}

bool GDALWarpAppOptions_setColorInterpretation_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->bSetColorInterpretation;
}

void GDALWarpAppOptions_setColorInterpretation_set( GDALWarpAppOptions *warpAppOptions, bool bSetColorInterpretation ) {
    warpAppOptions->bSetColorInterpretation = bSetColorInterpretation;
}

char **GDALWarpAppOptions_destOpenOptions_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->papszDestOpenOptions;
}

void GDALWarpAppOptions_destOpenOptions_set( GDALWarpAppOptions *warpAppOptions, char **papszDestOpenOptions ) {
    GDALWarpAppOptionsSetDestOpenOptions( warpAppOptions, papszDestOpenOptions );
}

int GDALWarpAppOptions_OvLevel_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->nOvLevel;
}

void GDALWarpAppOptions_OvLevel_set( GDALWarpAppOptions *warpAppOptions, int nOvLevel ) {
    warpAppOptions->nOvLevel = nOvLevel;
}

char *GDALWarpAppOptions_TE_SRS_get( GDALWarpAppOptions *warpAppOptions ) {
    return warpAppOptions->pszTE_SRS;
}

void GDALWarpAppOptions_TE_SRS_set( GDALWarpAppOptions *warpAppOptions, char *pszTE_SRS ) {
    CPLFree(warpAppOptions->pszTE_SRS);
    warpAppOptions->pszTE_SRS = CPLStrdup(pszTE_SRS);
}

%}

%clear GDALWarpAppOptions *warpAppOptions;

#ifdef SWIGPYTHON
%rename (WarpInternal) wrapper_GDALWarp;
#else
%rename (Warp) wrapper_GDALWarp;
#endif

%newobject wrapper_GDALWarp;

%inline %{
GDALDatasetShadow* wrapper_GDALWarp( const char* dest, GDALDatasetShadow* dstDS, int object_list_count, GDALDatasetShadow** poObjects, GDALWarpAppOptions* warpAppOptions )
{
    int usageError; /* ignored */
    GDALDatasetShadow **srcDS = (GDALDatasetShadow**) CPLMalloc( sizeof(GDALDatasetShadow*) * object_list_count );
    memcpy( srcDS, poObjects, sizeof(GDALDatasetShadow*) * object_list_count );
    return GDALWarp(dest, dstDS, object_list_count, srcDS, warpAppOptions, &usageError);
}

%}

%rename (WarpOptionsSetSrcSRS) GDALWarpAppOptionsSetSrcSRS;
void GDALWarpAppOptionsSetSrcSRS( GDALWarpAppOptions *psOptions, const char *pszSrcSRS );

%rename (WarpOptionsSetDstSRS) GDALWarpAppOptionsSetDstSRS;
void GDALWarpAppOptionsSetDstSRS( GDALWarpAppOptions *psOptions, const char *pszDstSRS );

%rename (WarpOptionsSetOrder) GDALWarpAppOptionsSetOrder;
void GDALWarpAppOptionsSetOrder( GDALWarpAppOptions *psOptions, const char *pszN);

%rename (WarpOptionsSetRefineGCPs) GDALWarpAppOptionsSetRefineGCPs;
void GDALWarpAppOptionsSetRefineGCPs( GDALWarpAppOptions *psOptions, const char *pszTolerance, const char *pszMinimumGCPs );

%rename (WarpOptionsSetMethod) GDALWarpAppOptionsSetMethod;
void GDALWarpAppOptionsSetMethod( GDALWarpAppOptions *psOptions, const char *pszMethod );

%rename (WarpOptionsSetTransformerOptions) GDALWarpAppOptionsSetTransformerOption;
void GDALWarpAppOptionsSetTransformerOption( GDALWarpAppOptions *psOptions, const char *pszTransformerOption );

#endif
