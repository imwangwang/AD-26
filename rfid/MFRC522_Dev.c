#include "includes.h"
#include "MFRC522_Dev.h"

#define delay_us(us)        nrf_delay_us(us)

static const nrf_drv_spi_t mspi = NRF_DRV_SPI_INSTANCE(0);  /**< SPI instance. */

/*****************************************************
** @函数名：PcdRequest
** @描述  ：寻卡
** @参数  ：req_code[IN]:寻卡方式
**                0x52 = 寻感应区内所有符合14443A标准的卡
**                0x26 = 寻未进入休眠状态的卡
**          pTagType[OUT]：卡片类型代码
**                0x4400 = Mifare_UltraLight
**                0x0400 = Mifare_One(S50)
**                0x0200 = Mifare_One(S70)
**                0x0800 = Mifare_Pro(X)
**                0x4403 = Mifare_DESFire
** @返回值：成功返回MI_OK
*****************************************************/
int8_t PcdRequest(uint8_t req_code, uint8_t *pTagType)
{
    int8_t   status;
    uint16_t unLen;
    uint8_t  ucComMF522Buf[MAXRLEN];

    ClearBitMask(Status2Reg, 0x08);
    WriteRawRC(BitFramingReg, 0x07); // 定义接收LSB放在bit0,发送的最后一个字节位数  7 bit
    SetBitMask(TxControlReg, 0x03);  // 设置TX1、TX2发送经13.56MHz调制载波

    ucComMF522Buf[0] = req_code;

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 1, ucComMF522Buf, &unLen);

    if ((status == MI_OK) && (unLen == 0x10)) {    // 接收卡发回来的数据字节数16Byte
        *pTagType     = ucComMF522Buf[0];
        *(pTagType + 1) = ucComMF522Buf[1];
    } else {
        status = MI_ERR;
    }

    return status;
}

/*****************************************************
** @函数名：PcdAnticoll
** @描述  ：防冲撞
** @参数  ：pSnr[OUT]:卡片序列号，4字节
** @返回值：成功返回MI_OK
*****************************************************/
int8_t PcdAnticoll(uint8_t *pSnr)
{
    int8_t   status;
    uint8_t  i, snr_check = 0;
    uint16_t unLen;
    uint8_t  ucComMF522Buf[MAXRLEN];

    ClearBitMask(Status2Reg, 0x08);
    WriteRawRC(BitFramingReg, 0x00);    // 暂停数据发送
                                        // 接收LSB放在bit0位上
                                        // 最后一个字节所有位都发送
    ClearBitMask(CollReg, 0x80);        // 所以接收的位在位冲突后都将清零

    ucComMF522Buf[0] = PICC_ANTICOLL1; // 防冲撞指令代码
    ucComMF522Buf[1] = 0x20;           // ?

    // 发送并接收数据指令
    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 2, ucComMF522Buf, &unLen);

    if (status == MI_OK) {
        for (i = 0; i < 4; i++) {
            *(pSnr + i) = ucComMF522Buf[i];  // 0扇0区 0--3 Byte 卡片序列号
            snr_check ^= ucComMF522Buf[i];  // 将卡片序列号按拉异或进行校验
        }
        if (snr_check != ucComMF522Buf[i]) {status = MI_ERR;    }  // 卡片序列号校验与Byte4相同，则正确
    }

    SetBitMask(CollReg, 0x80);
    return status;
}

