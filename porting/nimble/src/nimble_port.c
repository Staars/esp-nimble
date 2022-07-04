/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stddef.h>
#include "os/os.h"
#include "sysinit/sysinit.h"

#if CONFIG_BT_NIMBLE_ENABLED
#include "host/ble_hs.h"
#endif //CONFIG_BT_NIMBLE_ENABLED

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#ifdef ESP_PLATFORM
#include "esp_log.h"
#endif
#include "soc/soc_caps.h"

#if SOC_ESP_NIMBLE_CONTROLLER
#if CONFIG_SW_COEXIST_ENABLE
#include "esp_coexist_internal.h"
#endif
#endif


#include "nimble/ble_hci_trans.h"

#include "esp_intr_alloc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_bt.h"
#include "esp_nimble_hci.h"

#define NIMBLE_PORT_LOG_TAG          "BLE_INIT"

extern void os_msys_init(void);

#if CONFIG_BT_NIMBLE_ENABLED 

extern void ble_hs_deinit(void);
static struct ble_hs_stop_listener stop_listener;

#endif //CONFIG_BT_NIMBLE_ENABLED

static struct ble_npl_eventq g_eventq_dflt;
static struct ble_npl_sem ble_hs_stop_sem;
static struct ble_npl_event ble_hs_ev_stop;

<<<<<<< HEAD
extern void os_msys_init(void);
extern void os_mempool_module_init(void);

void
nimble_port_init(void)
=======
/**
 * Called when the host stop procedure has completed.
 */
static void
ble_hs_stop_cb(int status, void *arg)
>>>>>>> f8a79b04... Optimize the nimble host interface function and divide the host
{
    ble_npl_sem_release(&ble_hs_stop_sem);
}

static void
nimble_port_stop_cb(struct ble_npl_event *ev)
{
    ble_npl_sem_release(&ble_hs_stop_sem);
}

/**
 * @brief esp_nimble_init - Initialize the NimBLE host stack
 * 
 * @return esp_err_t 
 */
esp_err_t esp_nimble_init(void)
{
#if !SOC_ESP_NIMBLE_CONTROLLER
    /* Initialize the function pointers for OS porting */
    npl_freertos_funcs_init();

    npl_freertos_mempool_init();

#if true //need delete esp_nimble_hci_and_controller_init then can be use
    if(esp_nimble_hci_init() != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "hci inits failed\n");
        return ESP_FAIL;
    }
    printf("esp_nimble_hci_init\n");
#endif 

    /* Initialize default event queue */
    ble_npl_eventq_init(&g_eventq_dflt);
    /* Initialize the global memory pool */
    os_mempool_module_init();
    os_msys_init();
    /* Initialize transport */
    ble_transport_init();
    /* Initialize the host */
    ble_transport_hs_init();

#if NIMBLE_CFG_CONTROLLER
    ble_hci_ram_init();
#ifndef RIOT_VERSION
    hal_timer_init(5, NULL);
    os_cputime_init(32768);
#endif
    ble_transport_ll_init();
#endif

    /* Initialize the host */
    ble_hs_init();
    return ESP_OK;
}

/**
 * @brief esp_nimble_deinit - Deinitialize the NimBLE host stack
 * 
 * @return esp_err_t 
 */
esp_err_t esp_nimble_deinit(void)
{
#if !SOC_ESP_NIMBLE_CONTROLLER //need delete esp_nimble_hci_and_controller_init then can be use
    if(esp_nimble_hci_deinit() != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "hci deinit failed\n");
        return ESP_FAIL;
    }
#endif
#if !(SOC_ESP_NIMBLE_CONTROLLER && CONFIG_BT_CONTROLLER_ENABLED)
    ble_npl_eventq_deinit(&g_eventq_dflt);
#endif
    ble_hs_deinit();
    return ESP_OK;
}

void
nimble_port_init(void)
{
#if CONFIG_BT_CONTROLLER_ENABLED
    esp_bt_controller_config_t config_opts = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if(esp_bt_controller_init(&config_opts) != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "controller init failed\n");
        return;
    }
    if(esp_bt_controller_enable(ESP_BT_MODE_BLE) != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "controller enable failed\n");
        return;
    }
#endif

    if(esp_nimble_init() != 0) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "nimble host init failed\n");
        return;
    }
}

void
nimble_port_deinit(void)
{
    if(esp_nimble_deinit() != 0) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "nimble host deinit failed\n");
        return;
    }
#if CONFIG_BT_CONTROLLER_ENABLED
    if(esp_bt_controller_disable() != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "controller disable failed\n");
        return;
    }
    if(esp_bt_controller_deinit() != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "controller deinit failed\n");
        return;
    }
#endif
}


int
nimble_port_stop(void)
{
    esp_err_t err = ESP_OK;
    ble_npl_sem_init(&ble_hs_stop_sem, 0);

    /* Initiate a host stop procedure. */
    err = ble_hs_stop(&stop_listener, ble_hs_stop_cb,
                     NULL);
    if (err != 0) {
        ble_npl_sem_deinit(&ble_hs_stop_sem);
        return err;
    }

    /* Wait till the host stop procedure is complete */
    ble_npl_sem_pend(&ble_hs_stop_sem, BLE_NPL_TIME_FOREVER);

    ble_npl_event_init(&ble_hs_ev_stop, nimble_port_stop_cb,
                       NULL);
    ble_npl_eventq_put(&g_eventq_dflt, &ble_hs_ev_stop);

    /* Wait till the event is serviced */
    ble_npl_sem_pend(&ble_hs_stop_sem, BLE_NPL_TIME_FOREVER);

    ble_npl_sem_deinit(&ble_hs_stop_sem);

    return ESP_OK;
}

void
IRAM_ATTR nimble_port_run(void)
{
    struct ble_npl_event *ev;

    while (1) {
        ev = ble_npl_eventq_get(&g_eventq_dflt, BLE_NPL_TIME_FOREVER);
        ble_npl_event_run(ev);
        if (ev == &ble_hs_ev_stop) {
            break;
        }

    }
}

struct ble_npl_eventq *
IRAM_ATTR nimble_port_get_dflt_eventq(void)
{
    return &g_eventq_dflt;
}
