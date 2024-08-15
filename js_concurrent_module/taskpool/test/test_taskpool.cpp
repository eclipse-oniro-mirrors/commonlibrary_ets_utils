/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "test.h"

#include <unistd.h>

#include "helper/napi_helper.h"
#include "queue.h"
#include "task.h"
#include "taskpool.h"
#include "task_manager.h"
#include "task_runner.h"
#include "thread.h"
#include "tools/log.h"
#include "worker.h"

using namespace Commonlibrary::Concurrent::TaskPoolModule;
void GetSendableFunction(napi_env env, const char* str, napi_value& result)
{
    napi_value instance = SendableUtils::CreateSendableInstance(env);
    napi_value name = nullptr;
    napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &name);
    napi_get_property(env, instance, name, &result);
}

napi_value GeneratorTask(napi_env env, napi_value thisVar)
{
    napi_value func = nullptr;
    GetSendableFunction(env, "foo", func);
    napi_value argv[] = { func };
    std::string funcName = "TaskConstructor";
    napi_value cb = nullptr;
    napi_value task = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::TaskConstructor, nullptr, &cb);
    napi_call_function(env, thisVar, cb, 1, argv, &task);
    return task;
}

napi_value GeneratorTaskGroup(napi_env env, napi_value thisVar)
{
    std::string funcName = "TaskGroupConstructor";
    napi_value argv[] = {};
    napi_value cb = nullptr;
    napi_value group = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskGroup::TaskGroupConstructor, nullptr, &cb);
    napi_call_function(env, thisVar, cb, 0, argv, &group);
    return group;
}

napi_value GeneratorTaskGroupWithName(napi_env env, napi_value thisVar, const char* name)
{
    std::string funcName = "TaskGroupConstructor";
    napi_value str = nullptr;
    napi_create_string_utf8(env, name, NAPI_AUTO_LENGTH, &str);
    napi_value argv[] = { str };
    napi_value cb = nullptr;
    napi_value group = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskGroup::TaskGroupConstructor, nullptr, &cb);
    napi_call_function(env, thisVar, cb, 1, argv, &group);
    return group;
}

napi_value CreateTaskObject(napi_env env, TaskType taskType = TaskType::TASK)
{
    napi_value thisValue = NapiHelper::CreateObject(env);
    size_t argc = 0;
    napi_value func = nullptr;
    napi_create_string_utf8(env, "testFunc", NAPI_AUTO_LENGTH, &func);
    napi_value* args = new napi_value[argc];
    napi_value taskName = NapiHelper::CreateEmptyString(env);
    Task* task = Task::GenerateTask(env, thisValue, func, taskName, args, argc);
    task->UpdateTaskType(taskType);
    napi_wrap(
        env, thisValue, task,
        [](napi_env environment, void* data, void* hint) {
            auto obj = reinterpret_cast<Task*>(data);
            if (obj != nullptr) {
                delete obj;
            }
        }, nullptr, nullptr);
    return thisValue;
}

HWTEST_F(NativeEngineTest, TaskpoolTest001, testing::ext::TestSize.Level0)
{
    TaskManager &taskManager = TaskManager::GetInstance();
    uint32_t result = taskManager.GetThreadNum();
    ASSERT_TRUE(result == 0);
}

