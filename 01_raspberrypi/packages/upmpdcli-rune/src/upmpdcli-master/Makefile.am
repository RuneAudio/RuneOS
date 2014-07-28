
AM_CPPFLAGS = -DDEBUG -g -Wall \
            -DDATADIR=\"${pkgdatadir}\" -DCONFIGDIR=\"${sysconfdir}\"

AM_CXXFLAGS = -std=c++0x 

lib_LTLIBRARIES = libupnpp.la

libupnpp_la_SOURCES = \
    libupnpp/cdircontent.cxx \
    libupnpp/cdircontent.hxx \
    libupnpp/cdirectory.cxx \
    libupnpp/cdirectory.hxx \
    libupnpp/description.cxx \
    libupnpp/description.hxx \
    libupnpp/device.cxx \
    libupnpp/device.hxx \
    libupnpp/discovery.cxx \
    libupnpp/discovery.hxx \
    libupnpp/expatmm.hxx \
    libupnpp/getsyshwaddr.c \
    libupnpp/getsyshwaddr.h \
    libupnpp/ixmlwrap.cxx \
    libupnpp/ixmlwrap.hxx \
    libupnpp/log.cxx \
    libupnpp/log.hxx \
    libupnpp/md5.cxx \
    libupnpp/md5.hxx \
    libupnpp/ptmutex.hxx \
    libupnpp/soaphelp.cxx \
    libupnpp/soaphelp.hxx \
    libupnpp/upnpp_p.hxx \
    libupnpp/upnpplib.cxx \
    libupnpp/upnpplib.hxx \
    libupnpp/vdir.cxx \
    libupnpp/vdir.hxx \
    libupnpp/workqueue.hxx

libupnpp_la_LDFLAGS = -release $(VERSION)
libupnpp_la_LIBADD = $(LIBUPNPP_LIBS)

bin_PROGRAMS = upmpdcli #upexplorer 

#upexplorer_SOURCES = upexplo/upexplo.cxx
#upexplorer_LDADD = libupnpp.la

upmpdcli_SOURCES = \
     upmpd/conftree.cxx \
     upmpd/conftree.hxx \
     upmpd/mpdcli.cxx \
     upmpd/mpdcli.hxx \
     upmpd/upmpd.hxx \
     upmpd/upmpd.cxx \
     upmpd/upmpdutils.cxx \
     upmpd/upmpdutils.hxx \
     upmpd/renderctl.hxx \
     upmpd/renderctl.cxx \
     upmpd/avtransport.hxx \
     upmpd/avtransport.cxx \
     upmpd/conman.hxx \
     upmpd/conman.cxx \
     upmpd/ohproduct.hxx \
     upmpd/ohproduct.cxx \
     upmpd/ohinfo.hxx \
     upmpd/ohinfo.cxx \
     upmpd/ohtime.hxx \
     upmpd/ohtime.cxx \
     upmpd/ohvolume.hxx \
     upmpd/ohvolume.cxx \
     upmpd/ohplaylist.hxx \
     upmpd/ohplaylist.cxx \
     upmpd/base64.hxx \
     upmpd/base64.cxx
                           
upmpdcli_LDADD = libupnpp.la -lmpdclient 

dist_pkgdata_DATA = upmpd/description.xml upmpd/AVTransport.xml \
                  upmpd/RenderingControl.xml upmpd/ConnectionManager.xml \
                  upmpd/OHProduct.xml upmpd/OHInfo.xml upmpd/OHTime.xml \
                  upmpd/OHVolume.xml upmpd/OHPlaylist.xml upmpd/protocolinfo.txt

dist_sysconf_DATA = upmpd/upmpdcli.conf

dist_man1_MANS = man/upmpdcli.1

EXTRA_DIST = systemd/upmpdcli.service
dist-hook:
	test -z "`git status -s | grep -v upmpdcli-$(VERSION)`"
	git tag -f -a upmpdcli-v$(VERSION) -m 'version $(VERSION)'
