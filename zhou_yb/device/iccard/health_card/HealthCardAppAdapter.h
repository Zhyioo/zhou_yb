﻿//========================================================= 
/**@file HealthCardAppAdapter.h 
 * @brief 居民健康卡操作逻辑
 * 
 * @date 2014-08-30   14:24:00 
 * @author Zhyioo 
 * @version 1.0
 */ 
//--------------------------------------------------------- 
#ifndef _LIBZHOUYB_HEALTHCARDAPPADAPTER_H_
#define _LIBZHOUYB_HEALTHCARDAPPADAPTER_H_
//--------------------------------------------------------- 
#include "../base/ICCardCmd.h"
#include "../base/ICCardLibrary.h"
#include "../base/ICCardAppAdapter.h"
using namespace zhou_yb::device::iccard::base;

#include "../pboc/v1_0/PBOC_v1_0_Library.h"
using namespace zhou_yb::device::iccard::pboc;

#include "../pboc/v2_0/PBOC_v2_0_Library.h"
using namespace zhou_yb::device::iccard::pboc::v2_0;
//--------------------------------------------------------- 
namespace zhou_yb {
namespace device {
namespace iccard {
namespace health_card {
//--------------------------------------------------------- 
/// 文件数据结构  
struct FID_DATA_MSG
{
    /// 标签 
    byte Tag;
    /// 字段名 
    char Name[48];
    /// 数据格式 
    DOLDataType Type;
    /// 数据长度 
    size_t Length;
};
/// 文件类型
enum FID_Type
{
    /// 二进制文件 
    FID_Bianry,
    /// 变长记录 
    FID_Recode,
    /// 循环记录 
    FID_Circle
};
/// 文件信息
struct FID_MSG
{
    /// FID名称 
    char Name[48];
    /// FID文件类型 
    uint Type;
    /// FID记录数 
    size_t Count;
    /// FID文件结构数据表 
    const FID_DATA_MSG* Msg;
    /// FID数据表大小 
    size_t Length;
};
/// SAM卡密钥结构 
struct KEY_MSG
{
    /// 密钥名称 
    char Name[16];
    /// 用户卡标识  
    byte UsrTag;
    /// 用户卡组数
    byte KeyCount;
    /// 密钥长度
    byte KeyLen;
    /// 密钥用途 
    byte KeyUse;
    /// 密钥标识 
    byte KeyTag;
    /// 密钥级别
    byte KeyLv;
};
/// 密钥数据结构 
struct FID_KEY_MSG
{
    /// FID 
    char Fid[16];
    /// 读密钥 
    char RK[16];
    /// 写密钥 
    char UK[16];
    /// 擦除密钥 
    char EK[16];
};
//--------------------------------------------------------- 
/// 用户卡FID数据信息 
extern FID_MSG FID_INFO[];
/// FID SAM卡密钥对照表 
extern FID_KEY_MSG FID_KEY_MAP[];
/// FID SAM卡密钥对照表长度  
extern size_t FID_KEY_MAP_LENGTH;
/// SAM卡密钥表 
extern KEY_MSG SAM_KEY_MAP[];
/// SAM卡密钥表长度 
extern size_t SAM_KEY_MAP_LENGTH;
//--------------------------------------------------------- 
/**
 * @brief 居民健康卡操作逻辑 
 */ 
class HealthCardAppAdapter : public ICCardAppAdapter
{
public:
    //----------------------------------------------------- 
    /// 密钥模式 
    enum KeyMode
    {
        /// 读认证
        ReadKey = 0x00,
        /// 写认证
        UpdateKey,
        /// 擦除认证 
        EraseKey
    };
    //----------------------------------------------------- 
protected:
    //----------------------------------------------------- 
    /// 发送缓冲区 
    ByteBuilder _sendBuff;
    /// 接收缓冲区 
    ByteBuilder _recvBuff;
    //----------------------------------------------------- 
    /// 在表中查找FID信息 
    FID_MSG* _FindFID(const char* fid)
    {
        // 查找FID 
        for(size_t i = 0;i < FID_KEY_MAP_LENGTH; ++i)
        {
            if(StringConvert::Compare(FID_INFO[i].Name, fid, true))
                return &(FID_INFO[i]);
        }
        return NULL;
    }
    /// 选卡目录 
    bool _SelectAid(ITransceiveTrans& icDev, const char* aidPath, list<ByteBuilder>* pAidData)
    {
        LOGGER(_log << "AidPath:<" << _strput(aidPath) << ">\n");

        list<PBOC_Library::AID> aidlist;
        PBOC_Library::MakeAidList(aidPath, aidlist, '|');

        list<PBOC_Library::AID>::iterator itr;

        for(itr = aidlist.begin();itr != aidlist.end(); ++itr)
        {
            _sendBuff.Clear();
            _recvBuff.Clear();

            LOGGER(_log.WriteLine("AID:").WriteStream(itr->aid) << endl);
            SelectCmd::Make(_sendBuff, itr->aid, SelectCmd::First, SelectCmd::ByAid);
            ASSERT_FuncInfo(_apdu_s(_sendBuff, _recvBuff, &icDev) == ICCardLibrary::SuccessSW, "选文件失败");
            LOGGER(_log.WriteLine("AidData:").WriteStream(_recvBuff) << endl);

            if(pAidData != NULL)
            {
                pAidData->push_back(_recvBuff);
            }
        }

        return true;
    }
    /// 校验PIN码  
    bool _VerifyPin(ITransceiveTrans& icDev, const char* pinAscii, size_t* pErrCount = NULL)
    {
        _sendBuff.Clear();
        _recvBuff.Clear();

        ByteBuilder pin(4);
        PinFormater::FromAscii(pinAscii, pin);
        VerifyCmd::Make(_sendBuff, pin);

        ushort sw = ICCardLibrary::UnValidSW;
        sw = _apdu_s(_sendBuff, _recvBuff, &icDev);
        ICCardLibrary::GetAuthCount(sw, pErrCount);
        ASSERT_FuncErr(ICCardLibrary::IsSuccessSW(sw), DeviceError::DevVerifyErr);
        return true;
    }
    /// 读透明文件 
    bool _ReadBIN(size_t maxLen, ByteBuilder& recode)
    {
        const size_t MAX_REC_LENGTH = 250;

        ByteBuilder cmd(8);
        // 二进制文件 
        ByteConvert::FromAscii("00B0", cmd);
        // 已经读取的数据 
        size_t offset = 0;
        // 每次读取的长度 
        size_t len = 0;
        ushort sw = ICCardLibrary::UnValidSW;
        while(offset < maxLen)
        {
            cmd.Clear();
            ByteConvert::FromAscii("00B0", cmd);
            ByteConvert::FromObject(static_cast<ushort>(offset), cmd);
            len = _min(maxLen - offset, MAX_REC_LENGTH);
            cmd += static_cast<byte>(len);

            sw = _apdu_s(cmd, recode);
            if(!ICCardLibrary::IsSuccessSW(sw))
                return false;

            offset += len;
        }
        return true;
    }
    /**
     * @brief 读取卡片FID数据
     *
     * @param [in] fid 需要读取的FID数据表
     * @param [out] dst 获取到的数据
     * @param [in] splitFlag 每个字段之间的分隔符
     */
    bool _ReadFID(const FID_MSG& fid, ByteBuilder& dst, char splitFlag)
    {
        ByteBuilder recode(128);
        ByteBuilder buff(32);
        ByteBuilder cmd(8);
        size_t count = 0;
        size_t maxLen = 0;
        ushort sw = ICCardLibrary::UnValidSW;
        switch(fid.Type)
        {
        case FID_Bianry:
            // 计算透明文件的总长度 
            maxLen = 0;
            for(size_t j = 0;j < fid.Length; ++j)
                maxLen += fid.Msg[j].Length;
            if(!_ReadBIN(maxLen, recode))
                return false;
            break;
        case FID_Recode:
            break;
        case FID_Circle:
            for(size_t index = 0;index < fid.Count; ++index)
            {
                // 记录文件
                cmd.Clear();
                ByteConvert::FromAscii("00B2000400", cmd);
                // 记录号 
                cmd[ICCardLibrary::P1_INDEX] = static_cast<byte>(index + 1);

                sw = _apdu_s(cmd, recode);
                if(!ICCardLibrary::IsSuccessSW(sw))
                    return false;
            }
            break;
        }

        size_t offset = 0;
        for(size_t index = 0;index < fid.Count; ++index)
        {
            for(size_t i = 0;i < fid.Length; ++i)
            {
                // 二进制文件或循环记录 
                if(fid.Type != FID_Recode)
                {
                    // 切割数据 
                    buff.Clear();
                    buff += recode.SubArray(offset, fid.Msg[i].Length);

                    offset += fid.Msg[i].Length;
                }
                else
                {
                    cmd.Clear();
                    buff.Clear();

                    ReadRecodeCmd::Make(cmd, fid.Msg[i].Tag);
                    sw = _apdu_s(cmd, buff);
                    if(!ICCardLibrary::IsSuccessSW(sw) || buff[0] != fid.Msg[i].Tag)
                        return false;

                    cmd.Swap(buff);
                    buff.Clear();
                    PBOC_Library::GetTagValue(cmd, static_cast<ushort>(fid.Msg[i].Tag), &buff);
                }
                count += PBOC_Library::DolToString(buff, fid.Msg[i].Type, dst);
                dst += static_cast<byte>(splitFlag);
                ++count;
            }
        }
        // 删除最后一个多余的 分隔符  
        if(count > 0)
            dst.RemoveTail();
        return true;
    }
    //----------------------------------------------------- 
public:
    //----------------------------------------------------- 
    /// 用户卡ATR转换为分散因子 
    static bool AtrToSession(const ByteArray& usrAtr, ByteBuilder& session)
    {
        if(usrAtr.GetLength() < 12)
            return false;

        session.Append(usrAtr.SubArray(4, 8));
        return true;
    }
    /// 城市代码转换为分散因子 
    static void CityCodeToSession(byte cityCode, ByteBuilder& session)
    {
        // 转换成ASCII码 
        ByteConvert::ToAscii(cityCode, session);
        session.Append(static_cast<byte>(0x30), 6);
    }
    //----------------------------------------------------- 
    /**
     * @brief 选择SAM卡目录
     * @param [in] samIC 需要操作的SAM卡读卡器
     * @param [in] aidPath 需要选择的AID路径
     * @param [out] pAidData [default:NULL] 选择路径中的AID数据
     */
    bool SelectAid(ITransceiveTrans& samIC, const char* aidPath, list<ByteBuilder>* pAidData = NULL)
    {
        LOG_FUNC_NAME();
        LOGGER(_log.WriteLine("Select SAM AID:"));
        return _logRetValue(_SelectAid(samIC, aidPath, pAidData));
    }
    /**
     * @brief 选择用户卡AID目录
     *
     * @param [in] aidPath 需要选择的AID路径
     * @param [in] pAidData [default:NULL] 选择路径中的AID数据
     */
    bool SelectAid(const char* aidPath, list<ByteBuilder>* pAidData = NULL)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();
        return _logRetValue(_SelectAid(_pDev, aidPath, pAidData));
    }
    /**
     * @brief 校验SAM卡PIN码
     * @param [in] samIC 需要操作的IC卡读卡器(为NULL表示操作当前选择的读卡器)
     * @param [in] pinAscii 用户卡的PIN码
     * @param [out] pErrCount [default:NULL] 剩余尝试次数
     */
    bool VerifyPin(ITransceiveTrans& samIC, const char* pinAscii, size_t* pErrCount = NULL)
    {
        LOG_FUNC_NAME();
        LOGGER(_log.WriteLine("Verify SAM PIN:"));
        return _logRetValue(_VerifyPin(samIC, pinAscii, pErrCount));
    }
    /**
     * @brief 校验用户卡PIN码
     *
     * @param [in] pinAscii 输入的PIN码
     * @param [out] pErrCount [default:NULL] 剩余尝试次数
     */
    bool VerifyPin(const char* pinAscii, size_t* pErrCount = NULL)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();
        return _logRetValue(_VerifyPin(_pDev, pinAscii, pErrCount));
    }
    /**
     * @brief 取指定的标签数据
     */
    bool GetTAG(byte tag, ByteBuilder& data)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();