HWTEST_F(NativeEngineTest, TaskpoolTest002, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    Worker* worker = Worker::WorkerConstructor(env);
    usleep(50000);
    ASSERT_NE(worker, nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest003, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    napi_value taskClass = nullptr;
    napi_value result = TaskPool::InitTaskPool(env, taskClass);
    usleep(50000);
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest004, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    Worker* worker = Worker::WorkerConstructor(env);
    usleep(50000);
    ASSERT_NE(worker, nullptr);
    uint32_t workers = TaskManager::GetInstance().GetRunningWorkers();
    ASSERT_TRUE(workers == 0);
}

HWTEST_F(NativeEngineTest, TaskpoolTest005, testing::ext::TestSize.Level0)
{
    uint64_t taskId = 10;
    TaskManager &taskManager = TaskManager::GetInstance();
    Task* task = taskManager.GetTask(taskId);
    ASSERT_TRUE(task == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest006, testing::ext::TestSize.Level0)
{
    TaskManager &taskManager = TaskManager::GetInstance();
    std::pair<uint64_t, Priority> result = taskManager.DequeueTaskId();
    ASSERT_TRUE(result.first == 0);
    ASSERT_TRUE(result.second == Priority::LOW);
}

HWTEST_F(NativeEngineTest, TaskpoolTest007, testing::ext::TestSize.Level0)
{
    TaskManager &taskManager = TaskManager::GetInstance();
    uint32_t result = taskManager.GetTaskNum();
    ASSERT_TRUE(result == 0);
}

HWTEST_F(NativeEngineTest, TaskpoolTest008, testing::ext::TestSize.Level0)
{
    ExecuteQueue executeQueue;
    uint64_t result = executeQueue.DequeueTaskId();
    ASSERT_TRUE(result == 0);
}

HWTEST_F(NativeEngineTest, TaskpoolTest009, testing::ext::TestSize.Level0)
{
    ExecuteQueue executeQueue;
    bool result = executeQueue.IsEmpty();
    ASSERT_TRUE(result);
}

HWTEST_F(NativeEngineTest, TaskpoolTest010, testing::ext::TestSize.Level0)
{
    ExecuteQueue executeQueue;
    uint32_t result = executeQueue.GetTaskNum();
    ASSERT_TRUE(result == 0);
}

HWTEST_F(NativeEngineTest, TaskpoolTest011, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {nullptr};
    std::string funcName = "TaskGroupConstructor";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskGroup::TaskGroupConstructor, nullptr, &cb);
    napi_call_function(env, nullptr, cb, 0, argv, &result);
    ASSERT_TRUE(result != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest012, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {nullptr};
    std::string funcName = "TaskGroupConstructor";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskGroup::TaskGroupConstructor, nullptr, &cb);
    napi_call_function(env, nullptr, cb, 0, argv, &result);
    ASSERT_TRUE(result != nullptr);
    napi_value exception;
    napi_get_and_clear_last_exception(env, &exception);

    size_t argc1 = 0;
    napi_value argv1[] = {nullptr};
    funcName = "AddTask";
    cb = nullptr;
    napi_value result1 = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskGroup::AddTask, nullptr, &cb);
    napi_call_function(env, nullptr, cb, argc1, argv1, &result1);
    ASSERT_TRUE(result1 == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest013, testing::ext::TestSize.Level0)
{
    uint32_t number = 10;
    napi_env env = reinterpret_cast<napi_env>(engine_);
    napi_value value = NapiHelper::CreateUint32(env, number);
    napi_value result = TaskPool::InitTaskPool(env, value);
    usleep(50000);
    ASSERT_TRUE(result != nullptr);
}

napi_value TestFunction(napi_env env)
{
    napi_value result = nullptr;
    const char* message = "test taskpool";
    size_t length = strlen(message);
    napi_create_string_utf8(env, message, length, &result);
    return result;
}

HWTEST_F(NativeEngineTest, TaskpoolTest014, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    usleep(50000);
    uint32_t taskNum = taskManager.GetTaskNum();
    ASSERT_TRUE(taskNum == 0);
    uint32_t threadNum = taskManager.GetThreadNum();
    ASSERT_TRUE(threadNum != 0);
    uint32_t idleWorkers = taskManager.GetIdleWorkers();
    ASSERT_TRUE(idleWorkers != 0);
    uint32_t runningWorkers = taskManager.GetRunningWorkers();
    ASSERT_TRUE(runningWorkers == 0);
    uint32_t timeoutWorkers = taskManager.GetTimeoutWorkers();
    ASSERT_TRUE(timeoutWorkers == 0);
}

HWTEST_F(NativeEngineTest, TaskpoolTest015, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    std::pair<uint64_t, Priority> result = taskManager.DequeueTaskId();
    ASSERT_TRUE(result.first == 0);
    ASSERT_TRUE(result.second == Priority::LOW);
}

HWTEST_F(NativeEngineTest, TaskpoolTest016, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    ExceptionScope scope(env);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    Task* task = new Task();
    uint64_t taskId = reinterpret_cast<uint64_t>(task);
    taskManager.CancelTask(env, taskId);
    ASSERT_TRUE(taskId != 0);
    delete task;
}

HWTEST_F(NativeEngineTest, TaskpoolTest017, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    Task* task = new Task();
    uint64_t taskId = reinterpret_cast<uint64_t>(task);
    taskManager.TryTriggerExpand();
    ASSERT_TRUE(taskId != 0);
    delete task;
}

HWTEST_F(NativeEngineTest, TaskpoolTest018, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    Task* task = new Task();
    uint64_t taskId = reinterpret_cast<uint64_t>(task);
    uint64_t duration = 10;
    taskManager.UpdateExecutedInfo(duration);
    ASSERT_TRUE(taskId != 0);
    delete task;
}

HWTEST_F(NativeEngineTest, TaskpoolTest019, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {nullptr};
    std::string funcName = "TaskConstructor";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::TaskConstructor, nullptr, &cb);
    napi_call_function(env, nullptr, cb, 0, argv, &result);
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest020, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {nullptr};
    std::string funcName = "TaskConstructor";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::TaskConstructor, nullptr, &cb);
    napi_call_function(env, nullptr, cb, 0, argv, &result);
    ASSERT_TRUE(result == nullptr);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);

    cb = nullptr;
    napi_value result1 = nullptr;
    funcName = "SetTransferList";
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::SetTransferList, nullptr, &cb);
    napi_call_function(env, nullptr, cb, 0, argv, &result1);
    ASSERT_TRUE(result1 != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest021, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    size_t argc = 10;
    napi_value argv[] = {nullptr};
    std::string funcName = "TaskConstructor";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::TaskConstructor, nullptr, &cb);
    napi_call_function(env, nullptr, cb, 0, argv, &result);
    ASSERT_TRUE(result == nullptr);

    napi_value func = TestFunction(env);
    uint32_t number = 10;
    napi_value value = NapiHelper::CreateUint32(env, number);
    napi_value* args = new napi_value[argc];
    napi_value taskName = NapiHelper::CreateEmptyString(env);
    Task::GenerateTask(env, value, func, taskName, args, argc);
    ASSERT_TRUE(args != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest022, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    ExceptionScope scope(env);
    TaskGroupManager &taskGroupManager = TaskGroupManager::GetInstance();
    TaskGroup* group = new TaskGroup();
    uint64_t groupId = reinterpret_cast<uint64_t>(group);
    Task* task = new Task();
    uint64_t taskId = reinterpret_cast<uint64_t>(task);
    napi_value value = NapiHelper::CreateUint64(env, groupId);
    napi_ref reference = NapiHelper::CreateReference(env, value, 0);
    taskGroupManager.AddTask(groupId, reference, taskId);
    ASSERT_NE(reference, nullptr);
    delete task;
    delete group;
}

HWTEST_F(NativeEngineTest, TaskpoolTest023, testing::ext::TestSize.Level0)
{
    TaskGroupManager &taskGroupManager = TaskGroupManager::GetInstance();
    TaskGroup* group = new TaskGroup();
    uint64_t groupId = reinterpret_cast<uint64_t>(group);
    TaskGroup* taskGroup = taskGroupManager.GetTaskGroup(groupId);
    ASSERT_TRUE(taskGroup == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest024, testing::ext::TestSize.Level0)
{
    MessageQueue<int*> mesQueue;
    int testData = 42;
    mesQueue.EnQueue(&testData);

    auto result = mesQueue.DeQueue();
    ASSERT_EQ(testData, *result);
}

HWTEST_F(NativeEngineTest, TaskpoolTest025, testing::ext::TestSize.Level0)
{
    MessageQueue<std::string> mesQueue;
    ASSERT_EQ(mesQueue.IsEmpty(), true);

    std::string testStr = "hello";
    mesQueue.EnQueue(testStr);
    ASSERT_EQ(mesQueue.IsEmpty(), false);
}

HWTEST_F(NativeEngineTest, TaskpoolTest026, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {nullptr};
    std::string funcName = "SeqRunnerConstructor";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), SequenceRunner::SeqRunnerConstructor, nullptr, &cb);
    napi_call_function(env, nullptr, cb, 0, argv, &result);
    ASSERT_NE(result, nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest027, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {nullptr};
    std::string func = "SeqRunnerConstructor";
    napi_value callback = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, func.c_str(), func.size(), SequenceRunner::SeqRunnerConstructor, nullptr, &callback);
    napi_call_function(env, nullptr, callback, 0, argv, &result);
    ASSERT_NE(result, nullptr);

    size_t argc1 = 0;
    napi_value argv1[] = {nullptr};
    func = "Execute";
    callback = nullptr;
    napi_value result1 = nullptr;
    napi_create_function(env, func.c_str(), func.size(), SequenceRunner::Execute, nullptr, &callback);
    napi_call_function(env, nullptr, callback, argc1, argv1, &result1);
    ASSERT_TRUE(result1 == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest028, testing::ext::TestSize.Level0)
{
    TaskGroup taskGroup;
    uint32_t taskId = 10;
    uint32_t index = taskGroup.GetTaskIndex(taskId);
    ASSERT_EQ(index, 0);
}

HWTEST_F(NativeEngineTest, TaskpoolTest029, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskGroup taskGroup;
    taskGroup.NotifyGroupTask(env);
    TaskManager &taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint32_t taskId = 11;
    ASSERT_EQ(taskId, 11);
}

