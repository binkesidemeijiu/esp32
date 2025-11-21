uart 点灯指令：02 00 01 01 01 00 03 ：点灯
废话：出来工作2年了，工作几乎都有Android或者Linux的驱动有关系，很久不碰这些学生时代的板子了。心血来潮，买了一块s3的板子，玩一玩。
1:使用环境是ubantu 24.04 与vscode idf
2:Kconfig.projbuild 文件用于定义我们自己的编译宏，目前主要使用了BUILD_DEBUG_MODE区分不同的版本，比如user版本关闭几乎自定义的全部log
3:对于不同的外设除必要的打印log的uart 以外全部当作模块处理，每个程序注册Peripheral_Init_Entry_t，list_os_deal_with_t就行了
4:模块太过独立被编译器优化时，参考main里面的cmake文件注释

当前已经实现：uart0通信，通过uart0控制一个gpio电平

更新:支持st7789 spi屏，wifi，tf卡，camera ov2640（核心代码乐鑫写完了，所以其实这里其他的一样的用，就算里面没支持，自己添加一样的，就是有点费神，最要命的莫过于对焦马达这一块似乎全靠手，也没有闪光灯，set_gainceiling_sensor最好就默认）
camera当前支持2条指令cap:02 00 30 01 02 00 03,preview:02 00 30 01 03 00 03,无脑preview就好了，本来我以为可以不同的mode去写不同的参数以控制效果，后来发现想多了，不需要。

