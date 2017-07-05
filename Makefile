#Save files to
INSTALL_DIR = /opt/mmt/probe
MKDIR  = mkdir -p
TOP ?= $(shell pwd)
OUTPUT_DIR =$(TOP)/build
CC     = gcc-4.9
CP     = cp
RM     = rm -rf

#Name of executable file to generate
APP = probe

ifndef VERBOSE
        QUIET := @
endif

#get git version abbrev
GIT_VERSION := $(shell git log --format="%h" -n 1)
VERSION     := 1.2.2


#set of library
LIBS     := -L/opt/mmt/dpi/lib -L/opt/mmt/security/lib -lmmt_core -lmmt_tcpip -lmmt_security -lmmt_security2 -lxml2 -lconfuse -lhiredis -lpthread -lrdkafka -lrt

#for debuging
ifdef DEBUG
	CFLAGS   := -g
	CLDFLAGS := -g
else
	CFLAGS   := -O3
	CLDFLAGS := -O3
endif

# - - - - - - - - - - -
# FOR DPDK ENVIRONMENT
# - - - - - - - - - - - 

ifdef DPDK
RTE_SDK=/home/mmt/dpdk/
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

# For HTTP reconstruction option
ifdef HTTP_RECONSTRUCT
LIBS     += -lhtmlstreamparser -lz
CFLAGS   += -DHTTP_RECONSTRUCT
endif

# For showing message from debug(...)
ifndef NDEBUG
CLDFLAGS   += -DNDEBUG
CFLAGS 	   += -DNDEBUG
endif

#Name of executable file to generate
#APP = probe

SRCS-y := src/smp_main.c  src/processing.c src/web_session_report.c src/thredis.c \
src/send_msg_to_file.c src/send_msg_to_redis.c src/ip_statics.c src/init_socket.c src/rtp_session_report.c src/ftp_session_report.c \
src/event_based_reporting.c src/protocols_report.c src/ssl_session_report.c src/default_app_session_report.c \
src/microflows_session_report.c src/radius_reporting.c src/security_analysis.c src/parseoptions.c src/license.c src/dpdk_capture.c \
src/lib/security.c src/lib/data_spsc_ring.c src/lib/lock_free_spsc_ring.c src/lib/packet_hash.c src/lib/system_info.c src/attributes_extraction.c \
src/multisession_reporting.c src/security_msg_reporting.c src/condition_based_reporting.c  src/pcap_capture.c src/html_integration.c src/http_reconstruct.c \
src/send_msg_to_kafka.c

#set of library

LDLIBS   += $(LIBS)
CFLAGS   += $(WERROR_CFLAGS) -I /opt/mmt/dpi/include -I /opt/mmt/security/include -I /usr/local/include/librdkafka \
 -Wall -Wno-unused-variable -DVERSION=\"$(VERSION)\" -DGIT_VERSION=\"$(GIT_VERSION)\" -DDPDK
 
include $(RTE_SDK)/mk/rte.extapp.mk

endif
# - - - - - -
# END OF DPDK
# - - - - - -


# - - - - - - - - - - -
# FOR PCAP ENVIRONMENT
# - - - - - - - - - - - 

ifdef PCAP
#name of executable file to generate
#APP = probe
# For HTTP reconstruction option
ifdef HTTP_RECONSTRUCT
LIBS     += -lhtmlstreamparser -lz
CFLAGS   += -DHTTP_RECONSTRUCT
endif

# For showing message from debug(...)
ifndef NDEBUG
CLDFLAGS   += -DNDEBUG
CFLAGS 	  += -DNDEBUG
endif
#set of library
LIBS     += -lpcap -ldl
CFLAGS   += -Wall -Wno-unused-variable -DVERSION=\"$(VERSION)\" -DGIT_VERSION=\"$(GIT_VERSION)\" -DPCAP
CLDFLAGS += -I /opt/mmt/dpi/include -I /opt/mmt/security/include -I /usr/local/include/librdkafka 

#folders containing source files
SRCDIR = src

