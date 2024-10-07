/**
 *  \file
 *  \brief Asynchronous D-Bus client for Avahi daemon.
 *  \author Christian Eggers
 *  \copyright 2022 ARRI Lighting Stephanskirchen
 *
 *  These classes are based on giomm's D-Bus routines. Compared to the C library
 *  which is provided by the Avahi project, a better integration in C++ (e.g.
 *  memory management) is provided we use pure asynchronous I/O and no
 *  "D-Bus pseudo blocking".
 *
 *  \sa https://github.com/lathiat/avahi
 *  \sa https://smcv.pseudorandom.co.uk/2008/11/nonblocking/
 *  \sa https://networkmanager.dev/blog/notes-on-dbus/
 */

#ifndef SRC_AVAHI_CLIENT_HPP_
#define SRC_AVAHI_CLIENT_HPP_

#include <memory>

#include <glibmm/refptr.h>
#include <sigc++/functors/slot.h>
#include <sigc++/signal.h>

#include <avahi-common/defs.h>         // ::AvahiServerState

#include "Types.hpp"

extern "C" {
	/** "Forward declaration" which avoids including the full <glib.h> file. */
	typedef unsigned int    guint;
}

namespace Gio {
namespace DBus {
	class Connection;
}  // namespace DBus
}  // namespace Gio

namespace Glib {
	class Error;
	class ustring;
}  // namespace Glib

namespace Avahi {

class EntryGroup;
class RecordBrowser;
class ServiceBrowser;
class ServiceResolver;

/** Asynchronous D-Bus client for Avahi daemon. */
class Client
{
	friend EntryGroup;
	friend RecordBrowser;
	friend ServiceBrowser;
	friend ServiceResolver;

public:   // slots
	/** Type for handler to invoke when connection to Avahi daemon has been established. */
	using SlotConnected             = sigc::signal<void()>;
	/** Type for handler to invoke when connection to Avahi daemon has been lost. */
	using SlotDisconnected          = sigc::signal<void()>;
	/** Type for handler to invoke when Avahi server state has changed. */
	using SlotServerStateChanged    = sigc::signal<void(::AvahiServerState state, Glib::ustring const &error)>;

	/** Completion type for async_getServerState. */
	using SlotGetServerState        = sigc::slot<void(::AvahiServerState state, Glib::Error const &error)>;
	/** Completion type for async_getHostName. */
	using SlotGetHostName           = sigc::slot<void(Glib::ustring const &hostname, Glib::Error const &error)>;
	/** Completion type for async_setHostName. */
	using SlotSetHostName           = sigc::slot<void(Glib::Error const &error)>;
	/** Completion type for async_reconfirmRecord. */
	using SlotReconfirmRecord       = sigc::slot<void(Glib::Error const &error)>;
	/** Completion type for async_createEntryGroup. */
	using SlotCreateEntryGroup      = sigc::slot<void(std::shared_ptr<EntryGroup> const &group, Glib::Error const &error)>;
	/** Completion type for async_createRecordBrowser(). */
	using SlotCreateRecordBrowser  = sigc::slot<void(std::shared_ptr<RecordBrowser> const &browser, Glib::Error const &error)>;
	/** Completion type for async_createServiceBrowser(). */
	using SlotCreateServiceBrowser  = sigc::slot<void(std::shared_ptr<ServiceBrowser> const &browser, Glib::Error const &error)>;
	/** Completion type for async_createServiceResolver(). */
	using SlotCreateServiceResolver = sigc::slot<void(std::shared_ptr<ServiceResolver> const &browser, Glib::Error const &error)>;

	/** Handler to invoke when connection to Avahi daemon has been established.
	 *  \sa g_bus_watch_name */
	SlotConnected on_connected;

	/** Handler to invoke when connection to Avahi daemon has been lost.
	 *  \sa g_bus_watch_name */
	SlotDisconnected on_disconncted;

	/** Handler to invoke when Avahi server state has changed.
	 *  \sa   AvahiServerState
	 *  \note Carefully read instructions in %<avahi git%>/avahi-common/defs.h on
	 *        about in which Avahi states the server is ready for publishing
	 *        services.
	 */
	SlotServerStateChanged on_serverStateChanged;

public:   // methods
	Client();
	~Client();
	Client(Client const &other) = delete;
	Client(Client &&other) = delete;
	Client& operator=(Client const &other) = delete;
	Client& operator=(Client &&other) = delete;

