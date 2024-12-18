# Vidio_Player
购买的视频播放器资料
使用说明





## 1.前言

### 1.1.文件说明
Program_STM32F407_T100	//程序文件夹
pro1_usb_mass_disk			//U盘例程，用于往SPI_Flash或SD卡中拷贝资源
pro2_mp3_avi_nes_pic		//音乐播放器、视频播放器、游戏机与图片浏览器例程

Related_Lib_STM32F407		//程序中使用的lib库
lib_audio_decode			//音乐音频解码库，目前支持mp3与wav这两种
lib_helix						//mp3解码算法库
lib_nes						//nes游戏模拟器

SD_Card_Disk				//SD卡中存放的资源
SPI_Flash_Disk_8MB			//SPI-Flash中存放的资源

其余文件与文件夹根据名字就可知其意，没必要说明。

### 1.2.必看
如果你购买了板子实物，那么发货前已经在板子上构建好了音乐播放器、视频播放器与游戏机，拿到手后你可以跳过下面的一，根据二连接模块与开发板，然后根据三来体验效果。

## 2.如何在开发板上实现音乐播放器、视频播放器与游戏机
如果你的板子是空板（即没有下载本资料中的例程），那么你可以按照以下两步在你的开发板上实现音乐播放器与游戏机。

不建议你直接去编译程序，所有的工程都是编译好的，建议你按照下面的步骤先用STlink、FlyMcu或其他下载工具下载对应工程下的hex文件，体验一下流程与效果。等你把流程走通了，再去看代码，编译工程。

1.下载usb_mass_disk例程
本例程用于往SPI Flash或SD卡中拷贝相关资源，下载完本例程后，用USB线连接的电脑与板子，注意要连接板子的USB slave接口，而不是CH340转UART通信接口！连接完成电脑端会识别出如下磁盘：
   
根据板子上SPI Flash磁盘的容量，把SPI_Flash_Disk_8MB目录下的music、nes、video、picture、sys五个文件夹拷贝到SF_Disk的根目录。

特别提醒：

（1）必须使用本资料提供的usb_mask_disk例程，不要使用其他usb mass disk例程。

（2）是拷贝music、nes、video、picture、sys五个文件夹到SF_Disk的根目录，而不是拷贝SPI_Flash_Disk_8MB文件夹到SF_Disk的根目录。

（3）在以后的使用中，如果你想更新或者更换SPI Flash中的资源，就需要下载此U盘例程；否则就没有必要再下载此例程。

2.下载mp3_avi_nes_pic例程

本例程才是音乐播放器、视频播放器与游戏机的实现者，1中的例程用于为本例程构建环境。运行本例程时，可以不插SD卡，因为SPI Flash中已经存储了相关音乐、视频与游戏，若插上SD卡，则优先识别SD卡中的音乐与游戏。

[注]：

（1）插SD前，请先把SD_Card_Disk目录下的music、nes、video、picture文件夹拷贝到SD卡的根目录（这里同样强调一下，是拷贝music、nes、video、picture文件夹，而不是拷贝SD_Card_Disk文件夹）。

（2）如果开发板不识别SD卡，那么在使用SD卡之前请先格式化一下SD卡，格式化为FatFS文件系统。



## 3.开发板与模块连接说明
### 3.1.模块简介
 
PCM5102与MAX98357声音模块								游戏手柄转接板
PCM5102与MAX98357都是I2S接口的声音模块，用于播放声音，实际连接时只需连接其中任意一个即可。
游戏手柄转接板用于插接9孔的游戏手柄，可以插接2个手柄，支持双人游戏。

四个小玩意对着两个模块的需求：

音乐播放器：只需要连接声音模块。

视频播放器：只需要连接声音模块。

游戏机：需要同时连接声音模块与游戏手柄转接板。

图片浏览器：两者都不需要。

### 3.2.普中STM32F407开发板与声音模块的连接

（1）如果使用的是MAX98357模块，按如下方式连接
```
开发板   MAX98357模块模块
PB13 ------ BCLK
PC3  ------ DIN
PB12 ------ LRC
	     GAIN(悬空)
	     SD(悬空)
GND ------ GND
5V  ------ Vin
```
声音模块的输出端直接接小喇叭即可。

（2）如果你使用的是PCM5102模块，按如下方式连接
```
开发板    PCM5102模块
GND  ------ SCK(可以短接SCK丝印附近预留的焊盘来代替此连接；若没有短接，则此连接必不可少)
PB13  ------ BCK
PC3  ------ DIN
PB12 ------ LCK
GND ------ GND
5V   ------ Vin
```
PCM5102模块的耳机座直接插耳机或音箱即可听到声音。

