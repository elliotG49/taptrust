#include <nfc/nfc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(void) {
    nfc_context *context;
    nfc_device *pnd;
    nfc_target nt;
    // Define modulation parameters for ISO14443A at 106 kbps
    nfc_modulation nm[1] = {
        {
            .nmt = NMT_ISO14443A,
            .nbr = NBR_106
        }
    };

    uint8_t command[] = { 0x00, 0xA4, 0x04, 0x00 }; // Example APDU command (SELECT command)
    size_t command_len = sizeof(command);
    uint8_t response[256];
    int res;

    // Initialize libnfc
    nfc_init(&context);
    if (context == NULL) {
        fprintf(stderr, "Unable to init libnfc\n");
        exit(EXIT_FAILURE);
    }

    // Open the first available NFC device (e.g., ACR122U)
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

    // Poll for a target (card) before sending APDU command
    printf("Polling for target...\n");
    int targetCount = nfc_initiator_poll_target(pnd, nm, 1, 4, 4, &nt);
    if (targetCount <= 0) {
        fprintf(stderr, "No target found. Please place a card or enable card emulation on the client device.\n");
        nfc_close(pnd);
        nfc_exit(context);
        exit(EXIT_FAILURE);
    }
    printf("Target detected!\n");

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
