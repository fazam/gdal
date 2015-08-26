#!/usr/bin/env python
# -*- coding: utf-8 -*-
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  librarified ogr2ogr testing
# Author:   Faza Mahamood <fazamhd @ gmail dot com>
# 
###############################################################################
# Copyright (c) 2015, Faza Mahamood <fazamhd at gmail dot com>
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
###############################################################################

import sys
import os
import shutil

sys.path.append( '../pymod' )
sys.path.append( '../ogr' )

from osgeo import ogr
from osgeo import osr
import gdaltest
import ogrtest

###############################################################################
# Simple test

def test_ogr2ogr_lib_1():
    
    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    srcDS = ogr.Open('../ogr/data/poly.shp')
    ds = ogr.Translate('tmp/poly.shp',srcDS)
    if ds is None or ds.GetLayer(0).GetFeatureCount() != 10:
        return 'fail'

    feat0 = ds.GetLayer(0).GetFeature(0)
    if feat0.GetFieldAsDouble('AREA') != 215229.266:
        print(feat0.GetFieldAsDouble('AREA'))
        gdaltest.post_reason('Did not get expected value for field AREA')
        return 'fail'
    if feat0.GetFieldAsString('PRFEDEA') != '35043411':
        print(feat0.GetFieldAsString('PRFEDEA'))
        gdaltest.post_reason('Did not get expected value for field PRFEDEA')
        return 'fail'
    
    srcDS.Destroy()
    ds.Destroy()
    
    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test SQLStatement

def test_ogr2ogr_lib_2():

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    srcDS = ogr.Open('../ogr/data/poly.shp')
    ds = ogr.Translate('tmp/poly.shp', srcDS, SQLStatement='select * from poly')
    if ds is None or ds.GetLayer(0).GetFeatureCount() != 10:
        return 'fail'
    
    srcDS.Destroy()
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test WHERE

def test_ogr2ogr_lib_3():

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    srcDS = ogr.Open('../ogr/data/poly.shp')
    ds = ogr.Translate('tmp/poly.shp', srcDS, WHERE='EAS_ID=171')
    if ds is None or ds.GetLayer(0).GetFeatureCount() != 1:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test accessMode

def test_ogr2ogr_lib_4():

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    srcDS = ogr.Open('../ogr/data/poly.shp')
    ds = ogr.Translate('tmp/poly.shp', srcDS)
    ds.Destroy()
    ds = ogr.Translate('tmp/poly.shp', srcDS, accessMode=ogr.ACCESS_APPEND)
    if ds is None or ds.GetLayer(0).GetFeatureCount() != 20:
        return 'fail'
        
    feat10 = ds.GetLayer(0).GetFeature(10)
    if feat10.GetFieldAsDouble('AREA') != 215229.266:
        print(feat10.GetFieldAsDouble('AREA'))
        gdaltest.post_reason('Did not get expected value for field AREA')
        return 'fail'
    if feat10.GetFieldAsString('PRFEDEA') != '35043411':
        print(feat10.GetFieldAsString('PRFEDEA'))
        gdaltest.post_reason('Did not get expected value for field PRFEDEA')
        return 'fail'
        
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test transform

def test_ogr2ogr_lib_5():

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    srcDS = ogr.Open('../ogr/data/poly.shp')
    ds = ogr.Translate('tmp/poly.shp', srcDS, outputSRSDef='EPSG:4326', transform=True)
    if str(ds.GetLayer(0).GetSpatialRef()).find('1984') == -1:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test outputSRSDef

def test_ogr2ogr_lib_6():
    
    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    srcDS = ogr.Open('../ogr/data/poly.shp')
    ds = ogr.Translate('tmp/poly.shp', srcDS, outputSRSDef='EPSG:4326')
    if str(ds.GetLayer(0).GetSpatialRef()).find('1984') == -1:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test selFields

def test_ogr2ogr_lib_7():

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    srcDS = ogr.Open('../ogr/data/poly.shp')
    # Voluntary don't use the exact case of the source field names (#4502)
    ds = ogr.Translate('tmp/poly.shp', srcDS, selFields=['eas_id','prfedea'])
    lyr = ds.GetLayer(0)
    if lyr.GetLayerDefn().GetFieldCount() != 2:
        return 'fail'
    feat = lyr.GetNextFeature()
    ret = 'success'
    if feat.GetFieldAsDouble('EAS_ID') != 168:
        gdaltest.post_reason('did not get expected value for EAS_ID')
        print(feat.GetFieldAsDouble('EAS_ID'))
        ret = 'fail'
    elif feat.GetFieldAsString('PRFEDEA') != '35043411':
        gdaltest.post_reason('did not get expected value for PRFEDEA')
        print(feat.GetFieldAsString('PRFEDEA'))
        ret = 'fail'
    feat = None
    ds = None

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return ret

