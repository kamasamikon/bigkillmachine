
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


# 大杀器的编译:
- 编译相关的东西
    - build/config:
        - 编译目标平台的配置，比如 make arch.x86
    - build/install:
        - 生成的结果放到这里

# 命名
- dr : change runtime configure
- inotdo : 我不干，监视文件变化，并调用命令。
- dagou : 钩子
- daxia : 下水道
- dabao : 包装其他进程，并应用da环境变量
- dagan : 大干，应用DA
- daben : 大奔，bench-mark tool
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
### nemotv/src/network/Makefile.am
这个是为了能在network下调用的子进程中可以打印输出。

- Add:
    - dhclientscript\_LDADD += -lutils
    - dhclientscript\_CFLAGS =-Wl,-rpath,/lib:/usr/lib:/usr/local/lib
- Or:
    - meld nemotv/src/network/Makefile.am bigkillmachine/patch/network\_Makefile.am

### otvutils
- meld bigkillmachine/templ/ntvlog

### DBUS
- meld bigkillmachine/patch/dbus-connection.c bld\_tf/build/...
- rm .stamp\_built .stamp\_staging\_installed .stamp\_target\_installed 

### rcS 或者 sysinit:
- rcS
    - Before pcd
        - cd /root
        - PS1="BKM>  " /bin/sh
- sysinit
    - 使用 patch/spawn\_with\_env.c 来启动Shell。

### UI:
- bigkillmachine/patch/ui/ccomx/
- bigkillmachine/patch/ui/js/ 


