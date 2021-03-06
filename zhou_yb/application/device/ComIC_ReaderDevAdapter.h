﻿//========================================================= 
/**@file ComIC_ReaderDevAdapter.h 
 * @brief 串口IC卡读写器其他指令适配器 
 * 
 * @date 2012-10-17   16:37:25 
 * @author Zhyioo 
 * @version 1.0
 */ 
//--------------------------------------------------------- 
#ifndef _LIBZHOUYB_COMIC_READERDEVADAPTER_H_
#define _LIBZHOUYB_COMIC_READERDEVADAPTER_H_
//--------------------------------------------------------- 
#include "../../include/Base.h"

#include "../../device/cmd_adapter/StreamCmdAdapter.h"
using zhou_yb::device::cmd_adapter::StreamCmdAdapter;

#include "../../device/cmd_adapter/ComICCardCmdAdapter.h"
using zhou_yb::device::cmd_adapter::ComICCardCmdAdapter;
//---------------------------------------------------------
namespace zhou_yb {
namespace application {
namespace device {
//---------------------------------------------------------
/**
 * @brief 串口接触式IC卡交互适配器 
 * 
 * 适配串口发送的指令
 */
class ComIC_ReaderDevAdapter : public DevAdapterBehavior<IInteractiveTrans>
{
protected:
    //-----------------------------------------------------
    /// 卡序号 00H 或 01H
    byte _cardNum;
    /// 收发缓冲区(减小空间分配的性能开销)
    ByteBuilder _tmpBuffer;
    //-----------------------------------------------------
    /// 初始化数据成员 
    inline void _init()
    {
        _cardNum = 0;
    }
    //----------------------------------------------------- 
public:
    //-----------------------------------------------------
    ComIC_ReaderDevAdapter() : DevAdapterBehavior<IInteractiveTrans>() { _init(); }
    //-----------------------------------------------------
    /**
     * @brief 交互指令,并返回原始的状态码
     * 
     * @param [in] baseDev 底层通信的设备
     * @param [in] sCmd 要交互的指令
     * @param [out] rCmd 收到的数据
     * @param [out] pStatusCode 获取到的状态码
     */
    static bool Transmit(Ref<IInteractiveTrans> baseDev, const ByteArray& sCmd, ByteBuilder& rCmd, ushort* pStatusCode)
    {
        if(baseDev.IsNull())
            return false;

        ComICCardCmdAdapter cmdAdapter;
        cmdAdapter.SelectDevice(baseDev);
        StreamCmdAdapter tmpAdapter;
        if(!cmdAdapter.Write(sCmd) || !baseDev->Read(rCmd))
            return false;
        tmpAdapter.InputStream = rCmd;
        cmdAdapter.SelectDevice(tmpAdapter);

        cmdAdapter.Read(rCmd);
        cmdAdapter.SelectDevice(baseDev);

        if(pStatusCode != NULL)
        {
            (*pStatusCode) = cmdAdapter.GetStatusCode();
        }
        return true;
    }
    /**
     * @brief 返回指定的接触式卡槽是否有卡片
     * 
     * @warning 由于需要获取状态码,所以操作的设备需要是最底层的设备,这样能够支持获取错误码 
     * 
     * @param [in] baseDev 底层通信的设备 
     * @param [in] slot [default:0x00] 卡槽号(默认为接触式卡槽)
     * @param [out] hasCard [default:NULL] 返回是否检测到有卡(为NULL表示不需要该状态)
     * @param [out] hasPoweron [default:NULL] 返回卡片是否已经上电(为NULL表示不需要该状态)
     */
    static bool HasContactCard(Ref<IInteractiveTrans> baseDev, byte slot = 0x00, bool* hasCard = NULL, bool* hasPoweron = NULL)
    {
        ByteBuilder sCmd(8);
        DevCommand::FromAscii("32 21", sCmd);
        sCmd += slot;

        ushort statusCode = 0x00;
        ByteBuilder rCmd(2);
        if(!Transmit(baseDev, sCmd, rCmd, &statusCode))
            return false;

        bool isPowered = false;
        bool isHasCard = false;

        int errCode = ComICCardCmdAdapter::GetErrorCode(statusCode);
        switch(errCode)
        {
            // 有卡已上电 
        case ComICCardCmdAdapter::ContactlessCardAlreadyPowerOnErr:
        case ComICCardCmdAdapter::ContactCardAlreadyPowerOnErr:
            isHasCard = true;
            isPowered = true;
            break;
            // 有卡未上电 
        case ComICCardCmdAdapter::ContactlessCardNotActiveErr:
        case ComICCardCmdAdapter::ContactCardNoPowerOnErr:
            isHasCard = true;
            isPowered = false;
            break;
            // 只知道有卡,不知道卡状态 
        case DeviceError::Success:
            isHasCard = true;
            break;
            // 有多张卡
        case ComICCardCmdAdapter::ContactlessCardCountErr:
        default:
            break;
        }

        if(NULL != hasPoweron)
        {
            *hasPoweron = isPowered;
        }
        if(NULL != hasCard)
        {
            *hasCard = isHasCard;
        }

        return true;
    }
    /**
     * @brief 返回非接触式卡槽上是否有卡
     *
     * @warning 由于需要获取状态码,所以操作的设备需要是最底层的设备,这样能够支持获取错误码 
     * 
     * @param [in] baseDev 底层通信的设备
     * @param [out] hasCard [default:NULL] 返回是否检测到有卡(为NULL表示不需要该状态)
     * @param [out] hasPoweron [default:NULL] 返回卡片是否已经上电(为NULL表示不需要该状态)
     * @param [out] isMultiCount [default:NULL] 返回是否有多张非接卡(为NULL表示不需要该状态)
     */
    static bool HasContactlessCard(Ref<IInteractiveTrans> baseDev, bool* hasCard = NULL, bool* hasPoweron = NULL, bool* hasMultiCard = NULL)
    {
        if(baseDev.IsNull())
            return false;

        ByteBuilder sCmd(8);
        DevCommand::FromAscii("32 24 00 00", sCmd);

        ushort statusCode = 0x00;
        ByteBuilder rCmd(2);
        if(!Transmit(baseDev, sCmd, rCmd, &statusCode))
            return false;

        bool isHasCard = false;
        bool isPowered = false;
        bool isMultiCard = false;

        int errCode = ComICCardCmdAdapter::GetErrorCode(statusCode);
        switch(errCode)
        {
            // 有卡已上电 
        case ComICCardCmdAdapter::ContactlessCardAlreadyPowerOnErr:
        case DeviceError::Success:
            isHasCard = true;
            isPowered = true;
            break;
            // 有卡未上电 
        case ComICCardCmdAdapter::ContactlessCardNotActiveErr:
            isHasCard = true;
            break;
            // 有多张卡
        case ComICCardCmdAdapter::ContactlessCardCountErr:
            isHasCard = true;
            isMultiCard = true;
            break;
        default:
            break;
        }

        if(NULL != hasCard)
        {
            *hasCard = isHasCard;
        }
        if(NULL != hasPoweron)
        {
            *hasPoweron = isPowered;
        }
        if(NULL != hasMultiCard)
        {
            *hasMultiCard = isMultiCard;
        }

        return true;
    }
    //----------------------------------------------------- 
    /** 
     * @brief 设置设备波特率(对于设备来说slot参数不起实际作用) 
     * @param [in] baud 波特率 
     * @param [in] slot [default:0 接触]
     * - slot
     *  - 0:接触(默认) 
     *  - 1:非接 
     * .
     */ 
    bool SetDevBaud(uint baud, byte slot = 0x00)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();
        LOGGER(_log<<"设置波特率:<"<<baud<<">,卡槽号:<"<<_hex(slot)<<">\n");

