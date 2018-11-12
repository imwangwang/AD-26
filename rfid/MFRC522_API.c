#include "MFRC522_API.h"
#include "includes.h"

uint8_t PassWd[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };       //用于访问的密码（用户密码）
																																	//{0x00,0x00,0x00,0x00,0x00,0x00};
																																	//{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};       // 默认密码(密钥A) - 不能读出，能改写

uint8_t MLastSelectedSnr[4];      // 存储 卡片序列号


uint8_t NewKey[16] = { 0x01, 0xAB, 0x23, 0xCD, 0x45, 0xEF,
	0xff, 0x07, 0x80, 0x69,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};   // 新密码  应该是每扇区最后一个权限
		 // 密钥A(6Byte) + 权限(4Byte) + 密钥B(6Byte)

static const nrf_drv_spi_t mspi = NRF_DRV_SPI_INSTANCE(0);  /**< SPI instance. */

//const uint8_t PassWord_Default[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};           //默认的密码（卡原始密码）
//const uint8_t PassWord_user[6] = {0x01,0xAB,0x23,0xCD,0x45,0xEF};              //修改的密码（发卡后用户使用的密码）

/**
 * SPI falsh调试命令
 * @param argc 参数项数量
 * @param argv 参数列表
 */
static void rfid_Console(int argc, char* argv[]) {
	argc--;
	argv++;

	if (ARGV_EQUAL("readid")) {
//		uint8_t buf[16];
//		if (RfidReadData(&buf[0], 2, 1, 0)) {
//			DBG_LOG("RFID read card number:%#x.", *((uint32_t*)&MLastSelectedSnr[0]));
//		} else {
//			DBG_LOG("RFID not read card.");
//		}
		if (StartLinkRc522()) {
			DBG_LOG("RFID read card number:%#x.", *((uint32_t*)&MLastSelectedSnr[0]));
		} else {
			DBG_LOG("RFID not read card.");
		}

	} else if (ARGV_EQUAL("init")) {
		PcdReset();
		PcdAntennaOff();
		PcdAntennaOn();
		M500PcdConfigISOType('A');
		DBG_LOG("RFID device init.");
	} else if (ARGV_EQUAL("poweron")) {
		MFRC_EN();
		DBG_LOG("RFID poweron.");
	} else if (ARGV_EQUAL("poweroff")) {
		MFRC_DIS();
		DBG_LOG("RFID poweroff.");
	}
}

/*****************************************************
** @函数名：InitRc522
** @描述  ：初始化 RC522
** @参数  ：无
** @返回值：无
*****************************************************/
void MFRC522_Init(void) {
	nrf_drv_spi_config_t spi_config;
	
	MFRC_CS_OFF();

	nrf_drv_spi_uninit(&mspi);
	spi_config.frequency = NRF_DRV_SPI_FREQ_8M;
	spi_config.bit_order = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;
	spi_config.mode = NRF_DRV_SPI_MODE_0;
	spi_config.orc = 0xFF;
	spi_config.irq_priority = 0;

	spi_config.miso_pin = SPI_MISO_PIN;
	spi_config.mosi_pin = SPI_MOSI_PIN;
	spi_config.sck_pin = SPI_SCK_PIN;
	spi_config.ss_pin = NRF_DRV_SPI_PIN_NOT_USED;
	nrf_drv_spi_init(&mspi, &spi_config, NULL);

	MFRC_EN();
	nrf_delay_ms(2);

	PcdReset();                         //复位RC522
	PcdAntennaOff();                    //关闭天线
	PcdAntennaOn();                     //开启天线
	M500PcdConfigISOType('A');          //配置为ISO14443_A模式
	MFRC_DIS();

	CMD_ENT_DEF(rfid, rfid_Console);
	Cmd_AddEntrance(CMD_ENT(rfid));

	DBG_LOG("RFID init.");
}

/*****************************************************
** @函数名：StartLinkRc522
** @描述  ：开始连接启动 RC522
** @参数  ：无
** @返回值：成功返回1
*****************************************************/
uint8_t LinkMFRC522(uint8_t* SN) {
	uint8_t  status;
	uint8_t  ucRFIDCardSVN[6];

	PcdReset(); // 复位RC522、设置定时器预分频3390，定时器重载数值30 定时时间为 15ms
	status = PcdRequest(PICC_REQIDL, &ucRFIDCardSVN[0]);
	if (status != MI_OK) {
		return 0;
	}
	status = PcdAnticoll(&ucRFIDCardSVN[2]);
	if (status != MI_OK)    {
		return 0;
	}
	memcpy(SN, &ucRFIDCardSVN[2], 4);

	status = PcdSelect(SN); // 选卡
	if (status != MI_OK) {
		return 0;
	}
	return 1;
}

