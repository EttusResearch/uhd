#
# Try to find the wireshark library and its includes
#
# This snippet sets the following variables:
#  WIRESHARK_FOUND             True if wireshark library got found
#  WIRESHARK_INCLUDE_DIRS      Location of the wireshark headers 
#  WIRESHARK_LIBRARIES         List of libraries to use wireshark
#
#  Copyright (c) 2011 Reinhold Kainhofer <reinhold@kainhofer.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

# wireshark does not install its library with pkg-config information,
# so we need to manually find the libraries and headers

find_path( WIRESHARK_INCLUDE_DIRS epan/column.h PATH_SUFFIXES wireshark )
find_library( WIRESHARK_LIBRARIES wireshark )

# Report results
if ( WIRESHARK_LIBRARIES AND WIRESHARK_INCLUDE_DIRS )
  set( WIRESHARK_FOUND 1 )
else ( WIRESHARK_LIBRARIES AND WIRESHARK_INCLUDE_DIRS )
  message( SEND_ERROR "Could NOT find the wireshark library and headers" )
endif ( WIRESHARK_LIBRARIES AND WIRESHARK_INCLUDE_DIRS )

