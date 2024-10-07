/*
 * RecordBrowser.cpp
 *
 *  Created on: 26.09.2023
 *      Author: eggers
 */

#include <iostream>
#include <memory>

#include <avahi-common/address.h>    /* AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC */
#include <avahi-common/defs.h>       /* AVAHI_DNS_CLASS_IN, AVAHI_DNS_TYPE_A */
#include <giomm/init.h>
#include <glibmm/error.h>
#include <glibmm/main.h>
#include <sigc++/functors/ptr_fun.h>

#include "../Client.hpp"
#include "../RecordBrowser.hpp"

using namespace Avahi;

std::shared_ptr<Client> avahiClient;
std::shared_ptr<RecordBrowser> recordBrowser;

static void on_errorLog(char const *error)
{
	std::cerr << error << '\n';
}

static void on_itemNew(Interface interface, Protocol protocol, RecordName const &name, RecordClass clazz, RecordType const &type, RecordData const &/*rdata*/, ::AvahiLookupResultFlags /*flags*/)
{
	std::cout << "+ if:" << interface << ", proto: " << protocol << ", " << name << ", class: " << clazz << ", type: " << type << '\n';
}

static void on_itemRemove(Interface interface, Protocol protocol, RecordName const &name, RecordClass clazz, RecordType const &type, RecordData const &/*rdata*/, ::AvahiLookupResultFlags /*flags*/)
{
	std::cout << "- if:" << interface << ", proto: " << protocol << ", " << name << ", class: " << clazz << ", type: " << type << '\n';
}

static void on_failure(Error const &error)
{
	std::cerr << error << '\n';
}

static void on_allForNow()
{
	std::cout << "All for now\n";
}

static void on_cacheExhausted()
{
	std::cout << "Cache exhausted\n";
}

static void on_recordBrowserCreated(std::shared_ptr<RecordBrowser> const &browser, Glib::Error const &error)
{
	if (error)
	{
		std::cerr << "Cannot create record browser: " << error.what() << '\n';
		return;
	}

	browser->on_errorLog.connect(sigc::ptr_fun(on_errorLog));
	browser->on_itemNew.connect(sigc::ptr_fun(on_itemNew));
	browser->on_itemRemove.connect(sigc::ptr_fun(on_itemRemove));
	browser->on_failure.connect(sigc::ptr_fun(on_failure));
	browser->on_allForNow.connect(sigc::ptr_fun(on_allForNow));
	browser->on_cacheExhausted.connect(sigc::ptr_fun(on_cacheExhausted));
	recordBrowser = browser;
}

static void on_connected()
{
	avahiClient->async_createRecordBrowser(AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
			"_services._dns-sd._udp.local", AVAHI_DNS_CLASS_IN, AVAHI_DNS_TYPE_PTR, {},
			sigc::ptr_fun(on_recordBrowserCreated));
}

int main(int /*argc*/, char */*argv*/[])
{

	Gio::init();

	/* Create a own GMainContext for this thread and set it as default for this thread. */
	auto mainContext = Glib::MainContext::create();
	mainContext->push_thread_default();

	avahiClient = std::make_shared<Client>();
	avahiClient->on_connected.connect(sigc::ptr_fun(on_connected));

	/* Thread loop */
	auto mainLoop = Glib::MainLoop::create(mainContext, false);
	mainLoop->run();
}
