ifdef PS5_PAYLOAD_SDK
    include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
    $(error PS5_PAYLOAD_SDK is undefined)
endif

ELF := ps5-self-decrypter.elf
SOCK_ELF := ps5-self-decrypter-sock-log.elf

CFLAGS := -Wall -O2 -Iinclude

all: $(ELF)
sock: $(SOCK_ELF)

CFILES := $(wildcard source/*.c)

$(ELF): $(CFILES)
	$(CC) $(CFLAGS) -o bin/$@ $^

$(SOCK_ELF): $(CFILES)
	$(CC) $(CFLAGS) -DLOG_TO_SOCKET -o bin/$@ $^

clean:
	rm -f bin/$(ELF)

send: $(ELF)
	socat -t 99999999 - TCP:192.168.137.2:9021 < bin/$(ELF)