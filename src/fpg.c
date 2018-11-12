/**
	******************************************************************************
	* @file    fpg.c
	* @author  宋阳
	* @version V1.0
	* @date    2017.12.15
	* @brief   指纹传感器驱动理相关函数.
	*
	******************************************************************************
	*/


/* Includes ------------------------------------------------------------------*/
#include "includes.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void fpg_SendCmd(uint8_t cmd, uint8_t* arg, uint16_t arglen);
static void fpg_SendData(uint8_t type, uint8_t* data, uint16_t datalen);
static uint8_t fpg_ReadWaitAck(uint8_t* arg,  uint16_t timeout);
static uint8_t fpg_ReadWaitData(uint8_t* buff, uint16_t buflen, uint16_t timeout);
static void fpg_Console(int argc, char* argv[]);

/* Exported functions --------------------------------------------------------*/

/**
 * 指纹传感器初始化
 */
void FPG_init(void) {
	CMD_ENT_DEF(fpg, fpg_Console);
	Cmd_AddEntrance(CMD_ENT(fpg));

#if DEBUG == 0
	nrf_uart_txrx_pins_set(NRF_UART0, FPG_TX_PIN, FPG_RX_PIN);
#endif

	DBG_LOG("FPG init.");
}

/**
 * 录入图像
 * 功能说明：探测手指，探测到后录入指纹图像存于ImageBuffer。返回确认码表示录入成功、
 * 无手指等。
 *
 * @return 录入成功返回TRUE
 */
uint8_t FPG_PS_GetImage(void) {
	fpg_SendCmd(PS_GetImage, NULL, 0);
	return fpg_ReadWaitAck(NULL, 1000);
}

/**
 * 生成特征
 * 功能说明：将ImageBuffer中的原始图像生成指纹特征文件存于CharBuffer1或CharBuffer2
 *
 * @param charbuffer 1为缓冲区CharBuffer1，2为CharBuffer2
 * @return 生成成功返回TRUE
 */
uint8_t FPG_PS_GenChar(uint8_t charbuffer) {
	fpg_SendCmd(PS_GenChar, &charbuffer, 1);
	return fpg_ReadWaitAck(NULL, 1000);
}

/**
 * 精确比对两枚指纹特征
 * 功能说明：精确比对CharBuffer1 与CharBuffer2
 * 中的特征文件
 *
 * @param pscore 得分
 * @return 指纹匹配返回TRUE
 */
uint8_t FPG_PS_Match(uint16_t* pscore) {
	uint8_t buf[2], ret;

	fpg_SendCmd(PS_Match, NULL, 0);
	ret = fpg_ReadWaitAck(buf, 1000);
	if (ret == 0) {
		*pscore = (buf[0] << 8) | buf[1];
	}
	return ret;
}

/**
 * 搜索指纹
 * 功能说明：以CharBuffer1或CharBuffer2中的特征文件搜索整个或部分指纹库。若搜索到，
 * 则返回页码。
 *
 * @param charbuffer 1为缓冲区CharBuffer1，2为CharBuffer2
 * @param startpage  起始页
 * @param pagenum    页数
 * @param pageid     页码
 * @param pscore     得分
 * @return 搜索成功返回TRUE
 */
uint8_t FPG_PS_Search(uint8_t charbuffer, uint16_t startpage, uint16_t pagenum, uint16_t* pageid, uint16_t* pscore) {
	uint8_t buf[5], ret;

	buf[0] = charbuffer;
	buf[1] = (uint8_t)(startpage >> 8);
	buf[2] = (uint8_t)(startpage >> 0);
	buf[3] = (uint8_t)(pagenum >> 8);
	buf[4] = (uint8_t)(pagenum >> 0);

	fpg_SendCmd(PS_Search, buf, 5);
	ret = fpg_ReadWaitAck(buf, 3000);
	if (ret == 0 && pageid != NULL && pscore != NULL) {
		*pageid = (buf[0] << 8) | buf[1];
		*pscore = (buf[2] << 8) | buf[3];
	}
	return ret;
}

