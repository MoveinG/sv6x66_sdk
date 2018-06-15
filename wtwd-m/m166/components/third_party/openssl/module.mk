LIB_SRC := 
include $(TOPDIR)/components/third_party/openssl/crypto/Makefile

include $(TOPDIR)/components/third_party/openssl/crypto/aes/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/asn1/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/bio/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/bf/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/bn/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/buffer/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/camellia/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/cast/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/cmac/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/cms/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/comp/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/conf/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/des/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/dh/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/dsa/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/dso/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/ec/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/ecdh/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/ecdsa/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/engine/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/err/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/evp/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/hmac/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/idea/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/krb5/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/lhash/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/md4/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/md5/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/mdc2/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/modes/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/objects/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/ocsp/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/pem/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/pkcs7/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/pkcs12/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/pqueue/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/rand/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/rc2/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/rc4/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/ripemd/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/rsa/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/seed/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/sha/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/srp/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/stack/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/ts/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/txt_db/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/ui/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/whrlpool/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/x509/Makefile
include $(TOPDIR)/components/third_party/openssl/crypto/x509v3/Makefile

include $(TOPDIR)/components/third_party/openssl/ssl/Makefile

LIB_ASRC :=
LIBRARY_NAME := openssl
LOCAL_CFLAGS += -Wno-address
LOCAL_AFLAGS +=

LOCAL_INC += -I$(TOPDIR)/components/third_party/openssl/include
LOCAL_INC += -I$(TOPDIR)/components/third_party/openssl/crypto
LOCAL_INC += -I$(TOPDIR)/components/net/tcpip/lwip-1.4.0/src/include/lwip

RELEASE_SRC := 2
$(eval $(call build-lib,$(LIBRARY_NAME),$(LIB_SRC),$(LIB_ASRC),$(LOCAL_CFLAGS),$(LOCAL_INC),$(LOCAL_AFLAGS),$(MYDIR)))

