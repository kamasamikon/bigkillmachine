# NOTE:
- 工具链：
    - NEMOBUILD的工具链把**LD\_PRELOAD**干掉了，要用老的ld-uClibc-??? 才可以。
- Release版本的问题:
    - LXC中console被关闭了，dazhu/bkmpost.sh中set\_lxc\_console将其重新打开了。
    - 网络也被关闭了，如果需要Log通过网络输出到别的机器上，需要把网络打开。

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
-    dr : change runtime configure
- dagou : 钩子
- daxia : 下水道
- dabao : 包装其他进程，并应用da环境变量
- dagan : 大干，应用DA
- daxiu : 修复，清除DA
- daben : 大奔，bench-mark tool
- muzei : 木贼，偷梁换柱之用

# 如何使用
- `cd ntvtgt/shit`
- `ln -s daxia .`
- `ln -s bkm/tools/m m.bld.tf`    # 注：bld对应BUILD\_TYPE, tf对应CONFIG\_TYPE
- `./m.bld.tf`


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


# 其他修改
- nemotv/src/network/Makefile.am
    - Add:
        - dhclientscript_LDADD += -lutils
        - dhclientscript_CFLAGS =-Wl,-rpath,/lib:/usr/lib:/usr/local/lib
    - Or:
        - meld nemotv/src/network/Makefile.am bigkillmachine/patch/network_Makefile.am

- otvutils
    - meld bigkillmachine/templ/ntvlog

- BKM
    - ln -s bigkillmachine/dazhu .
    - ln -s bigkillmachine/dazhu/m m.bld.tf
        - bld : build type is bld
        - tf : configure type is tf, telefonica

- DBUS
    - meld bigkillmachine/patch/dbus-connection.c bld_tf/build/...
    - rm .stamp_built .stamp_staging_installed .stamp_target_installed 

- rcS 或者 sysinit:
    - rcS
        - Before pcd
            - cd /root
            - PS1="BKM>  " /bin/sh
    - sysinit
        - 使用 patch/spawn_with_env.c 来启动Shell。

- UI:
    - bigkillmachine/patch/ui/ccomx/
    - bigkillmachine/patch/ui/js/ 


