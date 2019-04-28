/* GeoLite2++ (C) 2016-2018 Stephane Charette <stephanecharette@gmail.com>
 * $Id: GeoLite2PP_error_category.cpp 2549 2018-06-08 18:48:31Z stephane $
 */


#include "GeoLite2PP_error_category.hpp"
#include <maxminddb.h>


const char *GeoLite2PP::ErrorCategory::name( void ) const noexcept
{
	return "GeoLite2PP";
}



std::string GeoLite2PP::ErrorCategory::message( int code ) const
{
	std::string msg = MMDB_strerror( code );
	if ( msg.empty() )
	{
		msg = "unknown MMDB error #" + std::to_string( code );
	}

	return msg;
}


const GeoLite2PP::ErrorCategory & GeoLite2PP::get_error_category( void ) noexcept
{
	static ErrorCategory ecat;
	return ecat;
}


std::error_code GeoLite2PP::make_error_code( GeoLite2PP::MMDBStatus s )
{
	return std::error_code( static_cast<int>(s), get_error_category() );
}


std::error_condition GeoLite2PP::make_error_condition( GeoLite2PP::MMDBStatus s )
{
	return std::error_condition( static_cast<int>(s), get_error_category() );
}
