﻿//========================================================= 
/**@file AndroidH002Driver.h
 * @brief Android下H002驱动
 * 
 * @date 2016-06-08   18:37:52
 * @author Zhyioo 
 * @version 1.0
 */ 
//--------------------------------------------------------- 
#ifndef _LIBZHOUYB_ANDROIDH002DRIVER_H_
#define _LIBZHOUYB_ANDROIDH002DRIVER_H_
//--------------------------------------------------------- 
#include "JniH002Driver.h"

#include "../../../extension/ability/idcard/AndroidWltDecoder.h"
using zhou_yb::extension::ability::AndroidWltDecoder;

#include "../../tools/IconvAppConvert.h"
using zhou_yb::application::tools::IDCardConvert;
using zhou_yb::application::tools::PbocTlvConvert;
//--------------------------------------------------------- 
namespace zhou_yb {
namespace application {
namespace driver {
//--------------------------------------------------------- 
/// Android下H002驱动
template<class TArgParser>
class AndroidH002Driver : public JniH002Driver<TArgParser>
{
protected:
    AndroidWltDecoder _wltDecoder;
public:
    AndroidH002Driver() : JniH002Driver<TArgParser>()
    {
        // 设置转换函数指针
        this->_h002.SetTlvConvert(PbocTlvConvert::GbkToUTF8);
        this->_h002.SetIdcConvert(IDCardConvert::UnicodeToUTF8);
        this->_h002.SetWltDecoder(_wltDecoder);
    }
    /**
     * @brief 设置身份证照片解码库路径
     * @date 2016-06-12 15:27
     * 
     * @param [in] Path : string 解码库路径
     */
    LC_CMD_METHOD(SetWltrsDir)
    {
        string path = arg["Path"].To<string>();
        _wltDecoder.SetWltRS(path.c_str());
        return true;
    }
};
//--------------------------------------------------------- 
} // nemespace driver
} // namespace application
} // namespace zhou_yb
//--------------------------------------------------------- 
#endif // _LIBZHOUYB_ANDROIDH002DRIVER_H_
//========================================================= 