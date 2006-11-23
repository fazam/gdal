/******************************************************************************
 * $Id$
 *
 * Name:     ogr_csharp_extensions.i
 * Project:  GDAL SWIG Interface
 * Purpose:  C# specific OGR extensions 
 * Author:   Tamas Szekeres (szekerest@gmail.com)
 *
 *
 * $Log$
 * Revision 1.1  2006/11/23 22:50:53  tamas
 * C# ExportToWkb support
 *
 *
 */
 
/******************************************************************************
 * OGR WKB export                                                             *
 *****************************************************************************/

%extend OGRGeometryShadow 
{
	%apply (void *buffer_ptr) {char *buffer};
	OGRErr ExportToWkb( int bufLen, char *buffer, OGRwkbByteOrder byte_order ) {
      if (bufLen < OGR_G_WkbSize( self ))
        CPLError(CE_Failure, 1, "Array size is small (ExportToWkb).");
      return OGR_G_ExportToWkb(self, byte_order, (unsigned char*) buffer );
    }
    %clear char *buffer;
}
