# Framework

A thread-pool based asynchronous task scheduling framework written in standard C++.

It organizes application code into **modules** and dispatches every unit of work —
tasks, events and timer callbacks — through a built-in thread pool. Each module can
choose how its tasks are executed: concurrently, strictly sequentially, immediately
at post time, or through a user-defined handler that binds several modules onto the
same worker thread.

## Features

- **Module-based architecture** — application logic is split into decoupled modules
that communicate by posting tasks/events to each other instead of calling each
other directly.
- **Four module execution modes**:

| Mode | Behavior |
| --- | --- |
| `concurrently_executing` | Tasks of the module may run in parallel on different workers. |
| `sequence_executing` | Tasks of the module are guaranteed to **never run concurrently** — they are executed one by one, which removes most lock contention inside the module. |
| `execute_task_when_post` | Tasks run **immediately in the posting thread** at `post_task()` time, without entering the pool. |
| `handler_shchedule` | Tasks are scheduled by a user-registered `module_task_handler`. |

- **Custom task handler (`module_task_handler`)** — several modules can share one
handler instance, which makes them execute sequentially on the **same worker
thread**. Use it to model module dependency/groups: modules that must not run in
parallel with each other are pinned to one thread together.
- **Self-tuning thread pool** — worker threads are created on demand when tasks
wait too long (see `thread_manager::s_max_task_time_out`) and long-idle workers
are dismissed automatically (bounded by `s_max_worker_num`).
- **Thread pool watchdog** — monitors how long each running task executes. A task
exceeding the timeout (default 10 s) is reported; by default the process is
aborted (`std::abort()`), and the behavior can be replaced via
`set_timeout_callback()`. Dead-locked or stuck workers are therefore detected
instead of hanging the system silently.
- **Timer module** — periodic / one-shot timers with optional name, trigger-count
limit, interval reset and cancel. The timeout callback can be dispatched to a
target module, so it executes in that module's task context.
- **Framework events** — power on / power off / power status changed events are
broadcast to all modules in a defined order.
- **Information manager** — a central, thread-safe registry for global
`abstract_information` objects (device info, configuration, state shared
between modules).
- **Portable logging utility** (`log_util`) and platform abstraction layer
(`internal/platform`).

## Architecture

```javascript
                       framework_manager (singleton)
        ┌──────────────────┼─────────────────────┐
        │                  │                     │
  module_manager     thread_manager       information_manager
   (module registry,  (thread pool,
    event dispatch)    task scheduler,
                       watchdog)
        │
   your modules  ── tasks/events ──>  module_manager::schedule_task()
                                     thread_manager::post_task()
```

A task carries a **target module name**. The thread manager looks up the module's
execution mode and schedules accordingly:

- sequence mode → appended to the module's pending queue; the whole queue is
assigned to one worker, so module code is always single-threaded;
- handler mode → delegated to the module's `module_task_handler`;
- concurrent mode → assigned to any idle worker immediately.

## Quick Start

### 1. Define a module

```cpp
class my_module : public framework::abstract_module
{
public:
    my_module()
    {
        set_name( "my_module" );
        // Choose one execution mode:
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
        // Handle tasks posted to this module.
        // In sequence_executing mode this code never runs concurrently
        // with itself — no locks needed for module state.
    }

    void handle_event( std::shared_ptr<framework::framework_event> a_event ) override
    {
        // Handle framework events (power on/off, ...).
    }
};
```

### 2. Define a task

```cpp
class my_task : public framework::abstract_task
{
public:
    my_task()
    {
        set_task_type( task_type::executable_task ); // or your own registered type
    }
    int m_parameter = 0;
};
```

Or wrap a plain lambda:

```cpp
auto task = std::make_shared<framework::executable_task>(
    []() { /* runs on task_runner_module */ return false; } );
```

### 3. Run the framework

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

    // Post tasks before or after run(); they are queued until the pool starts.
    fm.get_thread_manager().post_task( []()
        {
            auto task = std::make_shared<my_task>();
            task->set_target_module( "my_module" );
            framework::framework_manager::get_instance()
                .get_module_manager().schedule_task( task );
        }, framework::source_here );

    // Blocks the calling thread and runs the framework
    // (pass false to return immediately).
    fm.run( &make_modules, /*a_occupy_current_thread =*/ true );
    return 0;
}
```

### 4. Pin dependent modules to one thread

```cpp
auto handler = std::make_shared<framework::module_task_handler>( "media_group" );
module_a->set_task_handler( handler );
module_b->set_task_handler( handler );
// module_a and module_b now execute sequentially on the same worker thread
// and can never run in parallel with each other.
```

### 5. Use timers

```cpp
auto timer = fm.get_module_manager().get_module<framework::timer_module>(
    framework::abstract_module::s_timer_module_name );

uint32_t id = timer->register_timer(
    []() { /* periodic callback */ return false; },       // return true to cancel
    std::chrono::milliseconds( 1000 ),
    0,                                                    // 0 = run until canceled
    "my_module" );                                        // optional: dispatch to module

timer->register_once_timer( []() { /* once */ },
    std::chrono::milliseconds( 500 ), "startup_timer" );

timer->undregister_timer( id );
```

## Built-in Modules

| Module | Name constant | Purpose |
| --- | --- | --- |
| `module_manager` | `s_module_manager_name` | Module registry, task scheduling entry, framework event dispatch. |
| `timer_module` | `s_timer_module_name` | Periodic / one-shot timers. |
| `task_runner_module` | `s_task_runner_module_name` | Default target for `executable_task` (plain lambdas). |
| `general_seq_task_runner_module` | `s_general_seq_task_runner_module` | Sequential runner for arbitrary posted work. |

## Requirements & Build

- C++20 (uses `<format>`, concepts and modern library features)
- Visual Studio solution provided under `ms_build/` (framework DLL + test projects)
- Tests: `test/` contains press/functional tests, e.g. 20 sequential modules
processing 200k+ tasks with dynamic memory access (run 10 minutes without
crash = pass), module task handler test and timer test.

## License

MIT License. Copyright (c) 2009-2026 WangTingMan <75142601@qq.com>