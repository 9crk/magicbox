#include "driver/spi.h"
#include "driver/spi_overlap.h"
#include "driver/i2s_reg.h"
#include "driver/slc_register.h"
#include "driver/sdio_slv.h"
#include "driver/i2s.h"
 #include "osapi.h"
#include "os_type.h"
#include "gpio.h"

uint32 i2s_rx_buff1[IIS_RX_BUF_LEN/4];
uint32 i2s_rx_buff2[IIS_RX_BUF_LEN/4];

#ifdef I2S_RECEIVE
uint32 i2s_tx_buff1[IIS_TX_BUF_LEN/4];
uint32 i2s_tx_buff2[IIS_TX_BUF_LEN/4];
uint32 i2s_tx_buff3[IIS_TX_BUF_LEN/4];
uint32 i2s_tx_test[IIS_TX_BUF_LEN*n];
int tx_cnt=0;
#endif

int32 rx_buff1_cnt=0;
int32 rx_buff2_cnt=0;
int8 tab_idx=0;
uint32 send_len=0;           //control audio from 0 to last uint32
uint32 free_buf[IIS_RX_BUF_LEN/4];     //when no voice, send 0xff to cancel noise
struct sdio_queue i2s_rx_queue1, i2s_rx_queue2;/*,i2s_tx_queue1,i2s_tx_queue2,i2s_tx_queue3*/;

extern uint32 audio_voice_data;
extern uint32 file_total_size;
extern uint32 write_flash_end;

//create DMA buffer descriptors, unit of either size or length here is byte. 
//More details in I2S documents.
void creat_one_link(unsigned  char own, unsigned  char eof,unsigned  char sub_sof, uint16 size, uint16 length,
                       uint32* buf_ptr, struct sdio_queue* nxt_ptr, struct sdio_queue* i2s_queue)
 {
	uint32 link_data0;
       uint32 link_data1;
       uint32 link_data2;
       uint32 start_addr;

	i2s_queue->owner=own;
	i2s_queue->eof= eof;
	i2s_queue->sub_sof=sub_sof;
	i2s_queue->datalen=length;
	i2s_queue->blocksize=size;
	i2s_queue->buf_ptr=(uint32)buf_ptr;
	i2s_queue->next_link_ptr=(uint32)nxt_ptr;
	i2s_queue->unused=0;
}

//Initialize the I2S module
//More Registor details in I2S documents.
//初始化i2s本身，包括BCLK、WS、SDATA信号线的配置、时钟配置、通道、采样率、开启中断等。
void i2s_init(void)
{
    //CONFIG I2S RX PIN FUNC
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_I2SI_DATA);//设置I2S数据线//io12
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_I2SI_BCK);//设置I2S时钟线io13
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_I2SI_WS);//设置I2S声道选择线io14
	//CONFIG I2S TX PIN FUNC
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);//rxd0脚


    	//Enable a 160MHz clock to i2s subsystem
	i2c_writeReg_Mask_def(i2c_bbpll, i2c_bbpll_en_audio_clock_out, 1);

	//reset I2S
	CLEAR_PERI_REG_MASK(I2SCONF,I2S_I2S_RESET_MASK);
	SET_PERI_REG_MASK(I2SCONF,I2S_I2S_RESET_MASK);
	CLEAR_PERI_REG_MASK(I2SCONF,I2S_I2S_RESET_MASK);

	
	//Enable FIFO in i2s module
	SET_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN);
	
	//set I2S_FIFO
	//set rx,tx data size, both are "24-bit full data discountinue" here  
//	设置i2s模式
//	根据实际待播放音频的实际情况（可以查看声音的类型），设置为16位和24位，又分half和full模式。这个很重要，如果模式错误，播放的声音混乱。
//	SET_PERI_REG_MASK(I2S_FIFO_CONF,(I2S_I2S_RX_FIFO_MOD<<I2S_I2S_RX_FIFO_MOD_S)) //16位half模式
	SET_PERI_REG_MASK(I2S_FIFO_CONF, (I2S_I2S_RX_FIFO_MOD<<I2S_I2S_RX_FIFO_MOD_S)|(I2S_I2S_TX_FIFO_MOD<<I2S_I2S_TX_FIFO_MOD_S));

	//set I2S_CHAN 
	//set rx,tx channel mode, both are "two channel" here
