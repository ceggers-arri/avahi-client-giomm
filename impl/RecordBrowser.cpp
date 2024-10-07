/**
 *  \file
 *  \brief D-Bus wrapper for Avahi record browser
 *  \author Christian Eggers
 *  \copyright 2023 ARRI Lighting Stephanskirchen
 */

#include <cstdint>
#include <sstream>
#include <tuple>
#include <typeinfo>                    // std::bad_cast

#include <giomm/dbusconnection.h>
#include <glibmm/error.h>
#include <glibmm/refptr.h>
#include <glibmm/variant.h>

#include <avahi-common/dbus.h>
#include <glib.h>                      // G_MAXINT

#include "../Client.hpp"
#include "../RecordBrowser.hpp"        // IWYU pragma: associated

namespace Gio { class AsyncResult; }

namespace Avahi {

RecordBrowser::RecordBrowser(Token, Client &client, Glib::ustring const &objectPath)
: m_client(client)
, m_objectPath(objectPath)
{
	auto const &connection = m_client.getConnection();

	m_itemNew = connection->signal_subscribe(
		[this](Glib::RefPtr<Gio::DBus::Connection> const &/*connection*/,
		       Glib::ustring const &/*senderName*/,
		       Glib::ustring const &/*objectPath*/,
		       Glib::ustring const &/*interfaceName*/,
		       Glib::ustring const &/*signalName*/,
		       Glib::VariantContainerBase const &parameters)
		{
			using Flags = std::uint32_t;
			using Params = std::tuple<Interface, Protocol, RecordName, RecordClass, RecordType, RecordData, Flags>;

			try
			{
				auto params = Glib::VariantBase::cast_dynamic<Glib::Variant<Params>>(parameters);
				const auto &[interface, protocol, name, clazz, type, rdata, flags] = params.get();

				on_itemNew(interface, protocol, name, static_cast<RecordClass>(clazz), static_cast<RecordType>(type), rdata, static_cast<::AvahiLookupResultFlags>(flags));
			}
			catch (std::bad_cast const &e)
			{
				on_errorLog("RecordBrowser: Cannot parse \"ItemNew\" parameters");
				return;
			}
		},
		AVAHI_DBUS_NAME,
		AVAHI_DBUS_INTERFACE_RECORD_BROWSER,
		"ItemNew",
		m_objectPath);

	m_itemRemove = connection->signal_subscribe(
		[this](Glib::RefPtr<Gio::DBus::Connection> const &/*connection*/,
		       Glib::ustring const &/*senderName*/,
		       Glib::ustring const &/*objectPath*/,
		       Glib::ustring const &/*interfaceName*/,
		       Glib::ustring const &/*signalName*/,
		       Glib::VariantContainerBase const &parameters)
		{
			using Flags = std::uint32_t;
			using Params = std::tuple<Interface, Protocol, RecordName, RecordClass, RecordType, RecordData, Flags>;

			try
			{
				auto params = Glib::VariantBase::cast_dynamic<Glib::Variant<Params>>(parameters);
				const auto &[interface, protocol, name, clazz, type, rdata, flags] = params.get();

				on_itemRemove(interface, protocol, name, static_cast<RecordClass>(clazz), static_cast<RecordType>(type), rdata, static_cast<::AvahiLookupResultFlags>(flags));
			}
			catch (std::bad_cast const &e)
			{
				on_errorLog("RecordBrowser: Cannot parse \"ItemRemove\" parameters");
				return;
			}
		},
		AVAHI_DBUS_NAME,
		AVAHI_DBUS_INTERFACE_RECORD_BROWSER,
		"ItemRemove",
		m_objectPath);

	m_failure = connection->signal_subscribe(
		[this](Glib::RefPtr<Gio::DBus::Connection> const &/*connection*/,
		       Glib::ustring const &/*senderName*/,
		       Glib::ustring const &/*objectPath*/,
		       Glib::ustring const &/*interfaceName*/,
		       Glib::ustring const &/*signalName*/,
		       Glib::VariantContainerBase const &parameters)
		{
			using Params = std::tuple<Error>;

			try
			{
				auto params = Glib::VariantBase::cast_dynamic<Glib::Variant<Params>>(parameters);
				const auto &[error] = params.get();

				on_failure(error);
			}
			catch (std::bad_cast const &e)
			{
				on_errorLog("RecordBrowser: Cannot parse \"Failure\" parameters");
				return;
			}
		},
		AVAHI_DBUS_NAME,
		AVAHI_DBUS_INTERFACE_RECORD_BROWSER,
		"Failure",
		m_objectPath);

	m_allForNow = connection->signal_subscribe(
		[this](Glib::RefPtr<Gio::DBus::Connection> const &/*connection*/,
		       Glib::ustring const &/*senderName*/,
		       Glib::ustring const &/*objectPath*/,
		       Glib::ustring const &/*interfaceName*/,
		       Glib::ustring const &/*signalName*/,
		       Glib::VariantContainerBase const &/*parameters*/)
		{
			on_allForNow();
		},
		AVAHI_DBUS_NAME,
		AVAHI_DBUS_INTERFACE_RECORD_BROWSER,
		"AllForNow",
		m_objectPath);

	m_cacheExhausted = connection->signal_subscribe(
		[this](Glib::RefPtr<Gio::DBus::Connection> const &/*connection*/,
		       Glib::ustring const &/*senderName*/,
		       Glib::ustring const &/*objectPath*/,
		       Glib::ustring const &/*interfaceName*/,
		       Glib::ustring const &/*signalName*/,
		       Glib::VariantContainerBase const &/*parameters*/)
		{
			on_cacheExhausted();
		},
		AVAHI_DBUS_NAME,
		AVAHI_DBUS_INTERFACE_RECORD_BROWSER,
		"CacheExhausted",
		m_objectPath);

	/* Start service browser after registering all signals handlers */
	connection->call(
		m_objectPath, AVAHI_DBUS_INTERFACE_RECORD_BROWSER, "Start",
		Glib::VariantContainerBase(),
		[this,connection](Glib::RefPtr<Gio::AsyncResult> &result)
		{
			try
			{
				connection->call_finish(result);
			}
			catch (Glib::Error const &e)
			{
				std::stringstream ss;

				ss << "RecordBrowser: D-Bus call \"Start\" failed: " << e.what();
				on_errorLog(ss.str().c_str());
			}
		},
		/*bus_name*/ AVAHI_DBUS_NAME,
		/*timeout_msec*/G_MAXINT,
		Gio::DBus::CALL_FLAGS_NO_AUTO_START
	);
}

RecordBrowser::~RecordBrowser()
{
	auto const &connection = m_client.getConnection();

	if (connection)
	{
		connection->signal_unsubscribe(m_itemNew);
		connection->signal_unsubscribe(m_itemRemove);
		connection->signal_unsubscribe(m_failure);
		connection->signal_unsubscribe(m_allForNow);
		connection->signal_unsubscribe(m_cacheExhausted);
		connection->call(
			m_objectPath, AVAHI_DBUS_INTERFACE_RECORD_BROWSER, "Free",
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

} /* namespace Avahi */