/**
 * 合并特征（生成模板）： PS_RegModel
 * 功能说明：将CharBuffer1与CharBuffer2中的特征文件合并生成模板，结果存于
 * CharBuffer1与CharBuffer2
 *
 * @return 合并成功返回TRUE
 */
uint8_t FPG_PS_RegModel(void) {
	fpg_SendCmd(PS_RegModel, NULL, 0);
	return fpg_ReadWaitAck(NULL, 1000);
}

/**
 * 储存模板：PS_StoreChar
 * 功能说明：将CharBuffer1或CharBuffer2中的模板文件存到PageID号flash数据库位置。
 * 输入参数：BufferID(缓冲区号)，PageID（指纹库位置号）
 *
 * @param charbuffer 缓冲区号
 * @param pageid     指纹库位置号
 * @return 储存成功成功返回TRUE
 */
uint8_t FPG_PS_StoreChar(uint8_t charbuffer, uint16_t pageid) {
	uint8_t buf[3], ret;

	buf[0] = charbuffer;
	buf[1] = (uint8_t)(pageid >> 8);
	buf[2] = (uint8_t)(pageid >> 0);

	fpg_SendCmd(PS_StoreChar, buf, 3);
	ret = fpg_ReadWaitAck(NULL, 1000);
	return ret;
}

/**
 * 读出模板：PS_LoadChar
 * 功能说明：将flash数据库中指定ID号的指纹模板读入到模板缓冲区CharBuffer1或
 * CharBuffer2
 *
 * @param charbuffer 缓冲区号
 * @param pageid     指纹库模板号
 * @return 执行成功返回TRUE
 */
uint8_t FPG_PS_LoadChar(uint8_t charbuffer, uint16_t pageid) {
	uint8_t buf[3];

	buf[0] = charbuffer;
	buf[1] = (uint8_t)(pageid >> 8);
	buf[2] = (uint8_t)(pageid >> 0);

	fpg_SendCmd(PS_LoadChar, buf, 3);
	return fpg_ReadWaitAck(NULL, 1000);
}

/**
 * 上传特征或模板：PS_UpChar
 * 功能说明：将特征缓冲区中的特征文件上传给上位机
 *
 * @param charbuffer 缓冲区号
 * @param data 读出的指针
 * @return 执行成功返回TRUE
 */
uint8_t FPG_PS_UpChar(uint8_t charbuffer, uint8_t* data) {
	uint8_t ret;

	fpg_SendCmd(PS_UpChar, &charbuffer, 1);
	ret = fpg_ReadWaitAck(NULL, 1000);
	if (ret == 0) {
		if (fpg_ReadWaitData(data, 256, 1000) == 0) {
			fpg_ReadWaitData(data + 256, 256, 1000);
		}
	}
	return ret;
}

/**
 * 下载特征或模板：PS_DownChar
 * 功能说明：上位机下载特征文件到模块的一个特征缓冲区
 *
 * @param charbuffer 缓冲区号
 * @param data       写入的数据
 * @return 执行成功返回TRUE
 */
uint8_t FPG_PS_DownChar(uint8_t charbuffer, uint8_t* data) {
	uint8_t ret;

	fpg_SendCmd(PS_DownChar, &charbuffer, 1);
	ret = fpg_ReadWaitAck(NULL, 1000); 
	if (ret == 0) {
		fpg_SendData(PS_TYPE_DATA_LAST, data, 256);
		/*延时避免UART FIFO溢出*/
		nrf_delay_ms(10);
		fpg_SendData(PS_TYPE_DATA_LAST, data + 256, 256);
	}
	return ret;
}

/**
 * 上传图像：PS_UpImage
 * 功能说明：将图像缓冲区中的数据上传给上位机
 *
 * @return 执行成功返回TRUE
 */