        LOGGER(_log << "标签:<" << _hex(tag) << ">\n");

        _sendBuff.Clear();
        _recvBuff.Clear();

        ReadRecodeCmd::Make(_sendBuff, tag);
        ASSERT_FuncInfoRet(_apdu(_sendBuff, _recvBuff), "读记录标签失败");
        ASSERT_FuncInfoRet(_recvBuff[0] == tag, "记录中不包含指定标签");

        PBOC_Library::GetTagValue(_recvBuff, static_cast<ushort>(tag), &data);
        return _logRetValue(true);
    }
    /**
     * @brief 获取城市代码做分散因子(选择后卡片状态处于MF.EF05下)
     */
    bool GetCityCode(byte& cityCode)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();

        LOGGER(_log.WriteLine("选文件..."));
        ASSERT_FuncInfoRet(SelectAid("3F00|DDF1|EF05"), "选文件失败");

        ByteBuilder tmp(8);
        ASSERT_FuncInfoRet(GetTAG(57, tmp), "取城市代码失败");

        if(!tmp.IsEmpty())
        {
            cityCode = tmp[0];
        }

        return _logRetValue(true);
    }
    /**
     * @brief 用户卡内部认证
     * @param [in] samIC SAM卡所在的IC卡读卡器
     * @param [in] keyMsg 密钥信息
     * @param [in] session_lv1_8 8字节一级分散因子
     * @param [in] session_lv2_8 8字节二级分散因子
     * @param [in] session_lv3_8 8字节三级分散因子
     * - 分散因子说明
     *  - 一级分散因子一般为省级数据
     *  - 二级分散因子一般为市级数据
     *  - 三级分散因子一般为卡片数据
     *  - 如果分散级别达不到,则分散因子级别依次递减,后续的分散因子传""即可
     * .
     * @param [in] authData_8 authData_8 [default:""] 8字节认证数据(为空则随机产生8字节数据)
     * @param [in] keyVersion 密钥版本(密钥索引)
     * @retval bool
     * @return
     */
    bool InternalAuthenicate(ITransceiveTrans& samIC, 
        KEY_MSG& keyMsg,
        const ByteArray& session_lv1_8, 
        const ByteArray& session_lv2_8, 
        const ByteArray& session_lv3_8, 
        const ByteArray& authData_8 = "", 
        byte keyVersion = 0x01)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();

