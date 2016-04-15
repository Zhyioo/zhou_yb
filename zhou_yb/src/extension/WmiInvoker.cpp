﻿//========================================================= 
/**@file WmiInvoker.cpp 
 * @brief 
 * 
 * @date 2014-01-01   13:33:30 
 * @author Zhyioo 
 * @version 1.0
 */ 
//--------------------------------------------------------- 
#ifndef _LIBZHOUYB_WMIINVOKER_CPP_
#define _LIBZHOUYB_WMIINVOKER_CPP_
//--------------------------------------------------------- 
#include "../../extension/ability/win_helper/WmiInvoker.h"
//--------------------------------------------------------- 
namespace zhou_yb {
namespace extension {
namespace ability {
//--------------------------------------------------------- 
/// 空的属性 
WmiProperty WmiProperty::NullWmiProperty;
/// 对象引用计数  
size_t WmiInvoker::objCount = 0;
/// 是否成功初始化相关组件 
bool WmiInvoker::hasInitCOM = false;
/// WMI服务对象 
IWbemLocator* WmiInvoker::pWmiLocator = NULL;

/* 命名空间名 */
char WmiHelper::WMI_SPACE_NAME[][32] = 
{
    // DSPROV
    "ROOT\\DIRECTORY\\LDAP",
    // NTEVT
    "ROOT\\CIMV2",
    // STDPROV
    "ROOT\\DEFAULT"
    // CIMWIN32
    "ROOT\\CIMV2",
    // MSIPROV
    "ROOT\\CIMV2",
    ""
};
/* 服务属性连接名 */
char WmiHelper::WMI_SERVICE_NAME[][64] = 
{
    "Win32_Processor", 
    "Win32_PhysicalMemory",
    "Win32_Keyboard",
    "Win32_PointingDevice",
    "Win32_FloppyDrive",
    "Win32_DiskDrive",
    "Win32_CDROMDrive",
    "Win32_BaseBoard",
    "Win32_BIOS",
    "Win32_ParallelPort",
    "Win32_SerialPort",
    "Win32_SerialPortConfiguration",
    "Win32_SoundDevice",
    "Win32_SystemSlot",
    "Win32_USBController",
    "Win32_NetworkAdapter",
    "Win32_NetworkAdapterConfiguration",
    "Win32_Printer",
    "Win32_PrinterConfiguration",
    "Win32_PrintJob",
    "Win32_TCPIPPrinterPort",
    "Win32_POTSModem",
    "Win32_POTSModemToSerialPort",
    "Win32_DesktopMonitor",
    "Win32_DisplayConfiguration",
    "Win32_DisplayControllerConfiguration",
    "Win32_VideoController",
    "Win32_VideoSettings",
    "Win32_TimeZone",
    "Win32_SystemDriver",
    "Win32_DiskPartition",
    "Win32_LogicalDisk",
    "Win32_LogicalDiskToPartition",
    "Win32_LogicalMemoryConfiguration",
    "Win32_PageFile",
    "Win32_PageFileSetting",
    "Win32_BootConfiguration",
    "Win32_ComputerSystem",
    "Win32_OperatingSystem",
    "Win32_StartupCommand",
    "Win32_Service",
    "Win32_Group",
    "Win32_GroupUser",
    "Win32_UserAccount",
    "Win32_Process",
    "Win32_Thread",
    "Win32_Share",
    "Win32_NetworkClient",
    "Win32_NetworkProtocol",
    ""
};
//--------------------------------------------------------- 
} // namespace ability
} // namespace extension 
} // namespace zhou_yb
//--------------------------------------------------------- 
#endif // _LIBZHOUYB_WMIINVOKER_CPP_
//========================================================= 