/*****************************************************
** @函数名：PcdSelect
** @描述  ：选定卡片
** @参数  ：pSnr[IN]:卡片序列号，4字节
** @返回值：成功返回MI_OK
*****************************************************/
int8_t PcdSelect(uint8_t *pSnr)
{
    int8_t   status;
    uint8_t  i;
    uint16_t unLen;
    uint8_t  ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_ANTICOLL1;  // 防冲撞  0x93
    ucComMF522Buf[1] = 0x70;            // ?
    ucComMF522Buf[6] = 0;
    for (i = 0; i < 4; i++) {
        ucComMF522Buf[i + 2] = *(pSnr + i); // 选择卡片的序列号
        ucComMF522Buf[6] ^= *(pSnr + i); // 选择卡片的序列号校验码
    }
    // CRC校验  ucComMF522Buf[7] ucComMF522Buf[8] 分别存储校验结果LSB MSB数值
    CalulateCRC(ucComMF522Buf, 7, &ucComMF522Buf[7]);

    ClearBitMask(Status2Reg, 0x08);

    // 发送并接收数据
    // 发送防冲撞指令 卡片序列号  序列号校验码 序列号CRC计算结果值 等值
    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 9, ucComMF522Buf, &unLen);

    if ((status == MI_OK) && (unLen == 0x18)) { // 返回数据是否 24 bit == 3 Byte
        status = MI_OK;
    } else {
        status = MI_ERR;
    }

    return status;
}

/*****************************************************
** @函数名：PcdAuthState
** @描述  ：验证卡片密码
** @参数  ：auth_mode[IN]: 密码验证模式
**                 0x60 = 验证A密钥
**                 0x61 = 验证B密钥
**          addr[IN]：块地址
**          pKey[IN]：密码
**          pSnr[IN]：卡片序列号，4字节
** @返回值：成功返回MI_OK
*****************************************************/
int8_t PcdAuthState(uint8_t auth_mode, uint8_t addr, uint8_t *pKey, uint8_t *pSnr)
{
    int8_t   status;
    uint16_t unLen;
    uint8_t  i, ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = auth_mode;
    ucComMF522Buf[1] = addr;
    for (i = 0; i < 6; i++) {
        ucComMF522Buf[i + 2] = *(pKey + i);
    }
    for (i = 0; i < 6; i++) {  // i < 4 因为序列号只有4个字节
        ucComMF522Buf[i + 8] = *(pSnr + i);
    }
    //   memcpy(&ucComMF522Buf[2], pKey, 6);
    //   memcpy(&ucComMF522Buf[8], pSnr, 4);

    // 验证密钥
    status = PcdComMF522(PCD_AUTHENT, ucComMF522Buf, 12, ucComMF522Buf, &unLen);
    if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08))) {
        status = MI_ERR;
    }
    return status;
}

/*****************************************************
** @函数名：PcdRead
** @描述  ：读取M1卡一块数据
** @参数  ：addr[IN]：块地址
**          pData[OUT]：读出的数据，16字节
** @返回值：成功返回MI_OK
*****************************************************/
int8_t PcdRead(uint8_t addr, uint8_t *pData)
{
    int8_t   status;
    uint16_t unLen;
    uint8_t  i, ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_READ; //   读块   0x30
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);
    if ((status == MI_OK) && (unLen == 0x90)) // 读取回来共 144 bits == 18 Byte
                                              //   {   memcpy(pData, ucComMF522Buf, 16);   }
    {
        for (i = 0; i < 16; i++) {*(pData + i) = ucComMF522Buf[i];   }
    } else {status = MI_ERR;   }

    return status;
}

/*****************************************************
** @函数名：PcdWrite
** @描述  ：写数据到M1卡一块
** @参数  ：addr[IN]：块地址
**          pData[IN]：写入的数据，16字节
** @返回值：成功返回MI_OK
*****************************************************/
int8_t PcdWrite(uint8_t addr, uint8_t *pData)
{
    int8_t   status;
    uint16_t unLen;
    uint8_t  i, ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_WRITE;  // 0xA0 写块
    ucComMF522Buf[1] = addr;        // 块地址
                                    // CRC校验实际数值 LSB MSB分别存储在 ucComMF522Buf[2] ucComMF522Buf[3]
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    // 发送并接收数据
    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A)) {status = MI_ERR;   }

    if (status == MI_OK) {
        //memcpy(ucComMF522Buf, pData, 16);
        for (i = 0; i < 16; i++) {
            ucComMF522Buf[i] = *(pData + i);
        }

        CalulateCRC(ucComMF522Buf, 16, &ucComMF522Buf[16]);
        status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 18, ucComMF522Buf, &unLen);
        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A)) {status = MI_ERR;   }
    }

    return status;
}

