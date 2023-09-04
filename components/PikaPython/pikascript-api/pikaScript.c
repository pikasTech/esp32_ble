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

#include "PikaMain.h"
#include <stdio.h>
#include <stdlib.h>

volatile PikaObj *__pikaMain;
PikaObj *pikaPythonInit(void){
    pika_platform_printf("======[pikapython packages installed]======\r\n");
    pika_printVersion();
    pika_platform_printf("ESP32==v0.4.2\r\n");
    pika_platform_printf("PikaStdDevice==v2.4.0\r\n");
    pika_platform_printf("PikaStdLib==v1.12.5\r\n");
    pika_platform_printf("network==v0.1.3\r\n");
    pika_platform_printf("pika_libc==v1.0.2\r\n");
    pika_platform_printf("time==v0.1.9\r\n");
    pika_platform_printf("===========================================\r\n");
    PikaObj* pikaMain = newRootObj("pikaMain", New_PikaMain);
    __pikaMain = pikaMain;
    extern unsigned char pikaModules_py_a[];
    obj_linkLibrary(pikaMain, pikaModules_py_a);
#if PIKA_INIT_STRING_ENABLE
    obj_run(pikaMain,
            "import PikaStdLib\n"
            "import machine \n"
            "import bluetooth\n"
            "import _bluetooth\n"
            "import const\n"
            "# import mytest\n"
            "mem = PikaStdLib.MemChecker()\n"
            "print('mem used max:')\n"
            "mem.max()\n"
            "print('mem used now:')\n"
            "mem.now()\n"
            "print('hello PikaPython')\n"
            "a = bluetooth.BLE()\n"
            "b = a.active(1)\n"
            "a.advertise(0,1,1)\n"
            "def ble_irq(event,data):\n"
            "    # event = const._IRQ_CENTRAL_CONNECT\n"
            "    if event == const._IRQ_CENTRAL_CONNECT:\n"
            "        # A central has connected to this peripheral.\n"
            "        print(\"_IRQ_CENTRAL_CONNECT\")\n"
            "        print(data)\n"
            "    elif event == const._IRQ_GATTC_SERVICE_DONE:\n"
            "        print(\"_IRQ_GATTC_SERVICE_DONE\")\n"
            "        print(data)\n"
            "    \n"
            "a.irq(ble_irq)\n"
            "# a.test(1)\n"
            "# a.pyi_active(1)\n"
            "# b = a.active(1)\n"
            "# a.advertise(0,1,1)\n"
            "# a.gap_advertise(20,\"adv_data_test\")\n"
            "# c = a.run()\n"
            "# print(a.gap_name)\n"
            "# a.pyi_active(1)\n"
            "# a.config(\"mac\")\n"
            "# a.active(0)\n"
            "# a.__test2()\n"
            "# a.pyi_active()\n"
            "\"\"\"\n"
            "问题汇总\n"
            "1. super().func 报错\n"
            "2. py文件调用.pyi文件会打印返回值 ok\n"
            "3. pyi文件需要返回字符串、二进制码流如何处理\n"
            "4. config函数有些参数无法读取\n"
            "5. 进行BLE失能时,该如何处理\n"
            "问题汇总\n"
            "1. nvs_flash_init()这种调用一次的函数应该在哪里调用\n"
            "2. 启动蓝牙线程任务在哪里调用(已解决)\n"
            "3. print_addr函数的实现在哪里, 需要怎么样引用\n"
            "4. 引用头文件失败\n"
            "5. py文件 elif没通过\n"
            "6. ESP 的日志没打印出来？\n"
            "\"\"\"\n"
            "\n");
#else 
    obj_runModule((PikaObj*)pikaMain, "main");
#endif
    return pikaMain;
}