BOOL FPG_PS_UpImage(void) {
	fpg_SendCmd(PS_UpImage, NULL, 0);
	if (fpg_ReadWaitAck(NULL, 1000) == 0) {
		//补充图像文件发送
		return TRUE;
	}
	return FALSE;
}

/**
 * 下载图像：PS_DownImage
 * 功能说明：上位机下载图像数据给模块
 *
 * @return 执行成功返回TRUE
 */
BOOL FPG_PS_DownImage(void) {
	fpg_SendCmd(PS_DownImage, NULL, 0);
	if (fpg_ReadWaitAck(NULL, 1000) == 0) {
		//补充图像文件接收
		return TRUE;
	}
	return FALSE;
}

/**
 * 删除模板：PS_DeletChar
 * 功能说明：删除flash 数据库中指定ID号开始的N个指纹模板
 *
 * @param pageid 指纹库模板号
 * @param number 删除的模板个数
 * @return 执行成功返回TRUE
 */
uint8_t FPG_PS_DeletChar(uint16_t pageid, uint16_t number) {
	uint8_t buf[4];

	buf[0] = (uint8_t)(pageid >> 8);
	buf[1] = (uint8_t)(pageid >> 0);
	buf[2] = (uint8_t)(number >> 8);
	buf[3] = (uint8_t)(number >> 0);

	fpg_SendCmd(PS_DeletChar, buf, 4);
	return fpg_ReadWaitAck(NULL, 1000);
}

/**
 * 清空指纹库：PS_Empty
 * 功能说明：删除flash 数据库中所有指纹模板
 *
 * @return 执行成功返回TRUE
 */
uint8_t FPG_PS_Empty(void) {
	fpg_SendCmd(PS_Empty, NULL, 0);
	return fpg_ReadWaitAck(NULL, 2000);
}

/**
 * 写系统寄存器：PS_WriteReg
 * 功能说明：写模块寄存器
 *
 * @param reg    寄存器序号
 * @param res    写入内容
 * @return 执行成功返回TRUE
 */
uint8_t FPG_PS_WriteReg(uint8_t reg, uint8_t res) {
	uint8_t buf[2];

	buf[0] = (uint8_t)(reg);
	buf[1] = (uint8_t)(res);

	fpg_SendCmd(PS_WriteReg, buf, 2);
	return fpg_ReadWaitAck(NULL, 1000);
}

/**
 * 读系统基本参数：PS_ReadSysPara
 * 功能说明：读取模块的基本参数（波特率，包大小等）
 * 参数表前16 个字节存放了模块的基本通讯和配置信息，称为模块的基本参数
 *
 * @param par    读出的参数
 * @return 执行成功返回TRUE
 */
BOOL FPG_PS_ReadSysPara(FPG_SysPara_t* par) {
	uint8_t buf[16];

	memset(buf, 0, 12);

	fpg_SendCmd(PS_ReadSysPara, NULL, 0);
	if (fpg_ReadWaitAck(buf, 1000) == 0) {
		DBG_LOG("FPG_PS_ReadSysPara:%#02x,%#02x,%#02x,%#02x,%#02x,%#02x,%#02x,%#02x,%#02x,%#02x,%#02x,%#02x",
						buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);
		//待确认补充
		return TRUE;
	}
	return FALSE;
}

/**
 * 自动注册模板：PS_Enroll
 * 功能说明：采集一次指纹注册模板，在指纹库中搜索空位并存储，返回存储ID
 *
 * @param pageid 注册的页码
 * @return 执行成功返回TRUE
 */
uint8_t FPG_PS_Enroll(uint16_t* pageid) {
	uint8_t buf[2], ret;

	fpg_SendCmd(PS_Enroll, NULL, 0);
	ret = fpg_ReadWaitAck(buf, 3000);
	if (ret == 0) {
		*pageid = (buf[0] << 8) | buf[1];
	}
	return ret;
}

