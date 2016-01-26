﻿//========================================================= 
/**@file HID_IC_DevUpdater.h 
 * @brief HID IC卡固件升级程序 
 * 
 * @date 2015-01-03   11:13:01 
 * @author Zhyioo 
 * @version 1.0
 */ 
#pragma once 
//--------------------------------------------------------- 
#include "../DevUpdater.h"
//--------------------------------------------------------- 
namespace zhou_yb {
namespace application {
namespace updater {
//--------------------------------------------------------- 
/// HID读卡器切换升级模式连接器(负责发送切换指令)
struct HidUpdateModeTestLinker : public TestLinker<HidDevice>
{
    /// 检测是否有待升级状态的设备存在,没有的话发送指令进行切换  
    virtual bool Link(HidDevice& dev, const char* devArg, TextPrinter& printer)
    {
        // 检查设备是否为升级模式 
        string reader = "";
        string upgrade = "";

        ArgParser cfg;
        if(cfg.Parse(devArg))
        {
            cfg.GetValue("Updater", upgrade, true);
            cfg.GetValue("Reader", reader, true);
        }
        else
        {
            upgrade = _strput(devArg);
        }

        list<HidDevice::device_info> devlist;
        list<HidDevice::device_info>::iterator itr;
        dev.EnumDevice(devlist);

        for(itr = devlist.begin();itr != devlist.end(); ++itr)
        {
            if(StringConvert::Contains(ByteArray(itr->Name.c_str(), itr->Name.length()),
                ByteArray(upgrade.c_str(), upgrade.length()), true))
            {
                return true;
            }
        }

        if(HidDeviceHelper::OpenDevice<HidDevice>(dev, reader.c_str(), SIZE_EOF, &devlist) != DevHelper::EnumSUCCESS)
            return false;

        ComICCardCmdAdapter cmdAdapter;
        ByteBuilder updateRecv(8);

        cmdAdapter.SelectDevice(dev);
        cmdAdapter.Write(DevCommand::FromAscii("31 15"));

        return dev.Read(updateRecv);
    }
};
/// HID读卡器升级连接器
struct HidUpdaterTestLinker : public TestLinker<HidDevice>
{
    virtual bool Link(HidDevice& dev, const char* devArg, TextPrinter& printer)
    {
        string reader = "";
        ArgParser cfg;

        if(cfg.Parse(devArg))
        {
            cfg.GetValue("Updater", reader, true);
        }
        else
        {
            reader = _strput(devArg);
        }

        bool bLink = (HidDeviceHelper::OpenDevice(dev, reader.c_str(), SIZE_EOF) == DevHelper::EnumSUCCESS);
        return bLink;
    }
    virtual bool UnLink(HidDevice& dev, TextPrinter&)
    {
        dev.Close();
        return true;
    }
};
//--------------------------------------------------------- 
/// HID读卡器文件行升级器 
class HidUpdaterTestCase : public ITestCase< HidDevice >
{
protected:
    /// 发送的升级数据
    ByteBuilder _updateBin;
public:
    /// 升级文件行 
    virtual bool Test(Ref<HidDevice>& testObj, const ByteArray& testArg, TextPrinter&)
    {
        /* 将多个bin数据直接拼成HID设备的整包 */
        size_t len = testArg.GetLength() + _updateBin.GetLength();
        if(len < testObj->GetSendLength())
        {
            _updateBin += testArg;

            // 不是文件最后一行,需要继续补包
            if(testArg[0] != 0x00)
                return true;
        }

        /* 开始升级数据 */
        if(testObj->Write(_updateBin))
        {
            _updateBin.Clear();
            ByteBuilder tmp(8);
            if(!testObj->Read(tmp))
                return false;

            switch(tmp[0])
            {
            case 0x00:
                break;
            case 0x90:
                // 把当前行添加到升级数据缓冲区中
                _updateBin += testArg;
                return true;
            case 0x63:
            default:
                return false;
            }
        }

        return false;
    }
};
//--------------------------------------------------------- 
} // namespace updater
} // namespace application
} // namespace zhou_yb
//========================================================= 