#include "pikaScript.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
// #include "nimble/host/include/host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "services/ans/ble_svc_ans.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_log.h"
// #include "ble_uuid.h"
#include "cb_event_id.h"

#define printf __platform_printf

#define GATT_SVR_SVC_ALERT_UUID               0x1811

static const char *tag = "NimBLE_BLE";
bool BLE_ONLY = false;  //只使用BLE,默认否
bool BLE_FIRST_INIT = true;  //是否第一次初始化,默认是
// uint8_t own_addr_type;

// 函数声明
// gatt 服务回调函数
static int gatt_svc_access(uint16_t conn_handle, uint16_t attr_handle,struct ble_gatt_access_ctxt *ctxt, void *arg);
// 客户端写回调函数
static int ble_cliect_write_cb(uint16_t conn_handle,
                       const struct ble_gatt_error *error,
                       struct ble_gatt_attr *attr,
                       void *arg);
// 客户端读回调函数
static int ble_cliect_read_cb(uint16_t conn_handle,
                       const struct ble_gatt_error *error,
                       struct ble_gatt_attr *attr,
                       void *arg);
// gap回调函数
static int ble_nimble_gap_event(struct ble_gap_event *event, void *arg);


// gatt初始化基本服务
void gatt_svr_init(void);


// 事件监听器
PikaEventListener *g_pika_ble_listener = NULL;

// 蓝牙任务
void ble_host_task(void *param)
{
    ESP_LOGI(tag, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// 获取地址类型
uint8_t get_addr_type(int addr_mode)
{
    uint8_t own_addr_type;
    switch (addr_mode) {
    case 0:
        own_addr_type = BLE_OWN_ADDR_PUBLIC;
        break;
    case 1:
        own_addr_type = BLE_OWN_ADDR_RANDOM;
        break;
    case 2:
        own_addr_type = BLE_OWN_ADDR_RPA_PUBLIC_DEFAULT;
        break;
    case 3:
        own_addr_type = BLE_OWN_ADDR_RPA_RANDOM_DEFAULT;
        break;
    }
    return own_addr_type;
}

int _bluetooth_BLE_init(PikaObj *self)
{
    printf("_bluetooth_BLE___init__\r\n");
    //TODO: flash init 应该放到哪里？
    if (BLE_FIRST_INIT)
    {
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);
        // ret = nimble_port_init();
        nimble_port_init();
        if (ret != ESP_OK) {
            printf("Failed to init nimble %d \n", ret);
            return false;
        }
        BLE_FIRST_INIT = false;
    }
    return 1;
}

pika_bool _bluetooth_BLE_pyi_active(PikaObj *self, pika_bool active)
{
    printf("_bluetooth_BLE_pyi_active\r\n");
    if(active == true){
        //开始任务
        nimble_port_freertos_init(ble_host_task);
        return true;
    }else {
        nimble_port_stop();
        return true;
    }
}

pika_bool _bluetooth_BLE_pyi_check_active(PikaObj *self)
{
    printf("_bluetooth_BLE_pyi_check_active\r\n");
    return true;
}

int _bluetooth_BLE_pyi_test(PikaObj *self)
{
    printf("_bluetooth_BLE_test\r\n");
    return 1;
}

void addr_inver(const void *addr,char *addr_inver)
{
    const uint8_t *u8p;
    u8p = addr;
    for ( int i = 0; i < 6; i++)
    {
        addr_inver[i] =  u8p[5-i];
    }
}

void print_addr(const void *addr)
{
    const uint8_t *u8p;
    u8p = addr;
    MODLOG_DFLT(INFO, "%02x:%02x:%02x:%02x:%02x:%02x",
                u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);
}

/**
 * Logs information about a connection to the console.
 */
static void print_conn_desc(struct ble_gap_conn_desc *desc)
{
    MODLOG_DFLT(INFO, "handle=%d our_ota_addr_type=%d our_ota_addr=",
                desc->conn_handle, desc->our_ota_addr.type);
    print_addr(desc->our_ota_addr.val);
    MODLOG_DFLT(INFO, " our_id_addr_type=%d our_id_addr=",
                desc->our_id_addr.type);
    print_addr(desc->our_id_addr.val);
    MODLOG_DFLT(INFO, " peer_ota_addr_type=%d peer_ota_addr=",
                desc->peer_ota_addr.type);
    print_addr(desc->peer_ota_addr.val);
    MODLOG_DFLT(INFO, " peer_id_addr_type=%d peer_id_addr=",
                desc->peer_id_addr.type);
    print_addr(desc->peer_id_addr.val);
    MODLOG_DFLT(INFO, " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
                "encrypted=%d authenticated=%d bonded=%d\r\n",
                desc->conn_itvl, desc->conn_latency,
                desc->supervision_timeout,
                desc->sec_state.encrypted,
                desc->sec_state.authenticated,
                desc->sec_state.bonded);
}

int _bluetooth_BLE_advertise(PikaObj *self, int addr_mode, int interval_us, pika_bool connectable)
{
    printf("_bluetooth_BLE_gap_advertise\r\n");
    // ble_svc_gap_device_name_set("nimble-bleprph");
    //  声明并初始化广播结构体
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof fields);

    //TODO:貌似不起作用
    if(BLE_ONLY  == true){
        fields.flags |= BLE_HS_ADV_F_BREDR_UNSUP;
    }
    
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    char* name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;
    // TODO：UUID修改成可变的
    fields.uuids16 = (ble_uuid16_t[]) {
        BLE_UUID16_INIT(GATT_SVR_SVC_ALERT_UUID)
    };
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
        return -1 ;
    }

    // 声明并初始化广播结构体
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));

    // 获取地址类型
    uint8_t own_addr_type =  get_addr_type(addr_mode);
    
    // 连接模式
    uint8_t connet_mode;
    if(connectable == true){
        connet_mode = BLE_GAP_CONN_MODE_UND;
    }else {
        connet_mode = BLE_GAP_CONN_MODE_NON;
    }

    adv_params.conn_mode = connet_mode;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
  
    return ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_nimble_gap_event, NULL);
}