/*****************************************************
** @函数名：PcdHalt
** @描述  ：命令卡片进入休眠状态
** @参数  ：无
** @返回值：成功返回MI_OK
*****************************************************/
int8_t PcdHalt(void)
{
    int8_t   status;
    uint16_t unLen;
    uint8_t  ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_HALT;  // 0x50 休眠
    ucComMF522Buf[1] = 0;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    return status;
}

/*****************************************************
** @函数名：CalulateCRC
** @描述  ：用MF522计算CRC16函数
** @参数  ：无
** @返回值：无
*****************************************************/
void CalulateCRC(uint8_t *pIndata, uint8_t len, uint8_t *pOutData)
{
    uint8_t  i, n;

    ClearBitMask(DivIrqReg, 0x04);   // 清零 当CRC数据位有效且所有数据被处理时置位
    WriteRawRC(CommandReg, PCD_IDLE); // 发送空闲命令
    SetBitMask(FIFOLevelReg, 0x80);  // FIFO读写指令指针清0，错误与溢出标志位清0

    for (i = 0; i < len; i++) {
        WriteRawRC(FIFODataReg, *(pIndata + i)); // 将传入数据放入FIFO中
    }
    WriteRawRC(CommandReg, PCD_CALCCRC);       // 发送CRC计算

    i = 0xFF;  // 0xFF;
    do {
        n = ReadRawRC(DivIrqReg);
        i--;
    }while ((i != 0) && !(n & 0x04)); // 退出条件：
                                      // 1、自变量 i 自减到0
                                      // 2、CRC所有数据位有效且所有数据被处理时置位
                                      // 读取CRC校验结果 CRC计算的实际 MSB LSB 数值
    delay_us(10); // 2000  100 适合读写
    pOutData[0] = ReadRawRC(CRCResultRegL);
    pOutData[1] = ReadRawRC(CRCResultRegM);
}

/*****************************************************
** @函数名：PcdReset
** @描述  ：复位RC522
**          复位RC522、设置定时器预分频3390，
**          定时器重载数值30 定时时间为 15ms
** @参数  ：无
** @返回值：成功返回MI_OK
*****************************************************/
int8_t PcdReset(void)
{
//    MFRC_DIS();
//    delay_us(100);
//    MFRC_EN();
//
//    delay_us(10);
//    MFRC_RST_EN();
//    delay_us(10);
//    MFRC_RST_DIS();

//    delay_us(10);
    WriteRawRC(CommandReg, PCD_RESETPHASE);
//    delay_us(10);

    WriteRawRC(ModeReg, 0x3D);            //和Mifare卡通讯，CRC初始值0x6363
    WriteRawRC(TReloadRegL, 30);
    WriteRawRC(TReloadRegH, 0);           // Timer Reload Value 30
    WriteRawRC(TModeReg, 0x8D);
    WriteRawRC(TPrescalerReg, 0x3E);      // Timer F Div is 3390

    WriteRawRC(TxAutoReg, 0x40); //必须要

    return MI_OK;
}

/*****************************************************
** @函数名：M500PcdConfigISOType
** @描述  ：设置RC632的工作方式
** @参数  ：卡工作类型
** @返回值：成功返回MI_OK
*****************************************************/
int8_t M500PcdConfigISOType(uint8_t type)
{
    if (type == 'A') {                     //ISO14443_A
        ClearBitMask(Status2Reg, 0x08);
        WriteRawRC(ModeReg, 0x3D); //3F
        WriteRawRC(RxSelReg, 0x86); //84
        WriteRawRC(RFCfgReg, 0x7F);   //4F
        WriteRawRC(TReloadRegL, 30); //tmoLength);// TReloadVal = 'h6a =tmoLength(dec)
        WriteRawRC(TReloadRegH, 0);
        WriteRawRC(TModeReg, 0x8D);
        WriteRawRC(TPrescalerReg, 0x3E);
        delay_us(1000);
        PcdAntennaOn();
    } else {
        return -1;
    }

    return MI_OK;
}