HWTEST_F(NativeEngineTest, TaskpoolTest030, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskGroup taskGroup;
    taskGroup.CancelPendingGroup(env);
    TaskManager &taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint32_t taskId = 12;
    ASSERT_EQ(taskId, 12);
}

HWTEST_F(NativeEngineTest, TaskpoolTest031, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    Task* task = new Task();
    auto id = reinterpret_cast<uint64_t>(task);
    taskManager.StoreTask(id, task);
    Task* res = taskManager.GetTask(id);
    ASSERT_EQ(task, res);
}

HWTEST_F(NativeEngineTest, TaskpoolTest032, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 14;
    taskManager.RemoveTask(taskId);
    ASSERT_EQ(taskId, 14);
}

HWTEST_F(NativeEngineTest, TaskpoolTest033, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    usleep(50000);
    // the task will freed in the taskManager's Destuctor and will not cause memory leak
    Task* task = new Task();
    auto taskId = reinterpret_cast<uint64_t>(task);
    taskManager.EnqueueTaskId(taskId, Priority::HIGH);
    std::pair<uint64_t, Priority> result = taskManager.DequeueTaskId();
    ASSERT_TRUE(result.first == taskId);
    ASSERT_TRUE(result.second == Priority::HIGH);
}

HWTEST_F(NativeEngineTest, TaskpoolTest034, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    Worker* worker = Worker::WorkerConstructor(env);
    usleep(50000);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    taskManager.NotifyWorkerIdle(worker);
    ASSERT_NE(worker, nullptr);
    taskManager.NotifyWorkerCreated(worker);
    ASSERT_NE(worker, nullptr);
    taskManager.NotifyWorkerRunning(worker);
    ASSERT_NE(worker, nullptr);
    taskManager.RestoreWorker(worker);
    ASSERT_NE(worker, nullptr);
    taskManager.RemoveWorker(worker);
    ASSERT_NE(worker, nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest035, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint32_t step = 8;
    taskManager.GetIdleWorkersList(step);
    ASSERT_EQ(step, 8);
}

HWTEST_F(NativeEngineTest, TaskpoolTest036, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    napi_value res = taskManager.GetThreadInfos(env);
    ASSERT_NE(res, nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest037, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 16;
    std::shared_ptr<CallbackInfo> res = taskManager.GetCallbackInfo(taskId);
    ASSERT_EQ(res, nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest038, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 16;
    taskManager.RegisterCallback(env, taskId, nullptr);
    ASSERT_EQ(taskId, 16);
}

HWTEST_F(NativeEngineTest, TaskpoolTest039, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 17;
    taskManager.IncreaseRefCount(taskId);
    ASSERT_EQ(taskId, 17);
}

HWTEST_F(NativeEngineTest, TaskpoolTest040, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 18;
    taskManager.DecreaseRefCount(env, taskId);
    ASSERT_EQ(taskId, 18);
}

HWTEST_F(NativeEngineTest, TaskpoolTest041, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 19;
    bool res = taskManager.IsDependendByTaskId(taskId);
    ASSERT_EQ(res, false);
}

HWTEST_F(NativeEngineTest, TaskpoolTest042, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 20;
    taskManager.NotifyDependencyTaskInfo(taskId);
    ASSERT_EQ(taskId, 20);
}

HWTEST_F(NativeEngineTest, TaskpoolTest043, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 21;
    std::set<uint64_t> taskSet;
    taskSet.emplace(1);
    taskSet.emplace(2);
    bool res = taskManager.StoreTaskDependency(taskId, taskSet);
    ASSERT_EQ(res, true);
}

HWTEST_F(NativeEngineTest, TaskpoolTest044, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 22;
    uint64_t dependentId = 0;
    bool res = taskManager.RemoveTaskDependency(taskId, dependentId);
    ASSERT_EQ(res, false);
}

HWTEST_F(NativeEngineTest, TaskpoolTest045, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 23;
    std::set<uint64_t> dependentIdSet;
    dependentIdSet.emplace(1);
    std::set<uint64_t> idSet;
    idSet.emplace(2);
    bool res = taskManager.CheckCircularDependency(dependentIdSet, idSet, taskId);
    ASSERT_EQ(res, true);
}

HWTEST_F(NativeEngineTest, TaskpoolTest046, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 24;
    taskManager.EnqueuePendingTaskInfo(taskId, Priority::NUMBER);
    std::pair<uint64_t, Priority> res = taskManager.DequeuePendingTaskInfo(taskId);
    ASSERT_EQ(res.first, 24);
    ASSERT_EQ(res.second, Priority::NUMBER);
}

HWTEST_F(NativeEngineTest, TaskpoolTest047, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 24;
    taskManager.RemovePendingTaskInfo(taskId);
    ASSERT_EQ(taskId, 24);
}

HWTEST_F(NativeEngineTest, TaskpoolTest048, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 25;
    std::set<uint64_t> dependTaskIdSet;
    taskManager.StoreDependentTaskInfo(dependTaskIdSet, taskId);
    ASSERT_EQ(taskId, 25);
}

HWTEST_F(NativeEngineTest, TaskpoolTest049, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 26;
    uint64_t dependentTaskId = 26;
    taskManager.RemoveDependentTaskInfo(dependentTaskId, taskId);
    ASSERT_EQ(taskId, 26);
    ASSERT_EQ(dependentTaskId, 26);
}

HWTEST_F(NativeEngineTest, TaskpoolTest050, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 27;
    uint64_t totalDuration = 25;
    uint64_t cpuDuration = 8;
    taskManager.StoreTaskDuration(taskId, totalDuration, cpuDuration);
    ASSERT_EQ(taskId, 27);
    ASSERT_EQ(totalDuration, 25);
    ASSERT_EQ(cpuDuration, 8);
}

HWTEST_F(NativeEngineTest, TaskpoolTest051, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 28;
    std::string str = "testTaskpool";
    taskManager.GetTaskDuration(taskId, str);
    ASSERT_EQ(taskId, 28);
}

HWTEST_F(NativeEngineTest, TaskpoolTest052, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 29;
    taskManager.RemoveTaskDuration(taskId);
    ASSERT_EQ(taskId, 29);
}

HWTEST_F(NativeEngineTest, TaskpoolTest053, testing::ext::TestSize.Level0)
{
    TaskGroupManager& taskGroupManager = TaskGroupManager::GetInstance();
    uint64_t groupId = 30;
    TaskGroup* group = taskGroupManager.GetTaskGroup(groupId);
    taskGroupManager.StoreTaskGroup(groupId, group);
    ASSERT_EQ(groupId, 30);
}

HWTEST_F(NativeEngineTest, TaskpoolTest054, testing::ext::TestSize.Level0)
{
    TaskGroupManager& taskGroupManager = TaskGroupManager::GetInstance();
    uint64_t groupId = 31;
    taskGroupManager.RemoveTaskGroup(groupId);
    ASSERT_EQ(groupId, 31);
}

HWTEST_F(NativeEngineTest, TaskpoolTest055, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskGroupManager& taskGroupManager = TaskGroupManager::GetInstance();
    uint64_t groupId = 32;
    taskGroupManager.CancelGroup(env, groupId);
    ASSERT_EQ(groupId, 32);
}

HWTEST_F(NativeEngineTest, TaskpoolTest056, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskGroupManager& taskGroupManager = TaskGroupManager::GetInstance();
    uint64_t taskId = 33;
    TaskGroup* group = taskGroupManager.GetTaskGroup(taskId);
    taskGroupManager.CancelGroupTask(env, taskId, group);
    ASSERT_EQ(taskId, 33);
}

HWTEST_F(NativeEngineTest, TaskpoolTest057, testing::ext::TestSize.Level0)
{
    TaskGroupManager& taskGroupManager = TaskGroupManager::GetInstance();
    uint64_t groupId = 34;
    taskGroupManager.UpdateGroupState(groupId);
    ASSERT_EQ(groupId, 34);
}

HWTEST_F(NativeEngineTest, TaskpoolTest058, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskGroupManager& taskGroupManager = TaskGroupManager::GetInstance();
    uint64_t seqRunnerId = 35;
    Task *task = new Task();
    ASSERT_NE(task, nullptr);
    taskGroupManager.AddTaskToSeqRunner(seqRunnerId, task);
    taskGroupManager.TriggerSeqRunner(env, task);
    SequenceRunner sequenceRunner;
    taskGroupManager.StoreSequenceRunner(seqRunnerId, &sequenceRunner);
    taskGroupManager.RemoveSequenceRunner(seqRunnerId);
    ASSERT_EQ(seqRunnerId, 35);
    SequenceRunner *res = taskGroupManager.GetSeqRunner(seqRunnerId);
    ASSERT_EQ(res, nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest059, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    taskManager.TryTriggerExpand();
    usleep(50000);
    uint32_t result = taskManager.GetIdleWorkers();
    ASSERT_TRUE(result != 0);
}

HWTEST_F(NativeEngineTest, TaskpoolTest060, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    TaskManager& taskManager = TaskManager::GetInstance();
    taskManager.InitTaskManager(env);
    uint64_t taskId = 36;
    taskManager.EnqueueTaskId(taskId, Priority::LOW);
    ASSERT_EQ(taskId, 36);

    std::pair<uint64_t, Priority> result = taskManager.DequeueTaskId();
    ASSERT_TRUE(result.first == 36);
    ASSERT_TRUE(result.second == Priority::LOW);

    taskId = 37;
    taskManager.EnqueueTaskId(taskId, Priority::IDLE);
    ASSERT_EQ(taskId, 37);

    result = taskManager.DequeueTaskId();
    ASSERT_TRUE(result.first == 37);
    ASSERT_TRUE(result.second == Priority::IDLE);
    result = taskManager.DequeueTaskId();
    ASSERT_TRUE(result.first == 0);
    ASSERT_TRUE(result.second == Priority::LOW);
}

HWTEST_F(NativeEngineTest, TaskpoolTest061, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {nullptr, nullptr};
    std::string funcName = "TaskGroupConstructor";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskGroup::TaskGroupConstructor, nullptr, &cb);

    napi_call_function(env, nullptr, cb, 2, argv, &result);
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest062, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {nullptr};
    std::string funcName = "TaskGroupConstructor";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskGroup::TaskGroupConstructor, nullptr, &cb);

    napi_call_function(env, nullptr, cb, 1, argv, &result);
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest063, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskGroup taskGroup;
    uint32_t taskId = 10;
    taskGroup.taskIds_.push_back(taskId);
    uint32_t index = taskGroup.GetTaskIndex(taskId);
    ASSERT_EQ(index, 0);
}

HWTEST_F(NativeEngineTest, TaskpoolTest064, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskGroup taskGroup;
    uint32_t taskId = 11;
    taskGroup.taskIds_.push_back(taskId);
    uint32_t index = taskGroup.GetTaskIndex(1);
    ASSERT_EQ(index, 1);
}

HWTEST_F(NativeEngineTest, TaskpoolTest065, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskGroup taskGroup;
    GroupInfo* groupInfo = new GroupInfo();
    taskGroup.pendingGroupInfos_.push_back(groupInfo);
    taskGroup.NotifyGroupTask(env);
    delete groupInfo;
    groupInfo = nullptr;
    ASSERT_TRUE(taskGroup.pendingGroupInfos_.empty());
}

HWTEST_F(NativeEngineTest, TaskpoolTest066, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskGroupManager& taskGroupManager = TaskGroupManager::GetInstance();
    TaskGroup* taskGroup = new TaskGroup();
    uint64_t groupId = reinterpret_cast<uint64_t>(taskGroup);
    taskGroup->groupId_ = groupId;
    taskGroupManager.StoreTaskGroup(groupId, taskGroup);

    GroupInfo* groupInfo = new GroupInfo();
    groupInfo->priority = Priority::DEFAULT;
    taskGroup->pendingGroupInfos_.push_back(groupInfo);
    taskGroup->NotifyGroupTask(env);
    ASSERT_TRUE(taskGroup->pendingGroupInfos_.empty());
}

HWTEST_F(NativeEngineTest, TaskpoolTest067, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskGroupManager& taskGroupManager = TaskGroupManager::GetInstance();
    TaskGroup* taskGroup = new TaskGroup();
    uint64_t groupId = reinterpret_cast<uint64_t>(taskGroup);
    taskGroup->groupId_ = groupId;
    taskGroupManager.StoreTaskGroup(groupId, taskGroup);

    Task* task = new Task();
    task->taskType_ = TaskType::COMMON_TASK;
    task->groupId_ = groupId;
    task->taskId_ = reinterpret_cast<uint64_t>(task);
    napi_reference_ref(env, task->taskRef_, nullptr);
    napi_value thisValue = NapiHelper::CreateObject(env);
    napi_wrap(
        env, thisValue, task,
        [](napi_env environment, void* data, void* hint) {
            auto obj = reinterpret_cast<Task*>(data);
            if (obj != nullptr) {
                delete obj;
            }
        }, nullptr, nullptr);
    napi_ref ref = NapiHelper::CreateReference(env, thisValue, 1);
    taskGroupManager.AddTask(groupId, ref, task->taskId_);

    GroupInfo* groupInfo = new GroupInfo();
    groupInfo->priority = Priority::DEFAULT;
    taskGroup->pendingGroupInfos_.push_back(groupInfo);
    taskGroup->NotifyGroupTask(env);
    ASSERT_TRUE(taskGroup->pendingGroupInfos_.empty());
}

HWTEST_F(NativeEngineTest, TaskpoolTest068, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskGroupManager& taskGroupManager = TaskGroupManager::GetInstance();
    TaskGroup taskGroup;

    GroupInfo* groupInfo = new GroupInfo();
    groupInfo->priority = Priority::DEFAULT;
    taskGroup.pendingGroupInfos_.push_back(groupInfo);
    uint64_t taskId = 68;
    taskGroup.taskIds_.push_back(taskId);
    taskGroup.CancelPendingGroup(env);

    ASSERT_TRUE(taskGroup.pendingGroupInfos_.empty());
}

HWTEST_F(NativeEngineTest, TaskpoolTest069, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskManager& taskManager = TaskManager::GetInstance();
    Task* pointer = new Task();
    auto task = reinterpret_cast<uint64_t>(pointer);
    auto worker = Worker::WorkerConstructor(env);
    taskManager.StoreLongTaskInfo(task, worker);
    auto res = taskManager.GetLongTaskInfo(task);
    ASSERT_TRUE(worker == res);
    taskManager.TerminateTask(task);
    res = taskManager.GetLongTaskInfo(task);
    ASSERT_TRUE(res == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest070, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskManager& taskManager = TaskManager::GetInstance();
    Task* task = new Task();
    auto id = reinterpret_cast<uint64_t>(task);
    task->isLongTask_ = true;
    task->taskId_ = id;
    taskManager.StoreTask(id, task);
    taskManager.EnqueueTaskId(id);
    usleep(50000);
    auto res = taskManager.GetLongTaskInfo(id);
    ASSERT_NE(res, nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest071, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskManager& taskManager = TaskManager::GetInstance();
    char buf[4096]; // 4096: buffer length for thread state
    auto worker = Worker::WorkerConstructor(env);
    usleep(50000);
    bool res = taskManager.ReadThreadInfo(worker, buf, sizeof(buf));
    ASSERT_TRUE(res == true);
}

HWTEST_F(NativeEngineTest, TaskpoolTest072, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskManager& taskManager = TaskManager::GetInstance();
    Task* task = new Task();
    auto id = reinterpret_cast<uint64_t>(task);
    bool res = taskManager.IsDependendByTaskId(id);
    ASSERT_NE(res, true);
}

HWTEST_F(NativeEngineTest, TaskpoolTest073, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskManager& taskManager = TaskManager::GetInstance();
    Task* task = new Task();
    auto id = reinterpret_cast<uint64_t>(task);
    taskManager.StoreTask(id, task);
    bool res = taskManager.CheckTask(task);
    ASSERT_TRUE(res == true);
}

HWTEST_F(NativeEngineTest, TaskpoolTest074, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskManager& taskManager = TaskManager::GetInstance();
    auto res = taskManager.GetThreadInfos(env);
    ASSERT_TRUE(res != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest075, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskManager& taskManager = TaskManager::GetInstance();
    auto res = taskManager.GetTaskInfos(env);
    ASSERT_TRUE(res != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest076, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskManager& taskManager = TaskManager::GetInstance();
    Task* task = new Task();
    auto id = reinterpret_cast<uint64_t>(task);
    taskManager.StoreTaskDuration(id, 0, 0);
    auto totalTime = taskManager.GetTaskDuration(id, TASK_TOTAL_TIME);
    auto cpuTime = taskManager.GetTaskDuration(id, TASK_CPU_TIME);
    ASSERT_TRUE(totalTime == 0);
    ASSERT_TRUE(cpuTime == 0);
    taskManager.StoreTaskDuration(id, 100, 100); // 100: 100 seconds
    totalTime = taskManager.GetTaskDuration(id, TASK_TOTAL_TIME);
    cpuTime = taskManager.GetTaskDuration(id, TASK_CPU_TIME);
    ASSERT_TRUE(totalTime == 100);
    ASSERT_TRUE(cpuTime == 100);
}

HWTEST_F(NativeEngineTest, TaskpoolTest077, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    TaskManager& taskManager = TaskManager::GetInstance();
    Task* task = new Task(env, TaskType::COMMON_TASK, "test");
    auto id = reinterpret_cast<uint64_t>(task);
    taskManager.StoreTask(id, task);
    auto res = taskManager.GetTaskName(id);
    ASSERT_TRUE(strcmp(res.c_str(), "test") == 0);
}

HWTEST_F(NativeEngineTest, TaskpoolTest078, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {};
    napi_value result = NativeEngineTest::IsConcurrent(env, argv, 0);
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest079, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = { nullptr };
    napi_value result = NativeEngineTest::IsConcurrent(env, argv, 1);
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest080, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = { nullptr, nullptr };
    napi_value result = NativeEngineTest::IsConcurrent(env, argv, 2);
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest081, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value func = nullptr;
    GetSendableFunction(env, "foo", func);
    napi_value argv[] = { func };
    napi_value result = NativeEngineTest::IsConcurrent(env, argv, 1);
    bool res = true;
    napi_get_value_bool(env, result, &res);
    ASSERT_TRUE(res == false);
}

HWTEST_F(NativeEngineTest, TaskpoolTest082, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = { nullptr };
    napi_value result = NativeEngineTest::GetTaskPoolInfo(env, argv, 1);
    ASSERT_TRUE(result != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest083, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = { nullptr };
    napi_value result = NativeEngineTest::TerminateTask(env, argv, 1);
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest084, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value func = nullptr;
    GetSendableFunction(env, "foo", func);
    napi_value argv[] = { func };
    napi_value result = NativeEngineTest::Execute(env, argv, 1);
    ASSERT_TRUE(result != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest085, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value func = nullptr;
    GetSendableFunction(env, "foo", func);

    size_t delay = 1000;
    napi_value result = nullptr;
    napi_create_uint32(env, delay, &result);

    napi_value argv[] = { result, func };
    std::string funcName = "ExecuteDelayed";
    result = NativeEngineTest::ExecuteDelayed(env, argv, 2);
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest086, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    std::string func = "SeqRunnerConstructor";
    napi_value callback = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, func.c_str(), func.size(), SequenceRunner::SeqRunnerConstructor, nullptr, &callback);

    napi_value argv1[] = {nullptr};
    napi_create_uint32(env, 1, &argv1[0]);
    napi_call_function(env, nullptr, callback, 1, argv1, &result);
    ASSERT_NE(result, nullptr);

    napi_value argv2[2] = {nullptr};
    napi_create_uint32(env, 1, &argv2[0]);
    napi_create_string_utf8(env, "seq01", NAPI_AUTO_LENGTH, &argv2[1]);
    result = nullptr;
    napi_call_function(env, nullptr, callback, 2, argv2, &result);
    ASSERT_EQ(result, nullptr);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);

    napi_value argv3[2] = {nullptr};
    napi_create_string_utf8(env, "seq02", NAPI_AUTO_LENGTH, &argv3[0]);
    napi_create_uint32(env, 1, &argv3[1]);
    result = nullptr;
    napi_call_function(env, nullptr, callback, 2, argv3, &result);
    ASSERT_NE(result, nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest087, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    std::string func = "SeqRunnerConstructor";
    napi_value callback = nullptr;
    napi_create_function(env, func.c_str(), func.size(), SequenceRunner::SeqRunnerConstructor, nullptr, &callback);

    napi_value argv[2] = {nullptr};
    napi_create_string_utf8(env, "seq03", NAPI_AUTO_LENGTH, &argv[0]);
    napi_create_uint32(env, 1, &argv[1]);
    napi_value result = nullptr;
    napi_call_function(env, nullptr, callback, 2, argv, &result);
    ASSERT_NE(result, nullptr);

    func = "Execute";
    napi_value cb = nullptr;
    napi_value res = nullptr;
    napi_create_function(env, func.c_str(), func.size(), SequenceRunner::Execute, nullptr, &cb);

    napi_value argv1[] = {nullptr};
    napi_create_uint32(env, 1, &argv1[0]);
    napi_call_function(env, nullptr, cb, 1, argv1, &res);
    ASSERT_EQ(res, nullptr);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);

    napi_value thisValue = CreateTaskObject(env);
    napi_value argv2[] = {thisValue};
    res = nullptr;
    napi_call_function(env, nullptr, cb, 1, argv2, &res);
    ASSERT_NE(res, nullptr);
    exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);

    napi_value napiSeqRunnerId = NapiHelper::GetNameProperty(env, result, "seqRunnerId");
    uint64_t seqId = NapiHelper::GetUint64Value(env, napiSeqRunnerId);
    SequenceRunner seq;
    TaskGroupManager &taskGroupManager = TaskGroupManager::GetInstance();
    taskGroupManager.StoreSequenceRunner(seqId, &seq);

    res = nullptr;
    napi_call_function(env, nullptr, cb, 1, argv2, &res);
    ASSERT_NE(res, nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest088, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {nullptr};
    std::string func = "TaskGroupConstructor";
    napi_value callback = nullptr;
    napi_value taskGroupResult = nullptr;
    napi_create_function(env, func.c_str(), func.size(), TaskGroup::TaskGroupConstructor, nullptr, &callback);
    napi_call_function(env, nullptr, callback, 0, argv, &taskGroupResult);

    func = "AddTask";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, func.c_str(), func.size(), TaskGroup::AddTask, nullptr, &cb);
    
    napi_value napiGroupId = NapiHelper::GetNameProperty(env, taskGroupResult, "groupId");
    uint64_t groupId = NapiHelper::GetUint64Value(env, napiGroupId);

    TaskGroupManager& taskGroupManager = TaskGroupManager::GetInstance();
    TaskGroup* group = new TaskGroup();
    group->groupId_ = groupId;
    taskGroupManager.StoreTaskGroup(groupId, group);

    napi_value thisValue = CreateTaskObject(env);
    napi_value argv1[] = {thisValue};
    napi_call_function(env, nullptr, cb, 1, argv1, &result);
    ASSERT_NE(result, nullptr);

    napi_value thisValue2 = CreateTaskObject(env, TaskType::SEQRUNNER_TASK);
    napi_value argv2[] = {thisValue2};
    result = nullptr;
    napi_call_function(env, nullptr, cb, 1, argv2, &result);
    ASSERT_EQ(result, nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest089, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {nullptr};
    std::string func = "TaskGroupConstructor";
    napi_value callback = nullptr;
    napi_value taskGroupResult = nullptr;
    napi_create_function(env, func.c_str(), func.size(), TaskGroup::TaskGroupConstructor, nullptr, &callback);
    napi_call_function(env, nullptr, callback, 0, argv, &taskGroupResult);

    napi_value thisValue = NapiHelper::CreateObject(env);
    napi_value argv1[] = {thisValue};
    func = "AddTask";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, func.c_str(), func.size(), TaskGroup::AddTask, nullptr, &cb);
    napi_call_function(env, nullptr, cb, 1, argv1, &result);
    ASSERT_EQ(result, nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest090, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {nullptr};
    std::string funcName = "TaskGroupConstructor";
    napi_value callback = nullptr;
    napi_value taskGroupResult = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskGroup::TaskGroupConstructor, nullptr, &callback);
    napi_call_function(env, nullptr, callback, 0, argv, &taskGroupResult);

    auto func = [](napi_env environment, napi_callback_info info) -> napi_value {
        return nullptr;
    };
    napi_value thisValue = nullptr;
    napi_create_function(env, "testFunc", NAPI_AUTO_LENGTH, func, nullptr, &thisValue);
    napi_value argv1[] = {thisValue};
    funcName = "AddTask";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskGroup::AddTask, nullptr, &cb);
    napi_call_function(env, nullptr, cb, 1, argv1, &result);
    ASSERT_EQ(result, nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest091, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {nullptr};
    std::string funcName = "TaskGroupConstructor";
    napi_value callback = nullptr;
    napi_value taskGroupResult = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskGroup::TaskGroupConstructor, nullptr, &callback);
    napi_call_function(env, nullptr, callback, 0, argv, &taskGroupResult);

    napi_value argv1[] = {};
    napi_create_uint32(env, 1, &argv1[0]);
    funcName = "AddTask";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskGroup::AddTask, nullptr, &cb);
    napi_call_function(env, nullptr, cb, 1, argv1, &result);
    ASSERT_EQ(result, nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest092, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value global;
    napi_get_global(env, &global);
    napi_value argv[2] = {};
    std::string funcName = "OnReceiveData";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::OnReceiveData, nullptr, &cb);
    napi_call_function(env, global, cb, 0, argv, &result);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ExceptionScope scope(env);
    argv[0] = nullptr;
    argv[1] = nullptr;
    result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::OnReceiveData, nullptr, &cb);
    napi_call_function(env, global, cb, 2, argv, &result);
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest093, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value global;
    napi_get_global(env, &global);
    ExceptionScope scope(env);
    napi_value func = nullptr;
    GetSendableFunction(env, "foo", func);
    napi_value argv[] = { func };
    std::string funcName = "TaskConstructor";
    napi_value cb = nullptr;
    napi_value task = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::TaskConstructor, nullptr, &cb);
    napi_call_function(env, global, cb, 1, argv, &task);
    auto& taskManager = TaskManager::GetInstance();
    Task* pointer = nullptr;
    napi_unwrap(env, task, reinterpret_cast<void**>(&pointer));
    taskManager.StoreTask(pointer->taskId_, pointer);
    
    funcName = "OnReceiveData";
    cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::OnReceiveData, nullptr, &cb);
    napi_call_function(env, global, cb, 1, argv, &result);
    auto res = taskManager.GetCallbackInfo(pointer->taskId_);
    ASSERT_TRUE(res != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest094, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = { nullptr, nullptr };
    std::string funcName = "TaskConstructor";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::TaskConstructor, nullptr, &cb);
    napi_call_function(env, nullptr, cb, 2, argv, &result);
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest095, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    const char* str = "Task";
    napi_value name = nullptr;
    napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &name);
    napi_value argv[] = { name, nullptr };
    std::string funcName = "TaskConstructor";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::TaskConstructor, nullptr, &cb);
    napi_call_function(env, nullptr, cb, 2, argv, &result);
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest096, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value global;
    napi_get_global(env, &global);
    ExceptionScope scope(env);
    napi_value func = nullptr;
    GetSendableFunction(env, "foo", func);
    napi_value argv[] = { func };
    std::string funcName = "TaskConstructor";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::TaskConstructor, nullptr, &cb);
    napi_call_function(env, global, cb, 1, argv, &result);
    ASSERT_TRUE(result != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest097, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value global;
    napi_get_global(env, &global);
    ExceptionScope scope(env);
    napi_value func = nullptr;
    GetSendableFunction(env, "bar", func);
    napi_value argv[] = { func };
    std::string funcName = "LongTaskConstructor";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::LongTaskConstructor, nullptr, &cb);
    napi_call_function(env, global, cb, 1, argv, &result);
    ASSERT_TRUE(result != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest098, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = { nullptr };
    std::string funcName = "SendData";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::SendData, nullptr, &cb);
    napi_call_function(env, nullptr, cb, 1, argv, &result);
    ASSERT_TRUE(result == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest099, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value argv[] = {};
    std::string funcName = "AddDependency";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::AddDependency, nullptr, &cb);
    napi_call_function(env, nullptr, cb, 0, argv, &result);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest100, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value global;
    napi_get_global(env, &global);
    auto task = GeneratorTask(env, global);
    Task* pointer = nullptr;
    napi_unwrap(env, task, reinterpret_cast<void**>(&pointer));
    pointer->isPeriodicTask_ = true;

    napi_value argv[] = { nullptr };
    std::string funcName = "AddDependency";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::AddDependency, nullptr, &cb);
    napi_call_function(env, task, cb, 1, argv, &result);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest101, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value global;
    napi_get_global(env, &global);
    auto task = GeneratorTask(env, global);
    Task* pointer = nullptr;
    napi_unwrap(env, task, reinterpret_cast<void**>(&pointer));
    pointer->taskType_ = TaskType::COMMON_TASK;

    napi_value argv[] = { nullptr };
    std::string funcName = "AddDependency";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::AddDependency, nullptr, &cb);
    napi_call_function(env, task, cb, 1, argv, &result);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest102, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value global;
    napi_get_global(env, &global);
    auto task = GeneratorTask(env, global);
    auto dependentTask = GeneratorTask(env, global);

    napi_value argv[] = { task };
    std::string funcName = "AddDependency";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::AddDependency, nullptr, &cb);
    napi_call_function(env, task, cb, 1, argv, &result);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest103, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value global;
    napi_get_global(env, &global);
    auto task = GeneratorTask(env, global);
    napi_value obj;
    napi_create_object(env, &obj);
    auto dependentTask = GeneratorTask(env, obj);

    napi_value argv[] = { dependentTask };
    std::string funcName = "AddDependency";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::AddDependency, nullptr, &cb);
    napi_call_function(env, task, cb, 1, argv, &result);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest104, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value obj;
    napi_create_object(env, &obj);
    auto task = GeneratorTask(env, obj);
    napi_value argv[] = { task };
    NativeEngineTest::Execute(env, argv, 1);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest105, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value obj;
    napi_create_object(env, &obj);
    auto task = GeneratorTask(env, obj);
    napi_value argv[] = { task };
    NativeEngineTest::TerminateTask(env, argv, 1);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest106, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {};
    NativeEngineTest::TerminateTask(env, argv, 0);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest107, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value obj;
    napi_create_object(env, &obj);
    auto task = GeneratorTask(env, obj);
    Task* pointer = nullptr;
    napi_unwrap(env, task, reinterpret_cast<void**>(&pointer));
    pointer->isLongTask_ = true;
    napi_value argv[] = { task };
    NativeEngineTest::TerminateTask(env, argv, 1);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest108, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {};
    NativeEngineTest::Execute(env, argv, 0);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest109, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value obj;
    napi_create_object(env, &obj);
    auto task = GeneratorTask(env, obj);
    napi_value priority = nullptr;
    napi_create_uint32(env, 2, &priority); // 2: LOW priority
    napi_value argv[] = { task, priority };
    NativeEngineTest::Execute(env, argv, 2);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest110, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value obj;
    napi_create_object(env, &obj);
    auto task = GeneratorTask(env, obj);
    napi_value priority = nullptr;
    napi_create_uint32(env, 10, &priority); // 10: invalid priority
    napi_value argv[] = { task, priority };
    NativeEngineTest::Execute(env, argv, 2);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest111, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value obj;
    napi_create_object(env, &obj);
    auto group = GeneratorTaskGroup(env, obj);
    napi_value taskObj;
    napi_create_object(env, &taskObj);
    auto task = GeneratorTask(env, taskObj);
    napi_value argv[] = { task };
    std::string funcName = "AddTask";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskGroup::AddTask, nullptr, &cb);
    napi_call_function(env, group, cb, 1, argv, &result);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest112, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value obj;
    napi_create_object(env, &obj);
    auto group = GeneratorTaskGroup(env, obj);
    napi_value func = nullptr;
    GetSendableFunction(env, "foo", func);
    napi_value argv[] = { func };
    std::string funcName = "AddTask";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), TaskGroup::AddTask, nullptr, &cb);
    napi_call_function(env, group, cb, 1, argv, &result);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest113, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = {};
    NativeEngineTest::Cancel(env, argv, 0);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest114, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value argv[] = { nullptr };
    NativeEngineTest::Cancel(env, argv, 1);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest115, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value obj = nullptr;
    napi_create_object(env, &obj);
    auto task = GeneratorTask(env, obj);
    napi_value argv[] = { task };
    NativeEngineTest::Cancel(env, argv, 1);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest116, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    napi_value obj = nullptr;
    napi_create_object(env, &obj);
    auto task = GeneratorTask(env, obj);
    napi_value argv[] = { task };
    NativeEngineTest::Execute(env, argv, 1);
    NativeEngineTest::Cancel(env, argv, 1);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception == nullptr);
}


