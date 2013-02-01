# bcm4330
DHDCFLAGS = -DLINUX -DBCMDRIVER -DBCMDONGLEHOST -DUNRELEASEDCHIP      \
  -DBCMDMA32 -DBCMFILEIMAGE -DEMBEDDED_PLATFORM -DCSCAN -DDHDTHREAD   \
  -DDHD_GPL -DDHD_SCHED -DSDTEST -DBDC -DTOE -DDHD_BCMEVENTS          \
  -DSHOW_EVENTS -DDONGLEOVERLAYS -DBCMSDIO -DDHD_GPL -DBCMLXSDMMC     \
  -DBCMPLATFORM_BUS -DSDIO_ISR_THREAD -DWIFI_ACT_FRAME -DCUSTOMER_HW  \
  -Dlinux -Wall -Wstrict-prototypes -Werror -DDHD_SDALIGN=64 -DWLP2P  \
  -DMAX_HDR_READ=64 -DDHD_FIRSTREAD=64 -DPKT_FILTER_SUPPORT -DHW_OOB  \
  -DMMC_SDIO_ABORT -DMACH_MAHIMAHI -Idrivers/net/wireless/bcm4330     \
  -Idrivers/net/wireless/bcm4330/include

DHDOFILES =         \
  dhd_linux.o       \
  linux_osl.o       \
  bcmutils.o        \
  bcmevent.o        \
  dhd_common.o      \
  dhd_custom_gpio.o \
  siutils.o         \
  sbutils.o         \
  aiutils.o         \
  hndpmu.o          \
  bcmwifi.o         \
  dhd_sdio.o        \
  dhd_linux_sched.o \
  dhd_cdc.o         \
  bcmsdh_sdmmc.o    \
  bcmsdh.o          \
  bcmsdh_linux.o    \
  bcmsdh_sdmmc_linux.o

obj-$(CONFIG_BCM4330) += bcm4330.o
bcm4330-objs += $(DHDOFILES)

ifneq ($(CONFIG_WIRELESS_EXT),)
bcm4330-objs += wl_iw.o
DHDCFLAGS += -DSOFTAP -DWL_WIRELESS_EXT
endif

EXTRA_CFLAGS = $(DHDCFLAGS)
ifeq ($(CONFIG_BCM4330),m)
EXTRA_LDFLAGS += --strip-debug
endif
