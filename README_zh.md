# Framework

基于线程池的 C++ 异步任务调度框架。

本框架将应用程序代码组织为一个个**模块（module）**，所有工作单元——任务、事件、
定时器回调——都通过内置线程池统一调度。每个模块可以自主选择任务的执行方式：
并发执行、严格串行执行、投递时立即执行，或者通过自定义 handler 将多个模块
绑定到同一个工作线程上执行。

## 特性

- **模块化架构** —— 应用逻辑拆分为相互解耦的模块，模块之间通过投递任务/事件
  通信，而不是直接互相调用。
- **四种模块执行模式**：
  | 模式 | 行为 |
  |------|------|
  | `concurrently_executing` | 模块的任务可以并行运行在不同的工作线程上。 |
  | `sequence_executing` | 模块的任务**保证不会并发执行**，严格一条接一条地执行，从而消除模块内部的大部分锁竞争。 |
  | `execute_task_when_post` | 任务在 `post_task()` 时**直接在投递线程中同步执行**，不进入线程池。 |
  | `handler_shchedule` | 任务由用户注册的 `module_task_handler` 调度。 |
- **自定义任务调度器（`module_task_handler`）** —— 多个模块可以共享同一个 handler
  实例，这些模块将在**同一个工作线程上**依次串行执行。可用它来表达模块间的依赖
  /分组关系：要求互不并行执行的模块，绑定到同一个线程即可。
- **自适应线程池** —— 当任务等待时间过长（见 `thread_manager::s_max_task_time_out`）
  时按需创建新线程；长期空闲的线程自动回收（线程数上限为 `s_max_worker_num`）。
- **线程池看门狗** —— 监控每个任务的执行时长，超过阈值（默认 10 秒）即上报；
  默认行为是终止进程（`std::abort()`），可通过 `set_timeout_callback()` 替换。
  死锁或卡死的任务会被检测出来，而不是让整个系统无声地挂起。
- **定时器模块** —— 支持周期/单次定时器，可命名、限制触发次数、重置间隔、
  取消。超时回调可以分发到指定模块，从而在该模块的任务上下文中执行。
- **框架事件** —— 开机 / 关机 / 电源状态变化事件按序广播给所有模块。
- **信息管理中心** —— 全局 `abstract_information` 对象的线程安全注册表
  （设备信息、配置、模块间共享状态等）。
- **日志工具**（`log_util`）与平台抽象层（`internal/platform`）。

## 架构

```
                       framework_manager（单例）
        ┌──────────────────┼─────────────────────┐
        │                  │                     │
  module_manager     thread_manager       information_manager
  （模块注册表、      （线程池、
   事件分发）         任务调度、看门狗）
        │
   业务模块  ── 任务/事件 ──>  module_manager::schedule_task()
                             thread_manager::post_task()
```

每个任务都带有**目标模块名**。线程管理器根据该模块的执行模式进行调度：

- 串行模式 → 追加到模块的待执行队列，整个队列被分配给同一个工作线程，
  模块代码始终是单线程运行的；
- handler 模式 → 转交给模块的 `module_task_handler` 处理；
- 并发模式 → 立即分配给任意空闲工作线程。

## 快速上手

### 1. 定义一个模块

```cpp
class my_module : public framework::abstract_module
{
public:
    my_module()
    {
        set_name( "my_module" );
        // 选择一种执行模式：
        set_module_type( module_type::sequence_executing );
        // set_module_type( module_type::concurrently_executing );
        // set_module_type( module_type::execute_task_when_post );
    }

    void initialize() override
    {
        set_power_status( powering_status::power_on );
    }

    void deinitialize() override
    {
        set_power_status( powering_status::power_off );
    }

    void handle_task( std::shared_ptr<framework::abstract_task> a_task ) override
    {
        // 处理投递到本模块的任务。
        // 在 sequence_executing 模式下，此代码永远不会与自身并发执行，
        // 模块内状态无需加锁保护。
    }

    void handle_event( std::shared_ptr<framework::framework_event> a_event ) override
    {
        // 处理框架事件（开机、关机等）。
    }
};
```

### 2. 定义一个任务

```cpp
class my_task : public framework::abstract_task
{
public:
    my_task()
    {
        set_task_type( task_type::executable_task ); // 或使用自己注册的任务类型
    }
    int m_parameter = 0;
};
```

或者直接用 lambda 包装：

```cpp
auto task = std::make_shared<framework::executable_task>(
    []() { /* 运行在 task_runner_module 上 */ return false; } );
```

### 3. 启动框架

```cpp
std::vector<std::shared_ptr<framework::abstract_module>> make_modules()
{
    std::vector<std::shared_ptr<framework::abstract_module>> modules;
    modules.push_back( std::make_shared<my_module>() );
    modules.push_back( std::make_shared<framework::timer_module>() );
    return modules;
}

int main()
{
    auto& fm = framework::framework_manager::get_instance();

    // 可以在 run() 之前或之后投递任务；线程池启动前任务会排队等待。
    fm.get_thread_manager().post_task( []()
        {
            auto task = std::make_shared<my_task>();
            task->set_target_module( "my_module" );
            framework::framework_manager::get_instance()
                .get_module_manager().schedule_task( task );
        }, framework::source_here );

    // 阻塞当前线程并运行框架
    //（传入 false 则立即返回）。
    fm.run( &make_modules, /*a_occupy_current_thread =*/ true );
    return 0;
}
```

### 4. 将相互依赖的模块绑定到同一线程

```cpp
auto handler = std::make_shared<framework::module_task_handler>( "media_group" );
module_a->set_task_handler( handler );
module_b->set_task_handler( handler );
// module_a 与 module_b 现在会在同一个工作线程上依次串行执行，
// 两者永远不会并行运行。
```

### 5. 使用定时器

```cpp
auto timer = fm.get_module_manager().get_module<framework::timer_module>(
    framework::abstract_module::s_timer_module_name );

uint32_t id = timer->register_timer(
    []() { /* 周期回调 */ return false; },                  // 返回 true 表示取消该定时器
    std::chrono::milliseconds( 1000 ),
    0,                                                      // 0 = 一直运行直到被取消
    "my_module" );                                          // 可选：回调分发到指定模块执行

timer->register_once_timer( []() { /* 单次执行 */ },
    std::chrono::milliseconds( 500 ), "startup_timer" );

timer->undregister_timer( id );
```

## 内置模块

| 模块 | 名称常量 | 用途 |
|------|----------|------|
| `module_manager` | `s_module_manager_name` | 模块注册表、任务调度入口、框架事件分发。 |
| `timer_module` | `s_timer_module_name` | 周期 / 单次定时器。 |
| `task_runner_module` | `s_task_runner_module_name` | `executable_task`（普通 lambda）的默认目标模块。 |
| `general_seq_task_runner_module` | `s_general_seq_task_runner_module` | 通用串行任务执行器。 |

## 编译要求

- C++20（使用了 `<format>`、concept 等现代库特性）
- 仓库提供 Visual Studio 解决方案：`ms_build/`（framework DLL + 测试工程）
- 测试：见 `test/` 目录，包含压力/功能测试，例如 20 个串行模块处理 20 万+
  任务并随机访问动态内存（连续运行 10 分钟不崩溃即通过）、模块任务调度器测试、
  定时器测试。

## 许可证

MIT License。Copyright (c) 2009-2026 WangTingMan <75142601@qq.com>