HWTEST_F(NativeEngineTest, TaskpoolTest117, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value obj;
    napi_create_object(env, &obj);
    auto task = GeneratorTask(env, obj);
    napi_value argv[] = { nullptr, nullptr };
    std::string funcName = "SetTransferList";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::SetTransferList, nullptr, &cb);
    napi_call_function(env, task, cb, 2, argv, &result);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest118, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value obj;
    napi_create_object(env, &obj);
    auto task = GeneratorTask(env, obj);
    napi_value array;
    napi_create_array_with_length(env, 1, &array);

    napi_value argv[] = { array };
    std::string funcName = "SetTransferList";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::SetTransferList, nullptr, &cb);
    napi_call_function(env, task, cb, 1, argv, &result);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest119, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value obj;
    napi_create_object(env, &obj);
    auto task = GeneratorTask(env, obj);
    napi_value argv[] = { nullptr, nullptr };
    std::string funcName = "SetCloneList";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::SetCloneList, nullptr, &cb);
    napi_call_function(env, task, cb, 2, argv, &result);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest120, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value obj;
    napi_create_object(env, &obj);
    auto task = GeneratorTask(env, obj);
    napi_value argv[] = { nullptr };
    std::string funcName = "SetCloneList";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::SetCloneList, nullptr, &cb);
    napi_call_function(env, task, cb, 1, argv, &result);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception != nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest121, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value obj;
    napi_create_object(env, &obj);
    auto task = GeneratorTask(env, obj);
    napi_value array;
    napi_create_array_with_length(env, 1, &array);
    napi_value argv[] = { array };
    std::string funcName = "SetCloneList";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::SetCloneList, nullptr, &cb);
    napi_call_function(env, task, cb, 1, argv, &result);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception == nullptr);
}