//	单、双声道，根据实际待播放音频的实际情况（可以查看声道的类型：
//	SET_PERI_REG_MASK(I2SCONF_CHAN,(I2S_TX_CHAN_MOD<<I2S_TX_CHAN_MOD_S)) //单声道
	SET_PERI_REG_MASK(I2SCONF_CHAN, (I2S_TX_CHAN_MOD<<I2S_TX_CHAN_MOD_S)|(I2S_RX_CHAN_MOD<<I2S_RX_CHAN_MOD_S));

	//set RX eof num
	WRITE_PERI_REG(I2SRXEOF_NUM, RX_NUM);


#if defined (I2S_RECEIVE)
    //trans slave&rece master mode,
    //MSB_shift,right_first,MSB_right,
    //use I2S clock divider to produce a 32KHz Sample Rate
	CLEAR_PERI_REG_MASK(I2SCONF, I2S_RECE_SLAVE_MOD|
						(I2S_BITS_MOD<<I2S_BITS_MOD_S)|
						(I2S_BCK_DIV_NUM <<I2S_BCK_DIV_NUM_S)|
                                    	(I2S_CLKM_DIV_NUM<<I2S_CLKM_DIV_NUM_S));
	
	SET_PERI_REG_MASK(I2SCONF, I2S_RIGHT_FIRST|I2S_MSB_RIGHT|I2S_TRANS_SLAVE_MOD|
						I2S_RECE_MSB_SHIFT|I2S_TRANS_MSB_SHIFT|
						((26&I2S_BCK_DIV_NUM )<<I2S_BCK_DIV_NUM_S)|
						((4&I2S_CLKM_DIV_NUM)<<I2S_CLKM_DIV_NUM_S)|
						(8<<I2S_BITS_MOD_S));

#else
//	设置采样率
//	根据声音的情况设置相同的采样率。播放器设置为发送主模式、接受从模式，采样率设置为50和25分频倍数（下面代码的数字50和25）,BCLK为：160M/50/25=128K/s，由于设置的是16位单声道，因此采样率为128/16=8K/s，如果是其他采样率可计算出分频值（比如24位双声道，采样率44.1k，则160000K/24/2/44.1K=75，75=15*5，则如下设置：15&I2S_BCK_DIV_NUM，5&I2S_CLKM_DIV_NUM，注意前面数字一般比后边数字大，前面数 不超过63）
//	代码：
//	//trans master&rece slave mode
//	SET_PERI_REG_MASK(I2SCONF,I2S_RIGHT_FIRST|I2S_MSB_RIGHT|I2S_RECE_SLAVE_MOD|
//	I2S_RECE_MSB_SHIFT|I2S_TRANS_MSB_SHIFT|
//	((50&I2S_BCK_DIV_NUM )<<I2S_BCK_DIV_NUM_S)| //分频数50
//	((25&I2S_CLKM_DIV_NUM)<<I2S_CLKM_DIV_NUM_S)| //分频数25
//	(0<<I2S_BITS_MOD_S));
	//trans master&rece slave mode,
	//MSB_shift,right_first,MSB_right,
	//use I2S clock divider to produce a 8KHz Sample Rate
	CLEAR_PERI_REG_MASK(I2SCONF, I2S_TRANS_SLAVE_MOD|
						(I2S_BITS_MOD<<I2S_BITS_MOD_S)|
						(I2S_BCK_DIV_NUM <<I2S_BCK_DIV_NUM_S)|
                                    	(I2S_CLKM_DIV_NUM<<I2S_CLKM_DIV_NUM_S));
	
	SET_PERI_REG_MASK(I2SCONF, I2S_RIGHT_FIRST|I2S_MSB_RIGHT|I2S_RECE_SLAVE_MOD|
						I2S_RECE_MSB_SHIFT|I2S_TRANS_MSB_SHIFT|                   //63 :13k   52: 16k   38: 22.05k    19: 44.1k
						((50&I2S_BCK_DIV_NUM )<<I2S_BCK_DIV_NUM_S)|            // BCK = 160M / 25 / 50       SAMPLE_RATE = BCK / 16   = 8k
						((25&I2S_CLKM_DIV_NUM)<<I2S_CLKM_DIV_NUM_S)|
						(0<<I2S_BITS_MOD_S));
 
