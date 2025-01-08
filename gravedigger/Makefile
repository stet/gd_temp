# Installation directories following GNU conventions
prefix ?= /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
sbindir = $(exec_prefix)/sbin
datarootdir = $(prefix)/share
datadir = $(datarootdir)
includedir = $(prefix)/include
mandir = $(datarootdir)/man

BIN=bin
OBJ=obj
SRC=src
MODS=mods
CTRL=ctrl_mods

CC ?= gcc
CFLAGS ?= -Wextra -Wall -iquote$(SRC) -idirafter$(SRC)/missing
CLIBS ?= -lpthread

CTRL_OBJS = $(OBJ)/$(CTRL)/btk_help.o $(OBJ)/$(CTRL)/btk_privkey.o $(OBJ)/$(CTRL)/btk_pubkey.o $(OBJ)/$(CTRL)/btk_address.o $(OBJ)/$(CTRL)/btk_node.o $(OBJ)/$(CTRL)/btk_balance.o $(OBJ)/$(CTRL)/btk_config.o $(OBJ)/$(CTRL)/btk_version.o $(OBJ)/$(CTRL)/btk_vanity.o
MOD_OBJS = $(OBJ)/$(MODS)/network.o $(OBJ)/$(MODS)/database.o $(OBJ)/$(MODS)/chainstate.o $(OBJ)/$(MODS)/balance.o $(OBJ)/$(MODS)/txoa.o $(OBJ)/$(MODS)/node.o $(OBJ)/$(MODS)/privkey.o $(OBJ)/$(MODS)/pubkey.o $(OBJ)/$(MODS)/address.o $(OBJ)/$(MODS)/base58check.o $(OBJ)/$(MODS)/crypto.o $(OBJ)/$(MODS)/random.o $(OBJ)/$(MODS)/point.o $(OBJ)/$(MODS)/base58.o $(OBJ)/$(MODS)/base32.o $(OBJ)/$(MODS)/bech32.o $(OBJ)/$(MODS)/hex.o $(OBJ)/$(MODS)/compactuint.o $(OBJ)/$(MODS)/camount.o $(OBJ)/$(MODS)/txinput.o $(OBJ)/$(MODS)/txoutput.o $(OBJ)/$(MODS)/utxokey.o $(OBJ)/$(MODS)/utxovalue.o $(OBJ)/$(MODS)/transaction.o $(OBJ)/$(MODS)/block.o $(OBJ)/$(MODS)/script.o $(OBJ)/$(MODS)/message.o $(OBJ)/$(MODS)/serialize.o $(OBJ)/$(MODS)/json.o $(OBJ)/$(MODS)/jsonrpc.o $(OBJ)/$(MODS)/qrcode.o $(OBJ)/$(MODS)/input.o $(OBJ)/$(MODS)/output.o $(OBJ)/$(MODS)/opts.o $(OBJ)/$(MODS)/config.o $(OBJ)/$(MODS)/error.o $(OBJ)/$(MODS)/gd_vanity.o $(OBJ)/$(MODS)/debug.o
COM_OBJS = $(OBJ)/$(MODS)/commands/verack.o $(OBJ)/$(MODS)/commands/version.o
JSON_OBJS = $(OBJ)/$(MODS)/cJSON/cJSON.o
QRCODE_OBJS = $(OBJ)/$(MODS)/QRCodeGen/qrcodegen.o
GMP_OBJS = $(OBJ)/$(MODS)/GMP/mini-gmp.o
CRYPTO_OBJS = $(OBJ)/$(MODS)/crypto/rmd160.o $(OBJ)/$(MODS)/crypto/sha256.o
LEVELDB_OBJS = $(OBJ)/$(MODS)/leveldb/stub.o

## Install libgmp-dev
ifeq ($(shell ld -lgmp -M -o /dev/null 2>/dev/null | grep -c -m 1 libgmp ), 1)
   ifndef FORCE_BUILTIN_CRYPTO
      CLIBS += -lgmp
      GMP_OBJS = $()
   endif
endif

## Install libssl-dev
ifeq ($(shell ld -lcrypto -M -o /dev/null 2>/dev/null | grep -c -m 1 libcrypto ), 1)
   ifndef FORCE_BUILTIN_CRYPTO
      CLIBS += -lcrypto
      CRYPTO_OBJS = $()
   endif
endif

## Install libleveldb-dev
ifeq ($(shell ld -lleveldb -M -o /dev/null 2>/dev/null | grep -c -m 1 libleveldb ), 1)
   ifndef FORCE_BUILTIN_CRYPTO
      CLIBS += -lleveldb
      LEVELDB_OBJS = $()
   endif
endif

.PHONY: all test install uninstall clean gravedigger create-dirs

EXES = gravedigger

