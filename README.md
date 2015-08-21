
### 序：介绍
#### 目录结构
BKM的目录按照如下的方式组织。

> **注意：**
>
> * 斜体的文件必须存在。
> * #后边是注释部分

- bigkillmachine
	- *kfg*
	- *Makefile.defs*
	- *Makefile.rules*
	- *PLATFORMS/*
		- *CURRENT* # 符号链接，指向当前正在使用的PLATFORM比如`x86`。
		- x86
		- 7231
		- 7356
	- *BUILDS/*
		- *CURRENT* # 符号链接，指向当前正在使用的BUILD。
		- BUILD-NAME-A
		- BUILD-NAME-B
	- *SRCS/*
		- SRC-DIR-NAME-A/
			- README.md
			- *Makefile*
			- xxx.c
		- SRC-DIR-NAME-A/
	 - ...如此这般

#### 组件介绍
##### dr
运行时修改dalog的输出，直接运行 `dr` 可查看帮助。

E.g.
> `dr p:network m:DADBUS d` 
> 
> 注：打印进程network中DADBUS模块的d级别的日志

##### inotdo
我不干：监视文件变化，并调用命令，直接运行 `dr` 可查看帮助。

E.g.
> `inotdo /etc/configman/configman.xml.local/%gconf-tree.xml modify "ls; stat /tmp/chk_conn_stat && cat /tmp/scan.log"` 

> 注：  当configman.local变化时，调用 `ls` 命令，然后检查 `/tmp/chk_conn_stat` 文件是否存在，若存在，则打印 `/tmp/scan.log`  文件。

##### daxia 
下水道：通过网络接收来自dalog的打印，只要当`DALOG_TO_NETWOK` 设置的时候可用。直接运行 `dr` 可查看帮助。

这程序一般都在在PC上运行，所以应该将其编译成PC版本的。


### 跋：如何方法

> **特别注意：**
>
> * NEMOBUILD的工具链把**`LD_PRELOAD`**干掉了，要用老的**`ld-uClibc-???`**替换到当前使用的才可以。
> * Release版本的网络也被关闭了，如果需要Log通过网络输出到别的机器上（参考 DALOG\_TO\_NETWORK)，需要把网络打开。

#### 构造环境
所有的需要的文件都在`dazhu`里，需要把这个文件拷贝到某个Target目录下。 
> `python <whatever_the_dazhu_dir>/setupdazhu`

大杀器要对编译做些手脚，需要使用特殊的编译脚本。 在`setupdazhu`之后，用如下命令创建新的编译脚本：
> `cd <XYZ>/ntvtgt/<SHIT>`
> `ln -s dazhu/m m.<BUILD_TYPE>.<CONFIG_TYPE>`   

比如：
> `ln -s dazhu/m m.bld.tf`  

#### 替换ntvlog宏

- `meld dazhu/patch/ntvlog ../../nemotv/src/utils`

> **注意：**为了保险起见，需要合并*Makefile*文件，对于其他文件，直接合并即可。

#### 打印DBUS的活动
在 `dbus-connection.c` 中加入钩子函数 `dagou_dump_dbus_message()` 来达到此目的。

> **注意：** 这个操作在dbus工程 ***编译成功*** 后，再进行。

输入如下命令（以 bld_tf 目录为例）：
> - `meld dazhu/patch/dbus-connection.c bld_tf/build/dbus-1.4.16/dbus/dbus-connection.c`
> - `cd bld_tf/build/dbus-1.4.16`
> - `rm .stamp_built .stamp_staging_installed .stamp_target_installed`
> - `cd ../../..`
> - `./m.bld.tf`

#### 在UI中使用dalog:

首先修改`XCOMX`的代码，加入对`dalog`的支持：
> `meld dazhu/patch/ccomx_NpObj.cpp ../../nemotv/src/ccomx/npinf-plugin/ccomx_NpObj.cpp`
> 
> **注意：** 仅仅合并`__USE_DALOG__`宏包起来的部分即可。

在UI中调用*dalog*，有两种方法，首先是HOOK原有的日志输出，使用如下的命令：
> `meld dazhu/patch/Log.js ../../netcfg/???/mars/client/fw/apps/core/Log.js`

或者，直接参照`Log.js` ，直接在JS的代码中调用 `XCOM.dalog()` 函数。

#### 中断启动，修改配置：

* 非*Release*的工程，修改　`/etc/init.d/rcS`，在调用*pcd*之前，加上如下行：
> `PS1="BKM> " /bin/sh`
	
* *Release*版本，在sysinit调用***pcd***前，调用阻塞式的子进程来启动*/bin/sh*。
	* 使用 `patch/spawn_with_env.c` 来启动Shell（参加该文件开头的说明）。
	* 或者，使用 `init_run` 函数调用*/bin/sh*，这个函数的问题是不能传递环境变量。

#### 修改IPNETWORK
为了能在network下调用的子进程中可以打印输出，需要其能链接到`utils`的库。

* 直接修改*nemotv/src/network/Makefile.am*，在相应的位置增加如下两行：
	> `dhclientscript_LDADD += -lutils`
	> `dhclientscript_CFLAGS =-Wl,-rpath,/lib:/usr/lib:/usr/local/lib`

* 或者合并已经修改后的*Makefile.am*：
	> `meld dazhu/patch/network_Makefile.am ../../nemotv/src/network/Makefile.am`

###### nemotv/external/ntvwebkit/Source/WebKit/gtk/webkit/webkitwebview.cpp
修改 *NTV_WEBKITVIEW_LOG* 宏为 `#define NTV_WEBKITVIEW_LOG nsulog_info` 即可。
- `vim ../../nemotv/external/ntvwebkit/Source/WebKit/gtk/webkit/webkitwebview.cpp`

#### 编译
编译过程和直接使用*make*类似，区别是使用*m.bld.tf*替换到*make*即可。
`./m.bld.tf <其他make的参数>`

比如：
> `./m.bld.tf all image V=1`

