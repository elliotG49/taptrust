#include <stdio.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <unistd.h> 
#include <stdint.h> 
#include <linux/i2c-dev.h> 
#include <sys/ioctl.h>

int main(void) { const char *i2cDevice = "/dev/i2c-1"; int file;

// Open the I2C bus
if ((file = open(i2cDevice, O_RDWR)) < 0) {
    perror("Failed to open the I2C bus");
    exit(1);
}

// Set the I2C slave address for the PN532 (commonly 0x48)
int addr = 0x24;
if (ioctl(file, I2C_SLAVE, addr) < 0) {
    perror("Failed to acquire bus access or talk to PN532");
    close(file);
    exit(1);
}

/*
  Build the PN532 command frame for TgInitAsTarget.
  
  Frame structure (for I2C):
  - Preamble:      0x00
  - Start code:    0x00, 0xFF
  - LEN:           Length of data bytes (TFI + PDs)
  - LCS:           Checksum for LEN (0x100 - LEN)
  - Data:          [TFI, PD0, PD1, …, PDn]
     where:
       TFI = 0xD4 (from host to PN532)
       PD0 = Command code for TgInitAsTarget (0x8C)
       PD1 = Mode (0x00 for passive)
       PD2,PD3 = SENS_RES (example: 0x04, 0x00)
       PD4 = SEL_RES (example: 0x20)
       PD5 = NFCID length (example: 0x04 for a 4‑byte NFCID)
       PD6–PD9 = NFCID (example: 0x12, 0x34, 0x56, 0x78)
       PD10 = TL (ATS length, 0x00 if ATS data is omitted)
  - DCS:           Data checksum: 0x100 - (sum of Data bytes)
  - Postamble:     0x00
  
  In this example:
  Data bytes = {0xD4, 0x8C, 0x00, 0x04, 0x00, 0x20, 0x04, 0x12, 0x34, 0x56, 0x78, 0x00}
  LEN = 12 (0x0C)
  LCS = 0x100 - 0x0C = 0xF4
  
  Calculate the sum for DCS:
    Sum = 0xD4 + 0x8C + 0x00 + 0x04 + 0x00 + 0x20 + 0x04 + 0x12 + 0x34 + 0x56 + 0x78 + 0x00
        = 212 + 140 + 0 + 4 + 0 + 32 + 4 + 18 + 52 + 86 + 120 + 0 = 668
    668 mod 256 = 156 (0x9C? Let's recalc: 668 - 2*256 = 668 - 512 = 156)
    DCS = 0x100 - 156 = 256 - 156 = 100 = 0x64
  
  The full frame is then:
  {0x00, 0x00, 0xFF, 0x0C, 0xF4, 0xD4, 0x8C, 0x00, 0x04, 0x00, 0x20, 0x04, 0x12, 0x34, 0x56, 0x78, 0x00, 0x64, 0x00}
*/

uint8_t frame[] = {
    0x00,       // Preamble
    0x00, 0xFF, // Start code
    0x0C,       // LEN = 12 bytes of data
    0xF4,       // LCS = 0x100 - 0x0C
    0xD4,       // TFI (host -> PN532)
    0x8C,       // Command code: TgInitAsTarget
    0x00,       // Mode: 0x00 for passive target
    0x04, 0x00, // SENS_RES (example values)
    0x20,       // SEL_RES (example value)
    0x04,       // NFCID length (4 bytes)
    0x12, 0x34, 0x56, 0x78, // NFCID (example UID)
    0x00,       // TL: ATS length (0 = no ATS provided)
    0x64,       // DCS (calculated checksum for data)
    0x00        // Postamble
};

int frameLen = sizeof(frame);

// Write the command frame to the PN532
if (write(file, frame, frameLen) != frameLen) {
    perror("Failed to write command frame to PN532");
    close(file);
    exit(1);
}

// Optionally, read the response from PN532 (adjust timeout and response length as needed)
uint8_t response[32];
int bytesRead = read(file, response, sizeof(response));
if (bytesRead < 0) {
    perror("Failed to read response from PN532");
} else {
    printf("Response (%d bytes): ", bytesRead);
    for (int i = 0; i < bytesRead; i++) {
        printf("%02X ", response[i]);
    }
    printf("\n");
}

close(file);
return 0;
}