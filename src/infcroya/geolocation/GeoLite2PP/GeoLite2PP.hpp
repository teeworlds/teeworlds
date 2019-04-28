/* GeoLite2++ (C) 2016-2018 Stephane Charette <stephanecharette@gmail.com>
 * $Id: GeoLite2PP.hpp 2549 2018-06-08 18:48:31Z stephane $
 */

#pragma once

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <system_error>
#include <maxminddb.h>


/** The entire GeoLite2++ library is encapsulated in the GeoLite2PP namespace to prevent namespace pollution.
 *
 * Note that GeoLite2++ builds on top of the existing C library "libmaxminddb".  GeoLite2++ provides alternate C++
 * bindings, it is not a replacement!  GeoLite2++ was written to provide an easier method from C++ to access MaxMind's
 * GeoLite2 GeoIP database.
 *
 * The original C libmaxminddb library is here:  https://github.com/maxmind/libmaxminddb/
 */
namespace GeoLite2PP
{
	/// Map of std::string to std::string.
	typedef std::map<std::string, std::string> MStr;

	/// Vector of C-style string pointers.
	typedef std::vector<const char *> VCStr;

	/** The DB class is the primary class used by GeoLite2++.  It is used to point to the MaxMind's GeoLite2's database
	 * and to retrieve information based on IP addresses.
	 */
	class DB final
	{
		public:

			/// Destructor.
			~DB( void );

			/** Constructor.
			* @param [in] database_filename The database filename is the @p .mmdb file typically downloaded from
			* MaxMind.  Normally, this file is called @p GeoLite2-City.mmdb and may be downloaded for free from
			* http://dev.maxmind.com/geoip/geoip2/geolite2/#Downloads.  The name used can be relative to the current
			* working directory, or an absolute path and filename.
			*
			* ~~~~
			* std::string filename = "/opt/my_project_files/GeoLite2-City.mmdb";
			*
			* GeoLite2PP::DB db( filename );
			*
			* std::cout << db.get_mmdb_lib_version()  << std::endl;
			* std::cout << db.lookup_metadata()       << std::endl;
			* ~~~~
			*
			* @warning <b>An object of type @p GeoLite2PP::DB cannot be constructed without a valid database file!</b>
			* 
			* @note The database can be manually downloaded, or the script @p geolite2pp_get_database.sh can be used
			* to easily download and extract the database into the current working directory.  But either way, the
			* MaxMind database must be available for GeoLite2++ to work.
			*/
			DB( const std::string &database_filename = "GeoLite2-City.mmdb" );

			/** Get the MaxMind library version number.
			 *
			 * ~~~~
			 * GeoLite2PP::DB db;
			 * std::cout << db.get_lib_version_mmdb() << std::endl;
			 * ~~~~
			 *
			 * Example output:
			 * ~~~~{.txt}
			 * 1.2.0
			 * ~~~~
			 */
			std::string get_lib_version_mmdb( void ) const;

			/** Get the GeoLite2++ library version number.
			 *
			 * ~~~~
			 * GeoLite2PP::DB db;
			 * std::cout << db.get_lib_version_geolite2pp() << std::endl;
			 * ~~~~
			 *
			 * Example output:
			 * ~~~~{.txt}
			 * 0.0.1-1992
			 * ~~~~
			 */
			std::string get_lib_version_geolite2pp( void ) const;

			/** Get the database metadata.  This returns a raw @p MMDB_metadata_s structure.  This call is intended
			 * mostly for internal purposes, or to call directly into the original C MMDB API.
			 * @see @ref get_metadata()
			 */
			MMDB_metadata_s get_metadata_raw( void );

			/** Get the database metadata as a JSON string.
			 *
			 * ~~~~
			 * GeoLite2PP::DB db;
			 * std::cout << db.get_metadata() << std::endl;
			 * ~~~~
			 *
			 * Example output:
			 * ~~~~{.json}
			 * {
			 *     "binary_format_major_version" : 2,
			 *     "binary_format_minor_version" : 0,
			 *     "build_epoch" : 1475588619,
			 *     "database_type" : "GeoLite2-City",
			 *     "description" : { "en" : "GeoLite2 City database" },
			 *     "ip_version" : 6,
			 *     "languages" : [ "de", "en", "es", "fr", "ja", "pt-BR", "ru", "zh-CN" ],
			 *     "node_count" : 4065439,
			 *     "record_size" : 28
			 * }
			 * ~~~~
			 * @see @ref get_metadata_raw()
			 */
			std::string get_metadata( void );

			/** Look up an IP address.  This returns a raw @p MMDB_lookup_result_s structure.  This call is intended
			 * mostly for internal purposes, or to call directly into the original C MMDB API.
			 * @see @ref lookup()
			 */
			MMDB_lookup_result_s lookup_raw( const std::string &ip_address );

