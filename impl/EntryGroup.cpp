/**
 *  \file
 *  \brief D-Bus wrapper for Avahi entry group
 *  \author Christian Eggers
 *  \copyright 2022 ARRI Lighting Stephanskirchen
 */

#include <cstdint>
#include <tuple>
#include <typeinfo>                    // std::bad_cast

#include <giomm/dbusconnection.h>
#include <glibmm/error.h>
#include <glibmm/refptr.h>
#include <glibmm/variant.h>

#include <avahi-common/dbus.h>
#include <glib.h>                      // G_MAXINT

#include "../Client.hpp"
#include "../EntryGroup.hpp"           // IWYU pragma: associated

namespace Gio { class AsyncResult; }

namespace Avahi {

EntryGroup::EntryGroup(Token, Client &client, Glib::ustring const &objectPath)
: m_client(client)
, m_objectPath(objectPath)
{
	m_stateChanged = m_client.getConnection()->signal_subscribe(
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

				on_stateChanged(static_cast<AvahiEntryGroupState>(state), error);
			}
			catch (std::bad_cast const &e)
			{
				on_errorLog("EntryGroup: Cannot parse \"StateChanged\" parameters");
				return;
			}
		},
		AVAHI_DBUS_NAME,
		AVAHI_DBUS_INTERFACE_ENTRY_GROUP,
		"StateChanged",
		m_objectPath);
}

EntryGroup::~EntryGroup()
{
	auto const &connection = m_client.getConnection();

	if (connection)
	{
		connection->signal_unsubscribe(m_stateChanged);
		connection->call(
			m_objectPath, AVAHI_DBUS_INTERFACE_ENTRY_GROUP, "Free",
			Glib::VariantContainerBase(),
			[](Glib::RefPtr<Gio::AsyncResult> &/*result*/)
			{
				/* we are not interested in the result, so there's no need to
				 * run call_finish() */
			},
			/*bus_name*/ AVAHI_DBUS_NAME,
			/*timeout_msec*/G_MAXINT,
			Gio::DBus::CALL_FLAGS_NO_AUTO_START
		);
	}
}

void EntryGroup::async_addService(Interface interface, Protocol protocol, ::AvahiPublishFlags flags, ServiceName const &name, ServiceType const &type, Domain const &domain, Host const &host, Port port, Txt const &txt, SlotAddService const &completion)
{
	auto parameters = std::make_tuple(interface, protocol, static_cast<std::uint32_t>(flags), name, type, domain, host, port, txt);

	auto const &connection = m_client.getConnection();
	connection->call(
		m_objectPath, AVAHI_DBUS_INTERFACE_ENTRY_GROUP, "AddService",
		Glib::Variant<decltype(parameters)>::create(parameters),
		[connection, completion](Glib::RefPtr<Gio::AsyncResult> &result)
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

void EntryGroup::async_addServiceSubtype(Interface interface, Protocol protocol,
        ::AvahiPublishFlags flags, ServiceName const &name, ServiceType const &type, Domain const &domain,
        Subtype const &subtype, SlotAddServiceSubtype const &completion)
{
	auto parameters = std::make_tuple(interface, protocol, static_cast<std::uint32_t>(flags), name, type, domain, subtype);

	auto const &connection = m_client.getConnection();
	connection->call(
		m_objectPath, AVAHI_DBUS_INTERFACE_ENTRY_GROUP, "AddServiceSubtype",
		Glib::Variant<decltype(parameters)>::create(parameters),
		[connection, completion](Glib::RefPtr<Gio::AsyncResult> &result)
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

void EntryGroup::async_commit(SlotCommit const &completion)
{
	auto const &connection = m_client.getConnection();
	connection->call(
		m_objectPath, AVAHI_DBUS_INTERFACE_ENTRY_GROUP, "Commit",
		Glib::VariantContainerBase(),
		[connection, completion](Glib::RefPtr<Gio::AsyncResult> &result)
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

void EntryGroup::async_reset(SlotReset const &completion)
{
	auto const &connection = m_client.getConnection();
	connection->call(
		m_objectPath, AVAHI_DBUS_INTERFACE_ENTRY_GROUP, "Reset",
		Glib::VariantContainerBase(),
		[connection, completion](Glib::RefPtr<Gio::AsyncResult> &result)
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

void EntryGroup::async_updateServiceTxt(Interface interface, Protocol protocol,
        ::AvahiPublishFlags flags, ServiceName const &name, ServiceType const &type, Domain const &domain,
        Txt const &txt, SlotUpdateServiceTxt const &completion)
{
	auto parameters = std::make_tuple(interface, protocol, static_cast<std::uint32_t>(flags), name, type, domain, txt);

	auto const &connection = m_client.getConnection();
	connection->call(
		m_objectPath, AVAHI_DBUS_INTERFACE_ENTRY_GROUP, "UpdateServiceTxt",
		Glib::Variant<decltype(parameters)>::create(parameters),
		[connection, completion](Glib::RefPtr<Gio::AsyncResult> &result)
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

} /* namespace Avahi */
