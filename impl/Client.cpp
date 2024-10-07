/**
 *  \file
 *  \brief D-Bus client for Avahi daemon.
 *  \author Christian Eggers
 *  \copyright 2022 ARRI Lighting Stephanskirchen
 */

#include <cstdint>
#include <tuple>
#include <typeinfo>                    // std::bad_cast

#include <giomm/dbusconnection.h>
#include <giomm/dbuswatchname.h>
#include <glibmm/error.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <glibmm/variant.h>
#include <glibmm/variantdbusstring.h>

#include <avahi-common/dbus.h>
#include <glib.h>                      // G_MAXINT, G_VARIANT_PARSE_ERROR, G_VARIANT_PARSE_ERROR_FAILED

#include "../EntryGroup.hpp"
#include "../RecordBrowser.hpp"
#include "../ServiceBrowser.hpp"
#include "../ServiceResolver.hpp"
#include "../Client.hpp"               // IWYU pragma: associated

namespace Gio { class AsyncResult; }

namespace Avahi {

Client::Client()
: m_connection()
, m_stateChanged(0)
{
	m_watchHandle = Gio::DBus::watch_name(
		Gio::DBus::BusType::BUS_TYPE_SYSTEM,
		"org.freedesktop.Avahi",
		/* name_appeared_slot */
		[this](Glib::RefPtr<Gio::DBus::Connection> const &connection,
		       Glib::ustring const &/*name*/,
		       Glib::ustring const &/*nameOwner*/)
		{
			m_connection = connection;

			m_stateChanged = connection->signal_subscribe(
				[this](Glib::RefPtr<Gio::DBus::Connection> const &/*connection*/,
				       Glib::ustring const &/*senderName*/,
				       Glib::ustring const &/*objectPath*/,
				       Glib::ustring const &/*interfaceName*/,
				       Glib::ustring const &/*signalName*/,
				       Glib::VariantContainerBase const &parameters)
				{
					using State = std::int32_t;
					using Error = Glib::ustring;
					using Params = std::tuple<State, Error>;

					try
					{
						auto params = Glib::VariantBase::cast_dynamic<Glib::Variant<Params>>(parameters);
						const auto &[state, error] = params.get();

						on_serverStateChanged(static_cast<AvahiServerState>(state), error);
					}
					catch (std::bad_cast const &e)
					{
						on_serverStateChanged(AVAHI_SERVER_FAILURE, "Cannot parse \"StateChanged\" parameters");
						return;
					}
				},
				AVAHI_DBUS_NAME,
				AVAHI_DBUS_INTERFACE_SERVER,
				"StateChanged",
				"/");

			on_connected();
		},
		/* name_vanished_slot */
		[this](Glib::RefPtr<Gio::DBus::Connection> const &connection,
		       Glib::ustring const &/*name*/)
		{
			on_disconncted();
			m_connection = {};

			connection->signal_unsubscribe(m_stateChanged);
			m_stateChanged = 0;
		});
}

Client::~Client()
{
	Gio::DBus::unwatch_name(m_watchHandle);
}

Glib::RefPtr<Gio::DBus::Connection> const &Client::getConnection()
{
	return m_connection;
}

void Client::async_getServerState(SlotGetServerState const &completion)
{
	m_connection->call(
		"/", AVAHI_DBUS_INTERFACE_SERVER, "GetState",
		Glib::VariantContainerBase(),
		[connection = m_connection, completion](Glib::RefPtr<Gio::AsyncResult> &result)
		{
			try
			{
				auto response = connection->call_finish(result);

				try
				{
					using State = std::int32_t;
					using Params = std::tuple<State>;
					auto parsedParams = Glib::VariantBase::cast_dynamic<Glib::Variant<Params>>(response);
					auto const &[state] = parsedParams.get();  // unpack variant+tuple

					completion(static_cast<AvahiServerState>(state), Glib::Error());
				}
				catch (std::bad_cast const &e)
				{
					completion(AvahiServerState{}, Glib::Error(G_VARIANT_PARSE_ERROR, G_VARIANT_PARSE_ERROR_FAILED, "Client: Cannot parse response to \"GetState\""));
				}
			}
			catch (Glib::Error const &e)
			{
				completion(AvahiServerState{}, e);
			}
		},
		/*bus_name*/ AVAHI_DBUS_NAME,
		/*timeout_msec*/G_MAXINT,
		Gio::DBus::CALL_FLAGS_NO_AUTO_START
	);
}

void Client::async_getHostName(SlotGetHostName const &completion)
{
	m_connection->call(
		"/", AVAHI_DBUS_INTERFACE_SERVER, "GetHostName",
		Glib::VariantContainerBase(),
		[connection = m_connection, completion](Glib::RefPtr<Gio::AsyncResult> &result)
		{
			try
			{
				auto response = connection->call_finish(result);

				try
				{
					using ParamsType = std::tuple<Glib::ustring>;
					auto const parsedParams = Glib::VariantBase::cast_dynamic<Glib::Variant<ParamsType>>(response);
					auto const &[hostname] = parsedParams.get();  // unpack variant+tuple

					completion(hostname, Glib::Error());
				}
				catch (std::bad_cast const &e)
				{
					completion("", Glib::Error(G_VARIANT_PARSE_ERROR, G_VARIANT_PARSE_ERROR_FAILED, "Client: Cannot parse response to \"GetHostName\""));
				}
			}
			catch (Glib::Error const &e)
			{
				completion("", e);
			}
		},
		/*bus_name*/ AVAHI_DBUS_NAME,
		/*timeout_msec*/G_MAXINT,
		Gio::DBus::CALL_FLAGS_NO_AUTO_START
	);
}

void Client::async_setHostName(Glib::ustring const &name,
        SlotSetHostName const &completion)
{
	auto parameters = std::make_tuple(name);
	m_connection->call(
		"/", AVAHI_DBUS_INTERFACE_SERVER, "SetHostName",
		Glib::Variant<decltype(parameters)>::create(parameters),
		[connection = m_connection, completion](Glib::RefPtr<Gio::AsyncResult> &result)
		{
			try
			{
				connection->call_finish(result);
				completion(Glib::Error());
			}
			catch (Glib::Error const &e)
			{
				completion(e);
			}
		},
		/*bus_name*/ AVAHI_DBUS_NAME,
		/*timeout_msec*/G_MAXINT,
		Gio::DBus::CALL_FLAGS_NO_AUTO_START
	);
}

void Client::async_reconfirmRecord(Interface interface, Protocol protocol, RecordName const &name, RecordClass clazz, RecordType type, RecordData const &data, SlotReconfirmRecord const &completion)
{
	std::uint32_t flags = 0;
	auto parameters = std::make_tuple(
		interface,
		protocol,
		name,
		clazz,
		type,
		flags,
		data);
	m_connection->call(
		"/", AVAHI_DBUS_INTERFACE_SERVER, "ReconfirmRecord",
		Glib::Variant<decltype(parameters)>::create(parameters),
		[connection = m_connection, completion](Glib::RefPtr<Gio::AsyncResult> &result)
		{
			try
			{
				connection->call_finish(result);
				completion(Glib::Error());
			}
			catch (Glib::Error const &e)
			{
				completion(e);
			}
		},
		/*bus_name*/ AVAHI_DBUS_NAME,
		/*timeout_msec*/G_MAXINT,
		Gio::DBus::CALL_FLAGS_NO_AUTO_START
	);
}

void Client::async_createEntryGroup(SlotCreateEntryGroup const &completion)
{
	m_connection->call(
		"/", AVAHI_DBUS_INTERFACE_SERVER, "EntryGroupNew",
		Glib::VariantContainerBase{},
		[this, connection = m_connection, completion](Glib::RefPtr<Gio::AsyncResult> &result)
		{
			try
			{
				auto response = connection->call_finish(result);

				try
				{
					using ParamsType = std::tuple<Glib::DBusObjectPathString>;
					auto const parsedParams = Glib::VariantBase::cast_dynamic<Glib::Variant<ParamsType>>(response);
					auto const &[objectPath] = parsedParams.get();  // unpack variant+tuple

					auto entryGroup = std::make_shared<EntryGroup>(EntryGroup::Token{}, *this, objectPath);
					completion(entryGroup, Glib::Error());
				}
				catch (std::bad_cast const &e)
				{
					completion({}, Glib::Error(G_VARIANT_PARSE_ERROR, G_VARIANT_PARSE_ERROR_FAILED, "Client: Cannot parse response to \"EntryGroupNew\""));
				}
			}
			catch (Glib::Error const &e)
			{
				completion({}, e);
			}
		},
		/*bus_name*/ AVAHI_DBUS_NAME,
		/*timeout_msec*/G_MAXINT,
		Gio::DBus::CALL_FLAGS_NO_AUTO_START
	);
}

void Client::async_createRecordBrowser(Interface interface, Protocol protocol,
        RecordName const &name, RecordClass clazz, RecordType type, ::AvahiLookupFlags flags,
        SlotCreateRecordBrowser const &completion)
{
	auto parameters = std::make_tuple(
		interface,
		protocol,
		name,
		clazz,
		type,
		static_cast<std::uint32_t>(flags));

	m_connection->call(
		"/", AVAHI_DBUS_INTERFACE_SERVER2, "RecordBrowserPrepare",
		Glib::Variant<decltype(parameters)>::create(parameters),
		[this, connection = m_connection, completion](Glib::RefPtr<Gio::AsyncResult> &result)
		{
			try
			{
				auto response = connection->call_finish(result);

				try
				{
					using ParamsType = std::tuple<Glib::DBusObjectPathString>;
					auto const parsedParams = Glib::VariantBase::cast_dynamic<Glib::Variant<ParamsType>>(response);
					auto const &[objectPath] = parsedParams.get();  // unpack variant+tuple

					auto recordBrowser = std::make_shared<RecordBrowser>(RecordBrowser::Token{}, *this, objectPath);
					completion(recordBrowser, Glib::Error());
				}
				catch (std::bad_cast const &e)
				{
					completion({}, Glib::Error(G_VARIANT_PARSE_ERROR, G_VARIANT_PARSE_ERROR_FAILED, "Client: Cannot parse response to \"ServiceBrowserPrepare\""));
				}
			}
			catch (Glib::Error const &e)
			{
				completion({}, e);
			}
		},
		/*bus_name*/ AVAHI_DBUS_NAME,
		/*timeout_msec*/G_MAXINT,
		Gio::DBus::CALL_FLAGS_NO_AUTO_START
	);
}

void Client::async_createServiceBrowser(Interface interface, Protocol protocol,
        ServiceType const &type, Domain const &domain, ::AvahiLookupFlags flags,
        SlotCreateServiceBrowser const &completion)
{
	auto parameters = std::make_tuple(interface, protocol, type, domain, static_cast<std::uint32_t>(flags));

	m_connection->call(
		"/", AVAHI_DBUS_INTERFACE_SERVER2, "ServiceBrowserPrepare",
		Glib::Variant<decltype(parameters)>::create(parameters),
		[this, connection = m_connection, completion](Glib::RefPtr<Gio::AsyncResult> &result)
		{
			try
			{
				auto response = connection->call_finish(result);

				try
				{
					using ParamsType = std::tuple<Glib::DBusObjectPathString>;
					auto const parsedParams = Glib::VariantBase::cast_dynamic<Glib::Variant<ParamsType>>(response);
					auto const &[objectPath] = parsedParams.get();  // unpack variant+tuple

					auto serviceBrowser = std::make_shared<ServiceBrowser>(ServiceBrowser::Token{}, *this, objectPath);
					completion(serviceBrowser, Glib::Error());
				}
				catch (std::bad_cast const &e)
				{
					completion({}, Glib::Error(G_VARIANT_PARSE_ERROR, G_VARIANT_PARSE_ERROR_FAILED, "Client: Cannot parse response to \"ServiceBrowserPrepare\""));
				}
			}
			catch (Glib::Error const &e)
			{
				completion({}, e);
			}
		},
		/*bus_name*/ AVAHI_DBUS_NAME,
		/*timeout_msec*/G_MAXINT,
		Gio::DBus::CALL_FLAGS_NO_AUTO_START
	);
}

void Client::async_createServiceResolver(Interface interface, Protocol protocol,
        ServiceName const &name, ServiceType const &type, Domain const &domain,
        Protocol aprotocol, ::AvahiLookupFlags flags,
        SlotCreateServiceResolver const &completion)
{
	auto parameters = std::make_tuple(interface, protocol, name, type, domain, aprotocol, static_cast<std::uint32_t>(flags));

	m_connection->call(
		"/", AVAHI_DBUS_INTERFACE_SERVER2, "ServiceResolverPrepare",
		Glib::Variant<decltype(parameters)>::create(parameters),
		[this, connection = m_connection, completion](Glib::RefPtr<Gio::AsyncResult> &result)
		{
			try
			{
				auto response = connection->call_finish(result);

				try
				{
					using ParamsType = std::tuple<Glib::DBusObjectPathString>;
					auto const parsedParams = Glib::VariantBase::cast_dynamic<Glib::Variant<ParamsType>>(response);
					auto const &[objectPath] = parsedParams.get();  // unpack variant+tuple

					auto serviceBrowser = std::make_shared<ServiceResolver>(ServiceResolver::Token{}, *this, objectPath);
					completion(serviceBrowser, Glib::Error());
				}
				catch (std::bad_cast const &e)
				{
					completion({}, Glib::Error(G_VARIANT_PARSE_ERROR, G_VARIANT_PARSE_ERROR_FAILED, "Client: Cannot parse response to \"ServiceResolverPrepare\""));
				}
			}
			catch (Glib::Error const &e)
			{
				completion({}, e);
			}
		},
		/*bus_name*/ AVAHI_DBUS_NAME,
		/*timeout_msec*/G_MAXINT,
		Gio::DBus::CALL_FLAGS_NO_AUTO_START
	);
}

} /* namespace Avahi */
