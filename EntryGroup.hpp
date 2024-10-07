/**
 *  \file
 *  \brief D-Bus wrapper for Avahi entry group
 *  \author Christian Eggers
 *  \copyright 2022 ARRI Lighting Stephanskirchen
 */

#ifndef SRC_AVAHI_ENTRYGROUP_HPP_
#define SRC_AVAHI_ENTRYGROUP_HPP_

#include <glibmm/ustring.h>
#include <sigc++/functors/slot.h>
#include <sigc++/signal.h>

#include <avahi-common/defs.h>         // ::AvahiPublishFlags, ::AvahiEntryGroupState

#include "Types.hpp"

extern "C" {
	/** "Forward declaration" which avoids including the full <glib.h> file. */
	typedef unsigned int    guint;
}

namespace Glib {
	class Error;
}  // namespace Glib


namespace Avahi {

class Client;

/** Proxy for an Avahi entry group.  An entry group is required for publishing
 *  services.  An object of this class cannot be created directly.  Use
 *  Client::async_createEntryGroup() instead. */
class EntryGroup
{
	/* Use passkey idiom as constructor of EntryGroup cannot be made private
	 * (due to use of std::make_shared()). */
	friend Client;
	class Token {};

public:   // types
	/** Avahi service subtype type (e.g. "_orbiter._sub._http._tcp"). */
	using Subtype   = Glib::ustring;

public:   // slots
	/** Type for handler to invoke when an error message shall be printed to
	 *  the application's log file. */
	using SlotErrorLog     = sigc::signal<void(char const *error)>;
	/** Type for handler to invoke when the state of the entry group has
	 *  changed. */
	using SlotStateChanged = sigc::signal<void(::AvahiEntryGroupState state, Glib::ustring const &error)>;
	/** Handler to invoke when an error message shall be printed to the
	 *  application's log file.
	 *  \note Currently this is only used when a parsing error happens during
	 *        reception of a D-Bus signal.
	 */
	SlotErrorLog     on_errorLog;
	/** Handler to invoke when the state of the entry group has changed.
	 *
	 *  \note Carefully read instructions in %<avahi git%>/avahi-common/defs.h
	 *        on about how to handle entry group state changes.
	 */
	SlotStateChanged on_stateChanged;

	/** Completion type for async_addService. */
	using SlotAddService        = sigc::slot<void(Glib::Error const &error)>;
	/** Completion type for async_addServiceSubtype. */
	using SlotAddServiceSubtype = sigc::slot<void(Glib::Error const &error)>;
	/** Completion type for async_commit. */
	using SlotCommit            = sigc::slot<void(Glib::Error const &error)>;
	/** Completion type for async_reset. */
	using SlotReset             = sigc::slot<void(Glib::Error const &error)>;
	/** Completion type for async_updateServiceTxt. */
	using SlotUpdateServiceTxt  = sigc::slot<void(Glib::Error const &error)>;

public:   // methods
	/** Do not call this directly. Use Client::async_createEntryGroup() instead. */
	EntryGroup(Token, Client &client, Glib::ustring const &objectPath);
	~EntryGroup();
	EntryGroup(EntryGroup const &other) = delete;
	EntryGroup(EntryGroup &&other) = delete;
	EntryGroup& operator=(EntryGroup const &other) = delete;
	EntryGroup& operator=(EntryGroup &&other) = delete;

	/** Asynchronous method for adding a service to an entry group.
	 *
	 *  \param[in]  interface   (OS specific) interface index where the service
	 *                          shall be announced on. Use AVAHI_IF_UNSPEC to
	 *                          publish on all interfaces.
	 *  \param[in]  protocol    The protocol the service shall be announced
	 *                          with. Use AVAHI_PROTO_UNSPEC to announce on
	 *                          all protocols enabled on Avahi daemon.
	 *  \param[in]  flags       See AvahiPublishFlags.
	 *  \param[in]  name        The name for the new service. Must be a valid
	 *                          service name (i.e. shorter than 63 characters
	 *                          and valid UTF-8). May not be empty.
	 *  \param[in]  type        The service type for the new service, such as
	 *                          _http._tcp. May not be empty.
	 *  \param[in]  domain      The domain to register this service in. If empty,
	 *                          Avahi daemon sets to domain (e.g. ".local").
	 *  \param[in]  host        The host this services is residing on. If empty,
	 *                          the local host name will be used.
	 *  \param[in]  port        The TCP/UDP port number of this service.
	 *  \param[in]  txt         The TXT data for this service.  An internal copy
	 *                          of this data will be created.
	 *  \param[out] completion  Asynchronous completion handler which receives
	 *                          the result of this operation.  It is guaranteed
	 *                          that this handler will NOT be called from within
	 *                          this function.
	 *
	 *  \sa avahi_entry_group_add_service_strlst()
	 *  \note Carefully read instructions in %<avahi git%>/avahi-common/defs.h
	 *        on about when services can be added to an entry group.
	 */
	void async_addService(Interface interface, Protocol protocol,
	                      ::AvahiPublishFlags flags,
	                      ServiceName const &name, ServiceType const &type,
	                      Domain const &domain, Host const &host, Port port,
	                      Txt const &txt, SlotAddService const &completion);