/*****************************************************
** @函数名：StartLinkRc522
** @描述  ：开始连接启动 RC522
** @参数  ：无
** @返回值：成功返回1
*****************************************************/
uint8_t StartLinkRc522(void) {
	uint8_t ucNum;
	int8_t  status;
	uint8_t ucRFIDCardSVN[6];

	PcdReset(); // 复位RC522、设置定时器预分频3390，定时器重载数值30 定时时间为 15ms

	// 寻天线区内未进入休眠状态的卡
	// 返回卡片类型 2字节 卡片类型位于 0扇区0块 Byte6 -- Byte7
	// 存储在   ucRFIDCardSVN[0] ucRFIDCardSVN[1] 中
	status = PcdRequest(PICC_REQIDL, &ucRFIDCardSVN[0]);
	if (status != MI_OK) {
		return 0;
	}

	// 防冲撞，返回卡的序列号 4字节 序列号为：0扇区0块的 Byte0 -- Byte3
	// 返回成功则卡片序列号存储在 RevBuffer[2] RevBuffer[3] RevBuffer[4] RevBuffer[5] 中
	status = PcdAnticoll(&ucRFIDCardSVN[2]);
	if (status != MI_OK)    {
		return 0;
	}
	// 将卡片序列号转存于 MLastSelectedSnr 全局量中
	for (ucNum = 0; ucNum < 4; ucNum++) {
		MLastSelectedSnr[ucNum] = ucRFIDCardSVN[2 + ucNum];
	}

	status = PcdSelect(MLastSelectedSnr); // 选卡
	if (status != MI_OK) {
		return 0;
	}

	return 1;
}

/*****************************************************
** @函数名：RC522SendSnr
** @描述  ：从RFID卡中读取序列号
** @参数  ：None
** @返回值：成功返回1
*****************************************************/
uint8_t RC522SendSnr(void) {
	uint8_t ucRecDat = 0;

	ucRecDat = StartLinkRc522();
	if (1 != ucRecDat) {
//      OperateFailure(LINKCARD);
		return 0;
	}

//  OperateSuccess(MCUSENDSNR, 4);
//  for(ucRecDat = 0; ucRecDat < 4; ucRecDat++)
//  {
//      UART1_TxChar(MLastSelectedSnr[ucRecDat]);
//  }
	PcdHalt();                   // 系统休眠模式

	return 1;
}

/*****************************************************
** @函数名：RC522SnrPassward
** @描述  ：从RFID卡中读取序列号，将序列号生成6位密码
** @参数  ：NewKey:新密码
** @返回值：成功返回1
*****************************************************/
uint8_t RC522SnrPassward(void) {
	uint8_t ucNum    = 0;
	uint8_t DealData = 0;

	if (StartLinkRc522()) {
		for (ucNum = 0; ucNum < 4; ucNum++) {
			NewKey[ucNum] = MLastSelectedSnr[ucNum];
			NewKey[10 + ucNum] = 0xFF - MLastSelectedSnr[ucNum];
		}

		DealData = MLastSelectedSnr[0];
		for (ucNum = 1; ucNum < 4; ucNum++) {
			DealData ^= MLastSelectedSnr[ucNum];
		}
		NewKey[4] = DealData;
		NewKey[14] = 0xFF - DealData;

		DealData = MLastSelectedSnr[0];
		for (ucNum = 1; ucNum < 4; ucNum++) {
			DealData += MLastSelectedSnr[ucNum];
		}
		NewKey[5] = 0xFF - DealData;
		NewKey[15] = 0xFF - DealData;

		PcdHalt(); // 系统休眠模式
		return 1;
	}
	PcdHalt();   // 系统休眠模式
	return 0;
}

/*****************************************************
** @函数名：RC522ModifyPsw
** @描述  ：修改RFID卡指定块访问密码
** @参数  ：KuaiN  : 修改块
**          NewKey : 修改后的新密码
** @返回值：成功返回1
*****************************************************/
uint8_t RC522ModifyPsw(uint8_t KuaiN) {
	uint8_t ucRecDat = 0;

	ucRecDat = StartLinkRc522();
	if (1 != ucRecDat) {
//      OperateFailure(LINKCARD);
		return 0;
	}

	ucRecDat = PcdAuthState(PICC_AUTHENT1A, KuaiN, PassWd, MLastSelectedSnr);
	if (ucRecDat != MI_OK) {
//        OperateFailure(CHECKPSW);
		return 0;
	}

	ucRecDat = PcdWrite(KuaiN, &NewKey[0]); // 写入修改之后密码数据
	if (ucRecDat != MI_OK) {
//    OperateFailure(MCUMODIFYPSW);
		return 0;
	}
	PcdHalt();                             // 休眠
//  OperateSuccess(MCUMODIFYPSW, 0);

	return 1;
}