#endif
	//clear int
	SET_PERI_REG_MASK(I2SINT_CLR,   I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|I2S_I2S_RX_REMPTY_INT_CLR|
	I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
	CLEAR_PERI_REG_MASK(I2SINT_CLR,   I2S_I2S_TX_REMPTY_INT_CLR|I2S_I2S_TX_WFULL_INT_CLR|I2S_I2S_RX_REMPTY_INT_CLR|
	I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);

	//enable int
	SET_PERI_REG_MASK(I2SINT_ENA,   I2S_I2S_TX_REMPTY_INT_ENA|I2S_I2S_TX_WFULL_INT_ENA|I2S_I2S_RX_REMPTY_INT_ENA|
	I2S_I2S_RX_WFULL_INT_ENA|I2S_I2S_TX_PUT_DATA_INT_ENA|I2S_I2S_RX_TAKE_DATA_INT_ENA);

	CLEAR_PERI_REG_MASK(I2SCONF,I2S_I2S_TX_START|I2S_I2S_RX_START);
	SET_PERI_REG_MASK(I2SCONF,I2S_I2S_TX_START|I2S_I2S_RX_START);
}

//scl初始化，8266的i2s有两种模式，第一种是直接读写fifo，第二种方式是采用中断+dma的方式。采用中断+dma方
void slc_init()
{
    //Reset DMA
	SET_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST|SLC_TXLINK_RST); //这里设置复位控制位，开始复位流程
	CLEAR_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST|SLC_TXLINK_RST);//这里清除复位控制位，结束复位

	//Enable and configure DMA
	CLEAR_PERI_REG_MASK(SLC_CONF0, (SLC_MODE<<SLC_MODE_S));//这里清除掉原来的slc模型
	SET_PERI_REG_MASK(SLC_CONF0,(1<<SLC_MODE_S));//开启中断+DMA模式
	SET_PERI_REG_MASK(SLC_RX_DSCR_CONF,SLC_INFOR_NO_REPLACE|SLC_TOKEN_NO_REPLACE);//|0xfe 暂时不明白
	CLEAR_PERI_REG_MASK(SLC_RX_DSCR_CONF, SLC_RX_FILL_EN|SLC_RX_EOF_MODE | SLC_RX_FILL_MODE);//这里清除掉接收端的模型(暂时不明白)
	
	
	ETS_SLC_INTR_ATTACH(slc_isr, NULL);///注册中断函数
	/////enable sdio operation intr
	WRITE_PERI_REG(SLC_INT_ENA, SLC_INTEREST_EVENT);
	/////clear sdio initial random active intr signal 
	WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff);
