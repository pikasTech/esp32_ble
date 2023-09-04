/*
 * [Warning!] This file is auto-generated by pika compiler.
 * Do not edit it manually.
 * The source code is *.pyi file.
 * More details: 
 * English Doc:
 * https://pikadoc-en.readthedocs.io/en/latest/PikaScript%20%E6%A8%A1%E5%9D%97%E6%A6%82%E8%BF%B0.html
 * Chinese Doc:
 * http://pikapython.com/doc/PikaScript%20%E6%A8%A1%E5%9D%97%E6%A6%82%E8%BF%B0.html
 */

#ifndef ___bluetooth_BLE__H
#define ___bluetooth_BLE__H
#include <stdio.h>
#include <stdlib.h>
#include "PikaObj.h"

PikaObj *New__bluetooth_BLE(Args *args);

int _bluetooth_BLE_advertise(PikaObj *self, int own_addr_type, int interval_us, pika_bool connectable);
int _bluetooth_BLE_config_addr_mode_get(PikaObj *self);
int _bluetooth_BLE_config_addr_mode_update(PikaObj *self);
int _bluetooth_BLE_config_addr_rxbuf_get(PikaObj *self);
int _bluetooth_BLE_config_bond_get(PikaObj *self);
int _bluetooth_BLE_config_bond_update(PikaObj *self, pika_bool bond);
char* _bluetooth_BLE_config_gap_name_get(PikaObj *self);
int _bluetooth_BLE_config_gap_name_update(PikaObj *self, char* gap_name);
int _bluetooth_BLE_config_io_get(PikaObj *self);
int _bluetooth_BLE_config_io_update(PikaObj *self, int io);
int _bluetooth_BLE_config_le_secure_get(PikaObj *self);
int _bluetooth_BLE_config_le_secure_update(PikaObj *self, pika_bool le_secure);
int _bluetooth_BLE_config_mac_get(PikaObj *self);
int _bluetooth_BLE_config_mac_update(PikaObj *self);
int _bluetooth_BLE_config_mitm_get(PikaObj *self);
int _bluetooth_BLE_config_mitm_update(PikaObj *self, pika_bool mitm);
int _bluetooth_BLE_config_mtu_get(PikaObj *self);
int _bluetooth_BLE_config_mtu_update(PikaObj *self, int mtu);
int _bluetooth_BLE_config_rxbuf_update(PikaObj *self, int rxbuf);
int _bluetooth_BLE_gap_connect(PikaObj *self, int addr_type, char* addr, int scan_duration_ms);
int _bluetooth_BLE_gap_disconnect(PikaObj *self);
int _bluetooth_BLE_gap_scan(PikaObj *self, int addr_mode, int duration_ms, int interval_us, int window_us, pika_bool active);
int _bluetooth_BLE_gap_stop_scan(PikaObj *self);
int _bluetooth_BLE_init(PikaObj *self);
pika_bool _bluetooth_BLE_pyi_active(PikaObj *self, pika_bool active);
pika_bool _bluetooth_BLE_pyi_check_active(PikaObj *self);
int _bluetooth_BLE_pyi_test(PikaObj *self);
int _bluetooth_BLE_register_a_service(PikaObj *self, PikaObj* service_info);
void _bluetooth_BLE_setCallback(PikaObj *self, Arg* cb);
int _bluetooth_BLE_set_adv_data(PikaObj *self, char* data, int data_len);
int _bluetooth_BLE_set_rsp_data(PikaObj *self, char* data, int data_len);
int _bluetooth_BLE_stop_advertise(PikaObj *self);

#endif
