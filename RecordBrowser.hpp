/**
 *  \file
 *  \brief D-Bus wrapper for Avahi record browser
 *  \author Christian Eggers
 *  \copyright 2023 ARRI Lighting Stephanskirchen
 */

#ifndef SRC_AVAHI_RECORDBROWSER_HPP_
#define SRC_AVAHI_RECORDBROWSER_HPP_

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

/** Proxy for an Avahi record browser.  A record browser is used for enumerating
 *  arbitrary mDNS records from Avahi's internal database. */
class RecordBrowser
{
	/* Use passkey idiom as constructor of RecordBrowser cannot be made private
	 * (due to use of std::make_shared()). */
	friend Client;
	class Token {};

public:   // slots
	/** Type for handler to invoke when an error message shall be printed to
	 *  the application's log file. */
	using SlotErrorLog       = sigc::signal<void(char const *error)>;
	/** Type for handler to invoke when a new record has been found. */
	using SlotItemNew        = sigc::signal<void(Interface interface, Protocol protocol, RecordName const &name, RecordClass clazz, RecordType const &type, RecordData const &rdata, ::AvahiLookupResultFlags flags)>;
	/** Type for handler to invoke when an existing record has disappeared. */
	using SlotItemRemove     = sigc::signal<void(Interface interface, Protocol protocol, RecordName const &name, RecordClass clazz, RecordType const &type, RecordData const &rdata, ::AvahiLookupResultFlags flags)>;
	/** Type for handler to invoke when browsing has failed due to some reason. */
	using SlotFailure        = sigc::signal<void(Error const &error)>;
	/** Type for handler to invoke (one time) to notify the user that more
	 *  records will probably not show up in the near future, i.e. all cache
	 *  entries have been read and all static servers been queried. */
	using SlotAllForNow      = sigc::signal<void()>;
	/** Type for handler to invoke (one time) when all services from Avahi
	 *  daemon's cache have been reported via on_itemNew. */
	using SlotCacheExhausted = sigc::signal<void()>;

	/** Handler to invoke when an error message shall be printed to the
	 *  application's log file.
	 *  \note Currently this is only used when a parsing error happens during
	 *        reception of a D-Bus signal.
	 */
	SlotErrorLog on_errorLog;
	/** Handler to invoke when a new record has been found. */
	SlotItemNew on_itemNew;
	/** Handler to invoke when an existing record has disappeared. */
	SlotItemRemove on_itemRemove;
	/** Handler to invoke when browsing has failed due to some reason. */
	SlotFailure on_failure;
	/** Handler to invoke (one time) to notify the user that more records will
	 *  probably not show up in the near future, i.e. all cache entries have
	 *  been read and all static servers been queried. */
	SlotAllForNow on_allForNow;
	/** Handler to invoke (one time) when all services from Avahi daemon's cache
	 *  have been reported via on_itemNew. */
	SlotCacheExhausted on_cacheExhausted;

public:   // methods
	/** Do not call this directly. Use Client::async_createRecordBrowser() instead. */
	RecordBrowser(Token, Client &client, Glib::ustring const &objectPath);
	~RecordBrowser();
	RecordBrowser(RecordBrowser const &other) = delete;
	RecordBrowser(RecordBrowser &&other) = delete;
	RecordBrowser& operator=(RecordBrowser const &other) = delete;
	RecordBrowser& operator=(RecordBrowser &&other) = delete;

private:  // members
	Client &m_client;
	Glib::ustring m_objectPath;
	::guint m_itemNew, m_itemRemove, m_failure, m_allForNow, m_cacheExhausted;
};

} /* namespace Avahi */

#endif /* SRC_AVAHI_RECORDBROWSER_HPP_ */