        LOGGER(_log << "密钥用途:<" << _hex(keyMsg.KeyUse) << ">\n";
        _log << "密钥标识:<" << _hex(keyMsg.KeyTag) << ">\n";
        _log << "分散因子1:<";_log.WriteStream(session_lv1_8) << ">\n";
        _log << "分散因子2:<";_log.WriteStream(session_lv2_8) << ">\n";
        _log << "分散因子3:<";_log.WriteStream(session_lv3_8) << ">\n");

        _sendBuff.Clear();
        _recvBuff.Clear();

        /* SAM卡取随机数 */
        LOGGER(_log.WriteLine("SAM卡分散密钥..."));

        ByteBuilder samRAN(8);
        GetChallengeCmd::Make(_sendBuff, 8);
        ASSERT_FuncInfoRet(_apdu(_sendBuff, samRAN, &samIC), "SAM卡取随机数失败");
        LOGGER(_log.WriteLine("SAM卡产生的随机数:").WriteStream(samRAN) << endl);

        _sendBuff.Clear();
        DevCommand::FromAscii("80 DE 00 00 00", _sendBuff);
        _sendBuff[ICCardLibrary::P1_INDEX] = keyMsg.KeyUse;
        byte keyTag = keyMsg.KeyTag;
        keyTag += (keyVersion & 0x0F) << 5;
        _sendBuff[ICCardLibrary::P2_INDEX] = keyTag;