/**
 *  自动验证指纹：PS_Identify
 * 功能说明：自动采集指纹，在指纹库中搜索目标模板并返回搜索结果
 * 如果目标模板同当前采集的指纹比对得分大于最高阀值，并且目标模板为不完
 * 整特征则以采集的特征更新目标模板的空白区域。
 *
 * @param pageid 页码
 * @param pscore 得分
 * @return 执行成功返回TRUE
 */
uint8_t FPG_PS_Identify(uint16_t* pageid, uint16_t* pscore) {
	uint8_t buf[4], ret;

	fpg_SendCmd(PS_Identify, NULL, 0);
	ret = fpg_ReadWaitAck(buf, 3000);
	if (ret == 0) {
		*pageid = (buf[0] << 8) | buf[1];
		*pscore = (buf[2] << 8) | buf[3];
	}
	return ret;
}

/**
 * 设置口令：PS_SetPwd
 * 功能说明：设置模块握手口令
 *
 * @param pwd    口令
 * @return 执行成功返回TRUE
 */
uint8_t FPG_PS_SetPwd(uint32_t pwd) {
	uint8_t buf[4];

	buf[0] = ((uint8_t)(pwd >> 24));
	buf[1] = ((uint8_t)(pwd >> 18));
	buf[2] = ((uint8_t)(pwd >> 8));
	buf[3] = ((uint8_t)pwd);

	fpg_SendCmd(PS_SetPwd, buf, 4);
	return fpg_ReadWaitAck(NULL, 1000);
}

/**
 * 验证口令：PS_VfyPwd
 * 功能说明：验证模块握手口令
 *
 * @param pwd    口令
 * @return 执行成功返回TRUE
 */
uint8_t FPG_PS_VfyPwd(uint32_t pwd) {
	uint8_t buf[4];

	buf[0] = ((uint8_t)(pwd >> 24));
	buf[1] = ((uint8_t)(pwd >> 18));
	buf[2] = ((uint8_t)(pwd >> 8));
	buf[3] = ((uint8_t)pwd);

	fpg_SendCmd(PS_VfyPwd, buf, 4);
	return fpg_ReadWaitAck(NULL, 1000);
}

/**
 * 采样随机数：PS_GetRandomCode
 * 功能说明：令芯片生成一个随机数并返回给上位机
 *
 * @return 执行成功返回TRUE
 */
uint8_t FPG_PS_GetRandomCode(uint32_t* radom) {
	uint8_t buf[4], ret;

	fpg_SendCmd(PS_GetRandomCode, NULL, 0);
	ret = fpg_ReadWaitAck(buf, 1000);
	if (ret == 0) {
		*radom = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3] << 0);
	}
	return ret;
}

/**
 * 读flash信息页：PS_ReadINFpage
 * 功能说明：读取FLASH Information Page 所在的信息页(512bytes)
 *
 * @return 执行成功返回TRUE
 */
BOOL FPG_PS_ReadINFpage(void) {
	fpg_SendCmd(PS_ReadINFpage, NULL, 0);
	if (fpg_ReadWaitAck(NULL, 1000) == 0) {
		//读出数据
		return TRUE;
	}
	return FALSE;
}

/**
 * 高速搜索：PS_HighSpeedSearch
 * 功能说明：以CharBuffer1 或CharBuffer2 中的特征文件高速搜索整个或部分指纹库。若
 * 搜索到，则返回页码。
 * 该指令对于的确存在于指纹库中，且登录时质量很好的指纹，会很快给出搜索
 * 结果。
 *
 * @param charbuffer 1为缓冲区CharBuffer1，2为CharBuffer2
 * @param startpage  起始页
 * @param pagenum    页数
 * @param pageid     页码
 * @param pscore     得分
 * @return 搜索成功返回TRUE
 */
