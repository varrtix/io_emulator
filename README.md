# IO Emulator

基于 CTF 开发的仿真套件，支持 Windows 和 Linux 平台。套件提供了用于模拟真实电气的仿真驱动，以及用于仿真数据注入的旁路工具，名叫 `io_test` 。

> **NOTE:** 从本质上，仿真套件设计为增量型的工具；其通过仿真驱动替换真实的电气驱动，利用 CTF 的 IO 组件功能，实现了脱离真实电气的单元数据仿真。所以，从原理上，仿真套件类似于管道，作用于 IO 与 Drivers 之间。

```
+ - - - - - - - - - - - - - - - - - - - - -+
' IO Emulator                              '
'                                          '
' +-----------+         +----------------+ '
' |  IO TEST  | ------> | Emulator Core  | '
' +-----------+         +----------------+ '
'                                          '
+ - - - - - - - - - - - - - - - - - - - - -+
    |                     ^
    |                     |
    v                     |
+ - - - - - - - +     + - - - - - - - - - -+
' CTF           '     ' Emulated Drivers   '
'               '     '                    '
' +-----------+ '     ' +----------------+ '
' |    IO     | ' --> ' |    Driver A    | '
' +-----------+ '     ' +----------------+ '
' +-----------+ '     ' +----------------+ '
' |    LOG    | '     ' |    Driver B    | '
' +-----------+ '     ' +----------------+ '
' +-----------+ '     ' +----------------+ '
' |    ...    | '     ' |      ...       | '
' +-----------+ '     ' +----------------+ '
'               '     '                    '
+ - - - - - - - +     + - - - - - - - - - -+
```

## 快速开始

### Linux

首先，先了解一下 Linux 发行包的目录结构，如下：

```
.
├── bin
│   └── io_test // 数据注入程序
└── lib
    ├── libdriverA.so // 仿真驱动A
    ├── libdriverB.so // 仿真驱动B
    └── libxxx.so // ...
```

#### 步骤

1. 将 **lib** 目录下的各自所需的驱动，拷贝到对应产品工程的 `bin/<BuildType>/drivers` 目录下，替换真实的电气驱动；

> **注意：** 产品工程目录，非 CTF 目录。

2. 将 `io_test` 拷贝到 CTF 的 `ctf/bin/<BuildType>` 目录下；

> **TIPS:** 设置了上面第二步的目录至 **LD_LIBRARY_PATH**，原理上 `io_test` 可在任意的文件路径下执行。

3. 启动 `./ctf_log_server -n <LOG name>`；

4. 启动 `./ctf_io_server -n <IO name>`；

5. 启动 `path/to/io_test_folder/io_test -n <PromptName>`；

> **PromptName** 可在产品工程的 `workspace/conf/conf-equipment.xml` 中配置，`io_test` 支持被配置成一个类似 *PM* 单元，但是因为是 CLI 程序，所以无法通过 *ctf_service_manager* 启动。

### Windows

首先，先了解一下 Windows 发行包的目录结构，如下：

```
.
├── bin
│   ├── io_test.exe // 数据注入程序
│   ├── libdriverA.dll // 仿真驱动A
│   ├── libdriverB.dll // 仿真驱动B
│   └── libxxx.dll // ...
└── lib
    └── libxxx.lib // 保留，暂未使用
```

#### 步骤

1. 将 **bin** 目录下的各自所需的驱动，拷贝到对应产品工程的 `bin/<BuildType>/drivers` 目录下，替换真实的电气驱动；

> **注意：** 产品工程目录，非 CTF 目录。

2. 将 `io_test` 拷贝到 CTF 的 `ctf/bin/<BuildType>` 目录下；

> **TIPS:** 设置了上面第二步的目录至 **PATH**，原理上 `io_test` 可在任意的文件路径下执行。

3. 启动 CTF 的 LOG 和 IO 服务；

5. 启动 `path/to/io_test_folder/io_test -n <PromptName>`；

> **PromptName** 可在产品工程的 `workspace/conf/conf-equipment.xml` 中配置，`io_test` 支持被配置成一个类似 *PM* 单元，但是因为是 CLI 程序，所以无法通过 *ctf_service_manager* 启动。

## `io_test` 使用说明

`io_test` 为跨平台支持的 CLI 交互式可执行程序。主要用于仿真数据的注入与查询，提供命令和 *ITEM* 的自动补全。

### 交互操作

本程序本着 *最小惊讶原则*，严格遵循并保持了 **GNU/Emacs** 的交互操作习惯，支持如 `C-c`, `C-n` 等常见快捷键；同时，支持历史记录，自动补全和无条件中断等功能。

### 命令详解

程序的命令使用习惯借鉴了 `sftp` 的风格。提供了如下命令：

+ get \<ItemName\>: 查询对应 ItemName 的数值，支持补全

+ set \<ItemName\> \<value\>: 注入对应 ItemName 的数值，支持补全

+ info \<ItemName\> | all: 打印对应 ItemName 的信息，`info all` 打印所有 Item 的信息

+ help: 帮助界面

+ exit：退出本程序

> **注意：** 就行为上而言，`set` 命令会遇到配置中 `pw` 为空的情况。如遇到，注入操作会在 `pw` 为空的情况下，尝试以 `pr` 的值作为注入所需的键。考虑到 `conf-io.xml` 的配置特性，目前这种情况下的行为策略为固定的。

### 环境变量

+ **IOXML_CONF_PATH**: 支持外部自定义注入 `conf-io.xml`。值为配置文件路径，不可为配置所在的文件夹路径。

> *程序会优先读取环境变量指向的配置文件，仅当此环境变量未设置时，才会读取默认工程路径下的 `conf-io.xml`，这依赖于 CTF 对于路径配置的行为。*

## Q&A

+ `io_test` 高度依赖于 CTF 的 IO 服务，所以 IO 服务如果没有启动，`io_test` 便无法正常使用。

+ 通过 `io_test` 注入的仿真数据，默认是 *易失性* 的。即，其数据会伴随着 IO 服务的停止，而被自动清除。

> **解决方案：** 通过产品工程下 `workspace/conf/conf-io.xml` 配置对应 IO 类型 *ITEM* 的 **pst**，将其设置为 **True**；可利用 IO 服务的这个特性实现持久化，仿真数据会因此被存入 `persist.data` 中，便于复制与迁移备份。详细关于方案的持久化内容，参见 CTF 的 IO 使用说明。

+ `Windows` 下，`io_test` 出现执行后闪退的情况。

> **解决方案：** 通过追加设置 CTF 的执行目录至环境变量 `PATH` 解决。具体可参考命令：`set PATH=/path/to/ctf/bin/<BuildType>;%PATH% && io_test.exe -n <PromptName>`。