/*****************************************************
** @函数名：RfidReadData
** @描述  ：从RFID卡中读取数据
** @参数  ：pucKuaiDat   读取块数据
**          ucKuaiNum    RFID 读取块开始地址
**          ucKuaiLen    RFID 读取块的长度
**          ucContiFlg   读取块数据是否连续
**                        0 只读取数据块
**                        1 数据块与控制块一起读
** @返回值：成功返回1
*****************************************************/
uint8_t RfidReadData(uint8_t* pucKuaiDat,
										 uint8_t  ucKuaiNum,
										 uint8_t  ucKuaiLen,
										 uint8_t  ucContiFlg
										) {
	uint8_t ucStatus;
	uint8_t ucStartKuai;
	uint8_t ucKuaiLength;
	uint8_t ucNum;
	uint8_t ucDatNum;
	uint8_t ucPswFlg;     // 验证密码标志
	uint8_t ucReadDat[16];

	ucPswFlg     = 0;
	ucStartKuai  = ucKuaiNum;
	ucKuaiLength = ucKuaiLen;

	ucStatus = StartLinkRc522(); // 链接RDIF
	if (1 != ucStatus) {
		PcdHalt();
		return 0;
	}

	for (ucNum = 0; ucNum < ucKuaiLength; ucNum++, ucStartKuai++) {

		// 判断是数据块还是控制块
		if (ucStartKuai < MAXAREASTA) { // 小区数据
			if (MINCTRLKUAI == ucStartKuai % MINAREALEN) {
				ucPswFlg = 1;
			}
		} else {                  // 大区数据
			if (MAXCTRLKUAI == ucStartKuai % MAXAREALEN) {
				ucPswFlg = 1;
			}
		}

		// 判断是否连续读取
		if ((!ucContiFlg) && (ucPswFlg)) { // 只读取数据块
			ucStartKuai++;
		}

		//KuaiN = ucStartKuai;
		if ((!ucNum) || (ucPswFlg)) {
			ucPswFlg = 0;
			ucStatus = PcdAuthState(PICC_AUTHENT1A, ucStartKuai, PassWd, MLastSelectedSnr); // 验证卡片密码
			if (ucStatus != MI_OK) {
				PcdHalt();
				return 0;
			}
		}

		ucStatus = PcdRead(ucStartKuai, ucReadDat);
		if (ucStatus != MI_OK) {
			PcdHalt();
			return 0;
		} else if (pucKuaiDat != NULL) {
			for (ucDatNum = 0; ucDatNum < 16; ucDatNum++) {
				pucKuaiDat[ucNum * 16 + ucDatNum] = ucReadDat[ucDatNum];
			}
		}
	}
	PcdHalt();
	return 1;
}

/*****************************************************
** @函数名：RfidWriteData
** @描述  ：从RFID卡中连续区中写入数据
** @参数  ：ucKuaiNum    RFID 写数据块开始地址
**          ucKuaiLen    RFID 写数据块的长度
**         *pucDat       写入的数据
** @返回值：成功返回1
*****************************************************/
uint8_t RfidWriteData(uint8_t ucKuaiNum,
											uint8_t ucKuaiLen,
											uint8_t* pucDat
										 ) {
	uint8_t ucStatus;
	uint8_t ucStartKuai;
	uint8_t ucKuaiLength;
	uint8_t ucNum;
	uint8_t ucDatNum;
	uint8_t ucPswFlg;     // 验证密码标志
	uint8_t ucWriteDat[16];

	ucPswFlg     = 0;
	ucStartKuai  = ucKuaiNum;
	ucKuaiLength = ucKuaiLen;

	ucStatus = StartLinkRc522(); // 链接RDIF
	if (1 != ucStatus) {
		PcdHalt();
		return 0;
	}

	for (ucNum = 0; ucNum < ucKuaiLength; ucNum++, ucStartKuai++) {
		// 判断控制块
		if (ucStartKuai < MAXAREASTA) { // 小区数据
			if (MINCTRLKUAI == ucStartKuai % MINAREALEN) {
				ucPswFlg = 1;
			}
		} else {                  // 大区数据
			if (MAXCTRLKUAI == ucStartKuai % MAXAREALEN) {
				ucPswFlg = 1;
			}
		}

		// 判断是否连续读取
		if (ucPswFlg) { // 只读取数据块
			ucStartKuai++;
		}

		//KuaiN = ucStartKuai;
		if ((!ucNum) || (ucPswFlg)) {
			ucPswFlg = 0;
			ucStatus = PcdAuthState(PICC_AUTHENT1A, ucStartKuai, PassWd, MLastSelectedSnr); // 验证卡片密码
			if (ucStatus != MI_OK) {
				PcdHalt();
				return 0;
			}
		}

		for (ucDatNum = 0; ucDatNum < 16; ucDatNum++) {
			ucWriteDat[ucDatNum] = *pucDat++; // 写入16位数据
		}

		ucStatus = PcdWrite(ucStartKuai, &ucWriteDat[0]);
		if (ucStatus != MI_OK) {
			PcdHalt();
			return 0;
		}
	}
	PcdHalt();
	return 1;
}