uint8_t FPG_PS_HighSpeedSearch(uint8_t charbuffer, uint16_t startpage, uint16_t pagenum, uint16_t* pageid, uint16_t* pscore) {
	uint8_t buf[5], ret;

	buf[0] = charbuffer;
	buf[1] = (uint8_t)(startpage >> 8);
	buf[2] = (uint8_t)(startpage >> 0);
	buf[3] = (uint8_t)(pagenum >> 8);
	buf[4] = (uint8_t)(pagenum >> 0);

	fpg_SendCmd(PS_HighSpeedSearch, buf, 5);
	ret = fpg_ReadWaitAck(buf, 1000);
	if (ret == 0) {
		*pageid = (buf[0] << 8) | buf[1];
		*pscore = (buf[2] << 8) | buf[3];
	}
	return ret;
}

/**
 * 读有效模板个数：PS_ValidTempleteNum
 * 功能说明：读有效模板个数
 *
 * @param number 读出的模板个数
 * @return 执行成功返回TRUE
 */
uint8_t FPG_PS_ValidTempleteNum(uint16_t* number) {
	uint8_t buf[2], ret;

	fpg_SendCmd(PS_ValidTempleteNum, NULL, 0);
	ret = fpg_ReadWaitAck(buf, 200);
	if (ret == 0 && number != NULL) {
		*number = (buf[0] << 8) | buf[1];
	}
	return ret;
}

/**
 * 获取指纹传感器返回信息字符
 *
 * @param retnum 返回的数值
 * @return 返加字符串指针
 */
char* FPG_GetResultString(uint8_t retnum) {
	switch (retnum) {
		case 0x00:
			return "指令执行完毕或 OK";
		case 0x01:
			return "数据包接收错误";
		case 0x02:
			return "传感器上没有手指";
		case 0x03:
			return "录入指纹图像失败";
		case 0x04:
			return "指纹图像太干、太淡而生不成特征";
		case 0x05:
			return "指纹图像太湿、太模糊而生不成特征";
		case 0x06:
			return "指纹图像太乱而生不成特征";
		case 0x07:
			return "指纹图像正常，但特征点太少（或面积太小）而生不成特征";
		case 0x08:
			return "指纹不匹配";
		case 0x09:
			return "没搜索到指纹";
		case 0x0a:
			return "特征合并失败";
		case 0x0b:
			return "访问指纹库时地址序号超出指纹库范围";
		case 0x0c:
			return "从指纹库读模板出错或无效";
		case 0x0d:
			return "上传特征失败";
		case 0x0e:
			return "模块不能接收后续数据包";
		case 0x0f:
			return "上传图像失败";
		case 0x10:
			return "删除模板失败";
		case 0x11:
			return "清空指纹库失败";
		case 0x12:
			return "不能进入低功耗状态";
		case 0x13:
			return "口令不正确";
		case 0x14:
			return "系统复位失败";
		case 0x15:
			return "缓冲区内没有有效原始图而生不成图像";
		case 0x16 :
			return "在线升级失败";
		case 0x17:
			return "残留指纹或两次采集之间手指没有移动过";
		case 0x18:
			return "读写 FLASH 出错";
		case 0xf0:
			return "有后续数据包的指令，正确接收后用 0xf0 应答";
		case 0xf1:
			return "有后续数据包的指令，命令包用 0xf1 应答";
		case 0xf2:
			return "烧写内部 FLASH 时，校验和错误";
		case 0xf3:
			return "烧写内部 FLASH 时，包标识错误";
		case 0xf4:
			return "烧写内部 FLASH 时，包长度错误";
		case 0xf5:
			return "烧写内部 FLASH 时，代码长度太长";
		case 0xf6:
			return "烧写内部 FLASH 时，烧写 FLASH 失败";
		case 0x19:
			return "未定义错误";
		case 0x1a:
			return "无效寄存器号";
		case 0x1b:
			return "寄存器设定内容错误号";
		case 0x1c:
			return "记事本页码指定错误";
		case 0x1d:
			return "端口操作失败";
		case 0x1e:
			return "自动注册（enroll）失败";
		case 0x1f:
			return "指纹库满";
			/*自定义新增*/
		case 0xfe:
			return "上位机接收校验出错";
		case 0xff:
			return "超时";
		default :
			return "示知错误";
	}
}

