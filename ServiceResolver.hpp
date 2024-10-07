/**
 *  \file
 *  \brief D-Bus wrapper for Avahi service resolver
 *  \author Christian Eggers
 *  \copyright 2022 ARRI Lighting Stephanskirchen
 */

#ifndef SRC_AVAHI_SERVICERESOLVER_HPP_
#define SRC_AVAHI_SERVICERESOLVER_HPP_

#include <cstdint>

#include <glibmm/ustring.h>
#include <sigc++/signal.h>

#include <avahi-common/defs.h>         // ::AvahiLookupResultFlags

#include "Types.hpp"

extern "C" {
	/** "Forward declaration" which avoids including the full <glib.h> file. */
	typedef unsigned int    guint;
}

namespace Avahi {

class Client;

/** Proxy for an Avahi service resolver.  A service resolver is used to resolve
 *  the hostname/address/port/txt of services found by a ServiceBrowser. */
class ServiceResolver
{
	/* Use passkey idiom as constructor of ServiceResolver cannot be made
	 * private (due to use of std::make_shared()). */
	friend Client;
	class Token {};

public:   // types
	/** Specifies whether in IPv4 (A) or IPv6 (AAAA) address record shall be
	 * queried. AVAHI_PROTO_UNSPEC if the application can deal with IPv4 and IPv6. */
	using AProtocol = std::int32_t;

	/** IPv4/IPv6 address as string on the usual notation. */
	using Address   = Glib::ustring;

public:   // slots
	/** Type for handler to invoke when an error message shall be printed to
	 *  the application's log file. */
	using SlotErrorLog       = sigc::signal<void(char const *error)>;
	/** Type for handler to invoke when resolving was successful. */
	using SlotFound          = sigc::signal<void(/*Interface interface, Protocol protocol, */ServiceName const &name, /*ServiceType const &type, Domain const &domain, */Host const &host, AProtocol aprotocol, Address const &address, Port port, Txt const &txt, ::AvahiLookupResultFlags flags)>;
	/** Type for handler to invoke when resolving has failed due to some reason. */
	using SlotFailure        = sigc::signal<void(Error const &error)>;

	/** Handler to invoke when an error message shall be printed to the
	 *  application's log file.
	 *  \note Currently this is only used when a parsing error happens during
	 *        reception of a D-Bus signal.
	 */
	SlotErrorLog on_errorLog;
	/** Handler to invoke when resolving was successful. */
	SlotFound on_found;
	/** Handler to invoke when resolving has failed due to some reason. */
	SlotFailure on_failure;

public:   // methods
	/** Do not call this directly. Use Client::async_createEntryServiceResolver() instead. */
	ServiceResolver(Token, Client &client, Glib::ustring const &objectPath);
	~ServiceResolver();
	ServiceResolver(ServiceResolver const &other) = delete;
	ServiceResolver(ServiceResolver &&other) = delete;
	ServiceResolver& operator=(ServiceResolver const &other) = delete;
	ServiceResolver& operator=(ServiceResolver &&other) = delete;

private:  // members
	Client &m_client;
	Glib::ustring m_objectPath;
	::guint m_found, m_failure;
};

} /* namespace AvahiLib */

#endif /* SRC_AVAHI_SERVICERESOLVER_HPP_ */