int _bluetooth_BLE_gap_connect(PikaObj *self, int addr_type, char* addr, int64_t scan_duration_ms)
{
    printf("_bluetooth_BLE_gap_connect\r\n");
    return ble_gap_connect((uint8_t)addr_type,(ble_addr_t *)addr,scan_duration_ms,NULL,NULL,NULL);
}

int _bluetooth_BLE_gap_disconnect(PikaObj *self)
{
    printf("_bluetooth_BLE_gap_disconnect\r\n");
    return ble_gap_conn_cancel();
}

int _bluetooth_BLE_gap_scan(PikaObj *self, int addr_mode, int duration_ms, int interval_us, int window_us, pika_bool active)
{
    printf("_bluetooth_BLE_gap_scan\r\n");
    // 获取地址类型
    uint8_t own_addr_type =  get_addr_type(addr_mode);

    // 声明并初始化结构体实例
    struct ble_gap_disc_params disc_params = {
        .itvl = duration_ms / 0.625,
        .window = interval_us / 625,
        .passive = ~active,
    };
    return ble_gap_disc(own_addr_type, duration_ms, &disc_params, ble_nimble_gap_event, NULL);
}

// 停止扫描
// 已完成
int _bluetooth_BLE_gap_stop_scan(PikaObj *self)
{
    printf("_bluetooth_BLE_gap_stop_scan\r\n");
    return ble_gap_disc_cancel();
}

// 停止广播
// 已完成
int _bluetooth_BLE_stop_advertise(PikaObj *self)
{
    printf("_bluetooth_BLE_stop_advertise");
    return ble_gap_adv_stop();
}