//	WRITE_PERI_REG(SLC_INT_CLR, 0xa55aa55a);
	/////enable sdio intr in cpu
	ETS_SLC_INTR_ENABLE();//开中断
}
//有音频数据到来，将其拷贝至i2s_rx_buff1或者i2s_rx_buff2中，dma自动将音频数据送入i2s总线至音频芯片，当dma发送完成一次数据，中断产生，在拷贝数据，周而复始直至数据播放完成。
//中断处理每次拷贝接下来要播放数据，并判断是否播放完成：
//interrupt
//write new buff data into RX_DMA at the transmitter side
//or load buff data out from TX_DMA at the receiver side
void slc_isr(void *para)
{
	uint32 slc_intr_status;

	slc_intr_status = READ_PERI_REG(SLC_INT_STATUS); //读中断状态
        if (slc_intr_status == 0) 
	 {
           //No interested interrupts pending  
		return;
        }
	//clear all intrs
	WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff); //清中断
	
	//process every intr
	//Transimitter side
	if (slc_intr_status & SLC_RX_EOF_INT_ST) //DMA缓冲区空
	{
		if(READ_PERI_REG(SLC_RX_EOF_DES_ADDR)==(((uint32)&i2s_rx_queue1))) //dma缓冲区1      //user first buffer
		{	
			if(audio_voice_data != 0)                               //voice data coming from mqtt
			{
				os_printf("audio_voice_data = 0\n");
				if(write_flash_end)                              //voice write to flash complete,start to send to i2s
				{
					if(file_total_size >= IIS_RX_BUF_LEN)      //数据拷贝未至最后	 //not the last 128 uint32
					{
						spi_flash_read(AUDIO_FLASH_READ_START_ADDR + send_len * IIS_RX_BUF_LEN, (uint32 *)i2s_rx_buff1, IIS_RX_BUF_LEN);
						file_total_size -= IIS_RX_BUF_LEN;
						send_len++;
					} 
					else                       //the last 128 uint32 (or < 128 uint32)
					{
						spi_flash_read(AUDIO_FLASH_READ_START_ADDR + send_len * IIS_RX_BUF_LEN, (uint32 *)i2s_rx_buff1, file_total_size);
						audio_voice_data = 0;
						send_len = 0;
						file_total_size = 0;
						write_flash_end = 0;
					}
	 			}
			}
			else                               //no voice coming, send 0xff to i2s to cancel noise
			{
				memcpy(i2s_rx_buff1, free_buf, IIS_RX_BUF_LEN);              
			}
		} else  if (READ_PERI_REG(SLC_RX_EOF_DES_ADDR)==(((uint32)&i2s_rx_queue2)))  //user second buffer
		{
			if(audio_voice_data != 0)
			{
				if(write_flash_end) 
				{
					if(file_total_size >= IIS_RX_BUF_LEN) 
					{	
						spi_flash_read(AUDIO_FLASH_READ_START_ADDR + send_len * IIS_RX_BUF_LEN, (uint32 *)i2s_rx_buff2, IIS_RX_BUF_LEN);
						file_total_size -= IIS_RX_BUF_LEN;
						send_len++;
					} 
					else
					{
						spi_flash_read(AUDIO_FLASH_READ_START_ADDR + send_len * IIS_RX_BUF_LEN, (uint32 *)i2s_rx_buff2, file_total_size);
						audio_voice_data = 0;
						send_len = 0;
						file_total_size = 0;
						write_flash_end = 0;
					}
					
	 			}
			}
			else
			{
				memcpy(i2s_rx_buff2, free_buf, IIS_RX_BUF_LEN);
			}
		}
	}
	
#ifdef I2S_RECEIVE
	//Receiver side
	if (slc_intr_status & SLC_TX_EOF_INT_ST) 
	{
        	//os_printf("slc_intr_status222= %08x\n", slc_intr_status);
       	 //find the DMA which sends the interrupt signal
		if(READ_PERI_REG(SLC_TX_EOF_DES_ADDR)==(((uint32)&i2s_tx_queue1)))
		{
			//load out data in the buff	
			if(tx_cnt < 4*n)
			{
				memcpy((unsigned  char*)i2s_tx_test+IIS_TX_BUF_LEN*tx_cnt,(unsigned  char*)i2s_tx_buff1,IIS_TX_BUF_LEN);
			}
			
			//reset DMA discrpiter
			i2s_tx_queue1.next_link_ptr = (uint32)(&i2s_tx_queue2);
			i2s_tx_queue1.eof=0;
			i2s_tx_queue1.owner=1;
			i2s_tx_queue1.datalen = 0;
		} 
		else if(READ_PERI_REG(SLC_TX_EOF_DES_ADDR)==(((uint32)&i2s_tx_queue2)))
		{
			if(tx_cnt < 4*n)
			{
				memcpy((unsigned  char*)i2s_tx_test+IIS_TX_BUF_LEN*tx_cnt,(unsigned  char*)i2s_tx_buff2,IIS_TX_BUF_LEN);
			}

			i2s_tx_queue2.next_link_ptr = (uint32)(&i2s_tx_queue3);
			i2s_tx_queue2.eof=0;
			i2s_tx_queue2.owner=1;
			i2s_tx_queue2.datalen = 0;
		} 
		else if(READ_PERI_REG(SLC_TX_EOF_DES_ADDR)==(((uint32)&i2s_tx_queue3)))
		{
			if(tx_cnt < 4*n)
			{
				memcpy((unsigned  char*)i2s_tx_test+IIS_TX_BUF_LEN*tx_cnt,(unsigned  char*)i2s_tx_buff3,IIS_TX_BUF_LEN);
			}
			
			i2s_tx_queue3.next_link_ptr = (uint32)(&i2s_tx_queue1);
			i2s_tx_queue3.eof=0;
			i2s_tx_queue3.owner=1;
			i2s_tx_queue3.datalen =0;
		}
		
		if(tx_cnt++ == 7)
			tx_cnt = 0;
	}	
	#endif
}