        _tmpBuffer.Clear();

        DevCommand::FromAscii("30 01", _tmpBuffer);
        
        byte baudByte = 0;
        switch(baud)
        {
        case 19200:
            baudByte = 1;
            break;
        case 38400:
            baudByte = 2;
            break;
        case 57600:
            baudByte = 3;
            break;
        case 115200:
            baudByte = 4;
            break;
        case 9600:
        default:
            baudByte = 0;
            break;
        }
        _tmpBuffer += slot;
        _tmpBuffer += baudByte;

        LOGGER(_log<<"波特率对应序号:<"<<static_cast<int>(baudByte)<<">\n");

        ASSERT_FuncErrRet(_pDev->Write(_tmpBuffer), DeviceError::SendErr);

        _tmpBuffer.Clear();
        ASSERT_FuncErrInfoRet(_pDev->Read(_tmpBuffer), DeviceError::RecvErr, "设备不支持设置该波特率");

        return _logRetValue(true);
    }
    /**
     * @brief 获取设备版本信息 
     * @param [out] verBuff 获取到的版本信息 
     * @param [in] slot [default:0] 获取的卡槽 
     */ 
    bool GetVersion(ByteBuilder& verBuff, byte slot = 0x00)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();
        LOGGER(_log<<"卡槽号:<"<<slot<<">\n");

