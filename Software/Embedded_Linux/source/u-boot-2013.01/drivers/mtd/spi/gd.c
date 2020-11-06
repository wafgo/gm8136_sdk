
#include <common.h>
#include <malloc.h>
#include <spi_flash.h>

#include "spi_flash_internal.h"

struct gd_spi_flash_params {
	u16 idcode;
	u16 nr_blocks;
	const char *name;
};

static const struct gd_spi_flash_params gd_spi_flash_table[] = {
	{
		.idcode = 0x4019,
		.nr_blocks = 512,
		.name = "GD25Q256c",
	},    
	{
		.idcode = 0x4018,
		.nr_blocks = 256,
		.name = "GD25Q128c",
	},
	{
		.idcode = 0x4017,
		.nr_blocks = 128,
		.name = "GD25Q64c",
	},	
};

struct spi_flash *spi_flash_probe_gd(struct spi_slave *spi, u8 *idcode)
{
	const struct gd_spi_flash_params *params;
	struct spi_flash *flash;
	unsigned int i;
	u16 id = idcode[2] | idcode[1] << 8;

	for (i = 0; i < ARRAY_SIZE(gd_spi_flash_table); i++) {
		params = &gd_spi_flash_table[i];
		if (params->idcode == id)
			break;
	}

	if (i == ARRAY_SIZE(gd_spi_flash_table)) {
		debug("SF: Unsupported GD ID %04x\n", id);
		return NULL;
	}

	flash = malloc(sizeof(*flash));
	if (!flash) {
		debug("SF: Failed to allocate memory\n");
		return NULL;
	}

	flash->spi = spi;
	flash->name = params->name;

	flash->write = spi_flash_cmd_write_multi;
	flash->erase = spi_flash_cmd_erase;
	flash->read = spi_flash_cmd_read_fast;
	flash->page_size = 256;
	flash->sector_size = 256 * 16 * 16;
	flash->size = flash->sector_size * params->nr_blocks;

	/* Clear BP# bits for read-only flash */
	spi_flash_cmd_write_status(flash, 0);

	return flash;
}