	/** Asynchronous getter for the current server state.  Usually this should
	 *  be called once after the Client has been constructed and the
	 *  #on_serverStateChanged handler has been registered in order to get the
	 *  initial state of the Avahi daemon.
	 *
	 *  \param[out] completion  Asynchronous completion handler which receives
	 *                          the current server state.  It is guaranteed that
	 *                          this handler will NOT be called from within this
	 *                          function.
	 */
	void async_getServerState(SlotGetServerState const &completion);

	/** Asynchronous getter for the Avahi host name.  The Avahi daemon uses the
	 *  name provided by gethostname(2) as initial host name after startup.
	 *
	 *  \param[out] completion  Asynchronous completion handler which receives
	 *                          the current Avahi host name.  It is guaranteed
	 *                          that this handler will NOT be called from within
	 *                          this function.
	 */
	void async_getHostName(SlotGetHostName const &completion);

	/** Asynchronous setter for the Avahi host name. Setting the hostname may be
	 *  required if the name has changed after Avahi daemon startup.
	 *
	 *  \param[in]  name        New host name.
	 *  \param[out] completion  Asynchronous completion handler which receives
	 *                          the result of this operation.  It is guaranteed
	 *                          that this handler will NOT be called from within
	 *                          this function.
	 *
	 *  \note The Avahi daemon will respond with an error if called with the
	 *  same hostname as already set.
	 */
	void async_setHostName(Glib::ustring const &name, SlotSetHostName const &completion);

	/** Starts asynchronous reconfirmation of a mDNS entry.
	 *
	 *  \param[in]  interface   (OS specific) index of the network interface
	 *                          where to scan for services.  Using
	 *                          AVAHI_IF_UNSPEC is not allowed.
	 *  \param[in]  protocol    Protocol filter.  Use AVAHI_PROTO_UNSPEC to look
	 *                          for IPv4 and IPv6 services.
	 *  \param[in]  name        Name of the mDNS record (e.g. myhost.local).
	 *  \param[in]  clazz       Class of the mDNS record (e.g. AVAHI_DNS_CLASS_ANY
	 *                          or AVAHI_DNS_CLASS_IN).
	 *  \param[in]  type        Type of mDNS records (e.g. AVAHI_DNS_TYPE_ANY or
	 *                          AVAHI_DNS_TYPE_PTR).
	 *  \param[in]  data        Data of the record to reconfirm.  Use a
	 *                          RecordBrowser for getting the records's data.
	 *  \param[in]  completion  Asynchronous completion handler which receives
	 *                          the result of this operation.  It is guaranteed
	 *                          that this handler will NOT be called from within
	 *                          this function.
	 *
	 *  In order to reconfirm all records a specific host, this method has to
	 *  be called for each individual record (exception: if reconfirming a SRV
	 *  record, the according service type PTR record will also be reconfirmed.
	 *  But this applies not the service subtype PTR records.
	 */
	void async_reconfirmRecord(Interface interface, Protocol protocol, RecordName const &name, RecordClass clazz, RecordType type, RecordData const &data, SlotReconfirmRecord const &completion);

	/** Asynchronous builder for an Avahi entry group.  An entry group is
	 *  used for publishing services.
	 *
	 *  \param[out] completion  Asynchronous completion handler which receives
	 *                          the the newly created entry group.  It is
	 *                          guaranteed that this handler will NOT be called
	 *                          from within this function.
	 *
	 *  \note Carefully read instructions in %<avahi git%>/avahi-common/defs.h
	 *  about which server states are suitable for creating entry groups.
	 */
	void async_createEntryGroup(SlotCreateEntryGroup const &completion);

