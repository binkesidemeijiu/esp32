idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ".")

#这是一个极端无奈的方法，对于使用2条链表处理一切后，有一些模块可以做到不“显式的”被任何其他的文件调用，然后编译器就直接不链接这个.o
#所以可以类似extern 一个函数到main，但是我采用的方法时使用cmake去链接一个完全没用的函数钩子
#使用方法为，在你希望强制被链接的模块申明一个全局的没啥用的函数，然后直接在这里链接他
#需要注意的是，如果要移除这个模块，那么直接移除这里就行了。一切不能通过移除这里移除的模块都不写这里来。
target_link_options(${COMPONENT_LIB} INTERFACE "-Wl,-u,__led_link__")
target_link_options(${COMPONENT_LIB} INTERFACE "-Wl,-u,__wifi_link__") 