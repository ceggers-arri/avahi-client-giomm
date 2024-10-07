/**
 *  \file
 *  \brief Common data types used by Avahi D-Bus wrapper
 *  \author Christian Eggers
 *  \copyright 2022 ARRI Lighting Stephanskirchen
 */

#ifndef SRC_AVAHI_TYPES_HPP_
#define SRC_AVAHI_TYPES_HPP_

#include <cstdint>
#include <vector>

namespace Glib {
	class ustring;
}  // namespace Glib


/** \brief Namespace for Avahi D-Bus client library */
namespace Avahi {

/** OS dependent index for network interface, AVAHI_IF_UNSPEC means "all interfaces". */
using Interface   = std::int32_t;
/** Network protocol, possible values are AVAHI_PROTO_INET, AVAHI_PROTO_INET6 and AVAHI_PROTO_UNSPEC. */
using Protocol    = std::int32_t;
/** Avahi record name. */
using RecordName  = Glib::ustring;
/** Avahi record type (e.g. AVAHI_DNS_TYPE_*). */
using RecordType = std::uint16_t;
/** Avahi record class (e.g. AVAHI_DNS_CLASS_IN). */
using RecordClass = std::uint16_t;
/** Avahi record data. */
using RecordData  = std::vector<std::uint8_t>;
/** Avahi service name. */
using ServiceName = Glib::ustring;
/** Avahi service type (e.g. "_http._tcp"). */
using ServiceType = Glib::ustring;
/** Avahi domain. An empty string usually equals ".local". */
using Domain      = Glib::ustring;
/** Host name (of a system which provides a particular Avahi service). */
using Host        = Glib::ustring;
/** TCP/UDP port number of a service. */
using Port        = std::uint16_t;
/** TXT data of a service. */
using Txt         = std::vector<std::vector<std::uint8_t>>;
/** Error messages from Avahi daemon. */
using Error       = Glib::ustring;

} /* namespace Avahi */

#endif /* SRC_AVAHI_TYPES_HPP_ */
