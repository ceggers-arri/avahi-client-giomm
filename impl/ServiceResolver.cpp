/**
 *  \file
 *  \brief D-Bus wrapper for Avahi service resolver
 *  \author Christian Eggers
 *  \copyright 2022 ARRI Lighting Stephanskirchen
 */

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
#include "../ServiceResolver.hpp"      // IWYU pragma: associated

namespace Gio { class AsyncResult; }

namespace Avahi {

ServiceResolver::ServiceResolver(Token, Client &client, Glib::ustring const &objectPath)
: m_client(client)
, m_objectPath(objectPath)
{
	auto const &connection = m_client.getConnection();

	m_found = connection->signal_subscribe(
		[this](Glib::RefPtr<Gio::DBus::Connection> const &/*connection*/,
		       Glib::ustring const &/*senderName*/,
		       Glib::ustring const &/*objectPath*/,
		       Glib::ustring const &/*interfaceName*/,
		       Glib::ustring const &/*signalName*/,
		       Glib::VariantContainerBase const &parameters)
		{
			using Flags = std::uint32_t;
			using Params = std::tuple<Interface, Protocol, ServiceName, ServiceType, Domain, Host, AProtocol, Address, Port, Txt, Flags>;

			try
			{
				auto params = Glib::VariantBase::cast_dynamic<Glib::Variant<Params>>(parameters);
				const auto &[interface, protocol, name, type, domain, host, aprotocol, address, port, txt, flags] = params.get();

				on_found(/*interface, protocol, */name, /*type, domain, */host, aprotocol, address, port, txt, static_cast<::AvahiLookupResultFlags>(flags));
			}
			catch (std::bad_cast const &e)
			{
				on_errorLog("ServiceResolver: Cannot parse \"Found\" parameters");
				return;
			}
		},
		AVAHI_DBUS_NAME,
		AVAHI_DBUS_INTERFACE_SERVICE_RESOLVER,
		"Found",
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
				on_errorLog("ServiceResolver: Cannot parse \"Failure\" parameters");
				return;
			}
		},
		AVAHI_DBUS_NAME,
		AVAHI_DBUS_INTERFACE_SERVICE_RESOLVER,
		"Failure",
		m_objectPath);

	/* Start service resolver after registering all signals handlers */
	connection->call(
		m_objectPath, AVAHI_DBUS_INTERFACE_SERVICE_RESOLVER, "Start",
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

				ss << "ServiceResolver: D-Bus call \"Start\" failed: " << e.what();
				on_errorLog(ss.str().c_str());
			}
		},
		/*bus_name*/ AVAHI_DBUS_NAME,
		/*timeout_msec*/G_MAXINT,
		Gio::DBus::CALL_FLAGS_NO_AUTO_START
	);
}

ServiceResolver::~ServiceResolver()
{
	auto const &connection = m_client.getConnection();

	if (connection)
	{
		connection->signal_unsubscribe(m_found);
		connection->signal_unsubscribe(m_failure);
		connection->call(
			m_objectPath, AVAHI_DBUS_INTERFACE_SERVICE_RESOLVER, "Free",
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