/* Private function prototypes -----------------------------------------------*/

/**
 * 指纹传感器发送命令包
 *
 * @param cmd     指令
 * @param arg     参数值
 * @param arglen  参数的长度
 */
static void fpg_SendCmd(uint8_t cmd, uint8_t* arg, uint16_t arglen) {
	uint16_t add = 0, len = 0, i = 0;
#if DEBUG == 1
	/*延时待串口空闲*/
	nrf_delay_ms(10);
	nrf_uart_txrx_pins_set(NRF_UART0, FPG_TX_PIN, FPG_RX_PIN);
#endif

	app_uart_put((uint8_t)(PS_HEAD >> 8));
	app_uart_put((uint8_t)PS_HEAD);

	app_uart_put((uint8_t)(PS_ADDR_DEF >> 24));
	app_uart_put((uint8_t)(PS_ADDR_DEF >> 18));
	app_uart_put((uint8_t)(PS_ADDR_DEF >> 8));
	app_uart_put((uint8_t)PS_ADDR_DEF);

	app_uart_put(PS_TYPE_CMD);
	len = arglen + 3;
	app_uart_put(len >> 8);
	app_uart_put(len);
	app_uart_put(cmd);

	add = PS_TYPE_CMD;
	add += len;
	add += cmd;
	for (i = 0; i < arglen && arg != NULL; i++) {
		app_uart_put(arg[i]);
		add += arg[i];
	}
	app_uart_put(add >> 8);
	app_uart_put(add);

#if DEBUG == 1
	nrf_delay_ms(10);
	nrf_uart_txrx_pins_set(NRF_UART0, TX_PIN_NUMBER, RX_PIN_NUMBER);
#endif
}

/**
 * 指纹传感器发送数据包
 *
 * @param type    数据类型，数据包还是结束包
 * @param data    数据指针
 * @param datalen 数据长度
 */
static void fpg_SendData(uint8_t type, uint8_t* data, uint16_t datalen) {
	uint16_t add = 0, len = 0, i = 0;

#if DEBUG == 1
	/*延时待串口空闲*/
	nrf_delay_ms(10);
	nrf_uart_txrx_pins_set(NRF_UART0, FPG_TX_PIN, FPG_RX_PIN);
#endif

	app_uart_put((uint8_t)(PS_HEAD >> 8));
	app_uart_put((uint8_t)PS_HEAD);

	app_uart_put((uint8_t)(PS_ADDR_DEF >> 24));
	app_uart_put((uint8_t)(PS_ADDR_DEF >> 18));
	app_uart_put((uint8_t)(PS_ADDR_DEF >> 8));
	app_uart_put((uint8_t)PS_ADDR_DEF);

	app_uart_put(type);
	len = datalen + 2;
	app_uart_put(len >> 8);
	app_uart_put(len);

	add = type;
	add += len;
	for (i = 0; i < datalen && data != NULL; i++) {
		app_uart_put(data[i]);
		add += data[i];
	}
	app_uart_put(add >> 8);
	app_uart_put(add);

#if DEBUG == 1
	nrf_delay_ms(100);
	nrf_uart_txrx_pins_set(NRF_UART0, TX_PIN_NUMBER, RX_PIN_NUMBER);
#endif
}

/**
 * 等待指令应答
 *
 * @param arg     读出参数指针
 * @param timeout 超时时间
 * @return 返回确认码
 */
