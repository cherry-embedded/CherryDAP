//
// Created by yekai on 2024/3/29.
//

#include "elog.h"
#include "fal.h"
#include "fal_cfg.h"

static const char *TAG = "FAL";

struct fal_partition *bl_part = NULL;
struct fal_partition *app_part = NULL;
struct fal_partition *flashdb_part = NULL;
struct fal_partition *bl_b_part = NULL;

void fal_parts_init() {
    bl_part = (struct fal_partition *) fal_partition_find("bl");
    app_part = (struct fal_partition *) fal_partition_find("app");
    flashdb_part = (struct fal_partition *) fal_partition_find("flashdb");
    bl_b_part = (struct fal_partition *) fal_partition_find("bl_b");

    if (bl_part == NULL || app_part == NULL || flashdb_part == NULL || bl_b_part == NULL) {
        elog_w(TAG, "fal_parts_init failed");
        elog_w(TAG, "bl_part=%p, app_part=%p, flashdb_part=%p, bl_b_part=%p",
               bl_part, app_part, flashdb_part, bl_b_part);
    }
}