	/** Asynchronous method for adding a service to an entry group.
	 *
	 *  \param[in]  interface   (OS specific) interface index where the service
	 *                          shall be announced on. Use the same value as in
	 *                          async_addService().
	 *  \param[in]  protocol    The protocol the service shall be announced
	 *                          with.  Use the same value as in
	 *                          async_addService().
	 *  \param[in]  flags       Usually 0.
	 *  \param[in]  name        The name for the new service.  Use the same
	 *                          value as in async_addService().
	 *  \param[in]  type        The service type.  Use the same value as in
	 *                          async_addService().
	 *  \param[in]  domain      The domain of this servce.  Use the same value
	 *                          as in async_addService().
	 *  \param[in]  subtype     The new subtype to register for the specified
	 *                          service (e.g. "_orbiter._sub._http._tcp"). Must
	 *                          not be empty.
	 *  \param[out] completion  Asynchronous completion handler which receives
	 *                          the result of this operation.  It is guaranteed
	 *                          that this handler will NOT be called from within
	 *                          this function.
	 *
	 *  \sa avahi_entry_group_add_service_subtype()
	 */
	void async_addServiceSubtype(Interface interface, Protocol protocol,
	                             ::AvahiPublishFlags flags,
	                             ServiceName const &name, ServiceType const &type,
	                             Domain const &domain, Subtype const &subtype,
	                             SlotAddServiceSubtype const &completion);

	/** Asynchronous method for committing an EntryGroup. The entries in the
	 *  entry group are now registered on the network.
	 *
	 *  \param[out] completion  Asynchronous completion handler which receives
	 *                          the result of this operation.  It is guaranteed
	 *                          that this handler will NOT be called from within
	 *                          this function.
	 *
	 *  \note Committing empty entry groups is considered an error.
	 *  \note After calling async_reset() or async_updateServiceTxt(), no
	 *        further call to async_commit() is required.
	 */
	void async_commit(SlotCommit const &completion);

	/** Asynchronous method for resetting an EntryGroup. All entries will be
	 *  removed immediately.
	 *
	 *  \param[out] completion  Asynchronous completion handler which receives
	 *                          the result of this operation.  It is guaranteed
	 *                          that this handler will NOT be called from within
	 *                          this function.
	 */
	void async_reset(SlotReset const &completion);

	/** Asynchronous method for efficient update of service TXT data for
	 *  already existing services.
	 *
	 *  \param[in]  interface   (OS specific) interface index where the service
	 *                          shall be announced on. Use the same value as in
	 *                          async_addService().
	 *  \param[in]  protocol    The protocol the service shall be announced
	 *                          with.  Use the same value as in
	 *                          async_addService().
	 *  \param[in]  flags       Usually 0.
	 *  \param[in]  name        The name for the new service.  Use the same
	 *                          value as in async_addService().
	 *  \param[in]  type        The service type.  Use the same value as in
	 *                          async_addService().
	 *  \param[in]  domain      The domain of this servce.  Use the same value
	 *                          as in async_addService().
	 *  \param[in]  txt         The new TXT data for this service.  An internal
	 *                          copy of this data will be created.
	 *  \param[out] completion  Asynchronous completion handler which receives
	 *                          the result of this operation.  It is guaranteed
	 *                          that this handler will NOT be called from within
	 *                          this function.
	 *
	 *  \note No further call to async_commit() is required.
	 *  \sa avahi_entry_group_update_service_txt_strlst()
	 */
	void async_updateServiceTxt(Interface interface, Protocol protocol,
	                            ::AvahiPublishFlags flags,
	                            ServiceName const &name, ServiceType const &type,
	                            Domain const &domain, Txt const &txt,
	                            SlotUpdateServiceTxt const &completion);

private:
	Client &m_client;
	Glib::ustring m_objectPath;
	::guint m_stateChanged;
};

} /* namespace Avahi */

#endif /* SRC_AVAHI_ENTRYGROUP_HPP_ */