        _tmpBuffer.Clear();

        DevCommand::FromAscii("31 11", _tmpBuffer);
        _tmpBuffer += static_cast<byte>(slot);

        ASSERT_FuncErrRet(_pDev->Write(_tmpBuffer), DeviceError::SendErr);
        _tmpBuffer.Clear();
        ASSERT_FuncErrInfoRet(_pDev->Read(_tmpBuffer), DeviceError::RecvErr, "设备不支持获取版本信息");

        verBuff += _tmpBuffer;

        LOGGER(_log<<"版本信息:<"<<_tmpBuffer<<">\n");

        return _logRetValue(true);
    }
    /** 
     * @brief 软复位读写器 
     * @param [in] slot [default:0] 卡槽号 
     * - slot
     *  - 0:接触(默认) 
     *  - 1:非接 
     * .
     */ 
    bool ResetReader(byte slot = 0x00)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();
        LOGGER(_log<<"卡槽号:<"<<_hex(slot)<<">\n");

        _tmpBuffer.Clear();

        DevCommand::FromAscii("31 12", _tmpBuffer);
        _tmpBuffer += static_cast<byte>(slot);

        ASSERT_FuncErrRet(_pDev->Write(_tmpBuffer), DeviceError::SendErr);
        _tmpBuffer.Clear();
        ASSERT_FuncErrInfoRet(_pDev->Read(_tmpBuffer), DeviceError::RecvErr, "设备软复位失败");

        return _logRetValue(true);
    }
    /** 
     * @brief 蜂鸣器控制 
     * @param [in] durationTime 蜂鸣器鸣叫持续时间(单位:100ms) 
     * @param [in] intervalTime 蜂鸣器每次鸣叫间隔时间(单位:100ms)  
     * @param [in] times 蜂鸣器鸣叫次数(默认:1次)
     */ 
    bool BuzzerControl(byte durationTime, byte intervalTime, byte times)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();

        LOGGER(
        _log<<"鸣叫持续时间:<"<<static_cast<uint>(durationTime)<<"*100ms>,"
            <<"每次鸣叫间隔时间:<"<<static_cast<uint>(intervalTime)<<"*100ms>,"
            <<"鸣叫次数:<"<<static_cast<uint>(times)<<">\n");

        _tmpBuffer.Clear();

        DevCommand::FromAscii("31 13", _tmpBuffer);
        _tmpBuffer += durationTime;
        _tmpBuffer += intervalTime;

        ASSERT_FuncErrRet(_pDev->Write(_tmpBuffer), DeviceError::SendErr);
        _tmpBuffer.Clear();
        ASSERT_FuncErrInfoRet(_pDev->Read(_tmpBuffer), DeviceError::RecvErr, "蜂鸣器控制接收应答失败");