// TODO:服务的内容该如何传递
int _bluetooth_BLE_gatts_register_svcs(PikaObj *self, PikaObj* services_info)
{
    printf("_bluetooth_BLE_gatts_register_svcs\r\n");
    size_t service_count , chr_count, dsc_count;
    uint8_t i,j,k;
    service_count = pikaTuple_getSize(services_info);            //服务的个数,是不确定的
    printf("services_info service_count = %d\r\n",service_count);
    for (i = 0;i < service_count;i++){                           //对于每个服务
    Arg* aService = pikaTuple_getArg(services_info, i);
        PikaObj* oService =  arg_getObj(aService);    //读取服务
        printf("TYPE %d\r\n",pikaTuple_getType(services_info,i));

        service_count = pikaTuple_getSize(oService);            //服务的个数,是不确定的
        printf("services_info service_count = %d\r\n",service_count);

        // PikaObj* service_UUID = pikaTuple_getStr(service,0);
        // printf("TYPE %d",pikaTuple_getType(service_UUID));
        // Arg  * chrs = pikaTuple_getArg(service, 1);              //读取属性合集
        // chr_count = pikaTuple_getSize(chrs);                     // 属性的个数,是不确定的
        // printf("service %d UUID %s chrs size %d \r\n",i,service_UUID,chr_count);
    //     for (j = 0;j < chr_count;j++){                           // 对于每个属性
    //         Arg  * chr = pikaTuple_getArg(chrs, j);              //读取属性
    //         char * chr_UUID = pikaTuple_getStr(chrs,0);          //属性FLAG    
    //         uint64_t chr_flags = pikaTuple_getInt(chrs,1);
    //         Arg  * dscs = pikaTuple_getArg(chrs, 2);             // dscs = 描述符合集
    //         dsc_count = pikaTuple_getSize(dscs);                 //描述符的个数，是不确定的
    //         printf("chr_UUID : %s chr_flags : %d  dscs size %d \r\n",chr_UUID,chr_flags,dsc_count);
    //         for(k = 0;k < dsc_count;k++){                        //对于每个描述符
    //             Arg * dsc = pikaTuple_getArg(dscs, k);
    //             char * dscs_UUID = pikaTuple_getInt(dsc, 0);
    //             uint16_t dscs_flags = pikaTuple_getInt(dsc, 1);
    //             printf("dscs_UUID : %s, dscs_flags : %d",dscs_UUID,dscs_flags);
    //         }
    //     }
    }
    // gatt_svr_init();
    return 0;
}

int _bluetooth_BLE_set_adv_data(PikaObj *self, char* data, int data_len)
{
    printf("_bluetooth_BLE_set_adv_data\r\n");
    return ble_gap_adv_set_data((uint8_t*)data,data_len);
}

int _bluetooth_BLE_set_rsp_data(PikaObj *self, char* data, int data_len)
{
    printf("_bluetooth_BLE_set_rsp_data\r\n");
    return ble_gap_adv_rsp_set_data((uint8_t*)data,data_len);
}


// 回调函数注册
void _bluetooth_BLE_setCallback(PikaObj *self, Arg* cb)
{
    printf("_bluetooth_BLE_setCallback\r\n");
    if (g_pika_ble_listener == NULL) {
        pika_eventListener_init(&g_pika_ble_listener);
        printf("g_pika_ble_listener init\r\n");
    }
    uint32_t i = 0;
    for ( i = 0; i < 31; i++)
    {
        pika_eventListener_registEventCallback(g_pika_ble_listener,i,cb);
    }
}

int _bluetooth_BLE_config_mac_get(PikaObj *self)
{
    printf("_bluetooth_BLE_config_mac_get\r\n");
    // uint8_t addr[6];
    // ble_addr_t baddr;
    
    // /* 获取设备的MAC地址 */
    // printf("nimble_port_get_addr result: %d",nimble_port_get_addr(&baddr));
    
    // /* 将地址拷贝到 addr 数组中 */
    // memcpy(addr, baddr.val, sizeof(addr));
    
    // /* 打印MAC地址 */
    // printf("Device MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
    //        addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

    return 0;
}

char* _bluetooth_BLE_config_gap_name_get(PikaObj *self){
// int _bluetooth_BLE_config_gap_name_get(PikaObj *self)
    printf("_bluetooth_BLE_config_addr_gap_name_get\r\n");
    char *name = ble_svc_gap_device_name();
    // printf(name);
    // return 1;
    return name;
}

int _bluetooth_BLE_config_addr_mode_get(PikaObj *self){
    printf("_bluetooth_BLE_config_addr_mode_get\r\n");
    return 0;
}

int _bluetooth_BLE_config_mtu_get(PikaObj *self){
    printf("_bluetooth_BLE_config_mtu_get\r\n");
    return 0;
}

int _bluetooth_BLE_config_addr_rxbuf_get(PikaObj *self){
    printf("_bluetooth_BLE_config_addr_rxbuf_get\r\n");
    return 0;
}

int _bluetooth_BLE_config_bond_get(PikaObj *self){
    printf("_bluetooth_BLE_config_bond_get\r\n");
    return 0;
}

int _bluetooth_BLE_config_io_get(PikaObj *self)
{
    printf("_bluetooth_BLE_config_io_get\r\n");
    return 0;
}

int _bluetooth_BLE_config_le_secure_get(PikaObj *self)
{
    printf("_bluetooth_BLE_config_le_secure_get\r\n");
    return 0;
}