	/** Asynchronous builder for an Avahi record browser.  A record browser is
	 *  used for enumerating arbitrary mDNS records from Avahi's internal
	 *  database.
	 *
	 *  \param[in]  interface   (OS specific) index of the network interface
	 *                          where enumerate record. Use AVAHI_IF_UNSPEC
	 *                          to enumerate all interfaces.
	 *  \param[in]  protocol    Protocol filter. Use AVAHI_PROTO_UNSPEC to
	 *                          enumerate IPv4 and IPv6 records.
	 *  \param[in]  name        Record name (e.g. "myhost.local").  No wildcard
	 *                          nor empty string can be supplied here.
	 *  \param[in]  clazz       Class of mDNS records (e.g. AVAHI_DNS_CLASS_ANY
	 *                          or AVAHI_DNS_CLASS_IN).
	 *  \param[in]  type        Type of mDNS records (e.g. AVAHI_DNS_TYPE_ANY or
	 *                          AVAHI_DNS_TYPE_PTR).
	 *  \param[in]  flags       Flags. See AvahiLookupFlags for details.
	 *  \param[out] completion  Asynchronous completion handler which receives
	 *  the the newly created service browser.  It is guaranteed that this
	 *  handler will NOT be called from within this function.
	 *
	 *  \sa avahi_record_browser_new()
	 */
	void async_createRecordBrowser(Interface interface, Protocol protocol,
	                               RecordName const &name, RecordClass clazz,
	                               RecordType type, ::AvahiLookupFlags flags,
	                               SlotCreateRecordBrowser const &completion);

	/** Asynchronous builder for an Avahi service browser.  A service browser is
	 *  used for finding Avahi services on the network.
	 *
	 *  \param[in]  interface   (OS specific) index of the network interface
	 *                          where to scan for services. Use AVAHI_IF_UNSPEC
	 *                          to scan on all interfaces.
	 *  \param[in]  protocol    Protocol filter. Use AVAHI_PROTO_UNSPEC to scan
	 *                          for IPv4 and IPv6 services.
	 *  \param[in]  type        Service type filter (e.g. "_http._tcp"). If an
	 *                          empty string is provided, all service types will
	 *                          be returned. Also a service sub type (e.g.
	 *                          "_myservice._sub._http._tcp") can be used here.
	 *  \param[in]  domain      Browse domain. An empty string means ".local".
	 *  \param[in]  flags       Flags. See AvahiLookupFlags for details.
	 *  \param[out] completion  Asynchronous completion handler which receives
	 *  the the newly created service browser.  It is guaranteed that this
	 *  handler will NOT be called from within this function.
	 *
	 *  \sa avahi_service_browser_new()
	 */
	void async_createServiceBrowser(Interface interface, Protocol protocol,
	                                ServiceType const &type, Domain const &domain,
	                                ::AvahiLookupFlags flags,
	                                SlotCreateServiceBrowser const &completion);

	/** Asynchronous builder for an Avahi service resolver.  A service resolver
	 *  is used to resolve the hostname/address/port/txt of services found by
	 *  a ServiceBrowser.
	 *
	 *  \param[in]  interface   (OS specific) index of the network interface
	 *                          to use for resolving. Should match the actual
	 *                          protocol provided by service browser.
	 *  \param[in]  protocol    Protocol used for transport of mDNS queries.
	 *                          Should match the actual protocol provided by the
	 *                          service browser.
	 *  \param[in]  name        Pass the value received from the ServiceBrowser
	 *                          here.
	 *  \param[in]  type        Pass the value received from the ServiceBrowser
	 *                          here.
	 *  \param[in]  domain      Pass the value received from the ServiceBrowser
	 *                          here.
	 *  \param[in]  aprotocol   Specifies whether in IPv4 (A) or IPv6 (AAAA)
	 *                          address record shall be queried. AVAHI_PROTO_UNSPEC
	 *                          if the application can deal with IPv4 and IPv6.
	 *  \param[in]  flags       Flags. See AvahiLookupFlags for details.
	 *  \param[out] completion  Asynchronous completion handler which receives
	 *  the the newly created service resolver.  It is guaranteed that this
	 *  handler will NOT be called from within this function.
	 *
	 *  \note In order to receive further updates of service data (e.g. TXT),
	 *        the service resolver must be kept alive after its has returned
	 *        the service data (via its slot mechanism).
	 *
	 *  \sa avahi_service_resolver_new()
	 */
	void async_createServiceResolver(Interface interface, Protocol protocol,
	                                 ServiceName const &name, ServiceType const &type,
	                                 Domain const &domain, Protocol aprotocol,
	                                 ::AvahiLookupFlags flags,
	                                 SlotCreateServiceResolver const &completion);

private:  // methods
	Glib::RefPtr<Gio::DBus::Connection> const &getConnection();

private:  // members
	guint m_watchHandle;
	Glib::RefPtr<Gio::DBus::Connection> m_connection;
	guint m_stateChanged;
};

} /* namespace Avahi */

#endif /* SRC_AVAHI_CLIENT_HPP_ */