void ICACHE_FLASH_ATTR
	i2s_audio_init(void)
{	
	slc_init();
	//To receive data from the I2S module, counter-intuitively we use the TXLINK part, not the RXLINK part.
	//Vice versa.
	//Note:At the transimitter side,the size of the DMAs can not be smaller than 128*4 bytes which are the
	//size of the I2S FIFO.
	//Note:At the receiver side,the number of the DMAs can not be smaller than 3 which is limited by the 
	//hardware.
	
	#ifdef I2S_RECEIVE
	creat_one_link(1,0,0,IIS_TX_BUF_LEN,0,i2s_tx_buff1,&i2s_tx_queue2,&i2s_tx_queue1); 
    creat_one_link(1,0,0,IIS_TX_BUF_LEN,0,i2s_tx_buff2,&i2s_tx_queue3,&i2s_tx_queue2);
 	creat_one_link(1,0,0,IIS_TX_BUF_LEN,0,i2s_tx_buff3,&i2s_tx_queue1,&i2s_tx_queue3); 
	#endif
// 	创建两个dma缓冲区（可以是4个等）i2s_rx_queue1和i2s_rx_queue2，当音频播放的时候，将数据放入该缓冲区，dma自动将数据发送出去，创建：
// 	creat_one_link(1,1,0,IIS_RX_BUF_LEN,IIS_RX_BUF_LEN,i2s_rx_buff1,&i2s_rx_queue2,&i2s_rx_queue1);
// 	creat_one_link(1,1,0,IIS_RX_BUF_LEN,IIS_RX_BUF_LEN,i2s_rx_buff2,&i2s_rx_queue1,&i2s_rx_queue2);
// 	这个函数实际上是将i2s_rx_buff1和i2s_rx_buff2缓冲区的首地址指向结构i2s_rx_queue1和i2s_rx_queue2的数据指针，并将i2s_rx_queue1和i2s_rx_queue2做成循环链表。
	creat_one_link(1,1,0,IIS_RX_BUF_LEN,IIS_RX_BUF_LEN,i2s_rx_buff1,&i2s_rx_queue2,&i2s_rx_queue1); 
	creat_one_link(1,1,0,IIS_RX_BUF_LEN,IIS_RX_BUF_LEN,i2s_rx_buff2,&i2s_rx_queue1,&i2s_rx_queue2); 

	//config rx&tx link to hardware
	CONF_RXLINK_ADDR(&i2s_rx_queue1);
	#ifdef I2S_RECEIVE
	CONF_TXLINK_ADDR(&i2s_tx_queue1);	
	#endif
	
 	//启动发送控制，该函数实际上是调用了SET_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_START)，开启scl。
	START_RXLINK();
	
	#ifdef I2S_RECEIVE
 	START_TXLINK();
	#endif

	memset(free_buf, 0xffffffff, IIS_TX_BUF_LEN/4);
	//memset(free_buf, 0xa55aa55a, IIS_TX_BUF_LEN/4);
	i2s_init();
    //As mentioned about the use of TXLINK & RXLINK part above, 
    //rx_buff contains data sented and tx_buff contains data reveived.
    //Delay exists at the receiver side, and under this test condition the delay is 2 words.
    //However, delay may change depends on different I2S mode(like master_slave, data length, FIFO size etc).
}
