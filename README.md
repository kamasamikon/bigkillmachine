
# 目录结构
- Root/
    - konfigure
        - '''Generate Makefile?'''
        - konfigure targets
        - konfigure target name
        - konfigure platforms
        - konfigure platform name
    - Makefile
        - ''' 利用他的自动完成 '''
        - make target nameX
            - ln mk.targets/mk.target.nameX mk.target/mk.targets.now
        - make
            - Make by order of mk.targets.now/deps
    - Makefile.defs
    - Makefile.rules
    - README.md
    - mk.targets/
        - _mk.target.now_^
            - '''Current used, link to mk.target.nameX'''
        - mk.target.nameA/
            - '''Link the make directory to here'''
            - _deps_
                - '''Make order'''
            - aaa^
            - bbb^
        - mk.target.nameB/
    - dirA
        - README.md
            - '''README for this (dirA)'''
            - Makefile
                - all
                - clean
                - install
                - uninstall
    - dirB


# 命名
- dr : change runtime configure
- inotdo : 我不干，当文件变化时调用命令。
- dagou : 钩子
- daxia : 下水道
- dabao : 包装其他进程，并应用da环境变量
- daben : 大奔，奔驰马克工具
- muzei : 木贼，偷梁换柱之用


# 大杀器环境变量
- dagou
    - DALOG\_DFCFG 
        - Default configure
        - DALOG\_DFCFG=/home/df.cfg
    - DALOG\_RTCFG 
        - Runtime configure
        - DALOG\_DFCFG=/home/rt.cfg
    - DALOG\_TO\_LOCAL 
        - 输出到本地文件
        - DALOG\_TO\_LOCAL=/tmp/da.log
    - DALOG\_TO\_SYSLOG 
        - 输出到syslog
        - DALOG\_TO\_SYSLOG=YES|NO
    - DALOG\_TO\_NETWORK
        - 通过网络发送
        - DALOG\_TO\_NETWORK=localhost:3333

- daxia
    - DAXIA\_PORT
        - Listen port, see DALOG\_TO\_NETWORK
    - DAXIA\_FILE
        - Where the log will be saved to

# 如何使用

## 特别注意
1. 工具链问题
    - NEMOBUILD的工具链把**LD\_PRELOAD**干掉了，要用老的**ld-uClibc-???**替换到当前使用的才可以。
1. Release版本的问题
    - 网络也被关闭了，如果需要Log通过网络输出到别的机器上（参考 DALOG\_TO\_NETWORK)，需要把网络打开。

## 构造环境
所有的需要的文件都在`dazhu`里，需要把这个文件拷贝到某个Target目录下。 
>    `cp <xyz>/dazhu <XYZ>/ntvtgt/<SHIT>/ -frvL`

因为大杀器要对编译做些手脚，所以，需要使用特殊的编译脚本。 这里bld对应BUILD\_TYPE, tf对应CONFIG\_TYPE
> `cd <XYZ>/ntvtgt/<SHIT>`<br/>
> `ln -s dazhu/m m.<BUILD\_TYPE>.<CONFIG\_TYPE>`   

比如：
> `ln -s dazhu/m m.bld.tf`   


编译过程和直接使用*make*类似，区别是使用*m.bld.tf*替换到*make*即可。
> `./m.bld.tf <其他make的参数>`

比如：
> `./m.bld.tf all image V=1`

## 深入工程内部的其他修改
### 用*dalog*替换*utils*中的系列*ntvlog*宏。

使用如下命令覆盖*Makefile.am*之外的文件，
*Makefile.am*需要合并一下（仅仅是为了保险起见）。
- `meld dazhu/patch/ntvlog ../../nemotv/src/utils`

### 使大杀器能打印DBUS的活动

注意：
1. 这个操作在dbus工程编译成功后，再进行。
1. 以下以*bld_tf*目录为例。

输入如下命令：
- `meld dazhu/patch/dbus-connection.c bld_tf/build/dbus-1.4.16/dbus/dbus-connection.c`
- `cd bld_tf/build/dbus-1.4.16`
- `rm .stamp\_built .stamp\_staging\_installed .stamp\_target\_installed`
- `./m.bld.tf`

### 在UI中使用*dalog*:

仅仅合并`__USE_DALOG__`宏包起来的部分即可。
- `meld dazhu/patch/ccomx_NpObj.cpp ../../nemotv/src/ccomx/npinf-plugin/ccomx_NpObj.cpp`

在UI中使用*dalog*：
- `meld dazhu/patch/Log.js ../../netcfg/???/mars/client/fw/apps/core/Log.js`

### 启动的时候停一下，为了有机会配置一下大杀器：

1. 非*Release*的工程，修改　`/etc/init.d/rcS`，在调用*pcd*之前，加上如下行：
    > `PS1="BKM> " /bin/sh`
1. *Release*版本，在sysinit调用*pcd*前，调用阻塞式的子进程来启动*/bin/sh*。
    1. 使用 `patch/spawn_with_env.c` 来启动Shell（参加该文件开头的说明）。
    1. 使用 `init_run` 函数调用*/bin/sh*，这个函数的问题是不能传递环境变量。

### nemotv/src/network/Makefile.am
这个是为了能在network下调用的子进程中可以打印输出。

1. 直接修改*Makefile.am*，在相应的位置增加如下两行：
    - `dhclientscript_LDADD += -lutils`
    - `dhclientscript_CFLAGS =-Wl,-rpath,/lib:/usr/lib:/usr/local/lib`
1. 合并已经修改后的*Makefile.am*：
    - `meld dazhu/patch/network_Makefile.am ../../nemotv/src/network/Makefile.am`

### nemotv/external/ntvwebkit/Source/WebKit/gtk/webkit/webkitwebview.cpp
修改 *NTV_WEBKITVIEW_LOG* 宏为 `#define NTV_WEBKITVIEW_LOG nsulog_info` 即可。
- `vim ../../nemotv/external/ntvwebkit/Source/WebKit/gtk/webkit/webkitwebview.cpp`