static uint8_t fpg_ReadWaitAck(uint8_t* arg,  uint16_t timeout) {
	uint16_t timeindex = 0, index = 0, len = 0, add = 0;
	uint8_t head[10], addbuf[2], rst = 0x00;

#if DEBUG == 1

	nrf_uart_txrx_pins_set(NRF_UART0, FPG_TX_PIN, FPG_RX_PIN);
#endif

	do {
		if (app_uart_get(&head[index]) == NRF_SUCCESS) {
			index++;
			/*滤除非数据包*/
			if (index == 2 && (head[0] != (PS_HEAD >> 8) || head[1] != (uint8_t)PS_HEAD)) {
				index = 0;
			}
			if (index == 10 && head[6] == PS_TYPE_ACK) {
				len = (head[7] << 8) | head[8];
				rst = head[9];
				add = head[6] + head[7] + head[8] + head[9];
				/*读出参数*/
				len -= 3;
				index = 0;
				while (index < len) {
					if (app_uart_get(&arg[index]) == NRF_SUCCESS) {
						add += arg[index];
						index++;
					}
				}
				/*读出校验*/
				index = 0;
				while (index < 2) {
					if (app_uart_get(&addbuf[index]) == NRF_SUCCESS) {
						index++;
					}
				}
				/*检验校验*/
				if (addbuf[0] != (add >> 8) || addbuf[1] != (uint8_t)add) {
					rst = 0xFE;
				}
				break;
			}
		}
		nrf_delay_ms(1);
		timeindex++;
		WatchDog_Clear();
	} while (timeindex < timeout);

	if (timeindex >= timeout) {
		rst = 0xFF;
	}

#if DEBUG == 1
	nrf_uart_txrx_pins_set(NRF_UART0, TX_PIN_NUMBER, RX_PIN_NUMBER);
#endif
	if (rst != 0) {
		DBG_LOG("fpg_ReadWaitAck error:%s.", FPG_GetResultString(rst));
	}

	return rst;
}

/**
 * 读出数据
 *
 * @param buff    读出来的指针
 * @param buflen  读出的最大长度
 * @param timeout
 * @return 读出成功返回0,超时返回0xFF
 */
static uint8_t fpg_ReadWaitData(uint8_t* buff, uint16_t buflen, uint16_t timeout) {
	uint16_t timeindex = 0, index = 0, len = 0, add = 0;
	uint8_t head[10], addbuf[2], rst = 0x00;

#if DEBUG == 1

	nrf_uart_txrx_pins_set(NRF_UART0, FPG_TX_PIN, FPG_RX_PIN);
#endif

	do {
		if (app_uart_get(&head[index]) == NRF_SUCCESS) {
			index++;
			/*滤除非数据包*/
			if (index == 2 && (head[0] != (PS_HEAD >> 8) || head[1] != (uint8_t)PS_HEAD)) {
				index = 0;
			}
			if (index == 9 && (head[6] == PS_TYPE_DATA || head[6] == PS_TYPE_DATA_LAST)) {
				len = (head[7] << 8) | head[8];
				add = head[6] + head[7] + head[8];
				/*读出数据*/
				len -= 2;
				index = 0;
				if (len > buflen) {
					len = buflen;
					DBG_LOG("fpg_ReadWaitData buf overflow.");
				}
				while (index < len) {
					if (app_uart_get(&buff[index]) == NRF_SUCCESS) {
						add += addbuf[index];
						index++;
					}
				}
				/*读出校验*/
				index = 0;
				while (index < 2) {
					if (app_uart_get(&addbuf[index]) == NRF_SUCCESS) {
						index++;
					}
				}
				/*检验校验*/
				if (addbuf[0] != (add >> 8) || addbuf[1] != (uint8_t)add) {
					rst = 0xFE;
				}
				break;
			}
		}
		nrf_delay_ms(1);
		timeindex++;
	} while (timeindex < timeout);

	if (timeindex >= timeout) {
		rst = 0xFF;
	}
#if DEBUG == 1
	nrf_uart_txrx_pins_set(NRF_UART0, TX_PIN_NUMBER, RX_PIN_NUMBER);
#endif
	if (rst != 0) {
		DBG_LOG("fpg_ReadWaitData error:%s.", FPG_GetResultString(rst));
	}
	return rst;
}

/**
 *  指纹传感器调试接口
 */