### 3.3.普中STM32F407开发板与游戏手柄转接板的连接
```
开发板  游戏手柄转接板
PE6 ---- --VCC
PF0 ------ GND
PF2 ------ LAH
PF4 ------ CLK
PF6 ------ DT1
PF8 ------ DT2
```
[注]：这是直接使用管脚模拟电源给小功率设备供电，PE6、PF0可以替换为3.3V、GND。
（如果不玩游戏可不连接手柄）
## 4.操作说明
音乐播放器、视频播放器、游戏机与图片浏览器的操作需要3个按键，为了叙述方便，把这3个按键命名为SEL、DEC、INC，详细约定如下：
```
KEY1   --> SEL
KEY0   --> DEC
KEY_UP --> INC
```
同样为了叙述方便，再定义如下2个名词：
长按：按键持续按压3s以上不释放；
按：  按键按下后立即释放。

1.功能选择界面
上电后进入功能选择界面，如下：

此界面下，按DEC键或INC键，可以改变所选中的功能，粉色表示选中；按SEL键进入相关功能。

2.音乐播放器
音乐播放器支持mp3与wav这两种格式的音乐。
  
（1）在列表目录下，按INC键或DEC键可以改变当前选中的音乐，长按INC键或DEC键可以持续改变当前选中的对象；按SEL键打开选中的音乐，长按SEL按键返回到主界面。

（2）音乐播放界面下按键的功能
按SEL键，依次调出：
音量调节、快进/快退、模式设置
调出相关功能后，按INC与DEC键可进行相关调整，音量调节时长按INC或DEC可持续调节音量。

按DEC键，暂停播放，再按一次DEC键可恢复播放。
按INC键，关闭播放器，返回到音乐列表目录。
3.视频播放器
视频播放器支持avi格式的视频，其视频流采用MJPEG编码，音频流采用MP3编码，视频制作的详细过程请参考《视频格式转换说明.docx》。
 
（1）在列表目录下，按INC键或DEC键可以改变当前选中的视频，长按INC键或DEC键可以持续改变当前选中的对象；按SEL键打开选中的视频，长按SEL按键返回到主界面。

（2）视频播放界面下按键的功能

按SEL键，依次调出：PlaySeek、SetVolume.
调出相关功能后，按INC与DEC键可进行相关调整，长按这两个键持续进行调整。

按DEC键，暂停播放，再按一次DEC键可恢复播放。
按INC键，关闭播放器，退出视频列表目录。

4.游戏机
游戏机支持nes游戏文件，需要手柄的支持，玩游戏时需要插入游戏手柄。
   
（1）在列表目录下，按INC键或DEC键可以改变当前选中的游戏，长按INC键或DEC键可以持续改变当前选中的对象；按SEL键打开选中的游戏，长按SEL按键返回到主界面。

（2）游戏界面下按键的功能
按SEL键，退出游戏，返回到游戏列表界面；
按INC与DEC键可进行相关调整，音量调节时长按INC或DEC可持续调节音量。

[注]：调整音量时，屏幕左上方Volume指示当前音量，FPS为游戏帧率，nes游戏全速运行时的帧率为60，经过我的各种优化，此模拟器可全速运行nes游戏，所以游戏玩起来会非常流畅，体验非常爽。


5.图片浏览器
图片浏览器支持jpg与bmp格式的文件。
  
（1）在列表目录下，按INC键或DEC键可以改变当前选中的图片，长按INC键或DEC键可以持续改变当前选中的对象；按SEL键打开选中的图片，长按SEL按键返回到主界面。

（2）图片浏览界面下按键的功能

按SEL键，关闭当前所浏览的图片，返回到图片列表目录。
按DEC键，打开下一幅图片。
按INC键，打开上一幅图片。

在开发板上构建视频播放器的步骤：

1.给开发板下载usb_mass_disk例程，用flyMCU、Jlink、ST-Link等均可；

2.用USB线连接开发板的USB通信接口，往SPI_Flash中拷贝相关资源；

3.给开发板下载mp3_avi_nes_pic例程，此例程才是视频播放器(也包括音乐播放器、游戏机)的实现者。

    注意事项：
	（1）只能用本资料提供的usb_mass_disk例程，不要用其他的；
	（2）该usb_mass_disk例程支持SD卡，在给开发板上电前，若插上SD卡，则电脑能识别出2个磁盘，
	     此时既可以往SPI_Flash中拷贝资源，也可往SD卡中拷贝资源。
	（3）无论往SPI_Flash中拷贝资源，还要往SD卡中拷贝资源，都要注意文件目录，不要拷贝错了。
