/**
 * @file littlefs_api.c
 * @brief Maps the HAL of esp_partition <-> littlefs
 * @author Brian Pugh
 */

#include "esp_log.h"
#include "esp_partition.h"

// SPDX-License-Identifier: Apache-2.0
#include "common.h"
#include <fs.h>
#include <memzero.h>
#include <stdalign.h>
#include <string.h>

#define LOOKAHEAD_SIZE 16
#define CACHE_SIZE 128
#define WRITE_SIZE 8
#define READ_SIZE 1

// FIXME: configurable and according to partition table
#define FLASH_PAGE_SIZE 0x1000
#define FLASH_SIZE 0x100000

static struct lfs_config config;
static uint8_t read_buffer[CACHE_SIZE];
static uint8_t prog_buffer[CACHE_SIZE];
static alignas(4) uint8_t lookahead_buffer[LOOKAHEAD_SIZE];
extern uint8_t _lfs_begin;

static esp_partition_t* partition;

int littlefs_api_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size) {
    size_t part_off = (block * c->block_size) + off;
    esp_err_t err = esp_partition_read(partition, part_off, buffer, size);
    if (err) {
        ERR_MSG("failed to read addr %08x, size %08x, err %d", part_off, size, err);
        return LFS_ERR_IO;
    }
    return 0;
}

int littlefs_api_prog(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size) {
    size_t part_off = (block * c->block_size) + off;
    esp_err_t err = esp_partition_write(partition, part_off, buffer, size);
    if (err) {
        ERR_MSG("failed to write addr %08x, size %08x, err %d", part_off, size, err);
        return LFS_ERR_IO;
    }
    return 0;
}

int littlefs_api_erase(const struct lfs_config *c, lfs_block_t block) {
    size_t part_off = block * c->block_size;
    esp_err_t err = esp_partition_erase_range(partition, part_off, c->block_size);
    if (err) {
        ERR_MSG("failed to erase addr %08x, size %08x, err %d", part_off, c->block_size, err);
        return LFS_ERR_IO;
    }
    return 0;

}

int littlefs_api_sync(const struct lfs_config *c) {
    /* Unnecessary for esp-idf */
    return 0;
}

void littlefs_init() {
  memzero(&config, sizeof(config));
  config.read = littlefs_api_read;
  config.prog = littlefs_api_prog;
  config.erase = littlefs_api_erase;
  config.sync = littlefs_api_sync;
  config.read_size = READ_SIZE;
  config.prog_size = WRITE_SIZE;
  config.block_size = FLASH_PAGE_SIZE;
  config.block_count = FLASH_SIZE / FLASH_PAGE_SIZE;
  config.block_cycles = 100000;
  config.cache_size = CACHE_SIZE;
  config.lookahead_size = LOOKAHEAD_SIZE;
  config.read_buffer = read_buffer;
  config.prog_buffer = prog_buffer;
  config.lookahead_buffer = lookahead_buffer;
  DBG_MSG("Flash %u blocks (%u bytes)\r\n", config.block_count, FLASH_PAGE_SIZE);
  
  partition = (esp_partition_t*)esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, "lfs");
  if (!partition) {
    ERR_MSG("Can not find partition lfs!\n");
    for (;;)
      ;
  }

  int err;
  for (int retry = 0; retry < 3; retry++) {
    err = fs_mount(&config);
    if (!err) return;
  }
  // should happen for the first boot
  DBG_MSG("Formating data area...\r\n");
  fs_format(&config);
  err = fs_mount(&config);
  if (err) {
    ERR_MSG("Failed to mount FS after formating\r\n");
    for (;;)
      ;
  }
}