int _bluetooth_BLE_config_mitm_get(PikaObj *self)
{
    printf("_bluetooth_BLE_config_mitm_get\r\n");
    return 0;
}


int _bluetooth_BLE_config_addr_mode_update(PikaObj *self)
{
    printf("_bluetooth_BLE_config_addr_mode_update\r\n");
    return 0;
}

int _bluetooth_BLE_config_bond_update(PikaObj *self, pika_bool bond)
{
    printf("_bluetooth_BLE_config_bond_update\r\n");
    return 0;
}

int _bluetooth_BLE_config_gap_name_update(PikaObj *self, char* gap_name)
{
    printf("_bluetooth_BLE_config_gap_name_update\r\n");
    // struct ble_hs_adv_fields *adv_fields;
    // adv_fields = (struct ble_hs_adv_fields*)malloc(sizeof(struct ble_hs_adv_fields));
    // adv_fields->name = (unsigned char *)gap_name;
    // // adv_fields->name_is_complete
    // return ble_gap_adv_set_fields(adv_fields);
    return ble_svc_gap_device_name_set(gap_name);
}

int _bluetooth_BLE_config_io_update(PikaObj *self, int io){
    printf("_bluetooth_BLE_config_io_update\r\n");
    return 0;
}

int _bluetooth_BLE_config_le_secure_update(PikaObj *self, pika_bool le_secure)
{
    ESP_LOGD(tag, "_bluetooth_BLE_config_le_secure_update\r\n");
    // TODO:需要进行判断，若BLE处于广播状态则不能修改
    if(ble_gap_adv_active()){
        ESP_LOGI(tag, "an advertisement procedure is currently in progress\r\n");
        return -1;
    }
    else{
        BLE_ONLY = le_secure;
        ESP_LOGI(tag, "secure update succeed\r\n");
        return 0;
    }
}

int _bluetooth_BLE_config_mac_update(PikaObj *self)
{
    printf("_bluetooth_BLE_config_mac_update\r\n");
    return 0;
}

int _bluetooth_BLE_config_mitm_update(PikaObj *self, pika_bool mitm)
{
    printf("_bluetooth_BLE_config_mitm_update\r\n");
    return 0;
}

int _bluetooth_BLE_config_mtu_update(PikaObj *self, int mtu)
{
    printf("_bluetooth_BLE_config_mtu_update\r\n");
    return 0;
}

int _bluetooth_BLE_config_rxbuf_update(PikaObj *self, int rxbuf)
{
    printf("_bluetooth_BLE_config_rxbuf_update\r\n");
    return 0;
}

int _bluetooth_BLE_test2(PikaObj *self)
{
    printf("_bluetooth_BLE_test2\r\n");
    return 0;
}

int _bluetooth_BLE_gattc_dis_chrs(PikaObj *self, int conn_handle, int start_handle, int end_handle){
    return 0;
}

int _bluetooth_BLE_gattc_dis_chrs_by_uuid(PikaObj *self, int conn_handle, int start_handle, char* uuid){
    return 0;
}

int _bluetooth_BLE_gattc_dis_dscs(PikaObj *self, int conn_handle, int start_handle, int end_handle){
    return 0;
}

int _bluetooth_BLE_gattc_dis_svcs(PikaObj *self, int conn_handle){
    return 0;
}

int _bluetooth_BLE_gattc_dis_svcs_by_uuid(PikaObj *self, int conn_handle, char* uuid){
    return 0;
}

int _bluetooth_BLE_gattc_write_with_no_rsp(PikaObj *self, int conn_handle, int value_handle, char* data){
    return 0;
}

int _bluetooth_BLE_gattc_write_with_rsp(PikaObj *self, int conn_handle, int value_handle, char* data){
    return 0;
}

int _bluetooth_BLE_gatts_indicate_custom(PikaObj *self, int conn_handle, int value_handle, char* data){
    return 0;
}

int _bluetooth_BLE_gatts_indicate_no_data(PikaObj *self, int conn_handle, int value_handle){
    return 0;
}

int _bluetooth_BLE_gatts_notify_custom(PikaObj *self, int conn_handle, int value_handle, char* data){
    return 0;
}

int _bluetooth_BLE_gatts_notify_no_data(PikaObj *self, int conn_handle, int value_handle){
    return 0;
}

