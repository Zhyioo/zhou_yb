﻿//========================================================= 
/**@file pboc_app.cpp 
 * @brief IC卡应用客户端操作逻辑实现 
 *
 * @date 2013-06-24   22:23:45
 * @author Zhyioo 
 * @version 1.0
 */ 
//--------------------------------------------------------- 
#ifndef _LIBZHOUYB_PBOC_APP_CPP_
#define _LIBZHOUYB_PBOC_APP_CPP_
//--------------------------------------------------------- 
#include "pboc_app.h"
//--------------------------------------------------------- 
namespace zhou_yb {
namespace application {
namespace pboc {
//--------------------------------------------------------- 
/// 默认的获取信息转换对照表
const ushort PBOC_TransTable::INFORMATION[] = 
{
    // 应用PAN              'A' 
    0x41, 0x5A,
    // 持卡人姓名           'B'
    0x42, 0x5F20, 
    // 持卡人证件类型       'C'
    0x43, 0x9F62,
    // 持卡人证件号         'D'
    0x44, 0x9F61,
    // 磁条2 等效数据       'E'
    0x45, 0x57,
    // 磁条1 自定义数据     'F'
    0x46, 0x9F1F,
    // 电子现金余额         'G'
    0x47, 0x9F79,
    // 电子现金余额上限     'H'
    0x48, 0x9F77,
    // 应用失效日期         'I'
    0x49, 0x5F24,
    // 应用PAN 序列号       'J'
    0x4A, 0x5F34,
    // 电子现金单笔交易限额 'K'
    0x4B, 0x9F78,
    // 电子现金重置阈值     'L'
    0x4C, 0x9F6D,

    // 结束标记 
    TABLE_EOF, TABLE_EOF
};
//--------------------------------------------------------- 
/// ARQC转换对照表 
const ushort PBOC_TransTable::AMOUNT[] =
{
    // 授权金额(数值型)  'P'
    0x50, 0x9F02,  
    // 其他金额(数值形)  'Q'
    0x51, 0x9F03,
    // 交易货币代码      'R'
    0x52, 0x5F2A,
    // 交易日期          'S'
    0x53, 0x9A,
    // 交易类型          'T'
    0x54, 0x9C,
    // 交易时间          'U'
    0x55, 0x9F21,
    // 商户名称          'V'
    0x56, 0x9F4E,

    // 结束标记 
    TABLE_EOF, TABLE_EOF
};
//--------------------------------------------------------- 
/// 取记录转换对照表
const ushort PBOC_TransTable::DETAIL[] =
{
    // 授权金额(数值型)     'P'
    0x50, 0x9F02,
    // 其他金额(数值形)     'Q'
    0x51, 0x9F03,
    // 交易日期             'R'
    0x52, 0x9A,
    // 交易时间             'S'
    0x53, 0x9F21,
    // 交易类型             'T'
    0x54, 0x9C,
    // 终端国家代码         'U'
    0x55, 0x9F1A,
    // 交易货币代码         'V'
    0x56, 0x5F2A,
    // 商户名称             'W'
    0x57, 0x9F4E,
    // 应用交易计数器（ATC）'X'
    0x58, 0x9F36,

    // 结束标记 
    TABLE_EOF, TABLE_EOF
};
//--------------------------------------------------------- 
} // namespace pboc 
} // namespace application 
} // namespace zhou_yb 
//--------------------------------------------------------- 
#endif // _LIBZHOUYB_PBOC_APP_CPP_
//========================================================= 