HWTEST_F(NativeEngineTest, TaskpoolTest122, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    napi_value argv[] = {};
    std::string funcName = "IsCanceled";
    napi_value cb = nullptr;
    napi_value result = nullptr;
    napi_create_function(env, funcName.c_str(), funcName.size(), Task::IsCanceled, nullptr, &cb);
    napi_call_function(env, nullptr, cb, 0, argv, &result);
    bool value = true;
    napi_get_value_bool(env, result, &value);
    ASSERT_TRUE(value == false);
}

HWTEST_F(NativeEngineTest, TaskpoolTest123, testing::ext::TestSize.Level0)
{
    auto task = std::make_unique<Task>();
    task->taskId_ = reinterpret_cast<uint64_t>(task.get());
    task->ioTime_ = 100;
    task->startTime_ = 0;
    task->StoreTaskDuration();
    auto& taskManager = TaskManager::GetInstance();
    auto res = taskManager.GetTaskDuration(task->taskId_, "totalDuration");
    ASSERT_TRUE(res != 0);
    res = taskManager.GetTaskDuration(task->taskId_, "cpuDuration");
    ASSERT_TRUE(res != 0);
    res = taskManager.GetTaskDuration(task->taskId_, "ioDuration");
    ASSERT_TRUE(res == 0);
}