        if(!session_lv3_8.IsEmpty() && keyMsg.KeyLv < 3)
            _sendBuff.Append(session_lv3_8.SubArray(0, 8));
        if(!session_lv2_8.IsEmpty() && keyMsg.KeyLv < 2)
            _sendBuff.Append(session_lv2_8.SubArray(0, 8));
        if(!session_lv1_8.IsEmpty() && keyMsg.KeyLv < 1)
            _sendBuff.Append(session_lv1_8.SubArray(0, 8));

        _sendBuff += samRAN;
        _sendBuff[ICCardLibrary::LC_INDEX] = ICCardLibrary::GetLC(_sendBuff);

        ASSERT_FuncInfoRet(_apdu(_sendBuff, _recvBuff, &samIC), "SAM卡密钥分散失败");

        ByteBuilder authData(8);
        authData = authData_8;
        if(authData.IsEmpty())
        {
            authData.Append(0x00, 8);
        }
        else
        {
            ByteConvert::Fill(authData, 8);
        }

        LOGGER(_log.WriteLine("SAM卡产生认证数据..."));
        _sendBuff.Clear();
        _recvBuff.Clear();

        DevCommand::FromAscii("80 FA 00 00 10", _sendBuff);
        _sendBuff += authData;
        _sendBuff.Append(static_cast<byte>(0x00), 8);

