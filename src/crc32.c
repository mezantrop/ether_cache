#include <stdint.h>

/* ---------------------------------------------------------------------------- */
uint32_t calk_crc32(const uint8_t *data, unsigned data_len) {
	uint32_t i, j, bit;
	uint32_t crc;
	const static uint32_t polynomial = 0x04c11db7U;
	const static uint32_t initializer = 0xFFFFFFFFU;
	static uint32_t crc_table[256];
	static int crc_table_exists = 0;

	if (!crc_table_exists) {
		for (i = 0; i < 256;  i++) {
			crc = (i << 24);
			for (bit = 0;  bit < 8;  bit++) {
				if (crc & 0x80000000U)
					crc = (crc << 1) ^ polynomial;
				else
					crc = (crc << 1);
			}
			crc_table[i] = crc;
		}
		crc_table_exists = 1;
	}

	crc = initializer;
	for (j = 0;  j < data_len;  j++) {
		i = ((uint8_t)(crc>>24) ^ data[j]) & 0xff;
		crc = ( crc<<8 ) ^ crc_table[i];
	}

	return ~crc;
}

/* ---------------------------------------------------------------------------- */
uint32_t verify_crc32(char *fm, int size) {
        return calk_crc32(fm, size) == 0x38FB2284;
}