HWTEST_F(NativeEngineTest, TaskpoolTest124, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    auto task = std::make_unique<Task>();
    task->taskId_ = reinterpret_cast<uint64_t>(task.get());
    task->SetHasDependency(true);
    auto res = task->CanForSequenceRunner(env);
    ASSERT_TRUE(res == false);
    task->TryClearHasDependency();
    res = task->CanForSequenceRunner(env);
    ASSERT_TRUE(res == true);
    task->taskType_ = TaskType::SEQRUNNER_TASK;
    res = task->CanForSequenceRunner(env);
    ASSERT_TRUE(res == false);
    task->taskType_ = TaskType::GROUP_COMMON_TASK;
    res = task->CanForSequenceRunner(env);
    ASSERT_TRUE(res == false);
    task->UpdatePeriodicTask();
    res = task->CanForSequenceRunner(env);
    ASSERT_TRUE(res == false);
}

HWTEST_F(NativeEngineTest, TaskpoolTest125, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    auto task = std::make_unique<Task>();
    task->ThrowNoDependencyError(env);
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    ASSERT_TRUE(exception!= nullptr);
    auto res = task->CanExecutePeriodically(env);
    ASSERT_TRUE(res == true);
    task->UpdatePeriodicTask();
    res = task->CanExecutePeriodically(env);
    ASSERT_TRUE(res == false);
}