/*****************************************************
** @函数名：ReadRawRC
** @描述  ：读RC632寄存器
** @参数  ：Address[IN]:寄存器地址
** @返回值：读出的值
*****************************************************/
uint8_t ReadRawRC(uint8_t Address)
{
    uint8_t ucAddr;
    uint8_t wbuf[2], rbuf[2] = {0xff, 0xff};

    MFRC_CS_ON();
    ucAddr = ((Address << 1) & 0x7E) | 0x80;
    wbuf[0] = ucAddr;
    wbuf[1] = 0xFF;
    nrf_drv_spi_transfer(&mspi, wbuf, 2, rbuf, 2);
    MFRC_CS_OFF();

    return rbuf[1];
}

/*****************************************************
** @函数名：WriteRawRC
** @描述  ：写RC632寄存器
** @参数  ：Address[IN]:寄存器地址
**          value[IN]:写入的值
** @返回值：无
*****************************************************/
void WriteRawRC(uint8_t Address, uint8_t value)
{
    uint8_t ucAddr, buf[2];

    MFRC_CS_ON();
    ucAddr = ((Address << 1) & 0x7E);
    buf[0] = ucAddr;
    buf[1] = value;
    nrf_drv_spi_transfer(&mspi, buf, 2, NULL, 0);
    MFRC_CS_OFF();
}

/*****************************************************
** @函数名：SetBitMask
** @描述  ：置RC522寄存器位
** @参数  ：reg[IN]:寄存器地址
**          mask[IN]:置位值
** @返回值：无
*****************************************************/
void SetBitMask(uint8_t reg, uint8_t mask)
{
    int8_t tmp = 0x0;

    tmp = ReadRawRC(reg);
    WriteRawRC(reg, tmp | mask);  // set bit mask
}

/*****************************************************
** @函数名：ClearBitMask
** @描述  ：清RC522寄存器位
** @参数  ：reg[IN]:寄存器地址
**          mask[IN]:清位值
** @返回值：无
*****************************************************/
void ClearBitMask(uint8_t reg, uint8_t mask)
{
    int8_t tmp = 0x00;

    tmp = ReadRawRC(reg);
    WriteRawRC(reg, tmp & ~mask);  // clear bit mask
}

