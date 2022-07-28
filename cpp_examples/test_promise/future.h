/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#ifndef LIB_FUTURE_H_
#define LIB_FUTURE_H_

#include <functional>
#include <mutex>
#include <memory>
#include <condition_variable>

#include <list>

// 总结future/promise实现的技巧
// 需要由一个control block，内部至少包含一个条件变量cond,一个锁，一个value和一个bool complete
// promise.setValue 需要设置value的值和complete的值，以及唤醒条件变量
// future.get需要检查complete标记，并在未完成前cond.wait
// promise需要提供一个GetFuture()接口，其创建的Future必须和自身共享一个control block以传递变量值

// 在这个基础上， Pulsar的promise/future增加了额外的功能
//1. future支持添加回调函数
//2. 支持错误码，所以错误码需要在回调/promise/future中传递

typedef std::unique_lock<std::mutex> Lock;

namespace pulsar {

// pulsar的Result是类似于错误码，可以用Enum
// type是值类型
template <typename Result, typename Type>
struct InternalState {
    std::mutex mutex;
    std::condition_variable condition;
    Result result;
    Type value;
    bool complete;
    // future支持添加回调
    std::list<typename std::function<void(Result, const Type&)> > listeners;
};

template <typename Result, typename Type>
class Future {
   public:
    typedef std::function<void(Result, const Type&)> ListenerCallback;

    Future& addListener(ListenerCallback callback) {
        InternalState<Result, Type>* state = state_.get();
        Lock lock(state->mutex);

        //  如果promise已经设置好了值，那么直接执行回调
        if (state->complete) {
            lock.unlock();
            callback(state->result, state->value);
        } else {
        // 否则还不到回调执行时机，把回调加入到列表里，等待promise
            state->listeners.push_back(callback);
        }

        return *this;
    }

    Result get(Type& ValueResult) {
        InternalState<Result, Type>* state = state_.get();
        Lock lock(state->mutex);

        if (!state->complete) {
            // Wait for result
            while (!state->complete) {
                state->condition.wait(lock);
            }
        }

        ValueResult = state->value;
        return state->result;
    }

   private:
    typedef std::shared_ptr<InternalState<Result, Type> > InternalStatePtr;
    Future(InternalStatePtr state) : state_(state) {}

    std::shared_ptr<InternalState<Result, Type> > state_;

    template <typename U, typename V>
    friend class Promise;
};

template <typename Result, typename Type>
class Promise {
   public:
    Promise() : state_(std::make_shared<InternalState<Result, Type> >()) {}

    bool setValue(const Type& value) const {
        // 初始化错误码
        static Result DEFAULT_RESULT;
        InternalState<Result, Type>* state = state_.get();
        //要更改state的数据，给state内部的锁加锁
        Lock lock(state->mutex);

        // 对promise多次使用SetValue是错误的
        if (state->complete) {
            return false;
        }

        state->value = value;
        state->result = DEFAULT_RESULT;
        state->complete = true;

        //这里存在两种case
        // promise.setvalue的时候，future还没有调用add_listener，那么这里什么也没有,cb在future.add_listener的时候直接执行
        // promise.setvalue的时候，future已经设置好了所有回调，那么依次执行
        decltype(state->listeners) listeners;
        listeners.swap(state->listeners);

        lock.unlock();
        // Promise设置值以后，依次调用listeners里的回调
        for (auto& callback : listeners) {
            callback(DEFAULT_RESULT, value);
        }

        state->condition.notify_all();
        return true;
    }

    bool setFailed(Result result) const {
        // setFailed主要为了设置错误码
        static Type DEFAULT_VALUE;
        InternalState<Result, Type>* state = state_.get();
        Lock lock(state->mutex);

        if (state->complete) {
            return false;
        }

        state->result = result;
        state->complete = true;

        decltype(state->listeners) listeners;
        listeners.swap(state->listeners);

        lock.unlock();

        for (auto& callback : listeners) {
            callback(result, DEFAULT_VALUE);
        }

        state->condition.notify_all();
        return true;
    }

    bool isComplete() const {
        InternalState<Result, Type>* state = state_.get();
        Lock lock(state->mutex);
        return state->complete;
    }

    Future<Result, Type> getFuture() const { return Future<Result, Type>(state_); }

   private:
    typedef std::function<void(Result, const Type&)> ListenerCallback;
    std::shared_ptr<InternalState<Result, Type> > state_;
};

} /* namespace pulsar */

#endif /* LIB_FUTURE_H_ */