HWTEST_F(NativeEngineTest, TaskpoolTest126, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    auto task = std::make_unique<Task>();
    auto res = task->CanForTaskGroup(env);
    ASSERT_TRUE(res == true);
    task->taskId_ = reinterpret_cast<uint64_t>(task.get());
    task->SetHasDependency(true);
    res = task->CanForTaskGroup(env);
    ASSERT_TRUE(res == false);
    task->taskType_ = TaskType::SEQRUNNER_TASK;
    res = task->CanForTaskGroup(env);
    ASSERT_TRUE(res == false);
    task->taskType_ = TaskType::GROUP_COMMON_TASK;
    res = task->CanForTaskGroup(env);
    ASSERT_TRUE(res == false);
    task->isLongTask_ = true;
    res = task->CanForTaskGroup(env);
    ASSERT_TRUE(res == false);
    task->UpdatePeriodicTask();
    res = task->CanForTaskGroup(env);
    ASSERT_TRUE(res == false);
}

HWTEST_F(NativeEngineTest, TaskpoolTest127, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    auto task = std::make_unique<Task>();
    auto res = task->CanExecute(env);
    task->taskType_ = TaskType::GROUP_COMMON_TASK;
    res = task->CanExecute(env);
    ASSERT_TRUE(res == false);
    task->taskType_ = TaskType::SEQRUNNER_TASK;
    res = task->CanExecute(env);
    ASSERT_TRUE(res == false);
    task->taskType_ = TaskType::COMMON_TASK;
    res = task->CanExecute(env);
    ASSERT_TRUE(res == true);
    task->SetHasDependency(true);
    res = task->CanExecute(env);
    ASSERT_TRUE(res == false);
    task->TryClearHasDependency();
    task->isLongTask_ = true;
    res = task->CanExecute(env);
    ASSERT_TRUE(res == false);
    task->UpdatePeriodicTask();
    res = task->CanExecute(env);
    ASSERT_TRUE(res == false);
}

HWTEST_F(NativeEngineTest, TaskpoolTest128, testing::ext::TestSize.Level0)
{
    napi_env env = (napi_env)engine_;
    ExceptionScope scope(env);
    auto task = std::make_unique<Task>();
    auto res = task->CanExecuteDelayed(env);
    task->taskType_ = TaskType::GROUP_COMMON_TASK;
    res = task->CanExecuteDelayed(env);
    ASSERT_TRUE(res == false);
    task->taskType_ = TaskType::SEQRUNNER_TASK;
    res = task->CanExecuteDelayed(env);
    ASSERT_TRUE(res == false);
    task->taskType_ = TaskType::COMMON_TASK;
    res = task->CanExecuteDelayed(env);
    ASSERT_TRUE(res == true);
    task->isLongTask_ = true;
    res = task->CanExecute(env);
    ASSERT_TRUE(res == false);
    task->UpdatePeriodicTask();
    res = task->CanExecute(env);
    ASSERT_TRUE(res == false);
}