			/** Look up an IP address and return a JSON string of everything found.
			 *
			 * ~~~~
			 * GeoLite2PP::DB db;
			 * std::string json = db.lookup( "65.44.217.6" );
			 * std::cout << json << std::endl;
			 * ~~~~
			 *
			 * Example output:
			 * ~~~~{.json}
			 * {  "city"      : { "names" : { "en" : "Fresno" } },
			 *    "continent" : { "code" : "NA", "names" : { "en" : "North America" } },
			 *    "country"   : { "iso_code" : "US", "names" : { "en" : "United States" } },
			 *    "location"  : { "accuracy_radius" : 200,
			 *                    "latitude" : 36.6055,
			 *                    "longitude" : -119.752,
			 *                    "time_zone" : "America/Los_Angeles" },
			 *    "postal"    : { "code" : "93725" },
			 *    "subdivisions" : [ { "iso_code" : "CA", "names" : { "en" : "California" } } ]
			 * }
			 * ~~~~
			 * @note <i>For clarity, some of the JSON entries have been removed in this example output.</i>
			 */
			std::string lookup( const std::string &ip_address );

			/** Return a @p std::map of many of the key fields available when looking up an address.  This includes
			 * fields such as subdivision name, city name, country name, continent name, longitude, latitude, accuracy
			 * radius, and relevant iso codes.  Not all fields are available for all addresses and languages.
			 *
			 * @note The method is mis-named. It returns @b many fields, but definitely does not return @b all fields.
			 * To see all fields, see @ref lookup().
			 *
			 * ~~~~
			 * GeoLite2PP::DB db;
			 * GeoLite2PP::MStr m = db.get_all_fields( "65.44.217.6" );
			 * for ( const auto iter : m )
			 * {
			 * 		std::cout << iter.first << " -> " << iter.second << std::endl;
			 * }
			 * ~~~~
			 *
			 * Example output:
			 * ~~~~{.txt}
			 * accuracy_radius -> 200
			 * city -> Fresno
			 * continent -> North America
			 * country -> United States
			 * country_iso_code -> US
			 * latitude -> 36.605500
			 * longitude -> -119.752200
			 * postal_code -> 93725
			 * query_ip_address -> 65.44.217.6
			 * query_language -> en
			 * registered_country -> United States
			 * subdivision -> California
			 * subdivision_iso_code -> CA
			 * time_zone -> America/Los_Angeles
			 * ~~~~
			 */
			MStr get_all_fields( const std::string &ip_address, const std::string &language = "en" );

			/** Get a specific field, or an empty string if the field does not exist.
			 *
			 * A lookup of the given IP address needs to be performed every time this is called, which makes this less
			 * efficient than the other @p get_field() method which takes a @p MMDB_lookup_result_s parameter.
			 *
			 * ~~~~
			 * GeoLite2PP::DB db;
			 * std::string city     = db.get_field( "65.44.217.6", "en", GeoLite2PP::VCStr { "city"    , "names"    } );
			 * std::string country  = db.get_field( "65.44.217.6", "en", GeoLite2PP::VCStr { "country" , "names"    } );
			 * std::string latitude = db.get_field( "65.44.217.6", "en", GeoLite2PP::VCStr { "location", "latitude" } );
			 * ~~~~
			 */
			std::string get_field( const std::string &ip_address, const std::string &language, const VCStr &v );

			/** Get a specific field, or an empty string if the field does not exist.
			 *
			 * This is a more efficient call than the other @p get_field() method since the address doesn't need to
			 * be looked up in the database at every call.
			 *
			 * ~~~~
			 * GeoLite2PP::DB db;
			 * MMDB_lookup_result_s result = db.lookup_raw( "65.44.217.6" );
			 * std::string city     = db.get_field( &result, "en", GeoLite2PP::VCStr { "city"    , "names"    } );
			 * std::string country  = db.get_field( &result, "en", GeoLite2PP::VCStr { "country" , "names"    } );
			 * std::string latitude = db.get_field( &result, "en", GeoLite2PP::VCStr { "location", "latitude" } );
			 * ~~~~
			 */
			std::string get_field( MMDB_lookup_result_s *lookup, const std::string &language, const VCStr &v );

			/** Process the specified node list and return a JSON-format string.
			 *
			 * @warning This @b will de-allocate the node list prior to returning; do not call
			 * @p MMDB_free_entry_data_list() on the node.
			 *
			 * This call is intended mostly for internal purposes.  @see @ref lookup()  @see @ref get_metadata()
			 */
			std::string to_json( MMDB_entry_data_list_s *node );

		private:

			/// Internal handle to the database.  @see @p MMDB_open()
			MMDB_s mmdb;

			/// Traverse a list of nodes and build up a JSON string.  This method is not exposed.  It is for internal use only.
			void create_json_from_entry( std::stringstream &ss, size_t depth, uint32_t data_size, MMDB_entry_data_list_s * &next, const bool in_array = false );

			/// Look up a specific name (e.g., "city") and store it in the specified map.  This method is not exposed.  It is for internal use only.
			void add_to_map( GeoLite2PP::MStr &m, MMDB_lookup_result_s *node, const std::string &name, const std::string &language, const VCStr &v );
	};
}