        ASSERT_FuncInfoRet(_apdu(_sendBuff, _recvBuff, &samIC), "SAM卡产生认证数据失败");

        _sendBuff.Clear();

        ByteBuilder samAuthData(8);
        samAuthData.Append(_recvBuff.SubArray(0, 8));
        _sendBuff.Append(_recvBuff.SubArray(8, 8));

        LOGGER(_log << "认证数据分量1:<";
        _log.WriteStream(samAuthData) << ">\n";
        _log << "认证数据分量2:<";
        _log.WriteStream(_sendBuff) << ">\n");

        ByteConvert::Xor(_sendBuff, samAuthData);
        LOGGER(_log << "认证数据:<";
        _log.WriteStream(samAuthData) << ">\n");

        LOGGER(_log.WriteLine("用户卡内部认证数据..."));

        _sendBuff.Clear();
        _recvBuff.Clear();
        DevCommand::FromAscii("00 88 00 00 11", _sendBuff);
        _sendBuff += samRAN;
        _sendBuff += authData;
        _sendBuff += keyVersion;

        ASSERT_FuncInfoRet(_apdu(_sendBuff, _recvBuff), "获取用户卡内部认证数据失败");

        LOGGER(_log << "用户卡内部认证数据:<";
        _log.WriteStream(_recvBuff) << ">\n");

        // 比对认证数据 
        ASSERT_FuncInfoRet(samAuthData == _recvBuff, "比对内部认证数据失败");