#objects to generate
LIB_OBJS :=  $(patsubst %.c,%.o, $(wildcard $(SRCDIR)/lib/*.c)) \

#filter out 2 files: src/main.c and src/test_probe.c
MAIN_SRCS := $(wildcard   $(SRCDIR)/*.c)
MAIN_SRCS := $(filter-out $(SRCDIR)/main.c,       $(MAIN_SRCS))
MAIN_SRCS := $(filter-out $(SRCDIR)/test_probe.c, $(MAIN_SRCS))

MAIN_OBJS := $(patsubst %.c,%.o, $(MAIN_SRCS)) \

all: $(LIB_OBJS) $(MAIN_OBJS)
	@echo "[COMPILE] probe"
	$(QUIET) $(CC) -o $(APP) $(CLDFLAGS)  $^ $(LIBS)
%.o: %.c
	@echo "[COMPILE] $(notdir $@)"
	$(QUIET) $(CC) $(CFLAGS) $(CLDFLAGS) -c -o $@ $<
clean:
	$(QUIET) $(RM) $(MAIN_OBJS) $(LIB_OBJS) $(OUTPUT)
		
endif 
# - - - - - -
# END OF PCAP
# - - - - - -


FACE_ROOT_DIR=/tmp/probe/$(INSTALL_DIR)
#
# Install probe
#
copy_files: all
#create dir
	$(QUIET) $(RM) $(FACE_ROOT_DIR)
	$(QUIET) $(MKDIR) $(FACE_ROOT_DIR)/bin \
		$(FACE_ROOT_DIR)/log/online \
		$(FACE_ROOT_DIR)/log/offline \
		$(FACE_ROOT_DIR)/result/report/offline \
		$(FACE_ROOT_DIR)/result/report/online \
		$(FACE_ROOT_DIR)/result/behaviour/online \
		$(FACE_ROOT_DIR)/result/behaviour/offline \
		$(FACE_ROOT_DIR)/result/security/online \
		$(FACE_ROOT_DIR)/result/security/offline

#copy probe to existing dir from buit in DPDK
ifdef DPDK
	$(QUIET) $(CP) $(OUTPUT_DIR)/probe $(TOP)
endif

#copy to bin
	$(QUIET) $(CP) $(APP)           $(FACE_ROOT_DIR)/bin/probe

	$(QUIET) $(CP) mmt_online.conf  $(FACE_ROOT_DIR)/mmt-probe.conf

	@echo


create: copy_files
	$(QUIET) $(MKDIR) $(INSTALL_DIR)
	$(QUIET) $(CP) -r $(FACE_ROOT_DIR)/* $(INSTALL_DIR)

SYS_NAME    = $(shell uname -s)
SYS_VERSION = $(shell uname -p)

ifdef DPDK
	DEB_NAME = mmt-probe_$(VERSION)_$(GIT_VERSION)_$(SYS_NAME)_$(SYS_VERSION)_dpdk
else
	DEB_NAME = mmt-probe_$(VERSION)_$(GIT_VERSION)_$(SYS_NAME)_$(SYS_VERSION)_pcap
endif

deb: copy_files
	echo $(DEB_NAME)
	$(QUIET) $(RM) $(DEB_NAME)
	$(QUIET) $(MKDIR) $(DEB_NAME)/DEBIAN
	$(QUIET) echo "Package: mmt-probe \
        \nVersion: $(VERSION) \
        \nSection: base \
        \nPriority: standard \
        \nDepends: mmt-dpi, mmt-security \
        \nArchitecture: all \
        \nMaintainer: Montimage <contact@montimage.com> \
        \nDescription: MMT-Probe:  \
        \n  Version id: $(GIT_VERSION). Build time: `date +"%Y-%m-%d %H:%M:%S"` \
        \nHomepage: http://www.montimage.com" \
		> $(DEB_NAME)/DEBIAN/control

	$(QUIET) $(MKDIR) $(DEB_NAME)/usr/bin/
	$(QUIET) ln -s /opt/mmt/probe/bin/probe $(DEB_NAME)/usr/bin/mmt-probe
	
	$(QUIET) $(MKDIR) $(DEB_NAME)/etc/ld.so.conf.d/
	@echo "/opt/mmt/probe/lib" >> $(DEB_NAME)/etc/ld.so.conf.d/mmt-probe.conf
	
	$(QUIET) $(MKDIR) $(DEB_NAME)$(INSTALL_DIR)
	$(QUIET) $(CP) -r $(FACE_ROOT_DIR)/* $(DEB_NAME)$(INSTALL_DIR)
	
	$(QUIET) $(MKDIR) $(DEB_NAME)$(INSTALL_DIR)/lib
	$(QUIET) $(CP) /usr/local/lib/libhiredis.so.*  $(DEB_NAME)$(INSTALL_DIR)/lib
	$(QUIET) $(CP) /usr/local/lib/librdkafka.so.*  $(DEB_NAME)$(INSTALL_DIR)/lib
	$(QUIET) $(CP) /usr/lib/x86_64-linux-gnu/libconfuse.so.*  $(DEB_NAME)$(INSTALL_DIR)/lib
	
	$(QUIET) dpkg-deb -b $(DEB_NAME)
	$(QUIET) $(RM) $(DEB_NAME)
	$(QUIET) $(RM) $(FACE_ROOT_DIR)

rpm: copy_files
	$(QUIET) $(RM) $(DEB_NAME)
	
	$(QUIET) $(MKDIR) $(DEB_NAME)/usr/bin/
	$(QUIET) ln -s /opt/mmt/probe/bin/probe $(DEB_NAME)/usr/bin/mmt-probe
	
	$(QUIET) $(MKDIR) $(DEB_NAME)/etc/ld.so.conf.d/
	@echo "/opt/mmt/probe/lib" >> $(DEB_NAME)/etc/ld.so.conf.d/mmt-probe.conf
	
	$(QUIET) $(MKDIR) $(DEB_NAME)$(INSTALL_DIR)
	$(QUIET) $(CP) -r $(FACE_ROOT_DIR)/* $(DEB_NAME)$(INSTALL_DIR)
	
	$(QUIET) $(MKDIR) $(DEB_NAME)$(INSTALL_DIR)/lib
	$(QUIET) $(CP) /usr/local/lib/libhiredis.so.*  $(DEB_NAME)$(INSTALL_DIR)/lib
	$(QUIET) $(CP) /usr/local/lib/librdkafka.so.*  $(DEB_NAME)$(INSTALL_DIR)/lib
	$(QUIET) $(CP) /lib64/libconfuse.so.*  $(DEB_NAME)$(INSTALL_DIR)/lib/
	
	
	$(QUIET) $(MKDIR) ./rpmbuild/{RPMS,BUILD}
	
	$(QUIET) echo -e\
      "Summary:  MMT-Probe\
      \nName: mmt-probe\
      \nVersion: $(VERSION)\
      \nRelease: $(GIT_VERSION)\
      \nLicense: proprietary\
      \nGroup: Development/Tools\
      \nURL: http://montimage.com/\
      \n\
      \nRequires:  mmt-dpi >= 1.6.9, mmt-security >= 1.1.5\
      \nBuildRoot: %{_topdir}/BUILD/%{name}-%{version}-%{release}\
      \n\
      \n%description\
      \nMMT-Probe is a tool to analyze network traffic.\
      \nBuild date: `date +"%Y-%m-%d %H:%M:%S"`\
      \n\
      \n%prep\
      \nrm -rf %{buildroot}\
      \nmkdir -p %{buildroot}\
      \ncp -r %{_topdir}/../$(DEB_NAME)/* %{buildroot}/\
      \n\
      \n%clean\
      \nrm -rf %{buildroot}\
      \n\
      \n%files\
      \n%defattr(-,root,root,-)\
      \n/opt/mmt/probe/*\
      \n/usr/bin/mmt-probe\
      \n/etc/ld.so.conf.d/mmt-probe.conf\
      \n%post\
      \nldconfig\
   " > ./mmt-probe.spec
	
	$(QUIET) rpmbuild --quiet --rmspec --define "_topdir $(shell pwd)/rpmbuild" --define "_rpmfilename ../../$(DEB_NAME).rpm" -bb ./mmt-probe.spec
	$(QUIET) $(RM) rpmbuild
	@echo "[PACKAGE] $(DEB_NAME).rpm"
	
	$(QUIET) $(RM) $(DEB_NAME)
	$(QUIET) $(RM) $(FACE_ROOT_DIR)


keygen:
	$(QUIET) $(CC) -o keygen $(CLDFLAGS)  key_generator.c
		
dist-clean:
	$(QUIET) $(RM) -rf $(INSTALL_DIR)
	$(QUIET) $(RM) -rf /etc/init.d/probe_*_d