static void fpg_Console(int argc, char* argv[]) {
	uint16_t score = 0, page = 0;

	argv++;
	argc--;

	if (ARGV_EQUAL("poweron")) {
		FPG_EN();
		DBG_LOG("FPG poweron.");
	} else if (ARGV_EQUAL("poweroff")) {
		FPG_DIS();
		DBG_LOG("FPG poweroff.");
	} else if (ARGV_EQUAL("istouch")) {
		DBG_LOG("FPG is touch:%u.", IS_FPG_INT());
	} else if (ARGV_EQUAL("sendcmd")) {
		fpg_SendCmd(uatoi(argv[1]), NULL, 0);
		DBG_LOG("fpg_SendCmd:%u.", uatoi(argv[1]));
	} else if (ARGV_EQUAL("getimage")) {
		DBG_LOG("FPG_PS_GetImage:%u.", FPG_PS_GetImage());
	} else if (ARGV_EQUAL("genchar")) {
		DBG_LOG("FPG_PS_GenChar:%u.", FPG_PS_GenChar(uatoi(argv[1])));
	} else if (ARGV_EQUAL("match")) {
		DBG_LOG("FPG_PS_Match:%u, scroe:%u.", FPG_PS_Match(&score), score);
	} else if (ARGV_EQUAL("search")) {
		DBG_LOG("FPG_PS_Search:%u, pageid:%u, scroe:%u.",
						FPG_PS_Search(uatoi(argv[1]), uatoi(argv[2]), uatoi(argv[3]), &page, &score), page, score);
	} else if (ARGV_EQUAL("regmodel")) {
		DBG_LOG("FPG_PS_RegModel:%u.", FPG_PS_RegModel());
	} else if (ARGV_EQUAL("storechar")) {
		DBG_LOG("FPG_PS_StoreChar:%u.", FPG_PS_StoreChar(uatoi(argv[1]), uatoi(argv[2])));
	} else if (ARGV_EQUAL("loadchar")) {
		DBG_LOG("FPG_PS_LoadChar:%u.", FPG_PS_LoadChar(uatoi(argv[1]), uatoi(argv[2])));
	} else if (ARGV_EQUAL("deletchar")) {
		DBG_LOG("FPG_PS_DeletChar:%u.", FPG_PS_DeletChar(uatoi(argv[1]), uatoi(argv[2])));
	} else if (ARGV_EQUAL("empty")) {
		DBG_LOG("FPG_PS_Empty:%u.", FPG_PS_Empty());
	} else if (ARGV_EQUAL("readsyspara")) {
		FPG_PS_ReadSysPara(NULL);
	} else if (ARGV_EQUAL("enroll")) {
		DBG_LOG("FPG_PS_Enroll:%u, pageid:%u.", FPG_PS_Enroll(&page), page);
	} else if (ARGV_EQUAL("indentify")) {
		DBG_LOG("FPG_PS_Identify:%u, pageid:%u, score:%u.", FPG_PS_Identify(&page, &score), page, score);
	} else if (ARGV_EQUAL("highspeedsearch")) {
		DBG_LOG("FPG_PS_HighSpeedSearch:%u, pageid:%u, scroe:%u.",
						FPG_PS_HighSpeedSearch(uatoi(argv[1]), uatoi(argv[2]), uatoi(argv[3]), &page, &score), page, score);
	} else if (ARGV_EQUAL("validtempletenum")) {
		DBG_LOG("FPG_PS_ValidTempleteNum:%u, Number:%u.", FPG_PS_ValidTempleteNum(&page), page);
	} else if (ARGV_EQUAL("upchar")) {
		DBG_LOG("FPG_PS_UpChar:%u.", FPG_PS_UpChar(uatoi(argv[1]), mulitiBuf));
	} else if (ARGV_EQUAL("downchar")) {
		DBG_LOG("FPG_PS_DownChar:%u.", FPG_PS_DownChar(uatoi(argv[1]), mulitiBuf));
	}
}

/************************ (C) COPYRIGHT  *****END OF FILE****/
