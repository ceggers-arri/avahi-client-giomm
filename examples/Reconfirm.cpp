/*
 * Reconfirm.cpp
 *
 *  Created on: 26.09.2023
 *      Author: eggers
 */

#include <cstring>                   /* ::strcmp() */
#include <iostream>
#include <memory>

#include <avahi-common/address.h>    /* AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC */
#include <avahi-common/defs.h>       /* AVAHI_DNS_CLASS_IN, AVAHI_DNS_TYPE_A */
#include <avahi-core/rr.h>
#include <boost/io/ios_state.hpp>
#include <giomm/init.h>
#include <glibmm/error.h>
#include <glibmm/main.h>
#include <sigc++/functors/ptr_fun.h>

#include "../Client.hpp"
#include "../RecordBrowser.hpp"

using namespace Avahi;

std::shared_ptr<Client> avahiClient;

static void connectCommonHandlers(std::shared_ptr<RecordBrowser> const &browser)
{
	browser->on_errorLog.connect([](char const *error)
	{
		std::cerr << error << '\n';
	});

	browser->on_failure.connect([](Error const &error)
	{
		std::cerr << error << '\n';
	});

	// keep reference to browser until 'All for now' event
	browser->on_allForNow.connect([browser]
	{
		browser->on_allForNow.clear();
	});
}

static void do_reconfirm(Interface interface, Protocol protocol, RecordName const &name, RecordClass clazz, RecordType const &type, RecordData const &rdata, ::AvahiLookupResultFlags /*flags*/)
{
	avahiClient->async_reconfirmRecord(interface, protocol, name, clazz, type, rdata, [name](Glib::Error const &error)
	{
		if (error)
		{
			std::cerr << "Error on reconfirmation of \"" << name << "\": "  << error.what() << '\n';
			return ;
		}
	});
}

static void on_connected()
{
//	char const *serviceType               = "_myServiceType._tcp.local";
	char const *serviceSubtype1           = "_myServiceSubType1._sub._myServiceType._tcp.local";
	char const *serviceSubtype2           = "_myServiceSubType2._sub._myServiceType._tcp.local";
	char const *service                   = "myService._myServiceType._tcp.local";
#if 0
	char const *host                      = "myhost.local";
	char const *address                   = "fe80::798c:5aa0:fc70:e8a9";
	char addressReverse[64+8+1];
	//char const *addressReverse            = "9.a.8.e.0.7.c.f.0.a.a.5.c.8.9.7.0.0.0.0.0.0.0.0.0.0.0.0.0.8.e.f.ip6.arpa";

	::AvahiAddress avahiAddress;
	::avahi_reverse_lookup_name(::avahi_address_parse(address, AVAHI_PROTO_UNSPEC, &avahiAddress), addressReverse, sizeof(addressReverse));
#endif

	auto checkServicePtr = [service]
		(Interface interface, Protocol protocol, RecordName const &name,
		RecordClass clazz, RecordType const &type, RecordData const &rdata,
		::AvahiLookupResultFlags flags)
		{
			::AvahiRecord *r = ::avahi_record_new_full(
				name.c_str(),
				clazz,
				type,
				0);

			::avahi_rdata_parse(r, rdata.data(), rdata.size());
			if (::strcmp(r->data.ptr.name, service) == 0)
			{
				std::cout << "PTR::name = \"" << r->data.ptr.name << "\"\n";
				do_reconfirm(interface, protocol, name, clazz, type, rdata, flags);
			}

			::avahi_record_unref(r);
		};

	/* 1. Service sub type */
	avahiClient->async_createRecordBrowser(AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
		serviceSubtype1, AVAHI_DNS_CLASS_IN, AVAHI_DNS_TYPE_PTR, {},
		[checkServicePtr](std::shared_ptr<RecordBrowser> const &browser, Glib::Error const &error)
		{
			if (error)
			{
				std::cerr << "Cannot create record browser for service subtype 1:" << error.what() << '\n';
				return;
			}
			connectCommonHandlers(browser);
			browser->on_itemNew.connect(checkServicePtr);
		});

	avahiClient->async_createRecordBrowser(AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
		serviceSubtype2, AVAHI_DNS_CLASS_IN, AVAHI_DNS_TYPE_PTR, {},
		[checkServicePtr](std::shared_ptr<RecordBrowser> const &browser, Glib::Error const &error)
		{
			if (error)
			{
				std::cerr << "Cannot create record browser for service subtype 2:" << error.what() << '\n';
				return;
			}
			connectCommonHandlers(browser);
			browser->on_itemNew.connect(checkServicePtr);
		});

#if 0
	/* 2. Service type */
	avahiClient->async_createRecordBrowser(AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
		serviceType, RecordClass::IN, RecordType::PTR, {},
		serviceType, AVAHI_DNS_CLASS_IN, AVAHI_DNS_TYPE_PTR, {},
		[checkServicePtr](std::shared_ptr<RecordBrowser> const &browser, Glib::Error const &error)
		{
			if (error)
			{
				std::cerr << "Cannot create record browser for service type:" << error.what() << '\n';
				return;
			}
			connectCommonHandlers(browser);
			browser->on_itemNew.connect(checkServicePtr);
		});
#endif

	/* 3. Service TXT record */
	avahiClient->async_createRecordBrowser(AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
		service, AVAHI_DNS_CLASS_IN, AVAHI_DNS_TYPE_TXT, {},
		[](std::shared_ptr<RecordBrowser> const &browser, Glib::Error const &error)
		{
			if (error)
			{
				std::cerr << "Cannot create record browser for service TXT record:" << error.what() << '\n';
				return;
			}
			connectCommonHandlers(browser);
			browser->on_itemNew.connect(sigc::ptr_fun(do_reconfirm));
		});

	/* 4. Service SRV record */
	avahiClient->async_createRecordBrowser(AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
		service, AVAHI_DNS_CLASS_IN, AVAHI_DNS_TYPE_SRV, {},
		[](std::shared_ptr<RecordBrowser> const &browser, Glib::Error const &error)
		{
			if (error)
			{
				std::cerr << "Cannot create record browser for service SRV record:" << error.what() << '\n';
				return;
			}
			connectCommonHandlers(browser);
			browser->on_itemNew.connect(sigc::ptr_fun(do_reconfirm));
		});

#if 0
	/* 5. Address record */
	avahiClient->async_createRecordBrowser(AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
		host, AVAHI_DNS_CLASS_IN, AVAHI_DNS_TYPE_AAAA, {},
		[](std::shared_ptr<RecordBrowser> const &browser, Glib::Error const &error)
		{
			if (error)
			{
				std::cerr << "Cannot create record browser for address record:" << error.what() << '\n';
				return;
			}
			connectCommonHandlers(browser);
			browser->on_itemNew.connect(sigc::ptr_fun(do_reconfirm));
			browser->on_allForNow.connect([browser]{ browser->on_allForNow.clear(); });  // keep recored browser alive until 'All for now' event
		});

	/* 6. Reverse address record */
	avahiClient->async_createRecordBrowser(AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
		addressReverse, AVAHI_DNS_CLASS_IN, AVAHI_DNS_TYPE_PTR, {},
		[](std::shared_ptr<RecordBrowser> const &browser, Glib::Error const &error)
		{
			if (error)
			{
				std::cerr << "Cannot create record browser for reverse address record:" << error.what() << '\n';
				return;
			}
			connectCommonHandlers(browser);
			browser->on_itemNew.connect(sigc::ptr_fun(do_reconfirm));
		});
#endif
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