/*****************************************************
** @函数名：PcdComMF522
** @描述  ：通过RC522和ISO14443卡通讯
** @参数  ：Command[IN]:RC522命令字
**          pInData[IN]:通过RC522发送到卡片的数据
**          InLenByte[IN]:发送数据的字节长度
**          pOutData[OUT]:接收到的卡片返回数据
**         *pOutLenBit[OUT]:返回数据的位长度
** @返回值：成功返回MI_OK
*****************************************************/
int8_t PcdComMF522(uint8_t   Command,
                   uint8_t  *pInData,
                   uint8_t   InLenByte,
                   uint8_t  *pOutData,
                   uint16_t *pOutLenBit
                  )
{
    int8_t   status = MI_ERR;
    uint8_t  irqEn   = 0x00;
    uint8_t  waitFor = 0x00;
    uint8_t  lastBits;
    uint8_t  n;
    uint16_t i;

    switch (Command) {
    case PCD_AUTHENT:      // 验证密钥 验证密钥中只以送了数据，未返回密钥成功与否
        irqEn   = 0x12;
        waitFor = 0x10;
        break;
    case PCD_TRANSCEIVE:   // 发送并接收数据
        irqEn   = 0x77;
        waitFor = 0x30;
        break;
    default:
        break;
    }

    WriteRawRC(ComIEnReg, irqEn | 0x80);   // 设置中断请求允许控制
    ClearBitMask(ComIrqReg, 0x80);
    WriteRawRC(CommandReg, PCD_IDLE);    // 取消当前指令 发送空闲指令
    SetBitMask(FIFOLevelReg, 0x80);      // 清0内部FIFO读写寄存器指针,及错误与溢出标志位

    for (i = 0; i < InLenByte; i++) {
        WriteRawRC(FIFODataReg, pInData[i]);
    } // 将发送数据转存入FIFO中，data <= 64
    WriteRawRC(CommandReg, Command);              // 启动发送并允许接收指令

    if (Command == PCD_TRANSCEIVE) {                                             // 如果为验证密钥则不用接收数据                // 启动该命令发送后能接收数据
        SetBitMask(BitFramingReg, 0x80);           // 如果为发送指令，则启动发送命令
    }          // 该命令只与发送指令一起使用才有效，发送指令在上一句代码中

    //i = 600; //根据时钟频率调整，操作M1卡最大等待时间25ms
    i = 2000; //2000;
              // SetBitMask(ControlReg,0x40);           // start timer now --- Tiger.Yang
    do {
        n = ReadRawRC(ComIrqReg);
        i--;
    }while ((i != 0) && !(n & 0x01) && !(n & waitFor));  // 等待发送数据结束 结束条件
                                                         // 1、i 自减到0
                                                         // 2、定时器自减到0 但定时器未开
                                                         // 3、接收到有效数据流或有一个空闲命令

    ClearBitMask(BitFramingReg, 0x80);             // 停止发送指令

    if (i != 0) {                                     // 非计数变量 i 自减到0退出
        if (!(ReadRawRC(ErrorReg) & 0x1B)) {           // 操作未出现错误
            status = MI_OK;
            if (n & irqEn & 0x01) {                 // 定时器非自减到 0
                status = MI_NOTAGERR;
            }
            if (Command == PCD_TRANSCEIVE) {        // 发送并接收数据
                                                    // 写 FIFODataReg 时该字段值增加 发送阶段检测现有多少数据已发送
                                                    // 读 FIFODataReg 时该字段值减少 接收阶段检测还有多少数据未接收
                                                    // 读取接收字节数 Max = 64  FIFO = 64
                n = ReadRawRC(FIFOLevelReg);
                // 显示最后接收的数据有效位数，如果是0，则该字节都有效
                lastBits = ReadRawRC(ControlReg) & 0x07;

                // 计算一共接收bit数量
                if (lastBits) {
                    *pOutLenBit = (n - 1) * 8 + lastBits;
                } else {
                    *pOutLenBit = n * 8;
                }

                if (n == 0) {     // 如果接收到0个数据，则 n 置1
                    n = 1;
                }
                if (n > MAXRLEN) { // 如果接收数据超过设置接收最大值，则只取前接收最大值数据有效
                    n = MAXRLEN;
                }
                for (i = 0; i < n; i++) { // 从接收FIFO中读取数据出来
                    pOutData[i] = ReadRawRC(FIFODataReg);
                }
            }
        } else {
            status = MI_ERR;
        }
    }
    SetBitMask(ControlReg, 0x80);           // stop timer now
    WriteRawRC(CommandReg, PCD_IDLE);       // 发送一个空闲命令
    return status;
}

/*****************************************************
** @函数名：PcdAntennaOn
** @描述  ：开启天线
**          每次启动或关闭天线发射之间应至少有1ms的间隔
** @参数  ：无
** @返回值：无
*****************************************************/
void PcdAntennaOn(void)
{
    uint8_t i;

    i = ReadRawRC(TxControlReg);
    if (!(i & 0x03)) {
        SetBitMask(TxControlReg, 0x03);
    }
}

/*****************************************************
** @函数名：PcdAntennaOff
** @描述  ：关闭天线
**          清除屏蔽位为0 该代码中清除 TxControlReg
**          寄存器的0x03位 即低两位
** @参数  ：无
** @返回值：无
*****************************************************/
void PcdAntennaOff(void)
{
    ClearBitMask(TxControlReg, 0x03);
}

