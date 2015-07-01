#!/usr/bin/env python
# -*- coding: utf-8 -*-
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  test librarified gdal_translate
# Author:   Faza Mahamood <fazamhd at gmail dot com>
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

sys.path.append( '../pymod' )

from osgeo import gdal
import gdaltest

###############################################################################
# Simple test

def test_gdal_translate_1():
    
    ds = gdal.Open('../gcore/data/byte.tif')
    options = gdal.TranslateOptions()

    ds = gdal.Translate('tmp/test1.tif', ds, options)
    if ds is None:
        gdaltest.post_reason('got error/warning')
        print(err)
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 4672:
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None
    
    ds = gdal.Open('tmp/test1.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 4672:
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Cleanup

def test_gdal_translate_cleanup():
    for i in range(1):
        try:
            os.remove('tmp/test' + str(i+1) + '.tif')
        except:
            pass
        try:
            os.remove('tmp/test' + str(i+1) + '.tif.aux.xml')
        except:
            pass

    return 'success'

gdaltest_list = [
    test_gdal_translate_1,
    test_gdal_translate_cleanup
    ]


if __name__ == '__main__':

    gdaltest.setup_run( 'test_gdal_translate_lib' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()
