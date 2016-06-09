﻿//========================================================= 
/**@file ICCardCmdDriver.h
 * @brief IC卡命令
 * 
 * @date 2016-04-14   10:15:45
 * @author Zhyioo 
 * @version 1.0
 */ 
//--------------------------------------------------------- 
#ifndef _LIBZHOUYB_ICCARDCMDDRIVER_H_
#define _LIBZHOUYB_ICCARDCMDDRIVER_H_
//--------------------------------------------------------- 
#include "CommonCmdDriver.h"

#include "../../device/iccard/pboc/v2_0/PBOC_v2_0_AppAdapter.h"
using zhou_yb::device::iccard::pboc::v2_0::PBOC_v2_0_AppAdapter;
//--------------------------------------------------------- 
namespace zhou_yb {
namespace application {
namespace driver {
//--------------------------------------------------------- 
/// IC卡命令驱动
class ICCardCmdDriver : 
    public CommandCollection, 
    public DeviceBehavior,
    public InterruptBehavior,
    public RefObject
{
protected:
    Ref<IICCardDevice> _pDev;

    list<Ref<IICCardDevice> > _icList;
    list<string> _argList;
public:
    ICCardCmdDriver()
    {
        _Registe("IsCardPresent", (*this), &ICCardCmdDriver::IsCardPresent);
        _Registe("SelectSLOT", (*this), &ICCardCmdDriver::SelectSLOT);
        _Registe("WaitForCard", (*this), &ICCardCmdDriver::WaitForCard);

        Ref<Command> isActiveCmd = Command::Make(*this, &ICCardCmdDriver::IsActive);

        _Registe("IsActive", isActiveCmd);
        
        _Registe("PowerOn", (*this), &ICCardCmdDriver::PowerOn)->PreBind(isActiveCmd);
        _Registe("Apdu", (*this), &ICCardCmdDriver::Apdu)->PreBind(isActiveCmd);
        _Registe("ApduArray", (*this), &ICCardCmdDriver::ApduArray)->PreBind(isActiveCmd);
        _Registe("PowerOff", (*this), &ICCardCmdDriver::PowerOff)->PreBind(isActiveCmd);
    }
    //----------------------------------------------------- 
    /// 选择设备,返回当前已经选择的设备
    size_t SelectDevice(const Ref<IICCardDevice>& ic, const char* powerOnArg = NULL)
    {
        size_t count = _icList.size();
        if(ic.IsNull())
            return count;
        list<Ref<IICCardDevice> >::iterator itr;
        for(itr = _icList.begin();itr != _icList.end(); ++itr)
        {
            if((*itr) == ic)
                return count;
        }
        _icList.push_back(ic);
        _argList.push_back(_strput(powerOnArg));

        return (count + 1);
    }
    /// 释放设备,返回当前剩余的设备
    size_t ReleaseDevice(const Ref<IICCardDevice>& ic)
    {
        size_t count = _icList.size();
        size_t index = list_helper<Ref<IICCardDevice> >::position(_icList, ic);
        if(index == SIZE_EOF)
            return count;
        list_helper<Ref<IICCardDevice> >::erase_at(_icList, index);
        list_helper<string>::erase_at(_argList, index);

        return (count - 1);
    }
    void ReleaseDevice()
    {
        _icList.clear();
        _argList.clear();
        _pDev.Free();
    }
    //----------------------------------------------------- 
    /**
     * @brief 等待放入IC卡
     * @date 2016-05-16 21:52
     * 
     * @param [in] timeoutMs 等待的超时时间
     * @param [out] pIndex 实际连接到的卡槽[1-N]
     */
    bool WaitForCard(uint timeoutMs, size_t* pIndex = NULL)
    {
        Timer timer;
        list<Ref<IICCardDevice> >::iterator itr;
        list<string>::iterator argItr;
        size_t index = 0;
        while(timer.Elapsed() < timeoutMs)
        {
            if(InterruptBehavior::Implement(*this))
            {
                _logErr(DeviceError::OperatorInterruptErr);
                return false;
            }
            ++index;
            argItr = _argList.begin();
            for(itr = _icList.begin();itr != _icList.end(); ++itr)
            {
                if((*itr)->PowerOn(argItr->c_str(), NULL))
                {
                    _pDev = (*itr);
                    if(pIndex != NULL)
                    {
                        (*pIndex) = index;
                    }
                    return true;
                }
                ++argItr;
            }
        }
        _logErr(DeviceError::DevConnectErr, "等待放卡失败");
        return false;
    }
    /// 当前激活的IC卡
    inline Ref<IICCardDevice> ActiveIC()
    {
        return _pDev;
    }
    //----------------------------------------------------- 
    /**
     * @brief 判断读卡器上是否有卡
     * @date 2016-05-04 21:18
     * 
     * @param [in] SLOT : size_t 需要判断的卡片类型
     * @return bool 有无卡
     */
    LC_CMD_METHOD(IsCardPresent)
    {
        size_t slot = arg["SLOT"].To<size_t>(0);
        list<Ref<IICCardDevice> >::iterator itr = list_helper<Ref<IICCardDevice> >::index_of(_icList, slot);
        if(itr == _icList.end())
        {
            _logErr(DeviceError::ArgRangeErr);
            return false;
        }
        list<string>::iterator argItr = list_helper<string>::index_of(_argList, slot);
        return (*itr)->PowerOn(argItr->c_str(), NULL);
    }
    /**
     * @brief 选择卡槽号
     * @date 2016-05-04 21:21
     * 
     * @param [in] SLOT : size_t 卡槽号
     * - 参数
     *  - Contact 接触式
     *  - Contactless 非接
     *  - PSAM1 PSAM卡1
     *  - PSAM2 PSAM卡2
     * .
     */
    LC_CMD_METHOD(SelectSLOT)
    {
        size_t slot = arg["SLOT"].To<size_t>(0);
        list<Ref<IICCardDevice> >::iterator itr = list_helper<Ref<IICCardDevice> >::index_of(_icList, slot);
        if(itr == _icList.end())
        {
            _logErr(DeviceError::ArgRangeErr);
            return false;
        }

        _pDev = (*itr);
        return true;
    }
    /// 是否有卡片已经激活
    LC_CMD_METHOD(IsActive)
    {
        return _pDev.IsNull();
    }
    /**
     * @brief 等待放卡
     * @date 2016-05-11 21:16
     * 
     * @param [in] Timeout : uint 等待放卡的超时时间
     * 
     * @retval SLOT : size_t 实际放入的IC卡索引[1,N]
     */
    LC_CMD_METHOD(WaitForCard)
    {
        uint timeoutMs = arg["Timeout"].To<uint>(DEV_WAIT_TIMEOUT);
        size_t powerOnIndex = 0;
        if(WaitForCard(timeoutMs, &powerOnIndex))
        {
            rlt.PushValue("SLOT", ArgConvert::ToString<size_t>(powerOnIndex));
            return true;
        }
        return false;
    }
    /**
     * @brief 给卡片上电
     * 
     * @param [in] Timeout : uint 等待上电的超时时间
     *
     * @retval Atr : string 获取到的ATR信息
     */
    LC_CMD_METHOD(PowerOn)
    {
        size_t index = list_helper<Ref<IICCardDevice> >::position(_icList, _pDev);
        list<string>::iterator argItr = list_helper<string>::index_of(_argList, index);
        if(argItr == _argList.end())
        {
            _logErr(DeviceError::ArgRangeErr);
            return false;
        }

        ByteBuilder atr(16);
        Timer timer;
        uint timeoutMs = arg["Timeout"].To<uint>(DEV_OPERATOR_INTERVAL);
        while(timer.Elapsed() < timeoutMs)
        {
            if(InterruptBehavior::Implement(*this))
            {
                _logErr(DeviceError::OperatorInterruptErr);
                return false;
            }

            if(_pDev->PowerOn(argItr->c_str(), &atr))
            {
                ByteBuilder tmp(8);
                ByteConvert::ToAscii(atr, tmp);
                rlt.PushValue("Atr", tmp.GetString());
                return true;
            }
        }
        
        _logErr(DeviceError::WaitTimeOutErr, "等待放卡超时");
        return false;
    }
    /**
     * @brief 交互指令
     * 
     * @param [in] sApdu : string 需要发送的指令
     * 
     * @retval rApdu : string 接收到的APDU
     */
    LC_CMD_METHOD(Apdu)
    {
        ByteBuilder sCmd(32);
        ByteBuilder rCmd(32);

        string sApdu = arg["sApdu"].To<string>();
        ByteConvert::ToAscii(sApdu.c_str(), sCmd);

        if(!_pDev->Apdu(sCmd, rCmd))
            return false;
        ByteBuilder tmp(8);
        ByteConvert::ToAscii(rCmd, tmp);
        rlt.PushValue("rApdu", tmp.GetString());
        return true;
    }
    /**
     * @brief 交互多条APDU
     * 
     * @param [in] sApdu : string 需要发送的指令串
     * 
     * @retval rApdu : string 接收到的APDU
     */
    LC_CMD_METHOD(ApduArray)
    {
        ByteBuilder sCmd(32);
        ByteBuilder rCmd(32);

        bool bApdu = true;
        list<string> apduList;
        size_t count = arg.GetValue("sApdu", apduList);
        for(list<string>::iterator itr = apduList.begin();itr != apduList.end(); ++itr)
        {
            if(bApdu)
            {
                sCmd.Clear();
                ByteConvert::ToAscii(itr->c_str(), sCmd);

                rCmd.Clear();
                // 如果第一条指令交互都失败,则不再继续发送后续的指令
                bApdu = _pDev->Apdu(sCmd, rCmd);
                sCmd.Clear();
                ByteConvert::ToAscii(rCmd, sCmd);
                rlt.PushValue("rApdu", sCmd.GetString());
            }
        }
        return bApdu;
    }
    /// 卡片下电
    LC_CMD_METHOD(PowerOff)
    {
        _pDev->PowerOff();
        return true;
    }
    //----------------------------------------------------- 
};
//--------------------------------------------------------- 
} // nemespace driver
} // namespace application
} // namespace zhou_yb
//--------------------------------------------------------- 
#endif // _LIBZHOUYB_ICCARDCMDDRIVER_H_
//========================================================= 