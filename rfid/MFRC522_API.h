#ifndef __MFRC522_API_H
#define __MFRC522_API_H

#include "MFRC522_Dev.h"

typedef enum
{
    FPORDERDATA = 1,
    FPFINGEDATA = 2,
    FPNONEDATA  = 0
} FPDATATYPE;

typedef struct
{
    uint8_t ucBlockNum;    // RFID CARD 块开始地址数
    uint8_t ucBlockLen;    // RFID CARD 块长度
    uint8_t ucBlockPass[6]; // RFID CARD 块密码
}RFIDDATA_TYPE;

// 存储或删除最大有效指纹ID号
#define MAXFINID     121

// 接收FP数据数组最大值
#define MAXFINDATA   (2048)

// 链接指令
// 验证密码指令
#define  LINKCARD      0xBE
#define  CHECKPSW      0xBF

/*
** 指纹存储块起始地址
*/
#define FINGERDATA1ADR     40
#define FINGERDATA2ADR     128

/*
**  相应存储指纹块跨度
*/
#define DATA1STEPLEN       4
#define DATA2STEPLEN       16



/*
** 大区与小区数目
*/
#define MAXAREANUM         (8)
#define MINAREANUM         (32)

/*
** 大区开始块
*/
#define MAXAREASTA         (128)

/*
** 大区与小区参数出错
*/
#define MAXERRORPAR        (15)
#define MINERRORPAR        (3)

/*
** 大区与小区参数出错
*/
#define MAXCTRLKUAI        (15)
#define MINCTRLKUAI        (3)

/*
** 大区或小区中的地个区中块数
*/
#define MAXAREALEN         (16)
#define MINAREALEN         (4)

// 一个块的数据量
#define KUANDATALEN        (16)

/*
** 时间、房间号、标志位存储块
*/
#define TMRROOMADR         2


#define  MCUSENDSNR    0xA0
#define  MCUREADCARD   0xA1
#define  MCUWRITECARD  0xA2
#define  MCUMODIFYPSW  0xA3
#define  MCUSETTIME    0xA4
#define  MCULOGINFIG   0xA5
#define  MCUDELETEFIG  0xA6
#define  MCUMATCHFIG   0xA7
#define  MCUREADEXTCD  0xAA
#define  MCUWRITEXTCD  0xAB


// 处理器测试读
// 处理器测试写
// 处理器未操作
#define  MCUTESTREAD   0xA8
#define  MCUTESTWRITE  0xA9
#define  MCUNONEORDER  0x00





extern uint8_t NewKey[16];     // 新密码  应该是每扇区最后一个权限
                                                             // 密钥A(6Byte) + 权限(4Byte) + 密钥B(6Byte)
extern uint8_t PassWd[6];          // 密码 密钥A 密钥A不能读出，能改写
extern uint8_t MLastSelectedSnr[4];// 存储 卡片序列号

//extern const uint8_t PassWord_Default[6];           //默认的密码（卡原始密码）
//extern const uint8_t PassWord_user[6];              //修改的密码（发卡后用户使用的密码）





void    MFRC522_Init(void);
uint8_t LinkMFRC522 (uint8_t *SN);

uint8_t StartLinkRc522 (void);
uint8_t RC522SendSnr (void);
uint8_t RC522SnrPassward (void);
uint8_t RC522ModifyPsw (uint8_t KuaiN);
uint8_t RfidReadData (uint8_t *pucKuaiDat,
                      uint8_t  ucKuaiNum,
                      uint8_t  ucKuaiLen,
                      uint8_t  ucContiFlg
                     );
uint8_t RfidWriteData (uint8_t ucKuaiNum,
                       uint8_t ucKuaiLen,
                       uint8_t *pucDat
                      );

#endif
