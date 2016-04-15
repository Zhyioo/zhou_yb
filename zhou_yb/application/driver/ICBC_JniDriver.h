//========================================================= 
/**@file ICBC_JniDriver.h
 * @brief 工行交互终端驱动
 * 
 * @date 2016-04-02   13:32:49
 * @author Zhyioo 
 * @version 1.0
 */ 
#pragma once 
//--------------------------------------------------------- 
#include "CommandDriver.h"

#include "LC_JniDriver.h"

#include "PSBC_PinCmdDriver.h"
using zhou_yb::application::driver::PSBC_PinCmdDriver;
//--------------------------------------------------------- 
namespace zhou_yb {
namespace application {
namespace driver {
//--------------------------------------------------------- 
/// 工行交互终端驱动
class ICBC_JniDriver : public CommandDriver
{
protected:
    /* 设备 */
    LOGGER(FolderLogger _folder);
    JniEnvCmdDriver _jniDriver;
    BoolInterrupter _interrupter;

    /* 密码键盘 */
    PSBC_PinCmdDriver _pinDriver;
public:
    ICBC_JniDriver()
    {
        /* 公共部分 */
        RegisteCommand(Command::Bind("NativeInit", (*this), &ICBC_JniDriver::NativeInit));
        RegisteCommand(Command::Bind("NativeDestory", (*this), &ICBC_JniDriver::NativeDestory));
        RegisteCommand(Command::Bind("DriverVersion", (*this), &ICBC_JniDriver::DriverVersion));

        RegisteCommand(_jniDriver);

        /* 密码键盘部分 */
        RegisteCommand(_pinDriver);
    }

    /* Driver部分 */
    LC_CMD_METHOD(NativeInit)
    {
        LOGGER(_folder.Open(send.GetString(), "driver", 2, FILE_K(256));
        _log.Select(_folder));

        LOGGER(_pinDriver.SelectLogger(_log));

        _jniDriver.Interrupter = _interrupter;
        _pinDriver.SelectDevice(_jniDriver);

        return true;
    }
    LC_CMD_METHOD(NativeDestory)
    {
        LOGGER(_folder.Close();
        _log.Release());

        return true;
    }
    LC_CMD_METHOD(DriverVersion)
    {
        recv = "LC v1.0.0.1 20160402";
        return true;
    }
    /* 密码键盘部分 */
    LC_CMD_METHOD(GenerateKey_SM2)
    {
        return true;
    }
    LC_CMD_METHOD(DownloadMK)
    {
        ByteBuilder mk(32);
        DevCommand::FromAscii(send.GetString(), mk);
        if(mk.IsEmpty())
        {
            _logErr(DeviceError::ArgFormatErr, "输入的主密钥密文为空");
            return false;
        }
        return true;
    }
    LC_CMD_METHOD(DownloadWK)
    {
        ByteBuilder wk(32);
        DevCommand::FromAscii(send.GetString(), wk);
        if(wk.IsEmpty())
        {
            _logErr(DeviceError::ArgFormatErr, "输入的工作密钥密文为空");
            return false;
        }

        return true;
    }
    LC_CMD_METHOD(InputPassword)
    {
        return true;
    }
    LC_CMD_METHOD(WaitInput)
    {
        return true;
    }
    LC_CMD_METHOD(CancelInput)
    {
        return true;
    }
    LC_CMD_METHOD(GeneratePassword)
    {
        return true;
    }
};
//--------------------------------------------------------- 
} // namespace driver
} // namespace application
} // namespace zhou_yb
//========================================================= 