int _bluetooth_BLE_pyi_gattc_exchange_mtu(PikaObj *self, int conn_handle){
    return 0;
}

int _bluetooth_BLE_pyi_gattc_read(PikaObj *self, int conn_handle, int value_handle){
    return 0;
}


void gatt_svr_init(void)
{
    // TODO:另外两个初始化函数怎么是无定义的
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_ans_init();
}


// gattd服务回调函数
// static int gatt_svc_access(uint16_t conn_handle, uint16_t attr_handle,
//                 struct ble_gatt_access_ctxt *ctxt, void *arg){
//     const ble_uuid_t *uuid;
//     int rc;
//     switch (ctxt->op) {
//         case BLE_GATT_ACCESS_OP_READ_CHR:
//             if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
//                 MODLOG_DFLT(INFO, "Characteristic read; conn_handle=%d attr_handle=%d\n",
//                             conn_handle, attr_handle);
//             } else {
//                 MODLOG_DFLT(INFO, "Characteristic read by NimBLE stack; attr_handle=%d\n",
//                             attr_handle);
//             }
//             uuid = ctxt->chr->uuid;
//             if (attr_handle == gatt_svr_chr_val_handle) {
//                 rc = os_mbuf_append(ctxt->om,
//                                     &gatt_svr_chr_val,
//                                     sizeof(gatt_svr_chr_val));
//                 return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
//             }
//             goto unknown;

//         case BLE_GATT_ACCESS_OP_WRITE_CHR:
//             if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
//                 MODLOG_DFLT(INFO, "Characteristic write; conn_handle=%d attr_handle=%d",
//                             conn_handle, attr_handle);
//             } else {
//                 MODLOG_DFLT(INFO, "Characteristic write by NimBLE stack; attr_handle=%d",
//                             attr_handle);
//             }
//             uuid = ctxt->chr->uuid;
//             if (attr_handle == gatt_svr_chr_val_handle) {
//                 rc = gatt_svr_write(ctxt->om,
//                                     sizeof(gatt_svr_chr_val),
//                                     sizeof(gatt_svr_chr_val),
//                                     &gatt_svr_chr_val, NULL);
//                 ble_gatts_chr_updated(attr_handle);
//                 MODLOG_DFLT(INFO, "Notification/Indication scheduled for "
//                             "all subscribed peers.\n");
//                 return rc;
//             }
//             goto unknown;

//         case BLE_GATT_ACCESS_OP_READ_DSC:
//             if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
//                 MODLOG_DFLT(INFO, "Descriptor read; conn_handle=%d attr_handle=%d\n",
//                             conn_handle, attr_handle);
//             } else {
//                 MODLOG_DFLT(INFO, "Descriptor read by NimBLE stack; attr_handle=%d\n",
//                             attr_handle);
//             }
//             uuid = ctxt->dsc->uuid;
//             if (ble_uuid_cmp(uuid, &gatt_svr_dsc_uuid.u) == 0) {
//                 rc = os_mbuf_append(ctxt->om,
//                                     &gatt_svr_dsc_val,
//                                     sizeof(gatt_svr_chr_val));
//                 return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
//             }
//             goto unknown;

//         case BLE_GATT_ACCESS_OP_WRITE_DSC:
//             goto unknown;

//         default:
//             goto unknown;
//     }

// unknown:
//     /* Unknown characteristic/descriptor;
//      * The NimBLE host should not have called this function;
//      */
//     assert(0);
//     return BLE_ATT_ERR_UNLIKELY;
// }



// 客户端读服务回调函数
static int ble_cliect_read_cb(uint16_t conn_handle,
                       const struct ble_gatt_error *error,
                       struct ble_gatt_attr *attr,
                       void *arg)
{
    printf("Read complete for the subscribable characteristic; "
                "status=%d conn_handle=%d", error->status, conn_handle);


    //读取成功
    pika_eventListener_send(g_pika_ble_listener,_IRQ_GATTC_READ_DONE ,
        arg_newObj(New_pikaTupleFrom(
                arg_newInt(_IRQ_GATTC_READ_DONE),
                arg_newInt(conn_handle),
                arg_newInt(attr->handle),
                arg_newInt(error->status) 
                )));

    if (error->status == 0) {
        printf(" attr_handle=%d value=", attr->handle);
        //读到数据
        pika_eventListener_send(g_pika_ble_listener,_IRQ_GATTC_READ_RESULT,
            arg_newObj(New_pikaTupleFrom(
                    arg_newInt(_IRQ_GATTC_READ_RESULT),
                    arg_newInt(conn_handle),
                    arg_newInt(attr->handle),
                    arg_newStr("test string") //, 
                    // arg_newBytes(attr->om->om_databuf,attr->om->om_len) //TODO:未验证
                    )));
        // print_mbuf(attr->om); //TODO:该函数无引用，但在blecentn能够使用
    }
    return 0;
}