###############################################################################
# Test LCO

def test_ogr2ogr_lib_8():
    
    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    srcDS = ogr.Open('../ogr/data/poly.shp')
    ds = ogr.Translate('tmp/poly.shp', srcDS, LCO=['SHPT=POLYGONZ'])
    if ds.GetLayer(0).GetLayerDefn().GetGeomType() != ogr.wkbPolygon25D:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Add explicit source layer name

def test_ogr2ogr_lib_9():

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    srcDS = ogr.Open('../ogr/data/poly.shp')
    ds = ogr.Translate('tmp/poly.shp', srcDS, layers=['poly'])
    if ds is None or ds.GetLayer(0).GetFeatureCount() != 10:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test -segmentize

def test_ogr2ogr_lib_10():

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    srcDS = ogr.Open('../ogr/data/poly.shp')
    ds = ogr.Translate('tmp/poly.shp', srcDS, layers=['poly'], geomOp=ogr.GEOMOP_SEGMENTIZE, geomOpParam=100)
    if ds is None or ds.GetLayer(0).GetFeatureCount() != 10:
        return 'fail'
    feat = ds.GetLayer(0).GetNextFeature()
    if feat.GetGeometryRef().GetGeometryRef(0).GetPointCount() != 36:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'

###############################################################################
# Test overwrite with a shapefile

def test_ogr2ogr_lib_11():

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    srcDS = ogr.Open('../ogr/data/poly.shp')
    ds = ogr.Translate('tmp/poly.shp',srcDS)
    if ds is None or ds.GetLayer(0).GetFeatureCount() != 10:
        return 'fail'
    ds.Destroy()

    # Overwrite
    ds = ogr.Translate('tmp',srcDS,accessMode=ogr.ACCESS_OVERWRITE)
    if ds is None or ds.GetLayer(0).GetFeatureCount() != 10:
        return 'fail'
    ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    return 'success'

###############################################################################
# Test FIDToFetch

def test_ogr2ogr_lib_12():

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    src_ds = ogr.Open('../ogr/data/poly.shp')
    ds = ogr.Translate('tmp/poly.shp',src_ds,FIDToFetch=8)
    if ds is None or ds.GetLayer(0).GetFeatureCount() != 1:
        return 'fail'
    src_feat = src_ds.GetLayer(0).GetFeature(8)
    feat = ds.GetLayer(0).GetNextFeature()
    if feat.GetField("EAS_ID") != src_feat.GetField("EAS_ID"):
        return 'fail'
    ds.Destroy()
    src_ds.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    return 'success'

###############################################################################
# Test TranslateOptionsSetSpatialFilter()

def test_ogr2ogr_lib_13():

    try:
        os.stat('tmp/poly.shp')
        ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')
    except:
        pass

    srcDS = ogr.Open('../ogr/data/poly.shp')
    options = ogr.TranslateOptions()
    ogr.TranslateOptionsSetSpatialFilter(options,479609,4764629,479764,4764817)
    ds = ogr.Translate('tmp/poly.shp',srcDS,options)
    if ogrtest.have_geos():
        if ds is None or ds.GetLayer(0).GetFeatureCount() != 4:
            return 'fail'
    else:
        if ds is None or ds.GetLayer(0).GetFeatureCount() != 5:
            return 'fail'
    ds.Destroy()
    srcDS.Destroy()

    ogr.GetDriverByName('ESRI Shapefile').DeleteDataSource('tmp/poly.shp')

    return 'success'


gdaltest_list = [
    test_ogr2ogr_lib_1,
    test_ogr2ogr_lib_2,
    test_ogr2ogr_lib_3,
    test_ogr2ogr_lib_4,
    test_ogr2ogr_lib_5,
    test_ogr2ogr_lib_6,
    test_ogr2ogr_lib_7,
    test_ogr2ogr_lib_8,
    test_ogr2ogr_lib_9,
    test_ogr2ogr_lib_10,
    test_ogr2ogr_lib_11,
    test_ogr2ogr_lib_12,
    test_ogr2ogr_lib_13,
    ]

if __name__ == '__main__':

    gdaltest.setup_run( 'test_ogr2ogr_lib' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