        return _logRetValue(true);
    }
    /** 
     * @brief LED灯控制 
     * @param [in] isTurnOn 是否打开LED灯 
     * @param [in] isControlRed [default:true] 是否控制红灯(默认:true) 
     * @param [in] isControlGreen [default:true] 是否打开绿灯(默认:true)
     */ 
    bool LedControl(bool isTurnOn, bool isControlRed = true, bool isControlGreen = true)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();

        LOGGER(
        _log<<"LED控制,灯状态:<"<<(isTurnOn ? "开" : "关")<<">,控制红灯:<"
            <<isControlRed<<">,控制绿灯:<"<<isControlGreen<<">\n");

        _tmpBuffer.Clear();
        
        DevCommand::FromAscii("31 14", _tmpBuffer);
        byte state = 0x00;

        if(isControlRed)
            state |= 0x80;
        if(isControlGreen)
            state |= 0x40;

        _tmpBuffer += state;
        _tmpBuffer += static_cast<byte>(isTurnOn ? 0x01 : 0x00);

        ASSERT_FuncErrRet(_pDev->Write(_tmpBuffer), DeviceError::SendErr);
        _tmpBuffer.Clear();
        ASSERT_FuncErrInfoRet(_pDev->Read(_tmpBuffer), DeviceError::RecvErr, "LED控制失败");

        return _logRetValue(true);
    }
    /**
     * @brief 设置设备为升级模式 
     */ 
    bool SetUpdateMode()
    {
        LOG_FUNC_NAME();
        ASSERT_Device();

        _tmpBuffer.Clear();

        DevCommand::FromAscii("31 15", _tmpBuffer);

        ASSERT_FuncErrRet(_pDev->Write(_tmpBuffer), DeviceError::SendErr);
        _tmpBuffer.Clear();
        ASSERT_FuncErrInfoRet(_pDev->Read(_tmpBuffer), DeviceError::RecvErr, "设置设备为升级模式失败");

        return _logRetValue(true);
    }
    /**
     * @brief 设置设备是否自动处理 61,6C状态码  
     * @param [in] isHandle true 处理61,6C
     */ 
    bool SetSW_Mode(bool isHandle)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();

        LOGGER(_log << "是否处理61,6C:<" << isHandle << ">\n");

        _tmpBuffer.Clear();

        DevCommand::FromAscii("31 16", _tmpBuffer);
        _tmpBuffer += static_cast<byte>(isHandle ? 0x01 : 0x00);

        ASSERT_FuncErrRet(_pDev->Write(_tmpBuffer), DeviceError::SendErr);
        _tmpBuffer.Clear();
        ASSERT_FuncErrInfoRet(_pDev->Read(_tmpBuffer), DeviceError::RecvErr, "更改61,6C设置失败");

        return _logRetValue(true);
    }
    /**
     * @brief 获取非接卡片的数目
     * @warning 该指令为非标准指令,只有部分设备支持 
     *
     * @param [in] count 获取到的卡片数目
     * @attention >1表示有多张卡片,不代表具体的卡片数目
     */
    bool GetContactlessCardCount(size_t& count)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();

        _tmpBuffer.Clear();
        DevCommand::FromAscii("32 21 FF", _tmpBuffer);

        ASSERT_FuncErrRet(_pDev->Write(_tmpBuffer), DeviceError::SendErr);
        _tmpBuffer.Clear();
        ASSERT_FuncErrInfoRet(_pDev->Read(_tmpBuffer), DeviceError::RecvErr, "获取非接卡状态失败");
        ASSERT_FuncErrRet(_tmpBuffer.GetLength() > 0, DeviceError::RecvFormatErr);

        count = static_cast<size_t>(_tmpBuffer[0]);
        LOGGER(_log << "卡片数目:<" << count << ">\n");

        return _logRetValue(true);
    }
    //----------------------------------------------------- 
};
//---------------------------------------------------------
} // namespace device
} // namespace application
} // namespace zhou_yb
//--------------------------------------------------------- 
#endif // _LIBZHOUYB_COMIC_READERDEVADAPTER_H_
//=========================================================