// 客户端写服务回调函数
static int ble_cliect_write_cb(uint16_t conn_handle,
                        const struct ble_gatt_error *error,
                        struct ble_gatt_attr *attr,
                        void *arg)
{
    const struct peer_chr *chr;
    const struct peer *peer;
    int rc;

    MODLOG_DFLT(INFO,
                "Write to the custom subscribable characteristic complete; "
                "status=%d conn_handle=%d attr_handle=%d\n",
                error->status, conn_handle, attr->handle);
        //读到数据
        pika_eventListener_send(g_pika_ble_listener,_IRQ_GATTC_READ_RESULT,
            arg_newObj(New_pikaTupleFrom(
                    arg_newInt(_IRQ_GATTC_WRITE_DONE ),
                    arg_newInt(conn_handle),
                    arg_newInt(attr->handle),
                    arg_newInt(error->status) 
                    )));
    return 0;
}



static int ble_nimble_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;
    uint8_t * addr[6];

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT: //TODO:MicroPyhon 区分 服务端与客户端的连接
        /* A new connection was established or a connection attempt failed. */
        MODLOG_DFLT(INFO, "connection %s; status=%d ",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);
        if (event->connect.status == 0) {
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            print_conn_desc(&desc);
            addr_inver(desc.peer_ota_addr.val,&addr);
            pika_eventListener_send(g_pika_ble_listener,_IRQ_CENTRAL_CONNECT,
                        arg_newObj(New_pikaTupleFrom(
                                arg_newInt(_IRQ_CENTRAL_CONNECT),
                                arg_newInt(event->connect.conn_handle),
                                arg_newInt(desc.peer_id_addr.type),
                                // arg_newStr(addr_str) //TODO:修改为arg_newBytes(desc.peer_ota_addr.val,6)
                                arg_newBytes(addr,6)
                                )));
        }
        MODLOG_DFLT(INFO, "\n");

        if (event->connect.status != 0) {
            /* Connection failed; resume advertising. */
            // bleprph_advertise();
            // TODO: 重新广播
            printf("Connection failed; resume advertising.");
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT: //断开连接
        printf("disconnect; reason=%d ", event->disconnect.reason);
        print_conn_desc(&event->disconnect.conn);

        addr_inver(event->disconnect.conn.peer_ota_addr.val,&addr);
        pika_eventListener_send(g_pika_ble_listener,_IRQ_CENTRAL_DISCONNECT,
                            arg_newObj(New_pikaTupleFrom(
                                    arg_newInt(_IRQ_CENTRAL_DISCONNECT),
                                    arg_newInt(event->disconnect.conn.conn_handle),
                                    arg_newInt(desc.peer_id_addr.type),
                                    arg_newBytes(addr,6)
                                    )));
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE: //返回结果
        /* The central has updated the connection parameters. */
        printf("connection updated; status=%d ",event->conn_update.status);
        rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        assert(rc == 0);
        print_conn_desc(&desc);
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE_REQ :
        // MicroPython : conn_handle, conn_interval, conn_latency, supervision_timeout, status 
        pika_eventListener_send(g_pika_ble_listener,_IRQ_CONNECTION_UPDATE,
                    arg_newObj(New_pikaTupleFrom(
                            arg_newInt(_IRQ_CONNECTION_UPDATE),
                            arg_newInt(event->conn_update_req.conn_handle),
                            arg_newInt(event->conn_update_req.peer_params->itvl_min),
                            arg_newInt(event->conn_update_req.peer_params->latency),
                            arg_newInt(event->conn_update_req.peer_params->supervision_timeout)//,
                            // arg_newInt(event->conn_update.status) TODO:status在上一事件中
                            )));
         return 0;

    case BLE_GAP_EVENT_L2CAP_UPDATE_REQ :
        return 0;

    case BLE_GAP_EVENT_TERM_FAILURE:
        return 0;
    
    case BLE_GAP_EVENT_DISC: //扫描发现
    // MicroPython addr_type, addr, adv_type, rssi, adv_data
        struct ble_gap_conn_desc desc;
        struct ble_hs_adv_fields fields;
        int rc;
        rc = ble_hs_adv_parse_fields(&fields, event->disc.data,
                                     event->disc.length_data);
        if (rc != 0) {
            return 0;
        }

        /* An advertisment report was received during GAP discovery. */
        // print_adv_fields(&fields);

        /* Try to connect to the advertiser if it looks interesting. */
        // blecent_connect_if_interesting(&event->disc);

        addr_inver(event->disc.addr.val,&addr);
        uint8_t len = event->disc.length_data;
        char *adv_str = (char *)malloc(len + 1);
        memcpy(adv_str, event->disc.data, len);
        adv_str[len] = '\0';

        pika_eventListener_send(g_pika_ble_listener,_IRQ_SCAN_RESULT,
            arg_newObj(New_pikaTupleFrom(
                    arg_newInt(_IRQ_SCAN_RESULT),
                    arg_newInt(event->disc.addr.type),
                    arg_newBytes(addr,6),
                    arg_newInt(event->disc.event_type),
                    arg_newInt(event->disc.event_type),
                    arg_newInt(event->disc.rssi),
                    arg_newStr(adv_str)
                    )));
        free(adv_str);
        return 0;

    case BLE_GAP_EVENT_DISC_COMPLETE: // 扫描结束
    // MicroPython None
        printf("discovery complete; reason=%d\n",event->disc_complete.reason);
        pika_eventListener_send(g_pika_ble_listener,_IRQ_SCAN_DONE,
            arg_newObj(New_pikaTupleFrom(
                    arg_newInt(_IRQ_SCAN_DONE),
                    arg_newInt(event->disc_complete.reason)
                    )));
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE: //广播完成
    // MicroPython 没有这个事件
        printf("advertise complete; reason=%d",event->adv_complete.reason);
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
    // 暂时不理
        /* Encryption has been enabled or disabled for this connection. */
        MODLOG_DFLT(INFO, "encryption change event; status=%d ",
                    event->enc_change.status);
        rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        assert(rc == 0);
        print_conn_desc(&desc);
        MODLOG_DFLT(INFO, "\n");
        return 0;

    case BLE_GAP_EVENT_PASSKEY_ACTION :
    // 暂时不理
            ESP_LOGI(tag, "PASSKEY_ACTION_EVENT started \n");
        struct ble_sm_io pkey = {0};
        int key = 0;

        if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
            pkey.action = event->passkey.params.action;
            pkey.passkey = 123456; // This is the passkey to be entered on peer
            ESP_LOGI(tag, "Enter passkey %" PRIu32 "on the peer side", pkey.passkey);
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(tag, "ble_sm_inject_io result: %d\n", rc);
        } else if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
            ESP_LOGI(tag, "Passkey on device's display: %" PRIu32 , event->passkey.params.numcmp);
            ESP_LOGI(tag, "Accept or reject the passkey through console in this format -> key Y or key N");
            pkey.action = event->passkey.params.action;
            // if (scli_receive_key(&key)) {
            //     pkey.numcmp_accept = key;
            // } else {
            //     pkey.numcmp_accept = 0;
            //     ESP_LOGE(tag, "Timeout! Rejecting the key");
            // }
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(tag, "ble_sm_inject_io result: %d\n", rc);
        } else if (event->passkey.params.action == BLE_SM_IOACT_OOB) {
            static uint8_t tem_oob[16] = {0};
            pkey.action = event->passkey.params.action;
            for (int i = 0; i < 16; i++) {
                pkey.oob[i] = tem_oob[i];
            }
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(tag, "ble_sm_inject_io result: %d\n", rc);
        } else if (event->passkey.params.action == BLE_SM_IOACT_INPUT) {
            ESP_LOGI(tag, "Enter the passkey through console in this format-> key 123456");
            pkey.action = event->passkey.params.action;
            // if (scli_receive_key(&key)) {
            //     pkey.passkey = key;
            // } else {
            //     pkey.passkey = 0;
            //     ESP_LOGE(tag, "Timeout! Passing 0 as the key");
            // }
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(tag, "ble_sm_inject_io result: %d\n", rc);
        }
        return 0;

    case BLE_GAP_EVENT_NOTIFY_RX: // 客户端

        printf("received %s; conn_handle=%d attr_handle=%d attr_len=%d\r\n",
                event->notify_rx.indication ? "indication" : "notification",
                event->notify_rx.conn_handle,
                event->notify_rx.attr_handle,
                OS_MBUF_PKTLEN(event->notify_rx.om));

        if(event->notify_rx.indication == 1){ // indication
            // MicroPython : conn_handle, value_handle, notify_data
            uint16_t len = event->notify_rx.om->om_len;
            char *indic_str = (char *)malloc(len + 1);
            memcpy(indic_str, event->notify_rx.om->om_data, len);
            indic_str[len] = '\0';

            pika_eventListener_send(g_pika_ble_listener,_IRQ_GATTC_INDICATE,
                arg_newObj(New_pikaTupleFrom(
                        arg_newInt(_IRQ_GATTC_INDICATE),
                        arg_newInt(event->notify_rx.conn_handle),
                        arg_newInt(event->notify_rx.attr_handle),
                        arg_newStr(indic_str)
                        )));
            free(indic_str);
        }
        else { 
            uint16_t len = event->notify_rx.om->om_len;
            char *indic_str = (char *)malloc(len + 1);
            memcpy(indic_str, event->notify_rx.om->om_data, len);
            indic_str[len] = '\0';

            pika_eventListener_send(g_pika_ble_listener,_IRQ_GATTC_NOTIFY,
                arg_newObj(New_pikaTupleFrom(
                        arg_newInt(_IRQ_GATTC_NOTIFY),
                        arg_newInt(event->notify_rx.conn_handle),
                        arg_newInt(event->notify_rx.attr_handle),
                        arg_newStr(indic_str)
                        )));
            free(indic_str);
        }
        
        return 0;

    case BLE_GAP_EVENT_NOTIFY_TX: //通知发送完成
        printf("notify_tx event; conn_handle=%d attr_handle=%d status=%d is_indication=%d\r\n",
                    event->notify_tx.conn_handle,
                    event->notify_tx.attr_handle,
                    event->notify_tx.status,
                    event->notify_tx.indication);
        if(event->notify_tx.indication == 1){ // indication
            // MicroPython : conn_handle, value_handle, notify_data
            pika_eventListener_send(g_pika_ble_listener,_IRQ_GATTS_INDICATE_DONE,
                arg_newObj(New_pikaTupleFrom(
                        arg_newInt(_IRQ_GATTS_INDICATE_DONE),
                        arg_newInt(event->notify_tx.conn_handle),
                        arg_newInt(event->notify_tx.attr_handle),
                        arg_newStr(event->notify_tx.status)
                        )));
        }        
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE://订阅 客户端向服务端订阅
        MODLOG_DFLT(INFO, "subscribe event; conn_handle=%d attr_handle=%d "
                    "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
                    event->subscribe.conn_handle,
                    event->subscribe.attr_handle,
                    event->subscribe.reason,
                    event->subscribe.prev_notify,
                    event->subscribe.cur_notify,
                    event->subscribe.prev_indicate,
                    event->subscribe.cur_indicate);
                    // ble_gattc_disc_all_svcs
        return 0;

    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_IDENTITY_RESOLVED:
        return 0;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* We already have a bond with the peer, but it is attempting to
         * establish a new secure link.  This app sacrifices security for
         * convenience: just throw away the old bond and accept the new link.
         */

        /* Delete the old bond. */
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);

        /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
         * continue with the pairing operation.
         */
        return BLE_GAP_REPEAT_PAIRING_RETRY;

        case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE:
            return 0;

        case BLE_GAP_EVENT_EXT_DISC:
            return 0;

        case BLE_GAP_EVENT_PERIODIC_SYNC:
            return 0;

        case BLE_GAP_EVENT_PERIODIC_REPORT:
            return 0;

        case BLE_GAP_EVENT_PERIODIC_SYNC_LOST:
            return 0;

        case BLE_GAP_EVENT_SCAN_REQ_RCVD:
            return 0;

        case BLE_GAP_EVENT_PERIODIC_TRANSFER:
            return 0;

        case BLE_GAP_EVENT_PATHLOSS_THRESHOLD:
            return 0;

        case BLE_GAP_EVENT_TRANSMIT_POWER:
            return 0;

        case BLE_GAP_EVENT_SUBRATE_CHANGE:// 这个是啥事件？
            return 0;
    }
    return 0;
}