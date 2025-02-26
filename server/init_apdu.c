#include <nfc/nfc.h> 
#include <stdio.h> 
#include <stdlib.h>

int main(void) { nfc_context *context; nfc_device *pnd; uint8_t command[] = { 0x00, 0xA4, 0x04, 0x00 }; // Example APDU command (SELECT command) size_t command_len = sizeof(command); uint8_t response[256]; int res;

// Initialize libnfc
nfc_init(&context);
if (context == NULL) {
    fprintf(stderr, "Unable to init libnfc\n");
    exit(EXIT_FAILURE);
}

// Open the first available NFC device (PN532)
pnd = nfc_open(context, NULL);
if (pnd == NULL) {
    fprintf(stderr, "Unable to open NFC device\n");
    nfc_exit(context);
    exit(EXIT_FAILURE);
}

// Set device to initiator mode
if (nfc_initiator_init(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_init");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
}
printf("NFC device: %s opened in initiator mode\n", nfc_device_get_name(pnd));

// Send the APDU command and wait for the response
res = nfc_initiator_transceive_bytes(pnd, command, command_len, response, sizeof(response), 0);
if (res < 0) {
    nfc_perror(pnd, "nfc_initiator_transceive_bytes");
} else {
    printf("Response APDU: ");
    for (int i = 0; i < res; i++) {
        printf("%02X ", response[i]);
    }
    printf("\n");
}

// Cleanup
nfc_close(pnd);
nfc_exit(context);
return 0;
}