        return _logRetValue(true);
    }
    /**
     * @brief 用户卡外部认证
     * @param [in] samIC SAM卡所在的IC卡读卡器
     * @param [in] keyMsg 密钥信息
     * @param [in] session_lv1_8 8字节一级分散因子
     * @param [in] session_lv2_8 8字节二级分散因子
     * @param [in] session_lv3_8 8字节三级分散因子
     * - 分散因子说明
     *  - 一级分散因子一般为省级数据
     *  - 二级分散因子一般为市级数据
     *  - 三级分散因子一般为卡片数据
     *  - 如果分散级别达不到,则分散因子级别依次递减,后续的分散因子传""即可
     * .
     * @param [in] authData_8 authData_8 authData_8 [default:""] 8字节认证数据(为空则随机产生8字节数据)
     * @param [in] keyVersion 密钥版本(密钥索引)
     * @retval bool
     * @return
     */
    bool ExternalAuthenticate(ITransceiveTrans& samIC, 
        KEY_MSG& keyMsg,
        const ByteArray& session_lv1_8, 
        const ByteArray& session_lv2_8, 
        const ByteArray& session_lv3_8,
        const ByteArray& authData_8 = "", 
        byte keyVersion = 0x01)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();

        LOGGER(_log << "用户卡标识:<" << _hex(keyMsg.UsrTag) << ">\n";
        _log << "密钥用途:<" << _hex(keyMsg.KeyUse) << ">\n";
        _log << "密钥标识:<" << _hex(keyMsg.KeyTag) << ">\n";
        _log << "分散因子1:<";_log.WriteStream(session_lv1_8) << ">\n";
        _log << "分散因子2:<";_log.WriteStream(session_lv2_8) << ">\n";
        _log << "分散因子3:<";_log.WriteStream(session_lv3_8) << ">\n");

        _sendBuff.Clear();
        _recvBuff.Clear();

        LOGGER(_log.WriteLine("SAM卡分散密钥..."));

        /* USR卡取随机数 */
        ByteBuilder usrRAN(8);
        GetChallengeCmd::Make(_sendBuff, 8);
        ASSERT_FuncInfoRet(_apdu(_sendBuff, usrRAN), "用户卡取随机数失败");
        LOGGER(_log.WriteLine("用户卡产生的随机数:").WriteStream(usrRAN) << endl);

        _sendBuff.Clear();
        DevCommand::FromAscii("80 DE 00 00 00", _sendBuff);
        _sendBuff[ICCardLibrary::P1_INDEX] = keyMsg.KeyUse;
        byte keyTag = keyMsg.KeyTag;
        keyTag += (keyVersion & 0x0F) << 5;
        _sendBuff[ICCardLibrary::P2_INDEX] = keyTag;

        if(!session_lv3_8.IsEmpty() && keyMsg.KeyLv < 3)
            _sendBuff.Append(session_lv3_8.SubArray(0, 8));
        if(!session_lv2_8.IsEmpty() && keyMsg.KeyLv < 2)
            _sendBuff.Append(session_lv2_8.SubArray(0, 8));
        if(!session_lv1_8.IsEmpty() && keyMsg.KeyLv < 1)
            _sendBuff.Append(session_lv1_8.SubArray(0, 8));

        _sendBuff += usrRAN;
        _sendBuff[ICCardLibrary::LC_INDEX] = ICCardLibrary::GetLC(_sendBuff);

        ASSERT_FuncInfoRet(_apdu(_sendBuff, _recvBuff, &samIC), "SAM卡密钥分散失败");

        ByteBuilder authData(8);
        authData = authData_8;
        if(authData.IsEmpty())
        {
            authData.Append(0x00, 8);
        }
        else
        {
            ByteConvert::Fill(authData, 8);
        }

        LOGGER(_log.WriteLine("SAM卡产生认证数据..."));
        _sendBuff.Clear();
        _recvBuff.Clear();

        DevCommand::FromAscii("80 FA 00 00 10", _sendBuff);
        _sendBuff += authData;
        _sendBuff.Append(static_cast<byte>(0x00), 8);

        ASSERT_FuncInfoRet(_apdu(_sendBuff, _recvBuff, &samIC), "SAM卡产生认证数据失败");

        _sendBuff.Clear();

        ByteBuilder usrAuthData(8);
        usrAuthData.Append(_recvBuff.SubArray(0, 8));
        _sendBuff.Append(_recvBuff.SubArray(8, 8));

        LOGGER(_log << "认证数据分量1:<";
        _log.WriteStream(usrAuthData) << ">\n";
        _log << "认证数据分量2:<";
        _log.WriteStream(_sendBuff) << ">\n");

        ByteConvert::Xor(_sendBuff, usrAuthData);
        LOGGER(_log << "认证数据:<";
        _log.WriteStream(usrAuthData) << ">\n");

        LOGGER(_log.WriteLine("用户卡外部认证数据..."));

        _sendBuff.Clear();
        DevCommand::FromAscii("00 82 00 00 11", _sendBuff);
        _sendBuff[ICCardLibrary::P2_INDEX] = keyMsg.UsrTag;
        _sendBuff += usrAuthData;
        _sendBuff += authData;
        _sendBuff += keyVersion;

        ASSERT_FuncInfoRet(_apdu(_sendBuff, _recvBuff), "用户卡外部认证失败");

        return _logRetValue(true);
    }
    bool MacAuthenticate(ITransceiveTrans& samIC,
        KEY_MSG& keyMsg,
        const ByteArray& macData,
        ByteBuilder& mac,
        const ByteArray& session_lv1_8,
        const ByteArray& session_lv2_8,
        const ByteArray& session_lv3_8,
        byte keyVersion = 0x01)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();

        LOGGER(_log << "用户卡标识:<" << _hex(keyMsg.UsrTag) << ">\n";
        _log << "密钥用途:<" << _hex(keyMsg.KeyUse) << ">\n";
        _log << "密钥标识:<" << _hex(keyMsg.KeyTag) << ">\n";
        _log << "分散因子1:<";_log.WriteStream(session_lv1_8) << ">\n";
        _log << "分散因子2:<";_log.WriteStream(session_lv2_8) << ">\n";
        _log << "分散因子3:<";_log.WriteStream(session_lv3_8) << ">\n";
        _log << "MAC计算数据:<";_log.WriteStream(macData) << ">\n");

        _sendBuff.Clear();
        _recvBuff.Clear();

        LOGGER(_log.WriteLine("SAM卡计算MAC..."));

        /* USR卡取随机数 */
        ByteBuilder usrRAN(8);
        GetChallengeCmd::Make(_sendBuff, 8);
        ASSERT_FuncInfoRet(_apdu(_sendBuff, usrRAN), "用户卡取随机数失败");
        LOGGER(_log.WriteLine("用户卡产生的随机数:").WriteStream(usrRAN) << endl);

        _sendBuff.Clear();
        DevCommand::FromAscii("80 DE 00 00 00", _sendBuff);
        _sendBuff[ICCardLibrary::P1_INDEX] = keyMsg.KeyUse;
        byte keyTag = keyMsg.KeyTag;
        keyTag += (keyVersion & 0x0F) << 5;
        _sendBuff[ICCardLibrary::P2_INDEX] = keyTag;

        if(!session_lv1_8.IsEmpty())
            _sendBuff.Append(session_lv1_8.SubArray(0, 8));
        if(!session_lv2_8.IsEmpty())
            _sendBuff.Append(session_lv2_8.SubArray(0, 8));
        if(!session_lv3_8.IsEmpty())
            _sendBuff.Append(session_lv3_8.SubArray(0, 8));

        _sendBuff += usrRAN;
        _sendBuff[ICCardLibrary::LC_INDEX] = ICCardLibrary::GetLC(_sendBuff);



        return _logRetValue(true);
    }
    /**
     * @brief 初始化SAM卡设备(选择3F00|DF01并校验PIN码)
     *
     * @param [in] samIC SAM卡设备
     * @param [in] pinAscii 需要校验的PIN码(ASCII码格式)
     * 
     * @return bool 是否初始化成功
     */
    inline bool InitSAM(ITransceiveTrans& samIC, const char* pinAscii = NULL)
    {
        LOG_FUNC_NAME();
        if(!SelectAid(samIC, "3F00|DF01"))
            return _logRetValue(false);

        if(!VerifyPin(samIC, _is_empty_or_null(pinAscii) ? "123456" : pinAscii))
            return _logRetValue(false);
        return _logRetValue(true);
    }
    /**
     * @brief 认证指定的FID
     *
     * @param [in] samIC SAM卡设备
     * @warning SAM卡必须已经进行过PIN校验
     *
     * @param [in] fid 需要认证的FID路径
     * @param [in] mode 需要认证的类型 
     * @param [in] atrSession_8 根据ATR产生的分散因子
     * @param [in] cityCode 城市代码
     * @param [in] keyVersion [default:1] 用于认证的密钥版本(密钥索引) 
     *
     * @return bool 是否认证成功
     */
    bool AuthenticateFID(ITransceiveTrans& samIC, const char* fid, KeyMode mode, 
        const ByteArray& atrSession_8, byte cityCode, byte keyVersion = 0x01)
    {
        LOG_FUNC_NAME();
        LOGGER(_log << "FID:<" << _strput(fid) << ">\n");
        LOGGER(_log << "ATR SESSION:";
        _log.WriteStream(atrSession_8) << endl;
        _log << "认证类型:";
        switch(mode)
        {
        case UpdateKey:
            _log.WriteLine("写密钥");
            break;
        case EraseKey:
            _log.WriteLine("擦除密钥");
            break;
        default:
        case ReadKey:
            _log.WriteLine("读密钥");
            break;
        }
        _log << "城市代码:<" << _hex(cityCode) << ">,密钥版本:<" << _hex(keyVersion) << ">\n");

        size_t fidMapLen = FID_KEY_MAP_LENGTH;
        size_t keyMapLen = SAM_KEY_MAP_LENGTH;

        for(size_t i = 0;i < fidMapLen; ++i)
        {
            // 查找FID 
            if(StringConvert::Compare(FID_KEY_MAP[i].Fid, fid, true))
            {
                const char* keyName = NULL;
                switch(mode)
                {
                case UpdateKey:
                    keyName = FID_KEY_MAP[i].UK;
                    break;
                case EraseKey:
                    keyName = FID_KEY_MAP[i].EK;
                    break;
                case ReadKey:
                default:
                    keyName = FID_KEY_MAP[i].RK;
                    break;
                }

                if(!SelectAid(fid))
                {
                    LOGGER(_log.WriteLine("选文件失败"));
                    return _logRetValue(false);
                }

                LOGGER(_log << "KeyName:<" << keyName << ">\n");
                ASSERT_FuncErrInfoRet(!StringConvert::Compare(keyName, "NULL"), DeviceError::OperatorStatusErr, "密钥不允许访问");

                if(strlen(keyName) < 1)
                {
                    LOGGER(_log.WriteLine("权限不需要认证"));
                    return _logRetValue(true);
                }

                // 查表获取到密钥用途和密钥索引 
                for(size_t j = 0;j < keyMapLen; ++j)
                {
                    if(StringConvert::Compare(keyName, SAM_KEY_MAP[j].Name))
                    {
                        ByteBuilder citySession_8(8);
                        CityCodeToSession(cityCode, citySession_8);
                        if(!ExternalAuthenticate(samIC, SAM_KEY_MAP[j], "", citySession_8, atrSession_8, "", keyVersion))
                        {
                            _logErr(DeviceError::DevVerifyErr, "SAM卡外部认证失败");
                            return _logRetValue(false);
                        }

                        return _logRetValue(true);
                    }
                }
            }
        }

        _logErr(DeviceError::ArgFormatErr, "表中没有查找到密钥");
        return _logRetValue(false);
    }
    /**
     * @brief 读取指定FID的数据,每个字段之间用'|'号分隔  
     * 
     * @param [in] fid 输入的FID 
     * @param [out] dst 获取到的数据 
     * @param [in] splitFlag [default:SPLIT_CHAR] 字段分隔符 
     * 
     * @return bool 是否读取成功 
     */
    bool ReadFID(const char* fid, ByteBuilder& dst, char splitFlag = SPLIT_CHAR)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();

        LOGGER(_log << "FID:<" << _strput(fid) << ">\n");

        // 查找FID 
        FID_MSG* pFID = _FindFID(fid);
        if(pFID != NULL)
            return _logRetValue(_ReadFID(*pFID, dst, splitFlag));
        
        _logErr(DeviceError::ArgErr, "没有找到匹配的FID");
        return _logRetValue(false);
    }
    /// 读二进制数据 
    bool ReadBIN(const char* fid, ByteBuilder& recode)
    {
        LOG_FUNC_NAME();
        ASSERT_Device();

        LOGGER(_log << "FID:<" << _strput(fid) << ">\n");

        // 查找FID 
        FID_MSG* pFID = _FindFID(fid);
        if(pFID != NULL)
        {
            size_t maxLen = 0;
            // 计算透明文件的总长度 
            maxLen = 0;
            for(size_t j = 0;j < pFID->Length; ++j)
                maxLen += pFID->Msg[j].Length;

            return _logRetValue(_ReadBIN(maxLen, recode));
        }

        _logErr(DeviceError::ArgErr, "没有找到匹配的FID");
        return _logRetValue(false);
    }
    //----------------------------------------------------- 
};
//--------------------------------------------------------- 
} // namespace health_card
} // namespace iccard
} // namespace device
} // namespace zhou_yb
//--------------------------------------------------------- 
#endif // _LIBZHOUYB_HEALTHCARDAPPADAPTER_H_
//========================================================= 