all: $(EXES)

create-dirs:
	mkdir -p $(OBJ)
	mkdir -p $(OBJ)/$(CTRL)
	mkdir -p $(OBJ)/$(MODS)
	mkdir -p $(OBJ)/$(MODS)/commands
	mkdir -p $(OBJ)/$(MODS)/cJSON
	mkdir -p $(OBJ)/$(MODS)/QRCodeGen
	mkdir -p $(OBJ)/$(MODS)/GMP
	mkdir -p $(OBJ)/$(MODS)/crypto
	mkdir -p $(OBJ)/$(MODS)/leveldb
	mkdir -p $(BIN)
	mkdir -p /home/forge/tools.undernet.work/ape-playground/o/gravedigger

gravedigger: CC=/home/forge/tools.undernet.work/ape-playground/cosmocc/bin/cosmocc
gravedigger: CFLAGS=-Wextra -Wall -nostdinc -Isrc -Isrc/missing -I/home/forge/tools.undernet.work/ape-playground/cosmocc/include -DEVP_H_MISSING -DPROVIDER_H_MISSING -DGMP_H_MISSING -DC_H_MISSING
gravedigger: CLIBS=-lpthread
gravedigger: FORCE_BUILTIN_CRYPTO=1
gravedigger: GMP_OBJS=$(OBJ)/$(MODS)/GMP/mini-gmp.o
gravedigger: CRYPTO_OBJS=$(OBJ)/$(MODS)/crypto/rmd160.o $(OBJ)/$(MODS)/crypto/sha256.o
gravedigger: LEVELDB_OBJS=$(OBJ)/$(MODS)/leveldb/stub.o
gravedigger: create-dirs builtin-impl $(CTRL_OBJS) $(MOD_OBJS) $(COM_OBJS) $(JSON_OBJS) $(QRCODE_OBJS) $(OBJ)/btk.o
	$(CC) $(CFLAGS) -o /home/forge/tools.undernet.work/ape-playground/o/gravedigger/gravedigger.com $(GMP_OBJS) $(CRYPTO_OBJS) $(LEVELDB_OBJS) $(CTRL_OBJS) $(MOD_OBJS) $(COM_OBJS) $(JSON_OBJS) $(QRCODE_OBJS) $(OBJ)/btk.o $(CLIBS)

builtin-impl: create-dirs
	$(CC) $(CFLAGS) -o $(OBJ)/$(MODS)/GMP/mini-gmp.o -c $(SRC)/$(MODS)/GMP/mini-gmp.c
	$(CC) $(CFLAGS) -o $(OBJ)/$(MODS)/crypto/rmd160.o -c $(SRC)/$(MODS)/crypto/rmd160.c
	$(CC) $(CFLAGS) -o $(OBJ)/$(MODS)/crypto/sha256.o -c $(SRC)/$(MODS)/crypto/sha256.c
	$(CC) $(CFLAGS) -o $(OBJ)/$(MODS)/leveldb/stub.o -c $(SRC)/$(MODS)/leveldb/stub.c

$(OBJ)/$(CTRL)/%.o: $(SRC)/$(CTRL)/%.c | create-dirs
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ)/$(MODS)/commands/%.o: $(SRC)/$(MODS)/commands/%.c | create-dirs
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ)/$(MODS)/cJSON/%.o: $(SRC)/$(MODS)/cJSON/%.c | create-dirs
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ)/$(MODS)/QRCodeGen/%.o: $(SRC)/$(MODS)/QRCodeGen/%.c | create-dirs
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ)/$(MODS)/GMP/%.o: $(SRC)/$(MODS)/GMP/%.c | create-dirs
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ)/$(MODS)/crypto/%.o: $(SRC)/$(MODS)/crypto/%.c | create-dirs
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ)/$(MODS)/leveldb/%.o: $(SRC)/$(MODS)/leveldb/%.c | create-dirs
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ)/$(MODS)/%.o: $(SRC)/$(MODS)/%.c | create-dirs
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ)/%.o: $(SRC)/%.c | create-dirs
	$(CC) $(CFLAGS) -o $@ -c $<

test:
	python3 test/test.py

clean:
	rm -rf $(BIN)
	rm -rf $(OBJ)
	rm -rf /home/forge/tools.undernet.work/ape-playground/o/gravedigger

install:
	install -d $(DESTDIR)$(mandir)/man1
	install -m644 man/*.1 $(DESTDIR)$(mandir)/man1
	install -d $(DESTDIR)$(bindir)
	cd $(BIN) && install $(EXES) $(DESTDIR)$(bindir)

uninstall:
	rm -f $(DESTDIR)$(mandir)/man1/gravedigger*
	for exe in $(EXES); do rm $(DESTDIR)$(bindir)/$$exe; done
